
#pragma once

#include "Core/SharedPointer.h"
#include "Rendering/RenderDefine.h"


namespace Silex
{
    class Framebuffer : public Object
    {
        SL_CLASS(Framebuffer, Object)

    public:

        virtual ~Framebuffer() {}

        virtual void Bind()   const = 0;
        virtual void Unbind() const = 0;
        virtual void Clear()  const = 0;

        virtual void ClearColor()   const = 0;
        virtual void ClearDepth()   const = 0;
        virtual void ClearStencil() const = 0;

        virtual void Resize(uint32 width, uint32 height)                                                      = 0;
        virtual void BindAttachment(uint32 slot, uint32 attachmentIndex) const                                = 0;
        virtual void SetAttachmentTexture(uint32 attachmentIndex, uint32 textureID, RHI::AttachmentType type) = 0;

        virtual void ClearAttachment(uint32 attachmentIndex, glm::ivec4 value) = 0; 
        virtual void ClearAttachment(uint32 attachmentIndex, glm::vec4 value)  = 0;


        virtual glm::vec4  ReadPixelFloat(uint32 attachmentIndex, uint32 x, uint32 y) = 0;
        virtual glm::ivec4 ReadPixelInt(uint32 attachmentIndex, uint32 x, uint32 y)   = 0;

        virtual uint32 GetWidth()  const = 0;
        virtual uint32 GetHeight() const = 0;

        virtual uint32 GetID()                        const = 0;
        virtual uint32 GetAttachmentID(int index = 0) const = 0;
        virtual uint32 GetDepthID()                   const = 0;

    public:

        static Shared<Framebuffer> Create(const RHI::FramebufferDesc& desc);
    };
}