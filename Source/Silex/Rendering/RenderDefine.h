
#pragma once

#include "Core/Core.h"


namespace Silex
{
    namespace rhi
    {
        //==================================================
        // 列挙型
        //==================================================
        enum class RendererAPI
        {
            None,

            OpneGL,
        };

        enum class PrimitiveType
        {
            Point,
            Line,
            Triangle,
            TriangleStrip,
        };

        enum class MeshBufferUsage
        {
            None,

            Static,
            Dynamic,
        };

        enum class TextureWrap
        {
            None,

            ClampBorder,
            ClampEdge,
            Repeat,
            MirroredRepeat,
        };

        enum class AttachmentType
        {
            None,

            DepthStencil,
            Depth,
            Color,
        };

        enum class AttachmentBuffer
        {
            None,

            Stencil,
            Depth,
            Color,
        };

        enum class TextureFilter
        {
            None,

            Linear,
            Nearest,
        };

        enum class RenderFormat
        {
            None,

            R8UI,
            R16UI,
            R32UI,

            R8I,
            R16I,
            R32I,

            // 浮動小数点は 16 / 32 ビットのみ
            R16F,
            R32F,

            RG8,
            RG16F,
            RG32F,

            RG32I,

            RGB8,
            RGB16F,
            RGB32F,

            RGBA8,
            RGBA16F,
            RGBA32F,

            RGBA32I,

            D32F,
            D24S8,

            RG11B10F,
        };

        enum class StrencilOp
        {
            None,

            Never,
            Less,
            LEqual,
            Greater,
            GEqual,
            Equal,
            NotEqual,
            Always,
        };

        enum class CullFace
        {
            None,

            Back,
            Front,
            FrontAndBack,
        };

        enum class ShaderDataType
        {
            None,

            Float,
            Float2,
            Float3,
            Float4,
            Mat3,
            Mat4,
            Int,
            Int2,
            Int3,
            Int4,
            Bool,
        };

        enum class ShaderStageType
        {
            None,

            Vertex,
            Fragment,
            Geometry,
            TessellationControll,
            TessellationEvaluation,
            Compute,
        };

        enum class TextureType
        {
            Texture2D,
            Texture2DArray,
            TextureCube,
        };

        //==================================================
        // リソース仕様
        //==================================================

        // テクスチャ
        struct TextureDesc
        {
            TextureType   Type;
            uint32        Width;
            uint32        Height;
            RenderFormat  Format;
            TextureWrap   Wrap;
            TextureFilter Filter;
            uint32        Size      = 1; // TextureArrayの場合は配列数
            bool          GenMipmap = false;
            bool          sRGB      = false;
        };

        // フレームバッファアタッチメント（テクスチャ）
        struct FramebufferAttachmentDesc
        {
            RenderFormat   Format;
            AttachmentType AttachmentType;
            TextureType    TextureType;
            uint32         TextureArraySize = 1;
        };

        // フレームバッファ
        struct FramebufferDesc
        {
            uint32                                 Width;
            uint32                                 Height;
            glm::vec4                              ClearColor;
            std::vector<FramebufferAttachmentDesc> AttachmentDescs;
            uint32                                 ColorAttachmentCount;

            // アタッチメントテクスチャを後からバインドする場合は true
            //bool                                 BindAttachmentOnCreate = true;
        };


        // デバイス情報
        struct DeviceInfo
        {
            std::string Name;
        };


        //==================================================
        // 変換関数
        //==================================================
        inline uint32 ShaderDataTypeSize(ShaderDataType type)
        {
            switch (type)
            {
                default: break;

                case ShaderDataType::Float:    return 4;
                case ShaderDataType::Float2:   return 4 * 2;
                case ShaderDataType::Float3:   return 4 * 3;
                case ShaderDataType::Float4:   return 4 * 4;
                case ShaderDataType::Mat3:     return 4 * 3 * 3;
                case ShaderDataType::Mat4:     return 4 * 4 * 4;
                case ShaderDataType::Int:      return 4;
                case ShaderDataType::Int2:     return 4 * 2;
                case ShaderDataType::Int3:     return 4 * 3;
                case ShaderDataType::Int4:     return 4 * 4;
                case ShaderDataType::Bool:     return 4;
            }

            return 0;
        }

        inline uint32 ShaderDataTypeComponentSize(ShaderDataType type)
        {
            switch (type)
            {
                default: break;

                case ShaderDataType::Float:    return 1;
                case ShaderDataType::Float2:   return 2;
                case ShaderDataType::Float3:   return 3;
                case ShaderDataType::Float4:   return 4;
                case ShaderDataType::Mat3:     return 3;
                case ShaderDataType::Mat4:     return 4;
                case ShaderDataType::Int:      return 1;
                case ShaderDataType::Int2:     return 2;
                case ShaderDataType::Int3:     return 3;
                case ShaderDataType::Int4:     return 4;
                case ShaderDataType::Bool:     return 1;
            }

            return 0;
        }

        // テクスチャサイズからミップマップレベルを計算
        inline uint32 CalculateMipLevels(uint32 width, uint32 height)
        {
            uint32 levels = 1;
            while ((width | height) >> levels)
                levels++;

            return levels;
        }

        // 深度フォーマットかどうかを判定する
        inline bool IsDepthFormat(RenderFormat format)
        {
            switch (format)
            {
                case RenderFormat::D24S8:
                case RenderFormat::D32F:
                    return true;

                default:
                    return false;
            }
        }
    }
}