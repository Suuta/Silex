
#include "PCH.h"
#include "Rendering/ShaderCompiler.h"
#include <regex>

//==========================================================================
// NOTE:
// glslang + spirv_tool のコンパイルでは、静的ライブラリが複雑 + サイズが大きすぎる
// 特に "SPIRV-Tools-optd.lib" が 300MB あり、100MB制限で github にアップできない
// 共有ライブラリでビルドできる shaderc に移行
//==========================================================================
#define SHADERC 1
#if !SHADERC
    #include <glslang/Public/ResourceLimits.h>
    #include <glslang/SPIRV/GlslangToSpv.h>
    #include <glslang/Public/ShaderLang.h>
#endif

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_glsl.hpp>


namespace Silex
{
    static bool ReadFile(std::string& out_result, const std::string& filepath)
    {
        std::ifstream in(filepath, std::ios::in | std::ios::binary);

        if (in)
        {
            in.seekg(0, std::ios::end);
            out_result.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(&out_result[0], out_result.size());
        }
        else
        {
            SL_LOG_ERROR("ファイルの読み込みに失敗しました: {}", filepath.c_str());
            return false;
        }

        in.close();
        return true;
    }

    class ShaderIncluder : public shaderc::CompileOptions::IncluderInterface
    {
    public:

        // shadeerc が #include "header.h" のようなインクルードディレクティブを検出した時に呼び出される
        // そのファイルを読み込む方法と、読み込んで返すデータを定義する
        shaderc_include_result* GetInclude(const char* requestedSource, shaderc_include_type type, const char* requestingSource, size_t includeDepth) override 
        {
            std::filesystem::path readSource = std::filesystem::path(requestingSource).parent_path() / requestedSource;
            std::string includePath = readSource.string();

            std::replace(includePath.begin(), includePath.end(), '\\', '/');

            std::string content;
            bool res = ReadFile(content, includePath);

            shaderc_include_result* result = new shaderc_include_result;
            result->source_name        = _strdup(includePath.c_str());
            result->source_name_length = includePath.length();
            result->content            = _strdup(content.c_str());
            result->content_length     = content.length();
            result->user_data          = nullptr;

            return result;
        }

        // GetInclude で確保したメモリを解放するコード
        void ReleaseInclude(shaderc_include_result* include_result) override
        {
            if (include_result)
            {
                free((void*)include_result->source_name);
                free((void*)include_result->content);
                delete include_result;
            }
        }
    };


    static ShaderStage ToShaderStage(const std::string& type)
    {
        if (type == "VERTEX")   return SHADER_STAGE_VERTEX_BIT;
        if (type == "FRAGMENT") return SHADER_STAGE_FRAGMENT_BIT;
        if (type == "GEOMETRY") return SHADER_STAGE_GEOMETRY_BIT;
        if (type == "COMPUTE")  return SHADER_STAGE_COMPUTE_BIT;

        return SHADER_STAGE_ALL;
    }

    static const char* ToStageString(const ShaderStage stage)
    {
        if (stage & SHADER_STAGE_VERTEX_BIT)   return "VERTEX";
        if (stage & SHADER_STAGE_FRAGMENT_BIT) return "FRAGMENT";
        if (stage & SHADER_STAGE_GEOMETRY_BIT) return "GEOMETRY";
        if (stage & SHADER_STAGE_COMPUTE_BIT)  return "COMPUTE";

        return "SHADER_STAGE_ALL";
    }

    static ShaderDataType ToShaderDataType(const spirv_cross::SPIRType& type)
    {
        switch (type.basetype)
        {
            case spirv_cross::SPIRType::Boolean:  return SHADER_DATA_TYPE_BOOL;
            case spirv_cross::SPIRType::Int:
                if (type.vecsize == 1)            return SHADER_DATA_TYPE_INT;
                if (type.vecsize == 2)            return SHADER_DATA_TYPE_IVEC2;
                if (type.vecsize == 3)            return SHADER_DATA_TYPE_IVEC3;
                if (type.vecsize == 4)            return SHADER_DATA_TYPE_IVEC4;

            case spirv_cross::SPIRType::UInt:     return SHADER_DATA_TYPE_UINT;
            case spirv_cross::SPIRType::Float:
                if (type.columns == 3)            return SHADER_DATA_TYPE_MAT3;
                if (type.columns == 4)            return SHADER_DATA_TYPE_MAT4;

                if (type.vecsize == 1)            return SHADER_DATA_TYPE_FLOAT;
                if (type.vecsize == 2)            return SHADER_DATA_TYPE_VEC2;
                if (type.vecsize == 3)            return SHADER_DATA_TYPE_VEC3;
                if (type.vecsize == 4)            return SHADER_DATA_TYPE_VEC4;
        }

        return SHADER_DATA_TYPE_NONE;
    }


#if SHADERC
    static shaderc_shader_kind ToShaderC(const ShaderStage stage)
    {
        switch (stage)
        {
            case SHADER_STAGE_VERTEX_BIT:   return shaderc_vertex_shader;
            case SHADER_STAGE_FRAGMENT_BIT: return shaderc_fragment_shader;
            case SHADER_STAGE_COMPUTE_BIT:  return shaderc_compute_shader;
            case SHADER_STAGE_GEOMETRY_BIT: return shaderc_geometry_shader;
        }

        return {};
    }
#else
    static EShLanguage ToGlslang(const ShaderStage stage)
    {
        switch (stage)
        {
            case SHADER_STAGE_VERTEX_BIT:    return EShLangVertex;
            case SHADER_STAGE_FRAGMENT_BIT:  return EShLangFragment;
            case SHADER_STAGE_COMPUTE_BIT:   return EShLangCompute;
            case SHADER_STAGE_GEOMETRY_BIT:  return EShLangGeometry;
        }

        return {};
    }
#endif


    // ステージ間共有バッファ保存用変数  ※リフレクション時のバッファ複数回定義を防ぐ
    // map<descriptorset_index, map<bind_index, buffer>>
    static std::unordered_map<uint32, std::unordered_map<uint32, ShaderBuffer>> ExistUniformBuffers;
    static std::unordered_map<uint32, std::unordered_map<uint32, ShaderBuffer>> ExistStorageBuffers;
    static ShaderReflectionData                                                 ReflectionData;

    ShaderCompiler* ShaderCompiler::Get()
    {
        return instance;
    }

    bool ShaderCompiler::Compile(const std::string& filePath, ShaderCompiledData& out_compiledData)
    {
        bool result = false;
        
        // ファイル読み込み
        std::string rawSource;
        result = ReadFile(rawSource, filePath);
        SL_CHECK(!result, false);

        // コンパイル前処理として、ステージごとに分割する
        std::unordered_map<ShaderStage, std::string> parsedRawSources;
        parsedRawSources = _SplitStages(rawSource);

        SL_LOG_TRACE("**************************************************");
        SL_LOG_TRACE("Compile: {}", filePath.c_str());
        SL_LOG_TRACE("**************************************************");

        // ステージごとにコンパイル
        std::unordered_map<ShaderStage, std::vector<uint32>> spirvBinaries;
        for (const auto& [stage, source] : parsedRawSources)
        {
            std::string error = _CompileStage(stage, source, spirvBinaries[stage], filePath);
            if (!error.empty())
            {
                SL_LOG_ERROR("ShaderCompile: {}", error.c_str());
                return false;
            }
        }

        // ステージごとにリフレクション
        for (const auto& [stage, binary] : spirvBinaries)
        {
            _ReflectStage(stage, binary);
        }

        SL_LOG_TRACE("**************************************************");

        // コンパイル結果
        out_compiledData.reflection     = ReflectionData;
        out_compiledData.shaderBinaries = spirvBinaries;

        // リフレクションデータリセット
        ReflectionData.descriptorSets.clear();
        ReflectionData.pushConstantRanges.clear();
        ReflectionData.pushConstants.clear();
        ReflectionData.resources.clear();

        return true;
    }

#if 0

    // #include + ".glsl" を含むコードを探し出す
    static std::vector<std::string> _FindIncludeDirective(const std::string& shaderCode)
    {
        std::vector<std::string> includes;
        std::regex includePattern(R"(#include\s*"[^"]+\.glsl"\s*)");
        std::smatch matches;

        std::string::const_iterator searchStart(shaderCode.cbegin());
        while (std::regex_search(searchStart, shaderCode.cend(), matches, includePattern))
        {
            includes.push_back(matches[0]);
            searchStart = matches.suffix().first;
        }

        return includes;
    }
#endif

    static void _TrimBraces(std::string& str)
    {
        size_t startPos = str.find('{');

        if (startPos != std::string::npos)
            str.erase(startPos, 1);

        size_t endPos = str.rfind('}');

        if (endPos != std::string::npos)
            str.erase(endPos, 1);
    }

    std::unordered_map<ShaderStage, std::string> ShaderCompiler::_SplitStages(const std::string& source)
    {
        std::unordered_map<ShaderStage, std::string> shaderSources;
        std::string type;

        uint64 keywordLength = strlen("#pragma");
        uint64 pos           = source.find("#pragma", 0);
        uint64 lastPos       = 0;

        while (pos != std::string::npos)
        {
            uint64 eol = source.find_first_of("\r\n", pos);
            SL_ASSERT(eol != std::string::npos, "シンタックスエラー: シェーダーステージ指定が見つかりません");

            uint64 begin = pos + keywordLength + 1;
            uint64 end   = source.find_first_of(" \r\n", begin);
            type = source.substr(begin, end - begin);

            SL_ASSERT(type == "VERTEX" || type == "FRAGMENT" || type == "GEOMETRY" || type == "COMPUTE", "無効なシェーダーステージです");

            uint64 nextLinePos = source.find_first_not_of("\r\n", eol);
            pos = source.find("#pragma", nextLinePos);
            
            // #pragma XXXX を取り除いたソースを追加
            shaderSources[ToShaderStage(type)] = source.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
            
            // ステージ間の '{' '}' を取り除く
            _TrimBraces(shaderSources[ToShaderStage(type)]);
        }

        return shaderSources;
    }

    std::string ShaderCompiler::_CompileStage(ShaderStage stage, const std::string& source, std::vector<uint32>& out_putSpirv, const std::string& filepath)
    {
        //=====================================================================================================================
        // shaderc における GLSLコンパイラでは、エントリーポイントは "main" であると想定され、エントリーポイント指定は HLSL 専用の機能になっている
        //=====================================================================================================================
#if 0
        auto entrypointName = [](ShaderStage stage) -> std::string
        {
            switch (stage)
            {
                case Silex::SHADER_STAGE_VERTEX_BIT:                 return "vsmain";
                case Silex::SHADER_STAGE_TESSELATION_CONTROL_BIT:    return "tcsmain"; // hull
                case Silex::SHADER_STAGE_TESSELATION_EVALUATION_BIT: return "tesmain"; // domain
                case Silex::SHADER_STAGE_GEOMETRY_BIT:               return "gsmain";
                case Silex::SHADER_STAGE_FRAGMENT_BIT:               return "fsmain";
                case Silex::SHADER_STAGE_COMPUTE_BIT:                return "csmain";
            }

            return "";
        };

        std::string entrypoint = entrypointName(stage);
#endif

#if SHADERC

        shaderc::Compiler compiler;

        // コンパイル: SPIR-V バイナリ形式へコンパイル
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        options.SetWarningsAsErrors();
        options.SetIncluder(std::make_unique<ShaderIncluder>());

        
        const shaderc::SpvCompilationResult compileResult = compiler.CompileGlslToSpv(source, ToShaderC(stage), filepath.c_str(), options);
        if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            return compileResult.GetErrorMessage();
        }

        // SPIR-V は 4バイトアラインメント
        out_putSpirv = std::vector<uint32>(compileResult.begin(), compileResult.end());

        // エラーメッセージ無し
        return {};
#else
        glslang::InitializeProcess();
        EShLanguage glslangStage = ToGlslang(stage);
        EShMessages messages     = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);

        const char* sourceStrings[1];
        sourceStrings[0] = source.data();

        glslang::TShader shader(glslangStage);
        shader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_5);
        shader.setStrings(sourceStrings, 1);

        bool result = shader.parse(GetDefaultResources(), 100, false, messages);
        if (result)
        {
            return shader.getInfoLog();
        }
        
        glslang::TProgram program;
        program.addShader(&shader);

        result = program.link(messages);
        if (result)
        {
            return shader.getInfoLog();
        }

        glslang::GlslangToSpv(*program.getIntermediate(glslangStage), out_putSpirv);
        glslang::FinalizeProcess();
        return {};
#endif
    }

    void ShaderCompiler::_ReflectStage(ShaderStage stage, const std::vector<uint32>& spirv)
    {
        SL_LOG_TRACE("■ {}", ToStageString(stage));

        // リソースデータ取得
        spirv_cross::Compiler compiler(spirv);
        const spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        // ユニフォームバッファ
        for (const spirv_cross::Resource& resource : resources.uniform_buffers)
        {
            const auto activeBuffers = compiler.get_active_buffer_ranges(resource.id);

            if (activeBuffers.size())
            {
                const auto& name           = resource.name;
                const auto& bufferType     = compiler.get_type(resource.base_type_id);
                const uint32 memberCount   = bufferType.member_types.size();
                const uint32 binding       = compiler.get_decoration(resource.id, spv::DecorationBinding);
                const uint32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                const uint32 size          = compiler.get_declared_struct_size(bufferType);

                if (descriptorSet >= ReflectionData.descriptorSets.size())
                    ReflectionData.descriptorSets.resize(descriptorSet + 1);

                ShaderDescriptorSet& shaderDescriptorSet = ReflectionData.descriptorSets[descriptorSet];
                if (ExistUniformBuffers[descriptorSet].find(binding) == ExistUniformBuffers[descriptorSet].end())
                {
                    ShaderBuffer uniformBuffer;
                    uniformBuffer.setIndex     = descriptorSet;
                    uniformBuffer.bindingPoint = binding;
                    uniformBuffer.size         = size;
                    uniformBuffer.name         = name;
                    uniformBuffer.stage        = SHADER_STAGE_ALL;

                    ExistUniformBuffers.at(descriptorSet)[binding] = uniformBuffer;
                }
                else
                {
                    ShaderBuffer& uniformBuffer = ExistUniformBuffers.at(descriptorSet).at(binding);
                    if (size > uniformBuffer.size)
                    {
                        uniformBuffer.size = size;
                    }
                }

                shaderDescriptorSet.uniformBuffers[binding] = ExistUniformBuffers.at(descriptorSet).at(binding);

                SL_LOG_TRACE("  (set: {}, bind: {}) uniform {}", descriptorSet, binding, name);
            }
        }

        // ストレージバッファ
        for (const spirv_cross::Resource& resource : resources.storage_buffers)
        {
            const auto activeBuffers = compiler.get_active_buffer_ranges(resource.id);

            if (activeBuffers.size())
            {
                const auto& name           = resource.name;
                const auto& bufferType     = compiler.get_type(resource.base_type_id);
                const uint32 memberCount   = bufferType.member_types.size();
                const uint32 binding       = compiler.get_decoration(resource.id, spv::DecorationBinding);
                const uint32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                const uint32 size          = compiler.get_declared_struct_size(bufferType);

                if (descriptorSet >= ReflectionData.descriptorSets.size())
                    ReflectionData.descriptorSets.resize(descriptorSet + 1);

                ShaderDescriptorSet& shaderDescriptorSet = ReflectionData.descriptorSets[descriptorSet];
                if (ExistStorageBuffers[descriptorSet].find(binding) == ExistStorageBuffers[descriptorSet].end())
                {
                    ShaderBuffer storageBuffer;
                    storageBuffer.setIndex     = descriptorSet;
                    storageBuffer.bindingPoint = binding;
                    storageBuffer.size         = size;
                    storageBuffer.name         = name;
                    storageBuffer.stage        = SHADER_STAGE_ALL;

                    ExistStorageBuffers.at(descriptorSet)[binding] = storageBuffer;
                }
                else
                {
                    ShaderBuffer& storageBuffer = ExistStorageBuffers.at(descriptorSet).at(binding);
                    if (size > storageBuffer.size)
                    {
                        storageBuffer.size = size;
                    }
                }

                shaderDescriptorSet.storageBuffers[binding] = ExistStorageBuffers.at(descriptorSet).at(binding);

                SL_LOG_TRACE("  (set: {}, bind: {}) buffer {}", descriptorSet, binding, name);
            }
        }


        // イメージサンプラー
        for (const auto& resource : resources.sampled_images)
        {
            const auto& name     = resource.name;
            const auto& baseType = compiler.get_type(resource.base_type_id);
            const auto& type     = compiler.get_type(resource.type_id);
            uint32 binding       = compiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32 dimension     = baseType.image.dim;
            uint32 arraySize     = 1;

            if (!type.array.empty())
                arraySize = type.array[0];

            if (descriptorSet >= ReflectionData.descriptorSets.size())
                ReflectionData.descriptorSets.resize(descriptorSet + 1);

            ShaderImage& imageSampler = ReflectionData.descriptorSets[descriptorSet].imageSamplers[binding];
            imageSampler.bindingPoint = binding;
            imageSampler.setIndex     = descriptorSet;
            imageSampler.name         = name;
            imageSampler.stage        = stage;
            imageSampler.dimension    = dimension;
            imageSampler.arraySize    = arraySize;

            ShaderResourceDeclaration& resource = ReflectionData.resources[name]; 
            resource.name          = name;
            resource.setIndex      = descriptorSet;
            resource.registerIndex = binding;
            resource.count         = arraySize;

            SL_LOG_TRACE("  (set: {}, bind: {}) sampled_image {}", descriptorSet, binding, name);
        }

        // イメージ
        for (const auto& resource : resources.separate_images)
        {
            const auto& name     = resource.name;
            const auto& baseType = compiler.get_type(resource.base_type_id);
            const auto& type     = compiler.get_type(resource.type_id);
            uint32 binding       = compiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32 dimension     = baseType.image.dim;
            uint32 arraySize     = type.array[0];

            if (arraySize == 0)
                arraySize = 1;

            if (descriptorSet >= ReflectionData.descriptorSets.size())
                ReflectionData.descriptorSets.resize(descriptorSet + 1);

            ShaderDescriptorSet& shaderDescriptorSet = ReflectionData.descriptorSets[descriptorSet];
            auto& imageSampler        = shaderDescriptorSet.separateTextures[binding];
            imageSampler.bindingPoint = binding;
            imageSampler.setIndex     = descriptorSet;
            imageSampler.name         = name;
            imageSampler.stage        = stage;
            imageSampler.dimension    = dimension;
            imageSampler.arraySize    = arraySize;

            ShaderResourceDeclaration& resource = ReflectionData.resources[name];
            resource.name          = name;
            resource.setIndex      = descriptorSet;
            resource.registerIndex = binding;
            resource.count         = arraySize;

            SL_LOG_TRACE("  (set: {}, bind: {}) image {}", descriptorSet, binding, name);
        }

        // サンプラー
        for (const auto& resource : resources.separate_samplers)
        {
            const auto& name     = resource.name;
            const auto& baseType = compiler.get_type(resource.base_type_id);
            const auto& type     = compiler.get_type(resource.type_id);
            uint32 binding       = compiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32 dimension     = baseType.image.dim;
            uint32 arraySize     = type.array[0];

            if (arraySize == 0)
                arraySize = 1;

            if (descriptorSet >= ReflectionData.descriptorSets.size())
                ReflectionData.descriptorSets.resize(descriptorSet + 1);

            ShaderDescriptorSet& shaderDescriptorSet = ReflectionData.descriptorSets[descriptorSet];
            auto& imageSampler = shaderDescriptorSet.separateSamplers[binding];
            imageSampler.bindingPoint = binding;
            imageSampler.setIndex     = descriptorSet;
            imageSampler.name         = name;
            imageSampler.stage        = stage;
            imageSampler.dimension    = dimension;
            imageSampler.arraySize    = arraySize;

            ShaderResourceDeclaration& resource = ReflectionData.resources[name];
            resource.name          = name;
            resource.setIndex      = descriptorSet;
            resource.registerIndex = binding;
            resource.count         = arraySize;

            SL_LOG_TRACE("  (set: {}, bind: {}) sampler {}", descriptorSet, binding, name);
        }

        // ストレージイメージ
        for (const auto& resource : resources.storage_images)
        {
            const auto& name     = resource.name;
            const auto& type     = compiler.get_type(resource.type_id);
            uint32 binding       = compiler.get_decoration(resource.id, spv::DecorationBinding);
            uint32 descriptorSet = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32 dimension     = type.image.dim;
            uint32 arraySize     = type.array[0];

            if (arraySize == 0)
                arraySize = 1;

            if (descriptorSet >= ReflectionData.descriptorSets.size())
                ReflectionData.descriptorSets.resize(descriptorSet + 1);

            ShaderDescriptorSet& shaderDescriptorSet = ReflectionData.descriptorSets[descriptorSet];
            auto& imageSampler = shaderDescriptorSet.storageImages[binding];
            imageSampler.bindingPoint = binding;
            imageSampler.setIndex     = descriptorSet;
            imageSampler.name         = name;
            imageSampler.dimension    = dimension;
            imageSampler.arraySize    = arraySize;
            imageSampler.stage        = stage;

            ShaderResourceDeclaration& resource = ReflectionData.resources[name];
            resource.name          = name;
            resource.setIndex      = descriptorSet;
            resource.registerIndex = binding;
            resource.count         = arraySize;

            SL_LOG_TRACE("  (set: {}, bind: {}) storage_image {}", descriptorSet, binding, name);
        }

        // プッシュ定数
        for (const auto& resource : resources.push_constant_buffers)
        {
            const auto& name       = resource.name;
            const auto& bufferType = compiler.get_type(resource.base_type_id);
            uint32 bufferSize      = compiler.get_declared_struct_size(bufferType);
            uint32 memberCount     = bufferType.member_types.size();
            uint32 bufferOffset    = 0;

            //==================================================================================
            // NOTE: プッシュ定数のレイアウトについて
            //==================================================================================
            // 複数ステージで共有される際、プッシュ定数ブロック内のメンバ変数を分割宣言でき、
            // 分割した場合に、先頭からのオフセット指定をする必要がある。
            // 
            // 現状はステージ間で同じデータレイアウト(同じメンバ変数)を宣言されている前提で
            // 全てのステージのプッシュ定数ブロックは同じサイズでオフセットは 0 あることを要求する
            // 
            // ステージごとにオフセットを切り替えてバインドし直すのはコストがかかる？ ので
            // ステージ間でデータレイアウトを共有する方が効率的か...
            //==================================================================================
            //if (ReflectionData.pushConstantRanges.size())
            //     bufferOffset = ReflectionData.pushConstantRanges.back().offset + ReflectionData.pushConstantRanges.back().size;

            auto& pushConstantRange  = ReflectionData.pushConstantRanges.emplace_back();
            pushConstantRange.stage  = stage;
            pushConstantRange.size   = bufferSize;  // bufferSize - bufferOffset;
            pushConstantRange.offset = bufferOffset;

            if (name.empty())
                continue;

            // 以下、メンバー情報のデバッグ情報のため、実際には使用しない（※ステージ間で異なるレイアウトのリフレクションの取得を試みたが、出来なかった）
            {
                ShaderPushConstant& buffer = ReflectionData.pushConstants[name];
                buffer.name = name;
                buffer.size = bufferSize - bufferOffset;

                SL_LOG_TRACE("  (push_constant) {}", name);

                for (uint32 i = 0; i < memberCount; i++)
                {
                    const auto& memberName = compiler.get_member_name(bufferType.self, i);
                    auto& type             = compiler.get_type(bufferType.member_types[i]);
                    uint32 size            = compiler.get_declared_struct_member_size(bufferType, i);
                    uint32 offset          = compiler.type_struct_member_offset(bufferType, i) - bufferOffset;

                    std::string uniformName = std::format("{}.{}", name, memberName);

                    PushConstantMember& pushConstantMember = buffer.members[uniformName];
                    pushConstantMember.name   = uniformName;
                    pushConstantMember.type   = ToShaderDataType(type);
                    pushConstantMember.offset = offset;
                    pushConstantMember.size   = size;

                    SL_LOG_TRACE("      offset({}), size({}): {}", offset, size, uniformName);
                }
            }
        }
    }
}
