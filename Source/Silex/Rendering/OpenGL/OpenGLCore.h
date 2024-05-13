
#pragma once

#include <glad/glad.h>
#include "Rendering/RenderDefine.h"


namespace Silex
{
    namespace OpenGL
    {
        inline GLenum GLCullFace(RHI::CullFace face)
        {
            switch (face)
            {
                default: break;
                case RHI::CullFace::Back:         return GL_BACK;
                case RHI::CullFace::Front:        return GL_FRONT;
                case RHI::CullFace::FrontAndBack: return GL_FRONT_AND_BACK;
            }

            return 0;
        }

        inline GLenum GLStencilOp(RHI::StrencilOp op)
        {
            switch (op)
            {
                default: break;
                case RHI::StrencilOp::Never:    return GL_NEVER;
                case RHI::StrencilOp::Less:     return GL_LESS;
                case RHI::StrencilOp::Equal:    return GL_EQUAL;
                case RHI::StrencilOp::LEqual:   return GL_LEQUAL;
                case RHI::StrencilOp::Greater:  return GL_GREATER;
                case RHI::StrencilOp::NotEqual: return GL_NOTEQUAL;
                case RHI::StrencilOp::GEqual:   return GL_GEQUAL;
                case RHI::StrencilOp::Always:   return GL_ALWAYS;
            }

            return 0;
        }

        inline GLenum GLMeshBufferUsage(RHI::MeshBufferUsage usage)
        {
            switch (usage)
            {
                default: break;
                case RHI::MeshBufferUsage::Static:  return GL_STATIC_DRAW;
                case RHI::MeshBufferUsage::Dynamic: return GL_DYNAMIC_DRAW;
            }

            return 0;
        }


        inline GLenum GLPrimitivepeType(RHI::PrimitiveType type)
        {
            switch (type)
            {
                default: break;
                case RHI::PrimitiveType::Line:          return GL_LINES;
                case RHI::PrimitiveType::Point:         return GL_POINTS;
                case RHI::PrimitiveType::Triangle:      return GL_TRIANGLES;
                case RHI::PrimitiveType::TriangleStrip: return GL_TRIANGLE_STRIP;
            }

            return 0;
        }


        inline uint32 GLTextureWrap(const RHI::TextureWrap wrap)
        {
            switch (wrap)
            {
                default: return 0;

                case RHI::TextureWrap::ClampBorder:    return GL_CLAMP_TO_BORDER;
                case RHI::TextureWrap::ClampEdge:      return GL_CLAMP_TO_EDGE;
                case RHI::TextureWrap::Repeat:         return GL_REPEAT;
                case RHI::TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT;
            }
        }

        inline uint32 GLTextureMinFilter(const RHI::TextureFilter filter, bool mipmap)
        {
            switch (filter)
            {
                default: return 0;

                case RHI::TextureFilter::Linear:  return mipmap? GL_LINEAR_MIPMAP_LINEAR  : GL_LINEAR;
                case RHI::TextureFilter::Nearest: return mipmap? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST;
            }
        }

        inline uint32 GLTextureMagFilter(const RHI::TextureFilter filter)
        {
            switch (filter)
            {
                default: return 0;

                case RHI::TextureFilter::Linear:  return GL_LINEAR;
                case RHI::TextureFilter::Nearest: return GL_NEAREST;
            }
        }

        inline uint32 GLAttachmentType(const RHI::AttachmentType type)
        {
            switch (type)
            {
                default: return 0;

                case RHI::AttachmentType::Depth:        return GL_DEPTH_ATTACHMENT;
                case RHI::AttachmentType::DepthStencil: return GL_DEPTH_STENCIL_ATTACHMENT;
                case RHI::AttachmentType::Color:        return GL_COLOR_ATTACHMENT0; // +1 すれば ATTACHMENT1 になるので、複数の場合加算すれば良い
            }
        }

        inline uint32 GLBufferBit(const RHI::AttachmentBuffer buffer)
        {
            switch (buffer)
            {
                default: return 0;

                case RHI::AttachmentBuffer::Depth:   return GL_DEPTH_BUFFER_BIT;
                case RHI::AttachmentBuffer::Stencil: return GL_STENCIL_BUFFER_BIT;
                case RHI::AttachmentBuffer::Color:   return GL_COLOR_BUFFER_BIT;
            }
        }

        inline uint32 GLTextureType(const RHI::TextureType type)
        {
            switch (type)
            {
                default: return 0;

                case RHI::TextureType::Texture2D:      return GL_TEXTURE_2D;
                case RHI::TextureType::Texture2DArray: return GL_TEXTURE_2D_ARRAY;
                case RHI::TextureType::TextureCube:    return GL_TEXTURE_CUBE_MAP;
            }
        }

        inline uint32 GLInternalFormat(const RHI::RenderFormat format, bool sRGB = false)
        {
            switch (format)
            {
                default: return 0;

                // uint
                case RHI::RenderFormat::R8UI:    return GL_R8UI;
                case RHI::RenderFormat::R16UI:   return GL_R16UI;
                case RHI::RenderFormat::R32UI:   return GL_R32UI;

                // int
                case RHI::RenderFormat::R8I:     return GL_R8I;
                case RHI::RenderFormat::R16I:    return GL_R16I;
                case RHI::RenderFormat::R32I:    return GL_R32I;

                // int2
                case RHI::RenderFormat::RG32I :  return GL_RG32I;

                // int4
                case RHI::RenderFormat::RGBA32I: return GL_RGBA32I;

                // normalized
                case RHI::RenderFormat::RG8:     return GL_RG8;
                case RHI::RenderFormat::RGB8:    return sRGB ? GL_SRGB8        : GL_RGB8;
                case RHI::RenderFormat::RGBA8:   return sRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;

                // float
                case RHI::RenderFormat::R16F:    return GL_R16F;
                case RHI::RenderFormat::R32F:    return GL_R32F;

                // float2
                case RHI::RenderFormat::RG16F:   return GL_RG16F;
                case RHI::RenderFormat::RG32F:   return GL_RG32F;

                // float3
                case RHI::RenderFormat::RGB16F:  return GL_RGB16F;
                case RHI::RenderFormat::RGB32F:  return GL_RGB32F;

                // float4
                case RHI::RenderFormat::RGBA16F: return GL_RGBA16F;
                case RHI::RenderFormat::RGBA32F: return GL_RGBA32F;

                // depth
                case RHI::RenderFormat::D32F:    return GL_DEPTH_COMPONENT32F;
                case RHI::RenderFormat::D24S8:   return GL_DEPTH24_STENCIL8;

                // HDR (bloom用)
                case RHI::RenderFormat::RG11B10F: return GL_R11F_G11F_B10F;
            }
        }

        inline uint32 GLFormat(const RHI::RenderFormat format)
        {
            switch (format)
            {
                default: return 0;

                case RHI::RenderFormat::R8UI :   return GL_RED_INTEGER;
                case RHI::RenderFormat::R16UI:   return GL_RED_INTEGER;
                case RHI::RenderFormat::R32UI:   return GL_RED_INTEGER;

                case RHI::RenderFormat::R8I:     return GL_RED_INTEGER;
                case RHI::RenderFormat::R16I:    return GL_RED_INTEGER;
                case RHI::RenderFormat::R32I:    return GL_RED_INTEGER;

                case RHI::RenderFormat::R16F:    return GL_RED;
                case RHI::RenderFormat::R32F:    return GL_RED;

                case RHI::RenderFormat::RG8:     return GL_RG;
                case RHI::RenderFormat::RG16F:   return GL_RG;
                case RHI::RenderFormat::RG32F:   return GL_RG;

                case RHI::RenderFormat::RG32I:   return GL_RG;

                case RHI::RenderFormat::RGB8:    return GL_RGB;
                case RHI::RenderFormat::RGB16F:  return GL_RGB;
                case RHI::RenderFormat::RGB32F:  return GL_RGB;

                case RHI::RenderFormat::RGBA8:   return GL_RGBA;
                case RHI::RenderFormat::RGBA16F: return GL_RGBA;
                case RHI::RenderFormat::RGBA32F: return GL_RGBA;

                case RHI::RenderFormat::RGBA32I: return GL_RGBA_INTEGER;

                case RHI::RenderFormat::D32F:    return GL_DEPTH_COMPONENT;
                case RHI::RenderFormat::D24S8:   return GL_DEPTH_STENCIL;

                case RHI::RenderFormat::RG11B10F: return GL_RGB;
            }
        }

        inline uint32 GLFormatDataType(RHI::RenderFormat format)
        {
            switch (format)
            {
                default: return GL_UNSIGNED_BYTE;

                case RHI::RenderFormat::R32I:
                case RHI::RenderFormat::RG32I:
                case RHI::RenderFormat::RGBA32I:
                    return GL_INT;

                case RHI::RenderFormat::RG8:
                case RHI::RenderFormat::RGB8:
                case RHI::RenderFormat::RGBA8:
                    return GL_UNSIGNED_BYTE;

                case RHI::RenderFormat::D24S8:
                    return GL_UNSIGNED_INT_24_8;

                case RHI::RenderFormat::RG16F:
                case RHI::RenderFormat::RG32F:
                case RHI::RenderFormat::RGB16F:
                case RHI::RenderFormat::RGB32F:
                case RHI::RenderFormat::RGBA16F:
                case RHI::RenderFormat::RGBA32F:
                case RHI::RenderFormat::D32F:
                    return GL_FLOAT;
            }
        }
    }
}
