
#pragma once

#include "Asset/Asset.h"
#include "Rendering/RenderDefine.h"
#include "Core/SharedPointer.h"


namespace Silex
{
    //==================================================
    // テクスチャ 基底クラス
    //==================================================
    class Texture : public Asset
    {
        SL_DECLARE_CLASS(Texture, Asset)

    public:

        virtual ~Texture() = default;

        virtual void              Bind(uint32 slot = 0) const = 0;
        virtual RHI::RenderFormat GetFormat()           const = 0;
        virtual uint32            GetWidth()            const = 0;
        virtual uint32            GetHeight()           const = 0;
        virtual uint32            GetID()               const = 0;
        virtual uint32            GetSize()             const = 0;
    };


    //==================================================
    // テクスチャ2D
    //==================================================
    class Texture2D : public Texture
    {
        SL_DECLARE_CLASS(Texture2D, Texture)

    public:

        static Texture2D*        Create(const RHI::TextureDesc& desc);
        static Shared<Texture2D> Create(const RHI::TextureDesc& desc, const std::string& path);
    };


    //==================================================
    // テクスチャ配列
    //==================================================
    class Texture2DArray : public Texture
    {
        SL_DECLARE_CLASS(Texture2DArray, Texture)

    public:

        static Texture2DArray* Create(const RHI::TextureDesc& desc, uint32 size);
    };


    //==================================================
    // キューブマップ
    //==================================================
    class TextureCube : public Texture
    {
        SL_DECLARE_CLASS(TextureCube, Texture)

    public:

        static TextureCube* Create(const RHI::TextureDesc& desc, const std::string& filePath);
    };
}