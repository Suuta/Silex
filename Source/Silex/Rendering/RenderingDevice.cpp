
#include "PCH.h"

#include "Core/Window.h"
#include "Core/Engine.h"
#include "Rendering/RenderingDevice.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/Camera.h"
#include "Rendering/Vulkan/VulkanStructures.h"

#include <imgui/imgui_internal.h>
#include <imgui/imgui.h>


namespace Silex
{
    //======================================
    // シェーダーコンパイラ
    //======================================
    static ShaderCompiler shaserCompiler;


    struct Data
    {
        glm::vec3 pos;
        glm::vec4 color;
    };

    struct SceneData
    {
        glm::mat4 world = glm::mat4(1.0f);
        glm::mat4 view;
        glm::mat4 projection;
    };

    RenderingDevice* RenderingDevice::Get()
    {
        return instance;
    }

    RenderingDevice::RenderingDevice()
    {
        instance = this;
    }

    RenderingDevice::~RenderingDevice()
    {
        for (uint32 i = 0; i < frameData.size(); i++)
        {
            frameData[i].resourceQueue.Release();
        }

        api->UnmapBuffer(ubo);
        api->DestroyBuffer(ubo);
        api->DestroyBuffer(buffer);
        api->DestroyTexture(sceneTexture);

        for (uint32 i = 0; i < frameData.size(); i++)
        {
            api->DestroyCommandPool(frameData[i].commandPool);
            api->DestroySemaphore(frameData[i].presentSemaphore);
            api->DestroySemaphore(frameData[i].renderSemaphore);
            api->DestroyFence(frameData[i].fence);
        }

        api->DestroyCommandQueue(graphicsQueue);
        context->DestroyRendringAPI(api);

        instance = nullptr;
    }

    bool RenderingDevice::Initialize(RenderingContext* renderingContext)
    {
        context = renderingContext;

        // レンダーAPI実装クラスを生成
        api = context->CreateRendringAPI();
        SL_CHECK(!api->Initialize(), false);
       
        // グラフィックスをサポートするキューファミリを取得
        graphicsQueueFamily = api->QueryQueueFamily(QUEUE_FAMILY_GRAPHICS_BIT, Window::Get()->GetSurface());
        SL_CHECK(graphicsQueueFamily == INVALID_RENDER_ID, false);

        // コマンドキュー生成
        graphicsQueue = api->CreateCommandQueue(graphicsQueueFamily);
        SL_CHECK(!graphicsQueue, false);
      
        // フレームデータ生成
        for (uint32 i = 0; i < frameData.size(); i++)
        {
            // コマンドプール生成
            frameData[i].commandPool = api->CreateCommandPool(graphicsQueueFamily);
            SL_CHECK(!frameData[i].commandPool, false);

            // コマンドバッファ生成
            frameData[i].commandBuffer = api->CreateCommandBuffer(frameData[i].commandPool);
            SL_CHECK(!frameData[i].commandBuffer, false);

            // セマフォ生成
            frameData[i].presentSemaphore = api->CreateSemaphore();
            SL_CHECK(!frameData[i].presentSemaphore, false);

            frameData[i].renderSemaphore = api->CreateSemaphore();
            SL_CHECK(!frameData[i].renderSemaphore, false);

            // フェンス生成
            frameData[i].fence = api->CreateFence();
            SL_CHECK(!frameData[i].fence, false);

            // リソース解放キュー
            frameData[i].resourceQueue.Initialize();
        }

        return true;
    }


    static glm::vec3 ACES(glm::vec3 color)
    {
        const float a = 2.51;
        const float b = 0.03;
        const float c = 2.43;
        const float d = 0.59;
        const float e = 0.14;

        color = glm::clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0f, 1.0f);
        return color;
    }


    void RenderingDevice::TEST()
    {
        auto size = Window::Get()->GetSize();

        {
            swapchainPass = api->GetSwapChainRenderPass(Window::Get()->GetSwapChain());
        }

        {
            Attachment attachment = {};
            attachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            attachment.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachment.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp       = ATTACHMENT_STORE_OP_STORE;
            attachment.samples       = TEXTURE_SAMPLES_1;
            attachment.format        = RENDERING_FORMAT_R16G16B16A16_SFLOAT;

            AttachmentReference ref = {};
            ref.attachment = 0;
            ref.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            Subpass subpass = {};
            subpass.colorReferences.push_back(ref);

            SubpassDependency dependency = {};
            dependency.srcSubpass = UINT32_MAX;
            dependency.dstSubpass = 0;
            dependency.srcStages  = PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.dstStages  = PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            dependency.srcAccess  = 0;
            dependency.dstAccess  = BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            scenePass = api->CreateRenderPass(1, &attachment, 1, &subpass, 1, &dependency);
        }

        {
            SamplerInfo info = {};
            sceneSampler = api->CreateSampler(info);

            TextureFormat format = {};
            format.format    = RENDERING_FORMAT_R16G16B16A16_SFLOAT;
            format.width     = size.x;
            format.height    = size.y;
            format.type      = TEXTURE_TYPE_2D;
            format.usageBits = TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | TEXTURE_USAGE_SAMPLING_BIT;
            format.samples   = TEXTURE_SAMPLES_1;
            format.array     = 1;
            format.depth     = 1;
            format.mipmap    = 1;

            sceneTexture     = api->CreateTexture(format);
            sceneFramebuffer = api->CreateFramebuffer(scenePass, 1, sceneTexture, size.x, size.y);
        }

        InputBinding binding;
        binding.AddAttribute(0, 12, RENDERING_FORMAT_R32G32B32_SFLOAT);
        binding.AddAttribute(1, 16, RENDERING_FORMAT_R32G32B32A32_SFLOAT);
        InputLayout* layout = api->CreateInputLayout(1, &binding);

        PipelineInputAssemblyState ia = {};
        PipelineRasterizationState rs = {};
        PipelineMultisampleState   ms = {};
        PipelineDepthStencilState  ds = {};
        PipelineColorBlendState    bs = {false};

        {
            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Triangle.glsl", compiledData);
            shader   = api->CreateShader(compiledData);
            pipeline = api->CreateGraphicsPipeline(shader, layout, ia, rs, ms, ds, bs, scenePass);
        }

        {
            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Blit.glsl", compiledData);
            blitShader   = api->CreateShader(compiledData);
            blitPipeline = api->CreateGraphicsPipeline(blitShader, nullptr, ia, rs, ms, ds, bs, swapchainPass);
        }

        Data data[] =
        {
            {{ 0.0,  1.0, 0.0 }, {1.0, 0.0, 0.0, 0.5}},
            {{-1.0, -1.0, 0.0 }, {0.0, 1.0, 0.0, 0.5}},
            {{ 1.0, -1.0, 0.0 }, {0.0, 0.0, 1.0, 0.5}},
        };

        {
            Buffer* staging = api->CreateBuffer(sizeof(data), BUFFER_USAGE_VERTEX_BIT | BUFFER_USAGE_TRANSFER_SRC_BIT, MEMORY_ALLOCATION_TYPE_CPU);
        
            byte* mapped = api->MapBuffer(staging);
            std::memcpy(mapped, data, sizeof(data));
            api->UnmapBuffer(staging);
        
            buffer = api->CreateBuffer(sizeof(data), BUFFER_USAGE_VERTEX_BIT | BUFFER_USAGE_TRANSFER_DST_BIT, MEMORY_ALLOCATION_TYPE_GPU);

            api->ImmidiateExcute(graphicsQueue, frameData[frameIndex].commandBuffer, frameData[frameIndex].fence, [&](CommandBuffer* cmd)->bool
            {
                BufferCopyRegion region = {};
                region.size = sizeof(data);
        
                api->CopyBuffer(cmd, staging, buffer, 1, &region);
        
                return true;
            });
        
            api->DestroyBuffer(staging);
        }

        {
            SceneData sceneData;
            sceneData.world      = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
            sceneData.view       = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
            sceneData.projection = glm::perspectiveFov(glm::radians(60.0f), (float)size.x, (float)size.y, 0.1f, 1000.0f);
        
            ubo = api->CreateBuffer(sizeof(SceneData), (BUFFER_USAGE_UNIFORM_BIT | BUFFER_USAGE_TRANSFER_DST_BIT), MEMORY_ALLOCATION_TYPE_CPU);
            mappedSceneData = api->MapBuffer(ubo);
            std::memcpy(mappedSceneData, &sceneData, sizeof(SceneData));
        }
        
        {
            DescriptorHandle handles = {};
            handles.buffer = ubo;

            DescriptorInfo info = {};
            info.handles.push_back(handles);
            info.binding = 0;
            info.type    = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        
            descriptorSet = api->CreateDescriptorSet(1, &info, shader, 0);
        }

        {
            DescriptorHandle handles = {};
            handles.sampler = sceneSampler;
            handles.image   = sceneTexture;

            DescriptorInfo info = {};
            info.handles.push_back(handles);
            info.binding = 0;
            info.type    = DESCRIPTOR_TYPE_IMAGE_SAMPLER;

            blitSet = api->CreateDescriptorSet(1, &info, blitShader, 0);
        }
    }

    void RenderingDevice::DOCK_SPACE(Camera* camera)
    {
        bool isUsingCamera = Engine::Get()->GetEditor()->IsUsingEditorCamera();

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

        ImGuiWindowFlags windowFlags = 0;
        windowFlags |= ImGuiWindowFlags_NoDocking;
        windowFlags |= ImGuiWindowFlags_NoTitleBar;
        windowFlags |= ImGuiWindowFlags_NoMove;
        windowFlags |= ImGuiWindowFlags_NoResize;
        windowFlags |= ImGuiWindowFlags_NoCollapse;
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
        windowFlags |= ImGuiWindowFlags_NoNavFocus;
        windowFlags |= ImGuiWindowFlags_MenuBar;
        windowFlags |= isUsingCamera? ImGuiWindowFlags_NoInputs : 0;

        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
            ImGui::Begin("DockSpace", nullptr, windowFlags);
            ImGui::PopStyleVar();

            ImGuiDockNodeFlags docapaceFlag = ImGuiDockNodeFlags_NoWindowMenuButton;
            docapaceFlag |= isUsingCamera? ImGuiDockNodeFlags_NoResize : 0;

            ImGuiID dockspaceID = ImGui::GetID("Dockspace");
            ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), docapaceFlag);

            //============================
            // メニューバー
            //============================
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("編集"))
                {
                    if (ImGui::MenuItem("シーンを開く", "Ctr+O "));
                    if (ImGui::MenuItem("シーンを保存", "Ctr+S "));
                    if (ImGui::MenuItem("終了", "Alt+F4")) Engine::Get()->Close();

                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("ウィンドウ"))
                {
                    // ImGui::MenuItem("シーンビューポート", nullptr, &bShowScene);
                    // ImGui::MenuItem("アウトプットロガー", nullptr, &bShowLogger);
                    // ImGui::MenuItem("統計", nullptr, &bShowStats);
                    // ImGui::MenuItem("アウトライナー", nullptr, &bShowOutliner);
                    // ImGui::MenuItem("プロパティ", nullptr, &bShowProperty);
                    // ImGui::MenuItem("マテリアル", nullptr, &bShowMaterial);
                    // ImGui::MenuItem("アセットブラウザ", nullptr, &bShowAssetBrowser);
                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            ImGui::End();
        }

        // シーンプロパティ
        //m_ScenePropertyPanel.Render(&bShowOutliner, &bShowProperty);

        // アセットプロパティ
        //m_AssetBrowserPanel.Render(&bShowAssetBrowser, &bShowMaterial);


        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("シーン", nullptr, isUsingCamera? ImGuiWindowFlags_NoInputs : 0);

        // ビューポートへのホバー
        bool bHoveredViewport = ImGui::IsWindowHovered();

        // ウィンドウ内のコンテンツサイズ（タブやパディングは含めないので、ImGui::GetWindowSize関数は使わない）
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
        ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
        ImVec2 content = { contentMax.x - contentMin.x, contentMax.y - contentMin.y };

        // ウィンドウ左端からのオフセット
        ImVec2 viewportOffset = ImGui::GetWindowPos();

        // ImGuizmo用 相対ウィンドウ座標
        //m_RelativeViewportRect[0] = { contentMin.x + viewportOffset.x, contentMin.y + viewportOffset.y };
        //m_RelativeViewportRect[1] = { contentMax.x + viewportOffset.x, contentMax.y + viewportOffset.y };

        // シーン描画
        {
            // エディターが有効な状態では、シーンビューポートサイズがフレームバッファサイズになる
            if ((content.x > 0.0f && content.y > 0.0f) && (content.x != sceneFramebufferSize.x || content.y != sceneFramebufferSize.y))
            {
                sceneFramebufferSize.x = content.x;
                sceneFramebufferSize.y = content.y;

                SL_LOG_TRACE("Resize SceneFramebuffer: {}, {}", sceneFramebufferSize.x, sceneFramebufferSize.y);
            
                camera->SetViewportSize(sceneFramebufferSize.x, sceneFramebufferSize.y);
                AddResourceFreeQueue("Resize SceneFramebuffer", [this]()
                {
                    RESIZE(sceneFramebufferSize.x, sceneFramebufferSize.y);
                });
            }

            // シーン描画
            ImGui::Image(((VulkanDescriptorSet*)blitSet)->descriptorSet, content);
        }

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Begin("Viewport Info", nullptr, isUsingCamera ? ImGuiWindowFlags_NoInputs : 0);
        ImGui::Text("ReginonMin:     %d, %d", (int)contentMin.x, (int)contentMin.y);
        ImGui::Text("ReginonMax:     %d, %d", (int)contentMax.x, (int)contentMax.y);
        ImGui::Text("viewportOffset: %d, %d", (int)viewportOffset.x, (int)viewportOffset.y);
        ImGui::Text("Hover:          %s",     bHoveredViewport? "true" : "false");

        ImGui::End();

        ImGui::Begin("Test", nullptr, isUsingCamera ? ImGuiWindowFlags_NoInputs : 0);
        ImGui::Text("FPS: %d", Engine::Get()->GetFrameRate());
        ImGui::End();

        ImGui::PopStyleVar(2);
    }

    void RenderingDevice::RESIZE(uint32 width, uint32 height)
    {
        if (width == 0 || height == 0)
            return;

        api->WaitDevice();
        api->DestroyTexture(sceneTexture);

        TextureFormat format = {};
        format.format    = RENDERING_FORMAT_R16G16B16A16_SFLOAT;
        format.width     = width;
        format.height    = height;
        format.type      = TEXTURE_TYPE_2D;
        format.usageBits = TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | TEXTURE_USAGE_SAMPLING_BIT;
        format.samples   = TEXTURE_SAMPLES_1;
        format.array     = 1;
        format.depth     = 1;
        format.mipmap    = 1;

        sceneTexture = api->CreateTexture(format);

        api->DestroyFramebuffer(sceneFramebuffer);
        sceneFramebuffer = api->CreateFramebuffer(scenePass, 1, sceneTexture, width, height);


        DescriptorHandle handles = {};
        handles.image   = sceneTexture;
        handles.sampler = sceneSampler;

        DescriptorInfo info = {};
        info.handles.push_back(handles);
        info.binding = 0;
        info.type = DESCRIPTOR_TYPE_IMAGE_SAMPLER;

        api->UpdateDescriptorSet(blitSet, 1, &info);
    }

    void RenderingDevice::DRAW(Camera* camera)
    {
        FrameData& frame = frameData[frameIndex];
        const auto& windowSize   = Window::Get()->GetSize();
        const auto& viewportSize = camera->GetViewportSize();

        {
            SceneData sceneData;
            sceneData.projection = camera->GetProjectionMatrix();
            sceneData.view       = camera->GetViewMatrix();
            std::memcpy(mappedSceneData, &sceneData, sizeof(SceneData));
        }

        api->SetViewport(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);
        api->SetScissor(frame.commandBuffer,  0, 0, viewportSize.x, viewportSize.y);

        {
            api->BeginRenderPass(frame.commandBuffer, scenePass, sceneFramebuffer, COMMAND_BUFFER_TYPE_PRIMARY, 1, &defaultClearColor);
            
            uint64 offset = 0;
            api->BindPipeline(frame.commandBuffer, pipeline);
            api->BindDescriptorSet(frame.commandBuffer, descriptorSet, 0);
            api->BindVertexBuffers(frame.commandBuffer, 1, &buffer, &offset);
            api->Draw(frame.commandBuffer, 3, 1, 0, 0);

            api->EndRenderPass(frame.commandBuffer);
        }

        api->SetViewport(frame.commandBuffer, 0, 0, windowSize.x, windowSize.y);
        api->SetScissor(frame.commandBuffer, 0, 0, windowSize.x, windowSize.y);

        {
            api->BeginRenderPass(frame.commandBuffer, swapchainPass, swapchainFramebuffer, COMMAND_BUFFER_TYPE_PRIMARY, 1, &defaultClearColor);
            
            api->BindPipeline(frame.commandBuffer, blitPipeline);
            api->BindDescriptorSet(frame.commandBuffer, blitSet, 0);
            api->Draw(frame.commandBuffer, 3, 1, 0, 0);
        }
    }

    bool RenderingDevice::Begin()
    {
        SwapChain* swapchain = Window::Get()->GetSwapChain();
        FrameData& frame     = frameData[frameIndex];

        bool result = api->WaitFence(frame.fence);
        SL_CHECK(!result, false);

        frame.resourceQueue.Execute();

        swapchainFramebuffer = api->GetCurrentBackBuffer(swapchain, frame.presentSemaphore);
        SL_CHECK(!swapchainFramebuffer, false);

        result = api->BeginCommandBuffer(frame.commandBuffer);
        SL_CHECK(!result, false);

        return true;
    }

    bool RenderingDevice::End()
    {
        SwapChain* swapchain = Window::Get()->GetSwapChain();
        FrameData& frame     = frameData[frameIndex];
  
        api->EndRenderPass(frame.commandBuffer);
        api->EndCommandBuffer(frame.commandBuffer);

        api->SubmitQueue(graphicsQueue, frame.commandBuffer, frame.fence, frame.presentSemaphore, frame.renderSemaphore);

        return true;
    }

    bool RenderingDevice::Present()
    {
        SwapChain* swapchain = Window::Get()->GetSwapChain();
        FrameData& frame = frameData[frameIndex];

        api->Present(graphicsQueue, swapchain, frame.renderSemaphore);
        frameIndex = (frameIndex + 1) % 2;

        return true;
    }

    SwapChain* RenderingDevice::CreateSwapChain(Surface* surface, uint32 width, uint32 height, VSyncMode mode)
    {
        SwapChain* swapchain = api->CreateSwapChain(surface, width, height, swapchainBufferCount, mode);
        SL_CHECK(!swapchain, nullptr);

        return swapchain;
    }

    void RenderingDevice::DestoySwapChain(SwapChain* swapchain)
    {
        api->DestroySwapChain(swapchain);
    }

    RenderingContext* RenderingDevice::GetContext() const
    {
        return context;
    }

    RenderingAPI* RenderingDevice::GetAPI() const
    {
        return api;
    }

    CommandQueue* RenderingDevice::GetGraphicsCommandQueue() const
    {
        return graphicsQueue;
    }

    FrameData& RenderingDevice::GetFrameData()
    {
        return frameData[frameIndex];
    }

    const FrameData& RenderingDevice::GetFrameData() const
    {
        return frameData[frameIndex];
    }

    QueueFamily RenderingDevice::GetGraphicsQueueFamily() const
    {
        return graphicsQueueFamily;
    }

    bool RenderingDevice::ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, VSyncMode mode)
    {
        bool result = api->ResizeSwapChain(swapchain, width, height, swapchainBufferCount, mode);
        SL_CHECK(!result, false);

        return false;
    }
}
