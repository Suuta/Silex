
#pragma once

#include <glad/glad.h>
#include "Rendering/RenderDefine.h"


namespace Silex
{
    namespace OpenGL
    {
        inline GLenum GLCullFace(rhi::CullFace face)
        {
            switch (face)
            {
                default: break;
                case rhi::CullFace::Back:         return GL_BACK;
                case rhi::CullFace::Front:        return GL_FRONT;
                case rhi::CullFace::FrontAndBack: return GL_FRONT_AND_BACK;
            }

            return 0;
        }

        inline GLenum GLStencilOp(rhi::StrencilOp op)
        {
            switch (op)
            {
                default: break;
                case rhi::StrencilOp::Never:    return GL_NEVER;
                case rhi::StrencilOp::Less:     return GL_LESS;
                case rhi::StrencilOp::Equal:    return GL_EQUAL;
                case rhi::StrencilOp::LEqual:   return GL_LEQUAL;
                case rhi::StrencilOp::Greater:  return GL_GREATER;
                case rhi::StrencilOp::NotEqual: return GL_NOTEQUAL;
                case rhi::StrencilOp::GEqual:   return GL_GEQUAL;
                case rhi::StrencilOp::Always:   return GL_ALWAYS;
            }

            return 0;
        }

        inline GLenum GLMeshBufferUsage(rhi::MeshBufferUsage usage)
        {
            switch (usage)
            {
                default: break;
                case rhi::MeshBufferUsage::Static:  return GL_STATIC_DRAW;
                case rhi::MeshBufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
            }

            return 0;
        }


        inline GLenum GLPrimitivepeType(rhi::PrimitiveType type)
        {
            switch (type)
            {
                default: break;
                case rhi::PrimitiveType::Line:          return GL_LINES;
                case rhi::PrimitiveType::Point:         return GL_POINTS;
                case rhi::PrimitiveType::Triangle:      return GL_TRIANGLES;
                case rhi::PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
            }

            return 0;
        }


        inline uint32 GLTextureWrap(const rhi::TextureWrap wrap)
        {
            switch (wrap)
            {
                default: return 0;

                case rhi::TextureWrap::ClampBorder:    return GL_CLAMP_TO_BORDER;
                case rhi::TextureWrap::ClampEdge:      return GL_CLAMP_TO_EDGE;
                case rhi::TextureWrap::Repeat:         return GL_REPEAT;
                case rhi::TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
            }
        }

        inline uint32 GLTextureMinFilter(const rhi::TextureFilter filter, bool mipmap)
        {
            switch (filter)
            {
                default: return 0;

                case rhi::TextureFilter::Linear:  return mipmap? GL_LINEAR_MIPMAP_LINEAR  : GL_LINEAR;
                case rhi::TextureFilter::Nearest: return mipmap? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST;
            }
        }

        inline uint32 GLTextureMagFilter(const rhi::TextureFilter filter)
        {
            switch (filter)
            {
                default: return 0;

                case rhi::TextureFilter::Linear:  return GL_LINEAR;
                case rhi::TextureFilter::Nearest: return GL_NEAREST;
            }
        }

        inline uint32 GLAttachmentType(const rhi::AttachmentType type)
        {
            switch (type)
            {
                default: return 0;

                case rhi::AttachmentType::Depth:        return GL_DEPTH_ATTACHMENT;
                case rhi::AttachmentType::DepthStencil: return GL_DEPTH_STENCIL_ATTACHMENT;
                case rhi::AttachmentType::Color:        return GL_COLOR_ATTACHMENT0; // +1 すれば ATTACHMENT1 になるので、複数の場合加算すれば良い
            }
        }

        inline uint32 GLBufferBit(const rhi::AttachmentBuffer buffer)
        {
            switch (buffer)
            {
                default: return 0;

                case rhi::AttachmentBuffer::Depth:   return GL_DEPTH_BUFFER_BIT;
                case rhi::AttachmentBuffer::Stencil: return GL_STENCIL_BUFFER_BIT;
                case rhi::AttachmentBuffer::Color:   return GL_COLOR_BUFFER_BIT;
            }
        }

        inline uint32 GLTextureType(const rhi::TextureType type)
        {
            switch (type)
            {
                default: return 0;

                case rhi::TextureType::Texture2D:      return GL_TEXTURE_2D;
                case rhi::TextureType::Texture2DArray: return GL_TEXTURE_2D_ARRAY;
                case rhi::TextureType::TextureCube:    return GL_TEXTURE_CUBE_MAP;
            }
        }

        inline uint32 GLInternalFormat(const rhi::RenderFormat format, bool sRGB = false)
        {
            switch (format)
            {
                default: return 0;

                // uint
                case rhi::RenderFormat::R8UI:    return GL_R8UI;
                case rhi::RenderFormat::R16UI:   return GL_R16UI;
                case rhi::RenderFormat::R32UI:   return GL_R32UI;

                // int
                case rhi::RenderFormat::R8I:     return GL_R8I;
                case rhi::RenderFormat::R16I:    return GL_R16I;
                case rhi::RenderFormat::R32I:    return GL_R32I;

                // int2
                case rhi::RenderFormat::RG32I :  return GL_RG32I;

                // int4
                case rhi::RenderFormat::RGBA32I: return GL_RGBA32I;

                // normalized
                case rhi::RenderFormat::RG8:     return GL_RG8;
                case rhi::RenderFormat::RGB8:    return sRGB ? GL_SRGB8        : GL_RGB8;
                case rhi::RenderFormat::RGBA8:   return sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;

                // float
                case rhi::RenderFormat::R16F:    return GL_R16F;
                case rhi::RenderFormat::R32F:    return GL_R32F;

                // float2
                case rhi::RenderFormat::RG16F:   return GL_RG16F;
                case rhi::RenderFormat::RG32F:   return GL_RG32F;

                // float3
                case rhi::RenderFormat::RGB16F:  return GL_RGB16F;
                case rhi::RenderFormat::RGB32F:  return GL_RGB32F;

                // float4
                case rhi::RenderFormat::RGBA16F: return GL_RGBA16F;
                case rhi::RenderFormat::RGBA32F: return GL_RGBA32F;

                // depth
                case rhi::RenderFormat::D32F:    return GL_DEPTH_COMPONENT32F;
                case rhi::RenderFormat::D24S8:   return GL_DEPTH24_STENCIL8;

                // HDR (bloom用)
                case rhi::RenderFormat::RG11B10F: return GL_R11F_G11F_B10F;
            }
        }

        inline uint32 GLFormat(const rhi::RenderFormat format)
        {
            switch (format)
            {
                default: return 0;

                case rhi::RenderFormat::R8UI :   return GL_RED_INTEGER;
                case rhi::RenderFormat::R16UI:   return GL_RED_INTEGER;
                case rhi::RenderFormat::R32UI:   return GL_RED_INTEGER;

                case rhi::RenderFormat::R8I:     return GL_RED_INTEGER;
                case rhi::RenderFormat::R16I:    return GL_RED_INTEGER;
                case rhi::RenderFormat::R32I:    return GL_RED_INTEGER;

                case rhi::RenderFormat::R16F:    return GL_RED;
                case rhi::RenderFormat::R32F:    return GL_RED;

                case rhi::RenderFormat::RG8:     return GL_RG;
                case rhi::RenderFormat::RG16F:   return GL_RG;
                case rhi::RenderFormat::RG32F:   return GL_RG;

                case rhi::RenderFormat::RG32I:   return GL_RG;

                case rhi::RenderFormat::RGB8:    return GL_RGB;
                case rhi::RenderFormat::RGB16F:  return GL_RGB;
                case rhi::RenderFormat::RGB32F:  return GL_RGB;

                case rhi::RenderFormat::RGBA8:   return GL_RGBA;
                case rhi::RenderFormat::RGBA16F: return GL_RGBA;
                case rhi::RenderFormat::RGBA32F: return GL_RGBA;

                case rhi::RenderFormat::RGBA32I: return GL_RGBA_INTEGER;

                case rhi::RenderFormat::D32F:    return GL_DEPTH_COMPONENT;
                case rhi::RenderFormat::D24S8:   return GL_DEPTH_STENCIL;

                case rhi::RenderFormat::RG11B10F: return GL_RGB;
            }
        }

        inline uint32 GLFormatDataType(rhi::RenderFormat format)
        {
            switch (format)
            {
                default: return GL_UNSIGNED_BYTE;

                case rhi::RenderFormat::R32I:
                case rhi::RenderFormat::RG32I:
                case rhi::RenderFormat::RGBA32I:
                    return GL_INT;

                case rhi::RenderFormat::RG8:
                case rhi::RenderFormat::RGB8:
                case rhi::RenderFormat::RGBA8:
                    return GL_UNSIGNED_BYTE;

                case rhi::RenderFormat::D24S8:
                    return GL_UNSIGNED_INT_24_8;

                case rhi::RenderFormat::RG16F:
                case rhi::RenderFormat::RG32F:
                case rhi::RenderFormat::RGB16F:
                case rhi::RenderFormat::RGB32F:
                case rhi::RenderFormat::RGBA16F:
                case rhi::RenderFormat::RGBA32F:
                case rhi::RenderFormat::D32F:
                    return GL_FLOAT;
            }
        }
    }
}
