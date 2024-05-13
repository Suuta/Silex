
#include "PCH.h"

#include "Rendering/OpenGL/OpenGLCore.h"
#include "Rendering/OpenGL/GLFramebuffer.h"


namespace Silex
{
    GLFramebuffer::GLFramebuffer(const RHI::FramebufferDesc& desc)
        : Desc(desc)
        , ID(0)
        , AttachmentTypeFlagBits(0)
        , ColorAttachmentCount(desc.ColorAttachmentCount)
        , Width(desc.Width)
        , Height(desc.Height)
        , ClearColorValue(desc.ClearColor)
    { 
        for (const RHI::FramebufferAttachmentDesc& attachment : Desc.AttachmentDescs)
        {
            AttachmentDescs.emplace_back(attachment);
        }

        Attachments.resize(AttachmentDescs.size());


        // フレームバッファ生成
        glCreateFramebuffers(1, &ID);
        glBindFramebuffer(GL_FRAMEBUFFER, ID);

        // アタッチメント追加（生成時にアタッチメントをバインドする要求があれば（デフォルトでバインドする））
        for (int i = 0; i < Attachments.size(); i++)
        {
            AddAttachment(AttachmentDescs[i], Width, Height, i);
        }
    
        // カラーバッファの描画数を指定
        if (ColorAttachmentCount > 0)
        {
            GLenum buffers[8] =
            {
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1,
                GL_COLOR_ATTACHMENT2,
                GL_COLOR_ATTACHMENT3,
                GL_COLOR_ATTACHMENT4,
                GL_COLOR_ATTACHMENT5,
                GL_COLOR_ATTACHMENT6,
                GL_COLOR_ATTACHMENT7,
            };
        
            glDrawBuffers(ColorAttachmentCount, buffers);
        }
        else
        {
            // デプスのみなら描画しない
            glDrawBuffer(GL_NONE);
        }
        
        // パラメータ検証
        SL_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer の構成が正しくありません");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    GLFramebuffer::~GLFramebuffer()
    {
        glDeleteFramebuffers(1, &ID);
    }

    void GLFramebuffer::Resize(uint32 width, uint32 height)
    {
        if (Width == width && Height == height)
            return;

        Width                  = width;
        Height                 = height;
        AttachmentTypeFlagBits = 0;

        // リソースを破棄
        if (ID)
        {
            glDeleteFramebuffers(1, &ID);
            glDeleteTextures(Attachments.size(), Attachments.data());

            Attachments.clear();
        }

        Attachments.resize(AttachmentDescs.size());


        // フレームバッファ生成
        glCreateFramebuffers(1, &ID);
        glBindFramebuffer(GL_FRAMEBUFFER, ID);

        // アタッチメント追加（生成時にアタッチメントをバインドする要求があれば（デフォルトでバインドする））

        for (int i = 0; i < Attachments.size(); i++)
        {
            AddAttachment(AttachmentDescs[i], Width, Height, i);
        }

        // カラーバッファの描画数を指定
        if (ColorAttachmentCount > 0)
        {
            GLenum buffers[8] =
            {
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1,
                GL_COLOR_ATTACHMENT2,
                GL_COLOR_ATTACHMENT3,
                GL_COLOR_ATTACHMENT4,
                GL_COLOR_ATTACHMENT5,
                GL_COLOR_ATTACHMENT6,
                GL_COLOR_ATTACHMENT7,
            };

            glDrawBuffers(ColorAttachmentCount, buffers);
        }
        else
        {
            // デプスのみなら描画しない
            glDrawBuffer(GL_NONE);
        }

        SL_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer の構成が正しくありません");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLFramebuffer::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, ID);
        glViewport(0, 0, Width, Height);
    }

    void GLFramebuffer::Unbind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLFramebuffer::BindAttachment(uint32 slot, uint32 attachmentIndex) const
    {
        glBindTextureUnit(slot, Attachments[attachmentIndex]);
    }

    void GLFramebuffer::Clear() const
    {
        glClearColor(ClearColorValue.r, ClearColorValue.g, ClearColorValue.b, ClearColorValue.a);
        glClear(AttachmentTypeFlagBits);
    }

    void GLFramebuffer::ClearColor() const
    {
        glClearColor(ClearColorValue.r, ClearColorValue.g, ClearColorValue.b, ClearColorValue.a);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void GLFramebuffer::ClearDepth() const
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void GLFramebuffer::ClearStencil() const
    {
        glClear(GL_STENCIL_BUFFER_BIT);
    }

    glm::vec4 GLFramebuffer::ReadPixelFloat(uint32 attachmentIndex, uint32 x, uint32 y)
    {
        glm::vec4 pixelData;

        const auto& desc = AttachmentDescs[attachmentIndex];
        uint32 format    = OpenGL::GLFormat(desc.Format);
        uint32 type      = OpenGL::GLFormatDataType(desc.Format);

        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
        glReadPixels(x, y, 1, 1, format, type, glm::value_ptr(pixelData));
        
        return pixelData;
    }

    glm::ivec4 GLFramebuffer::ReadPixelInt(uint32 attachmentIndex, uint32 x, uint32 y)
    {
        glm::ivec4 pixelData;

        const auto& desc = AttachmentDescs[attachmentIndex];
        uint32 format = OpenGL::GLFormat(desc.Format);
        uint32 type = OpenGL::GLFormatDataType(desc.Format);

        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);
        glReadPixels(x, y, 1, 1, format, type, glm::value_ptr(pixelData));

        return pixelData;
    }

    void GLFramebuffer::ClearAttachment(uint32 attachmentIndex, glm::vec4 value)
    {
        const auto& desc = AttachmentDescs[attachmentIndex];
        uint32 type      = OpenGL::GLFormatDataType(desc.Format);
        uint32 format    = OpenGL::GLFormat(desc.Format);

        glClearTexImage(Attachments[attachmentIndex], 0, format, type, glm::value_ptr(value));
    }

    void GLFramebuffer::ClearAttachment(uint32 attachmentIndex, glm::ivec4 value)
    {
        const auto& desc = AttachmentDescs[attachmentIndex];
        uint32 type   = OpenGL::GLFormatDataType(desc.Format);
        uint32 format = OpenGL::GLFormat(desc.Format);

        glClearTexImage(Attachments[attachmentIndex], 0, format, type, glm::value_ptr(value));
    }

    void GLFramebuffer::SetAttachmentTexture(uint32 attachmentIndex, uint32 textureID, RHI::AttachmentType type)
    {
        Attachments[attachmentIndex] = textureID;
        glNamedFramebufferTexture(ID, OpenGL::GLAttachmentType(type), Attachments[attachmentIndex], 0);
    }

    void GLFramebuffer::AddAttachment(RHI::FramebufferAttachmentDesc desc, uint32 width, uint32 height, uint32 attachmentIndex)
    {
        uint32 internalFormat = OpenGL::GLInternalFormat(desc.Format);
        uint32 attachmentType = OpenGL::GLAttachmentType(desc.AttachmentType);

        // テクスチャ生成
        switch(desc.TextureType)
        {
            default: break;

            case RHI::TextureType::Texture2D:
            {
                glCreateTextures(GL_TEXTURE_2D, 1, &Attachments[attachmentIndex]);
                glTextureStorage2D(Attachments[attachmentIndex], 1, internalFormat, width, height);
                break;
            }
            case RHI::TextureType::Texture2DArray:
            {
                glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &Attachments[attachmentIndex]);
                glTextureStorage3D(Attachments[attachmentIndex], 1, internalFormat, width, height, desc.TextureArraySize);
                break;
            }
            case RHI::TextureType::TextureCube:
            {
                SL_ASSERT(false, "現在、TextureCube はフレームバッファに割り当てることができません");
                break;
            }
        }

        // サンプリング指定
        glTextureParameteri(Attachments[attachmentIndex], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(Attachments[attachmentIndex], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(Attachments[attachmentIndex], GL_TEXTURE_WRAP_R,     GL_CLAMP_TO_EDGE);
        glTextureParameteri(Attachments[attachmentIndex], GL_TEXTURE_WRAP_S,     GL_CLAMP_TO_EDGE);
        glTextureParameteri(Attachments[attachmentIndex], GL_TEXTURE_WRAP_T,     GL_CLAMP_TO_EDGE);

        if (desc.AttachmentType == RHI::AttachmentType::Depth)
        {
            glTextureParameteri(Attachments[attachmentIndex], GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTextureParameteri(Attachments[attachmentIndex], GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        }

        // フレームバッファにバインド
        switch(desc.AttachmentType)
        {
            default: break;

            case RHI::AttachmentType::DepthStencil:
            {
                glNamedFramebufferTexture(ID, attachmentType, Attachments[attachmentIndex], 0);
                DepthAttachmentID = Attachments[attachmentIndex];

                AttachmentTypeFlagBits |= GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;

                break;
            }
            case RHI::AttachmentType::Depth:
            {
                glNamedFramebufferTexture(ID, attachmentType, Attachments[attachmentIndex], 0);
                DepthAttachmentID = Attachments[attachmentIndex];

                AttachmentTypeFlagBits |= GL_DEPTH_BUFFER_BIT;

                break;
            }
            case RHI::AttachmentType::Color:
            {
                glNamedFramebufferTexture(ID, GL_COLOR_ATTACHMENT0 + attachmentIndex, Attachments[attachmentIndex], 0);

                AttachmentTypeFlagBits |= GL_COLOR_BUFFER_BIT;

                break;
            }
        }
    }
}
