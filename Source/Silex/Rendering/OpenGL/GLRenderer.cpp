
#include "PCH.h"

#include "Rendering/OpenGL/OpenGLCore.h"
#include "Rendering/OpenGL/GLRenderer.h"
#include "Rendering/OpenGL/GLFramebuffer.h"
#include "Core/Engine.h"
#include "Core/Timer.h"

#include <GLFW/glfw3.h>


namespace Silex
{
    static void GLDebugMessageCallback(
        GLenum        source,
        GLenum        type,
        GLuint        id,
        GLenum        severity,
        GLsizei       length,
        const GLchar* message,
        const void*   userParam)
    {
        switch (severity)
        {
            default: break;

            case GL_DEBUG_SEVERITY_HIGH:         SL_LOG_ERROR("[GLDebugMessageCallback] {0}", message); break;
            case GL_DEBUG_SEVERITY_MEDIUM:       SL_LOG_WARN( "[GLDebugMessageCallback] {0}", message); break;
            case GL_DEBUG_SEVERITY_LOW:          SL_LOG_INFO( "[GLDebugMessageCallback] {0}", message); break;
            //case GL_DEBUG_SEVERITY_NOTIFICATION: SL_LOG_INFO( "[GLDebugMessageCallback] {0}", message); break;
        }
    }

    void GLRenderer::Init()
    {
        SL_LOG_TRACE("GLRenderer::Init");

        bool result = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        if (!result)
        {
            SL_ASSERT(false, "GL関数の読み込みが失敗しました");
            return;
        }

        {
            int32 numExtensions;
            glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
            for (int32 i = 0; i < numExtensions; i++)
            {
                const char* extension = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
                m_Extentions.insert(extension);
            }

            int32 maxComputeWorkgroupCount[3];
            int32 maxComputeWorkgroupSize[3];
            int32 maxComputeWorkgroupInvocations;
            for (int32 idx = 0; idx < 3; idx++)
            {
                glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, idx, &maxComputeWorkgroupCount[idx]);
                glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, idx, &maxComputeWorkgroupSize[idx]);
            }

            glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxComputeWorkgroupInvocations);
        }

        glDebugMessageCallback(GLDebugMessageCallback, nullptr);

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);

        glEnable(GL_CULL_FACE);
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);

        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.0f, 0.0f);

        glDepthFunc(GL_LESS);
        glLineWidth(1.0f);

        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        m_DeviceName = (const char*)glGetString(GL_RENDERER);
        m_Window     = Window::Get()->GetGLFWWindow();
    }

    void GLRenderer::Shutdown()
    {
        SL_LOG_TRACE("GLRenderer::Shutdown");
        sldelete(this);
    }

    void GLRenderer::BeginFrame()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void GLRenderer::EndFrame()
    {
        glfwSwapBuffers(m_Window);
    }

    void GLRenderer::Resize(uint32 width, uint32 height)
    {
    }

    void GLRenderer::SetDefaultFramebuffer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GLRenderer::SetShaderTexture(uint32 slot, uint32 id)
    {
        glBindTextureUnit(slot, id);
    }

    void GLRenderer::SetViewport(uint32 width, uint32 height)
    {
        glViewport(0, 0, width, height);
    }

    void GLRenderer::SetStencilFunc(RHI::StrencilOp op, int32 ref, uint32 mask)
    {
        glStencilFunc(OpenGL::GLStencilOp(op), ref, mask);
    }

    void GLRenderer::SetCullFace(RHI::CullFace face)
    {
        glCullFace(OpenGL::GLCullFace(face));
    }

    void GLRenderer::EnableBlend(bool enable)
    {
        enable? glEnable(GL_BLEND) : glDisable(GL_BLEND);
    }

    void GLRenderer::BlitFramebuffer(const Shared<Framebuffer>& src, const Shared<Framebuffer>& dest, RHI::AttachmentBuffer buffer)
    {
        uint32 width  = src->GetWidth();
        uint32 height = src->GetHeight();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, src ? src->GetID()  : 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest? dest->GetID() : 0);

        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, OpenGL::GLBufferBit(buffer), GL_NEAREST);
    }

    void GLRenderer::Draw(RHI::PrimitiveType type, uint64 numVertices)
    {
        glDrawArrays(OpenGL::GLPrimitivepeType(type), 0, numVertices);
    }

    void GLRenderer::DrawInstance(RHI::PrimitiveType type, uint64 numVertices, uint64 numInstance)
    {
        glDrawArraysInstanced(OpenGL::GLPrimitivepeType(type), 0, numVertices, numInstance);
    }

    void GLRenderer::DrawIndexed(RHI::PrimitiveType type, uint64 numIndices)
    {
        glDrawElements(OpenGL::GLPrimitivepeType(type), numIndices, GL_UNSIGNED_INT, 0);
    }

    void GLRenderer::DrawIndexedInstance(RHI::PrimitiveType type, uint64 numIndices, uint64 numInstance)
    {
        glDrawElementsInstanced(OpenGL::GLPrimitivepeType(type), numIndices, GL_UNSIGNED_INT, 0, numInstance);
    }
}
