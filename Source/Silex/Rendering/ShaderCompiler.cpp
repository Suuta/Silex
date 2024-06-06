
#include "PCH.h"
#include "Rendering/ShaderCompiler.h"

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
            SL_LOG_ERROR("シェーダーファイルの読み込みに失敗しました: {}", filepath.c_str());
            return false;
        }

        in.close();
        return true;
    }

    static ShaderStage ToShaderStage(const std::string& type)
    {
        if (type == "VERTEX")   return SHADER_STAGE_VERTEX;
        if (type == "FRAGMENT") return SHADER_STAGE_FRAGMENT;
        if (type == "GEOMETRY") return SHADER_STAGE_GEOMETRY;
        if (type == "COMPUTE")  return SHADER_STAGE_COMPUTE;

        return SHADER_STAGE_MAX;
    }


#if SHADERC
    static shaderc_shader_kind ToShaderC(const ShaderStage stage)
    {
        switch (stage)
        {
            case SHADER_STAGE_VERTEX:   return shaderc_vertex_shader;
            case SHADER_STAGE_FRAGMENT: return shaderc_fragment_shader;
            case SHADER_STAGE_COMPUTE:  return shaderc_compute_shader;
            case SHADER_STAGE_GEOMETRY: return shaderc_geometry_shader;
        }

        return {};
    }
#else
    static EShLanguage ToGlslang(const ShaderStage stage)
    {
        switch (stage)
        {
            case SHADER_STAGE_VERTEX:    return EShLangVertex;
            case SHADER_STAGE_FRAGMENT:  return EShLangFragment;
            case SHADER_STAGE_COMPUTE:   return EShLangCompute;
            case SHADER_STAGE_GEOMETRY:  return EShLangGeometry;
        }

        return {};
    }
#endif


    // ステージ間共有バッファ保存用変数  ※リフレクション時のバッファ複数回定義を防ぐ
    // map<descriptorset_index, map<bind_index, buffer>>
    static std::unordered_map<uint32, std::unordered_map<uint32, ShaderBuffer>> ExistUniformBuffers;
    static std::unordered_map<uint32, std::unordered_map<uint32, ShaderBuffer>> ExistStorageBuffers;
    static ShaderReflectionData                                                 ReflectionData;

    ShaderCompiler* ShaderCompiler::Get() const
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

    std::unordered_map<ShaderStage, std::string> ShaderCompiler::_SplitStages(const std::string& source)
    {
        std::unordered_map<ShaderStage, std::string> shaderSources;

        uint64 keywordLength = strlen("#pragma");
        uint64 pos = source.find("#pragma", 0);

        while (pos != std::string::npos)
        {
            uint64 eol = source.find_first_of("\r\n", pos);
            SL_ASSERT(eol != std::string::npos, "シンタックスエラー: シェーダーステージ指定が見つかりません");

            uint64 begin = pos + keywordLength + 1;
            std::string type = source.substr(begin, eol - begin);

            SL_ASSERT(type == "VERTEX" || type == "FRAGMENT" || type == "GEOMETRY" || type == "COMPUTE", "無効なシェーダーステージです");

            uint64 nextLinePos = source.find_first_not_of("\r\n", eol);
            pos = source.find("#pragma", nextLinePos);

            shaderSources[ToShaderStage(type)] = source.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
        }

        return shaderSources;
    }

    std::string ShaderCompiler::_CompileStage(ShaderStage stage, const std::string& source, std::vector<uint32>& out_putSpirv, const std::string& filepath)
    {
#if SHADERC
        shaderc::Compiler compiler;

        // プリプロセス: SPIR-Vテキスト
        const shaderc::PreprocessedSourceCompilationResult preProcess = compiler.PreprocessGlsl(source, ToShaderC(stage), filepath.c_str(), {});
        if (preProcess.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            SL_LOG_WARN("fail to PreprocessGlsl: {}", filepath.c_str());
        }

        std::string preProcessedSource = std::string(preProcess.begin(), preProcess.end());

        // コンパイル: SPIR-Vバイナリ
        shaderc::CompileOptions options;
        options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);
        options.SetWarningsAsErrors();

        const shaderc::SpvCompilationResult compileResult = compiler.CompileGlslToSpv(preProcessedSource, ToShaderC(stage), filepath.c_str(), options);
        if (compileResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            return compileResult.GetErrorMessage();
        }

        out_putSpirv = std::vector<uint32>(compileResult.begin(), compileResult.end());
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
#endif

        return {};
    }

    void ShaderCompiler::_ReflectStage(ShaderStage stage, const std::vector<uint32>& spirv)
    {
        // リソースデータ取得
        spirv_cross::Compiler compiler(spirv);
        const spirv_cross::ShaderResources resources = compiler.get_shader_resources();

        // ユニフォームバッファ
        for (const spirv_cross::Resource& resource : resources.uniform_buffers)
        {
            const auto activeBuffers = compiler.get_active_buffer_ranges(resource.id);

            if (activeBuffers.size())
            {
                const std::string& name    = resource.name;
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

                SL_LOG_TRACE("  {} (set: {}, bind: {})", name, descriptorSet, binding);
                SL_LOG_TRACE("  members: {}", memberCount);
                SL_LOG_TRACE("  size:    {}", size);
                SL_LOG_TRACE("-------------------");
            }
        }

        // ストレージバッファ
        for (const spirv_cross::Resource& resource : resources.storage_buffers)
        {
            const auto activeBuffers = compiler.get_active_buffer_ranges(resource.id);

            if (activeBuffers.size())
            {
                const std::string& name    = resource.name;
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

                SL_LOG_TRACE("  {} (set: {}, bind: {})", name, descriptorSet, binding);
                SL_LOG_TRACE("  members: {}", memberCount);
                SL_LOG_TRACE("  size:    {}", size);
                SL_LOG_TRACE("-------------------");
            }
        }

        // プッシュ定数

        // サンプルイメージ

        // イメージ

        // サンプラー

        // ストレージイメージ
    }
}
