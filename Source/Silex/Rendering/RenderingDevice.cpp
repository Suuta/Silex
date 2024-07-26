
#include "PCH.h"

#include "Core/Window.h"
#include "Core/Engine.h"
#include "Rendering/RenderingDevice.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/RenderingUtility.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/Camera.h"
#include "Rendering/Mesh.h"
#include "Rendering/Vulkan/VulkanStructures.h"
#include "Rendering/MeshFactory.h"

#include <imgui/imgui_internal.h>
#include <imgui/imgui.h>


namespace Silex
{
    //======================================
    // シェーダーコンパイラ
    //======================================
    static ShaderCompiler shaserCompiler;




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
        api->DestroyBuffer(vb);
        api->DestroyBuffer(ib);
        api->DestroyTexture(sceneColorTexture);
        api->DestroyTexture(sceneDepthTexture);
        api->DestroyShader(shader);
        api->DestroyShader(blitShader);
        api->DestroyDescriptorSet(blitSet);
        api->DestroyDescriptorSet(descriptorSet);
        api->DestroyPipeline(pipeline);
        api->DestroyPipeline(blitPipeline);
        api->DestroyRenderPass(scenePass);
        api->DestroyFramebuffer(sceneFramebuffer);
        api->DestroySampler(sceneSampler);

        sldelete(cubeMesh);
        sldelete(sphereMesh);

        for (uint32 i = 0; i < frameData.size(); i++)
        {
            api->DestroyCommandBuffer(frameData[i].commandBuffer);
            api->DestroyCommandPool(frameData[i].commandPool);
            api->DestroySemaphore(frameData[i].presentSemaphore);
            api->DestroySemaphore(frameData[i].renderSemaphore);
            api->DestroyFence(frameData[i].fence);
        }

        //api->DestroyCommandBuffer(immidiateContext.commandBuffer);
        api->DestroyCommandPool(immidiateContext.commandPool);
        api->DestroyFence(immidiateContext.fence);

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


        // 即時コマンドデータ
        immidiateContext.commandPool = api->CreateCommandPool(graphicsQueueFamily);
        SL_CHECK(!immidiateContext.commandPool, false);

        immidiateContext.commandBuffer = api->CreateCommandBuffer(immidiateContext.commandPool);
        SL_CHECK(!immidiateContext.commandBuffer, false);

        immidiateContext.fence = api->CreateFence();
        SL_CHECK(!immidiateContext.fence, false);

        return true;
    }

    bool RenderingDevice::Begin()
    {
        SwapChain* swapchain = Window::Get()->GetSwapChain();
        FrameData& frame     = frameData[frameIndex];

        // GPU 待機
        bool result = api->WaitFence(frame.fence);
        SL_CHECK(!result, false);

        // 削除キュー実行
        frame.resourceQueue.Execute();

        // 描画先スワップチェインバッファを取得
        swapchainFramebuffer = api->GetCurrentBackBuffer(swapchain, frame.presentSemaphore);
        SL_CHECK(!swapchainFramebuffer, false);

        // コマンドバッファ開始
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



    void RenderingDevice::TEST()
    {
        auto size = Window::Get()->GetSize();

        {
            swapchainPass = api->GetSwapChainRenderPass(Window::Get()->GetSwapChain());
        }

        {
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            color.storeOp       = ATTACHMENT_STORE_OP_STORE;
            color.samples       = TEXTURE_SAMPLES_1;
            color.format        = RENDERING_FORMAT_R16G16B16A16_SFLOAT;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            Attachment depth = {};
            depth.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            depth.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            depth.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            depth.storeOp       = ATTACHMENT_STORE_OP_STORE;
            depth.samples       = TEXTURE_SAMPLES_1;
            depth.format        = RENDERING_FORMAT_D32_SFLOAT_S8_UINT;

            AttachmentReference depthRef = {};
            depthRef.attachment = 1;
            depthRef.layout     = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            Subpass subpass = {};
            subpass.colorReferences.push_back(colorRef);
            subpass.depthstencilReference = depthRef;

            // SubpassDependency colorDependency = {};
            // colorDependency.srcSubpass = UINT32_MAX;
            // colorDependency.dstSubpass = 0;
            // colorDependency.srcStages  = PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            // colorDependency.dstStages  = PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            // colorDependency.srcAccess  = 0;
            // colorDependency.dstAccess  = BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

            // SubpassDependency depthDependency = {};
            // depthDependency.srcSubpass = UINT32_MAX;
            // depthDependency.dstSubpass = 0;
            // depthDependency.srcStages  = PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            // depthDependency.dstStages  = PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            // depthDependency.srcAccess  = 0;
            // depthDependency.dstAccess  = BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            Attachment attachments[] = {color, depth};
            scenePass = api->CreateRenderPass(std::size(attachments), attachments, 1, &subpass, 0, nullptr);
        }

        {
            SamplerInfo info = {};
            sceneSampler = api->CreateSampler(info);

            TextureFormat color = {};
            color.format    = RENDERING_FORMAT_R16G16B16A16_SFLOAT;
            color.width     = size.x;
            color.height    = size.y;
            color.type      = TEXTURE_TYPE_2D;
            color.usageBits = TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | TEXTURE_USAGE_SAMPLING_BIT;
            color.samples   = TEXTURE_SAMPLES_1;
            color.array     = 1;
            color.depth     = 1;
            color.mipLevels = 1;

            sceneColorTexture = api->CreateTexture(color);

            TextureFormat depth = {};
            depth.format    = RENDERING_FORMAT_D32_SFLOAT_S8_UINT;
            depth.width     = size.x;
            depth.height    = size.y;
            depth.type      = TEXTURE_TYPE_2D;
            depth.usageBits = TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | TEXTURE_USAGE_SAMPLING_BIT;
            depth.samples   = TEXTURE_SAMPLES_1;
            depth.array     = 1;
            depth.depth     = 1;
            depth.mipLevels = 1;

            sceneDepthTexture = api->CreateTexture(depth);


            TextureHandle* textures[] = { sceneColorTexture , sceneDepthTexture };
            sceneFramebuffer = api->CreateFramebuffer(scenePass, std::size(textures), textures, size.x, size.y);
        }

        InputBinding binding;
        binding.AddAttribute(0, 12, RENDERING_FORMAT_R32G32B32_SFLOAT);
        binding.AddAttribute(1, 12, RENDERING_FORMAT_R32G32B32_SFLOAT);
        binding.AddAttribute(2,  8, RENDERING_FORMAT_R32G32_SFLOAT);
        binding.AddAttribute(3, 12, RENDERING_FORMAT_R32G32B32_SFLOAT);
        binding.AddAttribute(4, 12, RENDERING_FORMAT_R32G32B32_SFLOAT);

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

            api->DestroyInputLayout(layout);
        }

        {
            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Blit.glsl", compiledData);
            blitShader   = api->CreateShader(compiledData);
            blitPipeline = api->CreateGraphicsPipeline(blitShader, nullptr, ia, rs, ms, ds, bs, swapchainPass);
        }

        Vertex data[] =
        {
            {{-1.0,  1.0,  0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0 }}, // 0
            {{-1.0, -1.0,  0.0 }, { 0.0, 0.0, 0.0 }, { 1.0, 0.0 }}, // 1
            {{ 1.0, -1.0,  0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 1.0 }}, // 2
            {{ 1.0,  1.0,  0.0 }, { 0.0, 0.0, 0.0 }, { 1.0, 1.0 }}, // 3
        };

        uint32 indices[] = { 0, 1, 2,  2, 3, 0};


        vb = CreateVertexBuffer(data, sizeof(data));
        ib = CreateIndexBuffer(indices, sizeof(indices));

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
            //DescriptorHandle handles = {};
            //handles.buffer = image;
            //
            //DescriptorInfo info = {};
            //info.handles.push_back(handles);
            //info.binding = 0;
            //info.type    = DESCRIPTOR_TYPE_IMAGE_SAMPLER;
            //
            //textureSet = api->CreateDescriptorSet(1, &info, shader, 1);
        }

        {
            DescriptorHandle handles = {};
            handles.sampler = sceneSampler;
            handles.image   = sceneColorTexture;

            DescriptorInfo info = {};
            info.handles.push_back(handles);
            info.binding = 0;
            info.type    = DESCRIPTOR_TYPE_IMAGE_SAMPLER;

            blitSet = api->CreateDescriptorSet(1, &info, blitShader, 0);
        }

        cubeMesh   = MeshFactory::Cube();
        sphereMesh = MeshFactory::Sphere();
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
        ImVec2 content    = { contentMax.x - contentMin.x, contentMax.y - contentMin.y };

        // ウィンドウ左端からのオフセット
        ImVec2 viewportOffset = ImGui::GetWindowPos();

        // ImGuizmo用 相対ウィンドウ座標
        ImVec2 relativeViewportRect[2] =
        {
            { contentMin.x + viewportOffset.x, contentMin.y + viewportOffset.y },
            { contentMax.x + viewportOffset.x, contentMax.y + viewportOffset.y }
        };

        // シーン描画
        {
            // エディターが有効な状態では、シーンビューポートサイズがフレームバッファサイズになる
            if ((content.x > 0.0f && content.y > 0.0f) && (content.x != sceneFramebufferSize.x || content.y != sceneFramebufferSize.y))
            {
                sceneFramebufferSize.x = content.x;
                sceneFramebufferSize.y = content.y;

                camera->SetViewportSize(sceneFramebufferSize.x, sceneFramebufferSize.y);
                AddResourceFreeQueue("Resize SceneFramebuffer", [this]()
                {
                    SL_LOG_TRACE("Resize SceneFramebuffer: {}, {}", sceneFramebufferSize.x, sceneFramebufferSize.y);
                    RESIZE(sceneFramebufferSize.x, sceneFramebufferSize.y);
                });
            }

            // シーン描画
            ImGui::Image(((VulkanDescriptorSet*)blitSet)->descriptorSet, content);
        }

        ImGui::End();
        ImGui::PopStyleVar();


        ImGui::Begin("Viewport Info", nullptr, isUsingCamera ? ImGuiWindowFlags_NoInputs : 0);
        ImGui::Separator();
        ImGui::Text("FPS: %d", Engine::Get()->GetFrameRate());
        ImGui::Separator();
        ImGui::Text("ReginonMin:     %d, %d", (int)contentMin.x, (int)contentMin.y);
        ImGui::Text("ReginonMax:     %d, %d", (int)contentMax.x, (int)contentMax.y);
        ImGui::Text("viewportOffset: %d, %d", (int)viewportOffset.x, (int)viewportOffset.y);
        ImGui::Text("Hover:          %s",     bHoveredViewport? "true" : "false");
        ImGui::End();


        ImGui::PopStyleVar(2);
    }

    void RenderingDevice::RESIZE(uint32 width, uint32 height)
    {
        if (width == 0 || height == 0)
            return;

        api->WaitDevice();

        api->DestroyTexture(sceneColorTexture);
        api->DestroyTexture(sceneDepthTexture);
        api->DestroyFramebuffer(sceneFramebuffer);

        TextureFormat format = {};
        format.format    = RENDERING_FORMAT_R16G16B16A16_SFLOAT;
        format.width     = width;
        format.height    = height;
        format.type      = TEXTURE_TYPE_2D;
        format.usageBits = TEXTURE_USAGE_COLOR_ATTACHMENT_BIT | TEXTURE_USAGE_SAMPLING_BIT;
        format.samples   = TEXTURE_SAMPLES_1;
        format.array     = 1;
        format.depth     = 1;
        format.mipLevels = 1;

        sceneColorTexture = api->CreateTexture(format);

        TextureFormat depth = {};
        depth.format    = RENDERING_FORMAT_D32_SFLOAT_S8_UINT;
        depth.width     = width;
        depth.height    = height;
        depth.type      = TEXTURE_TYPE_2D;
        depth.usageBits = TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | TEXTURE_USAGE_SAMPLING_BIT;
        depth.samples   = TEXTURE_SAMPLES_1;
        depth.array     = 1;
        depth.depth     = 1;
        depth.mipLevels = 1;

        sceneDepthTexture = api->CreateTexture(depth);


        TextureHandle* textures[] = { sceneColorTexture , sceneDepthTexture };
        sceneFramebuffer = api->CreateFramebuffer(scenePass, std::size(textures), textures, width, height);


        DescriptorHandle handles = {};
        handles.image   = sceneColorTexture;
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
            defaultClearColor.color          = {0.1f, 0.1f, 0.1f, 1.0f};
            defaultClearDepthStencil.depth   = 1.0f;
            defaultClearDepthStencil.stencil = 0;

            RenderPassClearValue clearValue[] = { defaultClearColor, defaultClearDepthStencil };
            api->BeginRenderPass(frame.commandBuffer, scenePass, sceneFramebuffer, COMMAND_BUFFER_TYPE_PRIMARY, std::size(clearValue), clearValue);
            
            uint64 offset = 0;
            api->BindPipeline(frame.commandBuffer, pipeline);
            api->BindDescriptorSet(frame.commandBuffer, descriptorSet, 0);

#if 1
            Buffer* cvb       = sphereMesh->GetMeshSource()->GetVertexBuffer();
            Buffer* cib       = sphereMesh->GetMeshSource()->GetIndexBuffer();
            uint32 indexCount = sphereMesh->GetMeshSource()->GetIndexCount();
            api->BindVertexBuffers(frame.commandBuffer, 1, &cvb, &offset);
            api->BindIndexBuffer(frame.commandBuffer, cib, INDEX_BUFFER_FORMAT_UINT32, 0);
            api->DrawIndexed(frame.commandBuffer, indexCount, 1, 0, 0, 0);
#else
            api->BindVertexBuffers(frame.commandBuffer, 1, &vb, &offset);
            api->BindIndexBuffer(frame.commandBuffer, ib, INDEX_BUFFER_FORMAT_UINT32, 0);
            api->DrawIndexed(frame.commandBuffer, 6, 1, 0, 0, 0);
#endif

            api->EndRenderPass(frame.commandBuffer);
        }

        api->SetViewport(frame.commandBuffer, 0, 0, windowSize.x, windowSize.y);
        api->SetScissor(frame.commandBuffer, 0, 0, windowSize.x, windowSize.y);

        {
            api->BeginRenderPass(frame.commandBuffer, swapchainPass, swapchainFramebuffer, COMMAND_BUFFER_TYPE_PRIMARY, 1, &defaultClearColor);

            api->BindPipeline(frame.commandBuffer, blitPipeline);
            api->BindDescriptorSet(frame.commandBuffer, blitSet, 0);
            api->Draw(frame.commandBuffer, 6, 1, 0, 0);
        }
    }




    TextureHandle* RenderingDevice::LoadTextureFromFile(const byte* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap)
    {
        // ステージングバッファ
        Buffer* staging = api->CreateBuffer(dataSize, BUFFER_USAGE_TRANSFER_SRC_BIT, MEMORY_ALLOCATION_TYPE_CPU);

        // ステージングにテクスチャデータをコピー
        void* mappedPtr = api->MapBuffer(staging);
        std::memcpy(mappedPtr, pixelData, dataSize);
        api->UnmapBuffer(staging);

        // テクスチャ生成
        TextureUsageFlags flags = TEXTURE_USAGE_COPY_SRC_BIT | TEXTURE_USAGE_COPY_DST_BIT;
        TextureHandle* texture = CreateTexture(TEXTURE_TYPE_2D, RENDERING_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, genMipmap, flags);

        // データコピー
        api->ImmidiateCommands(graphicsQueue, immidiateContext.commandBuffer, immidiateContext.fence, [&](CommandBuffer* cmd)
        {
            TextureSubresourceRange range = {};
            range.aspect = TEXTURE_ASPECT_COLOR_BIT;

            TextureBarrierInfo info = {};
            info.texture = texture;
            info.subresources = range;
            info.srcAccess = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.oldLayout = TEXTURE_LAYOUT_UNDEFINED;
            info.newLayout = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;

            api->PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            TextureSubresource subresource = {};
            subresource.aspect = TEXTURE_ASPECT_COLOR_BIT;
            subresource.mipLevel = 0;
            subresource.baseLayer = 0;
            subresource.layerCount = 1;

            BufferTextureCopyRegion region = {};
            region.bufferOffset = 0;
            region.textureOffset = { 0, 0, 0 };
            region.textureRegionSize = { 0, 0, 0 };
            region.textureSubresources = subresource;

            api->CopyBufferToTexture(cmd, staging, texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            if (genMipmap)
            {
                GenerateMipmaps(cmd, texture, width, height);
            }
            else
            {
                info.oldLayout = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
                info.newLayout = TEXTURE_LAYOUT_READ_ONLY_OPTIMAL;
                api->PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
            }
        });

        api->DestroyBuffer(staging);
        return texture;
    }

    TextureHandle* RenderingDevice::CreateTexture2D(RenderingFormat format, uint32 width, uint32 height, bool genMipmap)
    {
        return CreateTexture(TEXTURE_TYPE_2D, format, width, height, 1, 1, genMipmap, 0);
    }

    TextureHandle* RenderingDevice::CreateTexture2DArray(RenderingFormat format, uint32 width, uint32 height, uint32 array, bool genMipmap)
    {
        return CreateTexture(TEXTURE_TYPE_2D_ARRAY, format, width, height, array, 1, genMipmap, 0);
    }

    TextureHandle* RenderingDevice::CreateTextureCube(RenderingFormat format, uint32 width, uint32 height, bool genMipmap)
    {
        return CreateTexture(TEXTURE_TYPE_CUBE, format, width, height, 1, 1, genMipmap, 0);
    }

    void RenderingDevice::GenerateMipmaps(CommandBuffer* cmd, TextureHandle* texture, uint32 width, uint32 height)
    {
        auto mipmaps = RenderingUtility::CalculateMipmap(width, height);

        for (uint32 i = 0; i < mipmaps.size(); i++)
        {

        }
    }

    TextureHandle* RenderingDevice::CreateTexture(TextureType type, RenderingFormat format, uint32 width, uint32 height, uint32 array, uint32 depth, bool genMipmap, TextureUsageFlags additionalFlags)
    {
        uint32 usage = RenderingUtility::IsDepthFormat(format)? TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
        auto mipmaps = RenderingUtility::CalculateMipmap(width, height);

        TextureFormat texformat = {};
        texformat.format    = format;
        texformat.width     = width;
        texformat.height    = height;
        texformat.type      = type;
        texformat.usageBits = usage | additionalFlags | TEXTURE_USAGE_SAMPLING_BIT;
        texformat.samples   = TEXTURE_SAMPLES_1;
        texformat.array     = array;
        texformat.depth     = depth;
        texformat.mipLevels = genMipmap? mipmaps.size() : 1;

        TextureHandle* texture = api->CreateTexture(texformat);
        SL_CHECK(!texture, nullptr);

        return texture;
    }



    Buffer* RenderingDevice::CreateVertexBuffer(void* data, uint64 dataByte)
    {
         Buffer* staging = api->CreateBuffer(dataByte, BUFFER_USAGE_VERTEX_BIT | BUFFER_USAGE_TRANSFER_SRC_BIT, MEMORY_ALLOCATION_TYPE_CPU);
         
         byte* mapped = api->MapBuffer(staging);
         std::memcpy(mapped, data, dataByte);
         api->UnmapBuffer(staging);
         
         Buffer* buffer = api->CreateBuffer(dataByte, BUFFER_USAGE_VERTEX_BIT | BUFFER_USAGE_TRANSFER_DST_BIT, MEMORY_ALLOCATION_TYPE_GPU);

         api->ImmidiateCommands(graphicsQueue, immidiateContext.commandBuffer, immidiateContext.fence, [&](CommandBuffer* cmd)
         {
             BufferCopyRegion region = {};
             region.size = dataByte;
         
             api->CopyBuffer(cmd, staging, buffer, 1, &region);
         });
         
         api->DestroyBuffer(staging);

        return buffer;
    }

    Buffer* RenderingDevice::CreateIndexBuffer(void* data, uint64 dataByte)
    {
        Buffer* staging = api->CreateBuffer(dataByte, BUFFER_USAGE_INDEX_BIT | BUFFER_USAGE_TRANSFER_SRC_BIT, MEMORY_ALLOCATION_TYPE_CPU);
        
        byte* mapped = api->MapBuffer(staging);
        std::memcpy(mapped, data, dataByte);
        api->UnmapBuffer(staging);
        
        Buffer* buffer = api->CreateBuffer(dataByte, BUFFER_USAGE_INDEX_BIT | BUFFER_USAGE_TRANSFER_DST_BIT, MEMORY_ALLOCATION_TYPE_GPU);
        
        api->ImmidiateCommands(graphicsQueue, immidiateContext.commandBuffer, immidiateContext.fence, [&](CommandBuffer* cmd)
        {
            BufferCopyRegion region = {};
            region.size = dataByte;
        
            api->CopyBuffer(cmd, staging, buffer, 1, &region);
        });
        
        api->DestroyBuffer(staging);

        return buffer;
    }

    void RenderingDevice::DestroyBuffer(Buffer* buffer)
    {
        api->DestroyBuffer(buffer);
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

    bool RenderingDevice::ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, VSyncMode mode)
    {
        bool result = api->ResizeSwapChain(swapchain, width, height, swapchainBufferCount, mode);
        SL_CHECK(!result, false);

        return false;
    }

    bool RenderingDevice::Present()
    {
        SwapChain* swapchain = Window::Get()->GetSwapChain();
        FrameData& frame = frameData[frameIndex];

        api->Present(graphicsQueue, swapchain, frame.renderSemaphore);
        frameIndex = (frameIndex + 1) % 2;

        return true;
    }



    const DeviceInfo& RenderingDevice::GetDeviceInfo() const
    {
        return context->GetDeviceInfo();
    }

    RenderingContext* RenderingDevice::GetContext() const
    {
        return context;
    }

    RenderingAPI* RenderingDevice::GetAPI() const
    {
        return api;
    }

    FrameData& RenderingDevice::GetFrameData()
    {
        return frameData[frameIndex];
    }

    const FrameData& RenderingDevice::GetFrameData() const
    {
        return frameData[frameIndex];
    }
    
    CommandQueue* RenderingDevice::GetGraphicsCommandQueue() const
    {
        return graphicsQueue;
    }

    QueueFamily RenderingDevice::GetGraphicsQueueFamily() const
    {
        return graphicsQueueFamily;
    }
}
