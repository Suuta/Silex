
#pragma once

#include "Rendering/Texture.h"
#include "Rendering/OpenGL/OpenGLCore.h"


namespace Silex
{
    //==================================================
    // OpenGL テクスチャ2D
    //==================================================
    class GLTexture2D : public Texture2D
    {
        SL_CLASS(GLTexture2D, Texture2D)

    public:

        GLTexture2D(const rhi::TextureDesc& desc);
        GLTexture2D(const rhi::TextureDesc& desc, const std::string& filePath);
        ~GLTexture2D();

    public:

        void Bind(uint32 slot = 0) const override;

        rhi::RenderFormat GetFormat() const override { return Desc.Format; }
        uint32            GetWidth()  const override { return Desc.Width;  }
        uint32            GetHeight() const override { return Desc.Height; }
        uint32            GetID()     const override { return ID;          }
        uint32            GetSize()   const override { return 1;           }

    private:

        rhi::TextureDesc  Desc;
        uint32            ID;
    };

    //==================================================
    // OpenGL テクスチャ2D配列
    //==================================================
    class GLTexture2DArray : public Texture2DArray
    {
    private:
        SL_CLASS(GLTexture2DArray, Texture2DArray)

    public:

        GLTexture2DArray(const rhi::TextureDesc& desc, uint32 size);
        ~GLTexture2DArray();

        void Bind(uint32 slot = 0) const override;

        rhi::RenderFormat GetFormat() const override { return Desc.Format; }
        uint32            GetWidth()  const override { return Desc.Width;  }
        uint32            GetHeight() const override { return Desc.Height; }
        uint32            GetID()     const override { return ID;          }
        uint32            GetSize()   const override { return Size;        }

    private:

        rhi::TextureDesc Desc;
        uint32           ID;
        uint32           Size;
    };

    //==================================================
    // OpenGL テクスチャキューブ
    //==================================================
    class GLTextureCube : public TextureCube
    {
        SL_CLASS(GLTextureCube, TextureCube)

    public:

        GLTextureCube(const rhi::TextureDesc& desc, const std::string& filePath);
        ~GLTextureCube();

        void Bind(uint32 slot = 0) const override;

        rhi::RenderFormat GetFormat() const override { return Desc.Format; }
        uint32            GetWidth()  const override { return Desc.Width;  }
        uint32            GetHeight() const override { return Desc.Height; }
        uint32            GetID()     const override { return ID;          }
        uint32            GetSize()   const override { return 1;           }

    private:

        rhi::TextureDesc Desc;
        uint32           ID;
    };
}