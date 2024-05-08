
#pragma once

#include "Core/Core.h"

#include <glad/glad.h>
#include <iostream>
#include <filesystem>
#include <fstream>


#define LOG_SHADER_UNIFORMS 1


namespace Silex
{
    class Shader : public Object
    {
        SL_DECLARE_CLASS(Shader, Object)

    public:

        Shader()
        {}

        Shader(const char* filePath)
        {
            Load(filePath);
        }

        void Load(const char* filePath)
        {
            std::filesystem::path path(filePath);
            SL_LOG_TRACE("LoadShader: {}", path.stem().string());


            std::vector<GLuint> ids;

            std::string rawCode = ReadShaderFile(filePath);
            std::unordered_map<GLenum, std::string> codes = SplitPerShaderStage(rawCode);

            m_ID = glCreateProgram();

            // シェーダーステージごとに 生成・コンパイル
            for (auto& [stage, code] : codes)
            {
                uint32 id = 0;
                const char* data = code.c_str();

                // コンパイル
                id = glCreateShader(stage);
                glShaderSource(id, 1, &data, NULL);
                glCompileShader(id);


                // コンパイル失敗
                int32 isCompiled = 0;
                glGetShaderiv(id, GL_COMPILE_STATUS, &isCompiled);
                if (isCompiled == GL_FALSE)
                {
                    int32 maxLength = 0;
                    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);

                    std::vector<char> infoLog(maxLength);
                    glGetShaderInfoLog(id, maxLength, &maxLength, &infoLog[0]);

                    SL_LOG_ERROR("シェーダーコンパイル失敗 {0}:\n{1}", filePath, &infoLog[0]);
                    SL_DEBUG_BREAK();
                }

                ids.push_back(id);
                glAttachShader(m_ID, id);
            }

            // リンク
            glLinkProgram(m_ID);


            // リンク後は　デタッチして削除
            for (auto id : ids)
            {
                glDetachShader(m_ID, id);
                glDeleteShader(id);
            }

            // uniform 変数取得
            RefrectUniforms();
        }


        std::string ReadShaderFile(const std::string& filepath)
        {
            std::string result;
            std::ifstream in(filepath, std::ios::in | std::ios::binary);

            if (in)
            {
                in.seekg(0, std::ios::end);
                result.resize(in.tellg());
                in.seekg(0, std::ios::beg);
                in.read(&result[0], result.size());
            }
            else
            {
                SL_ASSERT(false, "シェーダーファイルの読み込みに失敗しました");
            }

            in.close();
            return result;
        }

        GLenum ToShaderType(const std::string& type)
        {
            if (type == "VERTEX")          return GL_VERTEX_SHADER;
            if (type == "FRAGMENT")        return GL_FRAGMENT_SHADER;
            if (type == "GEOMETRY")        return GL_GEOMETRY_SHADER;
            if (type == "TESS_EVALUATION") return GL_TESS_EVALUATION_SHADER;
            if (type == "TESS_CONTROL")    return GL_TESS_CONTROL_SHADER;
            if (type == "COMPUTE")         return GL_COMPUTE_SHADER;

            return GL_NONE;
        }

        std::unordered_map<GLenum, std::string> SplitPerShaderStage(const std::string& source)
        {
            std::unordered_map<GLenum, std::string> shaderSources;

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

                shaderSources[ToShaderType(type)] = source.substr(nextLinePos, pos - (nextLinePos == std::string::npos ? source.size() - 1 : nextLinePos));
            }

            return shaderSources;
        }


        void RefrectUniforms()
        {
            GLint num_uniforms;
            glGetProgramiv(m_ID, GL_ACTIVE_UNIFORMS, &num_uniforms);

            for (int i = 0; i < num_uniforms; i++)
            {
                GLchar  uniformName[256] = {};
                GLsizei length;
                GLint   size;
                GLenum  type;

                glGetActiveUniform(m_ID, i, sizeof(uniformName), &length, &size, &type, uniformName);

                std::string name = uniformName;

                // 配列名でアクセスしたいので、[] は削除
                // 配列型のuniform変数は、先頭要素のみ変数名でもアクセスできるので、それを利用する
                // 少し違和感があるが、逆にいえば "変数名[i]" の文字列の指定でアクセスできてしまう
                uint64 pos = name.rfind("[");
                if (pos != std::string::npos)
                {
                    name.erase(pos);
                }

                GLint location = glGetUniformLocation(m_ID, name.c_str());
                m_Uniforms[name] = location;

                // ユニフォームブロックはプッシュ定数で扱うものではない(UBOでアクセス) 
                // GLchar  uniform_block_name[256] = {};
                // glGetActiveUniformBlockName(ID, i, sizeof(uniform_block_name), &length, uniform_block_name);
                // std::string unifromBlockName = uniform_block_name;
                // UniformBlocks[unifromBlockName] = i;

#if LOG_SHADER_UNIFORMS
                SL_LOG_TRACE("{}", "  -" + name);
#endif
            }
        }

        void Bind() const
        {
            glUseProgram(m_ID);
        }

        void Unbind() const
        {
            glUseProgram(0);
        }

        GLint Get(const char* name)
        {
            return m_Uniforms.at(name);
        }

        // bool
        void Set(const char* name, bool value) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniform1i(slot, (int)value);
        }

        // int32
        void Set(const char* name, int32 value) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniform1i(slot, value);
        }

        // uint32
        void Set(const char* name, uint32 value) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniform1ui(slot, value);
        }

        // float[]
        void Set(const char* name, const float* value, uint32 size = 1) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniform1fv(slot, size, value);
        }


        //==================================================
        // float
        //==================================================
        void Set(const char* name, float value) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniform1f(slot, value);
        }

        //==================================================
        // vec2
        //==================================================
        void Set(const char* name, float x, float y) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniform2f(slot, x, y);
        }

        void Set(const char* name, const glm::vec2& value) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniform2f(slot, value.x, value.y);
        }

        //==================================================
        // vec3
        //==================================================
        void Set(const char* name, float x, float y, float z) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniform3f(slot, x, y, z);
        }

        void Set(const char* name, const glm::vec3& value) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniform3f(slot, value.x, value.y, value.z);
        }

        //==================================================
        // vec4
        //==================================================
        void Set(const char* name, float x, float y, float z, float w) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniform4f(slot, x, y, z, w);
        }

        void Set(const char* name, const glm::vec4& value) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniform4f(slot, value.x, value.y, value.z, value.w);
        }


        //==================================================
        // 行列
        //==================================================
        void Set(const char* name, const glm::mat2& mat, uint32 size = 1) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniformMatrix2fv(slot, size, GL_FALSE, &mat[0][0]);
        }

        void Set(const char* name, const glm::mat3& mat, uint32 size = 1) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniformMatrix3fv(slot, size, GL_FALSE, &mat[0][0]);
        }

        void Set(const char* name, const glm::mat4& mat, uint32 size = 1) const
        {
            GLint slot = m_Uniforms.at(name);
            glUniformMatrix4fv(slot, size, GL_FALSE, &mat[0][0]);
        }


    private:

        uint32 m_ID = 0;
        std::unordered_map<std::string, uint32> m_Uniforms;
    };



    class ShaderManager
    {
    public:

        static ShaderManager* Get()
        {
            static ShaderManager manager;
            return &manager;
        }

        void Add(const char* name, const Shader& shader)
        {
            SL_ASSERT(!Shaders.contains(name), "登録済みのシェーダー名です");
            Shaders.emplace(name, shader);
        }

        Shader* Load(const char* name)
        {
            SL_ASSERT(Shaders.contains(name), "指定されたシェーダーが見つかりません");
            return &Shaders[name];
        }

    private:

        ShaderManager()               = default;
        ShaderManager(ShaderManager&) = default;


        static inline std::unordered_map<const char*, Shader> Shaders;
    };
}
