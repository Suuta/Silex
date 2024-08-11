
#include "PCH.h"

#include "Rendering/OpenGL/GLTexture.h"
#include "Asset/TextureReader.h"
#include <glad/glad.h>


namespace Silex
{
    GLTexture2D::GLTexture2D(const rhi::TextureDesc& desc)
        : Desc(desc)
        , ID(0)
    {
        auto internaFormat = OpenGL::GLInternalFormat(Desc.Format);
        auto wrap          = OpenGL::GLTextureWrap(Desc.Wrap);
        auto minFilter     = OpenGL::GLTextureMinFilter(Desc.Filter, Desc.GenMipmap);
        auto magFilter     = OpenGL::GLTextureMagFilter(Desc.Filter);

        glCreateTextures(GL_TEXTURE_2D, 1, &ID);

        glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, magFilter);
        glTextureParameteri(ID, GL_TEXTURE_WRAP_R,     wrap);
        glTextureParameteri(ID, GL_TEXTURE_WRAP_S,     wrap);
        glTextureParameteri(ID, GL_TEXTURE_WRAP_T,     wrap);

        uint32 mipCount = 1;
        glTextureStorage2D(ID, mipCount, internaFormat, Desc.Width, Desc.Height);
    }

    GLTexture2D::GLTexture2D(const rhi::TextureDesc& desc, const std::string& filePath)
        : Desc(desc)
        , ID(0)
    {
        // テクスチャファイル読み込み
        TextureReader reader;
        byte* pixels     = reader.Read8bit(filePath.c_str());
        uint32 width     = reader.data.width;
        uint32 height    = reader.data.height;
        uint32 component = reader.data.channels;

        GLenum format         = 0;
        GLenum internalFormat = 0;

        if (component == 1)
        {
            format         = GL_RED;
            internalFormat = GL_R8;
        }
        else if (component == 2)
        {
            format         = GL_RG;
            internalFormat = GL_RG8;
        }
        else if (component == 3)
        {
            format         = GL_RGB;
            internalFormat = GL_RGB8;
        }
        else if (component == 4)
        {
            format         = GL_RGBA;
            internalFormat = GL_RGBA8;
        }

        // OpenGLフォーマット変換
        auto wrap           = OpenGL::GLTextureWrap(Desc.Wrap);
        auto minFilter      = OpenGL::GLTextureMinFilter(Desc.Filter, Desc.GenMipmap);
        auto magFilter      = OpenGL::GLTextureMagFilter(Desc.Filter);
        auto dataType       = OpenGL::GLFormatDataType(Desc.Format);

        // テクスチャ生成
        glCreateTextures(GL_TEXTURE_2D, 1, &ID);

        // サンプリングパラメータ
        glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, magFilter);
        glTextureParameteri(ID, GL_TEXTURE_WRAP_R,     wrap);
        glTextureParameteri(ID, GL_TEXTURE_WRAP_S,     wrap);
        glTextureParameteri(ID, GL_TEXTURE_WRAP_T,     wrap);

        // データ割り当て
        uint32 mipCount = rhi::CalculateMipLevels(width, height);
        uint32 mipLevel = 0;
        glTextureStorage2D(ID, mipCount, internalFormat, width, height);
        glTextureSubImage2D(ID, mipLevel, 0, 0, width, height, format, dataType, pixels);

        // ミップマップ生成
        if (Desc.GenMipmap)
            glGenerateTextureMipmap(ID);
    }

    GLTexture2DArray::GLTexture2DArray(const rhi::TextureDesc& desc, uint32 size)
        : Desc(desc)
        , ID(0)
    {
    }

    GLTextureCube::GLTextureCube(const rhi::TextureDesc& desc, const std::string& filePath)
        : Desc(desc)
        , ID(0)
    {
    }




    GLTextureCube::~GLTextureCube()
    {
        glDeleteTextures(1, &ID);
    }

    GLTexture2D::~GLTexture2D()
    {
        glDeleteTextures(1, &ID);
    }

    GLTexture2DArray::~GLTexture2DArray()
    {
        glDeleteTextures(1, &ID);
    }




    void GLTexture2D::Bind(uint32 slot) const
    {
        glBindTextureUnit(slot, ID);
    }

    void GLTexture2DArray::Bind(uint32 slot) const
    {
        glBindTextureUnit(slot, ID);
    }

    void GLTextureCube::Bind(uint32 slot) const
    {
        glBindTextureUnit(slot, ID);
    }
}
