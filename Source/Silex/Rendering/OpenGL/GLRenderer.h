
#pragma once

#include "Rendering/Renderer.h"

struct GLFWwindow;

namespace Silex
{
    class GLRenderer final : public RendererPlatform
    {
        SL_DECLARE_CLASS(GLRenderer, RendererPlatform)

    public:

        void Init()                              override;
        void Shutdown()                          override;
        void BeginFrame()                        override;
        void EndFrame()                          override;
        void Resize(uint32 width, uint32 height) override;

    public:

        void SetDefaultFramebuffer() override;

        void SetShaderTexture(uint32 slot, uint32 id)                   override;
        void SetViewport(uint32 width, uint32 height)                   override;
        void SetStencilFunc(RHI::StrencilOp op, int32 ref, uint32 mask) override;
        void SetCullFace(RHI::CullFace face)                            override;
        void EnableBlend(bool enable)                                   override;

        void BlitFramebuffer(const Shared<Framebuffer>& src, const Shared<Framebuffer>& dest, RHI::AttachmentBuffer buffer) override;

        void Draw(RHI::PrimitiveType type, uint64 numVertices)                                   override;
        void DrawInstance(RHI::PrimitiveType type, uint64 numVertices, uint64 numInstance)       override;

        void DrawIndexed(RHI::PrimitiveType type, uint64 numIndices)                             override;
        void DrawIndexedInstance(RHI::PrimitiveType type, uint64 numIndices, uint64 numInstance) override;

    private:

        std::unordered_set<std::string> m_Extentions;
        std::string                     m_DeviceName;
        GLFWwindow*                     m_Window;
    };
}