#pragma once

#include "Core/SharedPointer.h"
#include "Core/TaskQueue.h"
#include "Rendering/RenderDefine.h"


namespace Silex
{
    class Material;
    class Mesh;
    class Texture2D;
    class Framebuffer;

    class RendererPlatform : public Object
    {
        SL_CLASS(RendererPlatform, Object)

    public:

        virtual ~RendererPlatform() = default;

    public:

        static RendererPlatform* Create();

        virtual void Init()                              = 0;
        virtual void Shutdown()                          = 0;
        virtual void BeginFrame()                        = 0;
        virtual void EndFrame()                          = 0;
        virtual void Resize(uint32 width, uint32 height) = 0;

        virtual void SetShaderTexture(uint32 slot, uint32 id) = 0;
        virtual void SetViewport(uint32 width, uint32 height) = 0;

        virtual void SetStencilFunc(RHI::StrencilOp op, int32 ref, uint32 mask) = 0;
        virtual void SetCullFace(RHI::CullFace face) = 0;

        virtual void EnableBlend(bool enable) = 0;

        virtual void SetDefaultFramebuffer() = 0;
        virtual void BlitFramebuffer(const Shared<Framebuffer>& src, const Shared<Framebuffer>& dest, RHI::AttachmentBuffer buffer) = 0;

        virtual void Draw(RHI::PrimitiveType type, uint64 numVertices)                             = 0;
        virtual void DrawInstance(RHI::PrimitiveType type, uint64 numVertices, uint64 numInstance) = 0;

        virtual void DrawIndexed(RHI::PrimitiveType type, uint64 numIndices)                             = 0;
        virtual void DrawIndexedInstance(RHI::PrimitiveType type, uint64 numIndices, uint64 numInstance) = 0;
    };


    class Renderer : public Object
    {
        SL_CLASS(Renderer, Object)

    public:

        static Renderer* Get();

    public:

        void Init();
        void Shutdown();

        void BeginFrame();
        void EndFrame();
        void Resize(uint32 width, uint32 height);

    public:

        void SetShaderTexture(uint32 slot, uint32 id);
        void SetViewport(uint32 width, uint32 height);

        void SetStencilFunc(RHI::StrencilOp op, int32 ref, uint32 mask);
        void SetCullFace(RHI::CullFace face);

        void EnableBlend(bool enable);

        void SetDefaultFramebuffer();
        void BlitFramebuffer(const Shared<Framebuffer>& src, const Shared<Framebuffer>& dest, RHI::AttachmentBuffer buffer);

        void Draw(RHI::PrimitiveType type, uint64 numVertices);
        void DrawInstance(RHI::PrimitiveType type, uint64 numVertices, uint64 numInstance);

        void DrawIndexed(RHI::PrimitiveType type, uint64 numIndices);
        void DrawIndexedInstance(RHI::PrimitiveType type, uint64 numIndices, uint64 numInstance);

    public:

        void DrawSphere();
        void DrawScreenQuad();
        void DrawCube();

    public:

        TaskQueue&             GetRenderTaskQueue()       { return m_TaskQueue;  }
        const RHI::DeviceInfo& GetDeviceInfo()      const { return m_DeviceInfo; }

        Shared<Mesh>& GetSphereMesh() { return  m_SphereMesh;}
        Shared<Mesh>& GetQuadMesh()   { return  m_QuadMesh;  }
        Shared<Mesh>& GetCubeMesh()   { return  m_CubeMesh;  }

        Shared<Material>&  GetDefaultMaterial()    { return m_DefaultMaterial;     }
        Shared<Texture2D>& GetDefaultTexture()     { return m_DefaultTexture;      }
        Shared<Texture2D>& GetCheckerboardexture() { return m_CheckerboardTexture; }

    private:

        TaskQueue       m_TaskQueue;
        RHI::DeviceInfo m_DeviceInfo;

        Shared<Mesh>    m_SphereMesh;
        Shared<Mesh>    m_QuadMesh;
        Shared<Mesh>    m_CubeMesh;

        Shared<Material>  m_DefaultMaterial;
        Shared<Texture2D> m_DefaultTexture;
        Shared<Texture2D> m_CheckerboardTexture;

    private:

        static RendererPlatform* s_RendererPlatform;
    };


#define SL_ENQUEUE_RENDER_COMMAND(debugCommandName, command) Renderer::Get()->GetRenderTaskQueue().Enqueue(debugCommandName, command);
}
