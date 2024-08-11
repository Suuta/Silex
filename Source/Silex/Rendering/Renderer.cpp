
#include "PCH.h"

#include "Core/Timer.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"
#include "Rendering/Renderer.h"
#include "Rendering/Framebuffer.h"
#include "Rendering/MeshFactory.h"

#include <GLFW/glfw3.h>



namespace Silex
{
    static void QueryMonitorInfo()
    {
        int32 monitorCount;
        GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

        SL_LOG_TRACE("===== Monitor Info =====");

        for (int32 i = 0; i < monitorCount; i++)
        {
            const char* name        = glfwGetMonitorName(monitors[i]);
            const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);

            int32 widthMM, heightMM;
            glfwGetMonitorPhysicalSize(monitors[i], &widthMM, &heightMM);

            const uint32 dpiw = glm::round(mode->width  / (widthMM  / 25.4));
            const uint32 dpih = glm::round(mode->height / (heightMM / 25.4));

            uint32 monitorSizeMM   = glm::sqrt((widthMM * widthMM) + (heightMM * heightMM));
            uint32 monitorSizeInch = glm::round(monitorSizeMM / 25.4);

            SL_LOG_TRACE(" refreshRate: {}", mode->refreshRate);
            SL_LOG_TRACE(" dpi:         {}", dpiw);
            SL_LOG_TRACE(" inch:        {}", monitorSizeInch);
            SL_LOG_TRACE("-------------------------");
        }
    }


    static Renderer s_Renderer;

    // レンダープラットフォーム
    RendererPlatform* Renderer::s_RendererPlatform;


    Renderer* Renderer::Get()
    {
        return &s_Renderer;
    }

    void Renderer::Init()
    {
        // レンダータスクキュー初期化
        m_TaskQueue.Initialize();

        // レンダーAPI 初期化
        s_RendererPlatform = RendererPlatform::Create();
        s_RendererPlatform->Init();

        // シェーダー
        {
            // IBL
            ShaderManager::Get()->Add("Equirectangular",     Shader("Assets/Shaders/IBL/Equirectangular.glsl"));
            ShaderManager::Get()->Add("Irradiance",          Shader("Assets/Shaders/IBL/Irradiance.glsl"));
            ShaderManager::Get()->Add("Prefilter",           Shader("Assets/Shaders/IBL/Prefilter.glsl"));
            ShaderManager::Get()->Add("BRDF",                Shader("Assets/Shaders/IBL/BRDF.glsl"));

            // PostProcess
            ShaderManager::Get()->Add("FXAA",                Shader("Assets/Shaders/PostProcess/FXAA.glsl"));
            ShaderManager::Get()->Add("Outline",             Shader("Assets/Shaders/PostProcess/Outline.glsl"));
            ShaderManager::Get()->Add("ChromaticAberration", Shader("Assets/Shaders/PostProcess/ChromaticAberration.glsl"));
            ShaderManager::Get()->Add("Tonemap",             Shader("Assets/Shaders/PostProcess/Tonemap.glsl"));
            ShaderManager::Get()->Add("Bloom",               Shader("Assets/Shaders/PostProcess/Bloom/Bloom.glsl"));
            ShaderManager::Get()->Add("DownSampling",        Shader("Assets/Shaders/PostProcess/Bloom/DownSampling.glsl"));
            ShaderManager::Get()->Add("UpSampling",          Shader("Assets/Shaders/PostProcess/Bloom/UpSampling.glsl"));
            ShaderManager::Get()->Add("PreDownSampling",     Shader("Assets/Shaders/PostProcess/Bloom/PreDownSampling.glsl"));

            // Lighting
            ShaderManager::Get()->Add("DeferredLighting",    Shader("Assets/Shaders/DeferredLighting.glsl"));
            ShaderManager::Get()->Add("GBuffer",             Shader("Assets/Shaders/GBuffer.glsl"));
            ShaderManager::Get()->Add("Sky",                 Shader("Assets/Shaders/Sky.glsl"));
            ShaderManager::Get()->Add("Shadow",              Shader("Assets/Shaders/Shadow.glsl"));
        }

        // メッシュ
        //m_QuadMesh = Shared<Mesh>(MeshFactory::Quad());
        //m_QuadMesh->SetPrimitiveType(RHI::PrimitiveType::TriangleStrip);
        //m_QuadMesh->GetFilePath() = "Quad";

        m_CubeMesh = Shared<Mesh>(MeshFactory::Cube());
        m_CubeMesh->SetPrimitiveType(rhi::PrimitiveType::Triangle);
        m_CubeMesh->GetFilePath() = "Cube";

        m_SphereMesh = Shared<Mesh>(MeshFactory::Sphere());
        m_SphereMesh->SetPrimitiveType(rhi::PrimitiveType::TriangleStrip);
        m_SphereMesh->GetFilePath() = "Sphere";

        // テクスチャ
        rhi::TextureDesc desc = {};
        desc.Filter    = rhi::TextureFilter::Linear;
        desc.Wrap      = rhi::TextureWrap::Repeat;
        desc.GenMipmap = true;

        m_DefaultTexture      = Texture2D::Create(desc, "Assets/Textures/default.png");
        m_CheckerboardTexture = Texture2D::Create(desc, "Assets/Textures/checkerboard.png");

        // マテリアル
        m_DefaultMaterial = CreateShared<Material>();
    }

    void Renderer::Shutdown()
    {
        m_SphereMesh.Reset();
        m_QuadMesh.Reset();
        m_CubeMesh.Reset();

        m_DefaultMaterial.Reset();
        m_DefaultTexture.Reset();
        m_CheckerboardTexture.Reset();

        s_RendererPlatform->Shutdown();
        m_TaskQueue.Release();
    }

    void Renderer::BeginFrame()
    {
        SL_SCOPE_PROFILE("BeginFrame");
        s_RendererPlatform->BeginFrame();
    }

    void Renderer::EndFrame()
    {
        SL_SCOPE_PROFILE("Present");
        s_RendererPlatform->EndFrame();
    }

    void Renderer::Resize(uint32 width, uint32 height)
    {
        s_RendererPlatform->Resize(width, height);
    }

    void Renderer::EnableBlend(bool enable)
    {
        s_RendererPlatform->EnableBlend(enable);
    }

    void Renderer::SetDefaultFramebuffer()
    {
        s_RendererPlatform->SetDefaultFramebuffer();
    }

    void Renderer::SetShaderTexture(uint32 slot, uint32 id)
    {
        s_RendererPlatform->SetShaderTexture(slot, id);
    }

    void Renderer::SetViewport(uint32 width, uint32 height)
    {
        s_RendererPlatform->SetViewport(width, height);
    }

    void Renderer::SetStencilFunc(rhi::StrencilOp op, int32 ref, uint32 mask)
    {
        s_RendererPlatform->SetStencilFunc(op, ref, mask);
    }

    void Renderer::SetCullFace(rhi::CullFace face)
    {
        s_RendererPlatform->SetCullFace(face);
    }

    void Renderer::BlitFramebuffer(const Shared<Framebuffer>& src, const Shared<Framebuffer>& dest, rhi::AttachmentBuffer buffer)
    {
        s_RendererPlatform->BlitFramebuffer(src, dest, buffer);
    }

    void Renderer::Draw(rhi::PrimitiveType type, uint64 numVertices)
    {
        s_RendererPlatform->Draw(type, numVertices);
    }

    void Renderer::DrawIndexed(rhi::PrimitiveType type, uint64 numIndices)
    {
        s_RendererPlatform->DrawIndexed(type, numIndices);
    }

    void Renderer::DrawIndexedInstance(rhi::PrimitiveType type, uint64 numIndices, uint64 numInstance)
    {
        s_RendererPlatform->DrawIndexedInstance(type, numIndices, numInstance);
    }

    void Renderer::DrawInstance(rhi::PrimitiveType type, uint64 numVertices, uint64 numInstance)
    {
        s_RendererPlatform->DrawInstance(type, numVertices, numInstance);
    }




    void Renderer::DrawSphere()
    {
        m_SphereMesh->GetMeshSource(0)->Bind();
        DrawIndexed(m_SphereMesh->GetPrimitiveType(), m_SphereMesh->GetMeshSource(0)->GetIndexCount());
    }

    void Renderer::DrawScreenQuad()
    {
        m_QuadMesh->GetMeshSource(0)->Bind();
        Draw(m_QuadMesh->GetPrimitiveType(), 4);
    }

    void Renderer::DrawCube()
    {
        m_CubeMesh->GetMeshSource(0)->Bind();
        DrawIndexed(m_CubeMesh->GetPrimitiveType(), m_CubeMesh->GetMeshSource(0)->GetIndexCount());
    }
}
