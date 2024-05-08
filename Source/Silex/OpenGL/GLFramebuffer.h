
#pragma once

#include "Renderer/Framebuffer.h"
#include "Renderer/Texture.h"


namespace Silex
{
    class GLFramebuffer : public Framebuffer
    {
        SL_DECLARE_CLASS(GLFramebuffer, Framebuffer)

    public:

        GLFramebuffer(const RHI::FramebufferDesc& desc);
        ~GLFramebuffer();

        void Bind()   const override;
        void Unbind() const override;
        void Clear()  const override;

        void ClearColor()   const override;
        void ClearDepth()   const override;
        void ClearStencil() const override;

        void Resize(uint32 width, uint32 height)                                                      override;
        void BindAttachment(uint32 slot, uint32 attachmentIndex) const                                override;
        void SetAttachmentTexture(uint32 attachmentIndex, uint32 textureID, RHI::AttachmentType type) override;

        void ClearAttachment(uint32 attachmentIndex, glm::ivec4 value) override;
        void ClearAttachment(uint32 attachmentIndex, glm::vec4 value)  override;

        glm::vec4  ReadPixelFloat(uint32 attachmentIndex, uint32 x, uint32 y) override;
        glm::ivec4 ReadPixelInt(uint32 attachmentIndex, uint32 x, uint32 y)   override;

        uint32 GetWidth()  const override { return Width;  }
        uint32 GetHeight() const override { return Height; }

        uint32 GetID()                        const override { return ID;                 }
        uint32 GetAttachmentID(int index = 0) const override { return Attachments[index]; }
        uint32 GetDepthID()                   const override { return DepthAttachmentID;  }

    private:

        void AddAttachment(
            RHI::FramebufferAttachmentDesc desc,
            uint32                         width,
            uint32                         height,
            uint32                         attachmentIndex = 0
        );

    private:

        RHI::FramebufferDesc Desc;
        uint32               ID;

        std::vector<uint32>                         Attachments;
        std::vector<RHI::FramebufferAttachmentDesc> AttachmentDescs;

        uint32 AttachmentTypeFlagBits;
        uint32 ColorAttachmentCount;
        uint32 DepthAttachmentID;

        uint32 Width;
        uint32 Height;

        glm::vec4 ClearColorValue;
    };
}

