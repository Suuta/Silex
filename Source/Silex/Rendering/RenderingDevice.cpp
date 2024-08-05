
#include "PCH.h"

#include "Core/Window.h"
#include "Core/Engine.h"
#include "Rendering/RenderingDevice.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/RenderingUtility.h"
#include "Rendering/Camera.h"
#include "Rendering/Mesh.h"
#include "Rendering/Vulkan/VulkanStructures.h"
#include "Rendering/MeshFactory.h"
#include "Asset/TextureReader.h"

#include <imgui/imgui_internal.h>
#include <imgui/imgui.h>


namespace Silex
{
    namespace Test
    {
        struct SceneData
        {
            glm::mat4 world = glm::mat4(1.0f);
            glm::mat4 view;
            glm::mat4 projection;
        };

        struct GridData
        {
            glm::mat4 view;
            glm::mat4 projection;
            glm::vec4 pos;
        };

        struct Light
        {
            glm::vec4 directionalLight;
            glm::vec4 cameraPos;
        };
    }

    //======================================
    // 定数
    //======================================
    // TODO: エディターの設定項目として扱える形にする
    const uint32 swapchainBufferCount = 3;

    //======================================
    // シェーダーコンパイラ
    //======================================
    static ShaderCompiler shaserCompiler;


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
        api->WaitDevice();

        sldelete(cubeMesh);
        sldelete(sphereMesh);
        sldelete(sponzaMesh);

        api->UnmapBuffer(sceneUBO);
        api->DestroyBuffer(sceneUBO);
        api->UnmapBuffer(gridUBO);
        api->DestroyBuffer(gridUBO);
        api->UnmapBuffer(lightUBO);
        api->DestroyBuffer(lightUBO);
        api->DestroyBuffer(vb);
        api->DestroyBuffer(ib);
        api->DestroyTexture(sceneColorTexture);
        api->DestroyTexture(sceneDepthTexture);
        api->DestroyTexture(textureFile);
        api->DestroyShader(shader);
        api->DestroyShader(gridShader);
        api->DestroyShader(blitShader);
        api->DestroyDescriptorSet(descriptorSet);
        api->DestroyDescriptorSet(textureSet);
        api->DestroyDescriptorSet(lightSet);
        api->DestroyDescriptorSet(gridSet);
        api->DestroyDescriptorSet(blitSet);
        api->DestroyPipeline(pipeline);
        api->DestroyPipeline(gridPipeline);
        api->DestroyPipeline(blitPipeline);
        api->DestroyRenderPass(scenePass);
        api->DestroyFramebuffer(sceneFramebuffer);
        api->DestroySampler(sceneSampler);

        for (uint32 i = 0; i < frameData.size(); i++)
        {
            DestroyPendingResources(i);

            api->DestroyCommandBuffer(frameData[i].commandBuffer);
            api->DestroyCommandPool(frameData[i].commandPool);
            api->DestroySemaphore(frameData[i].presentSemaphore);
            api->DestroySemaphore(frameData[i].renderSemaphore);
            api->DestroyFence(frameData[i].fence);
        }

        api->DestroyCommandBuffer(immidiateContext.commandBuffer);
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
        SL_CHECK(graphicsQueueFamily == RENDER_INVALID_ID, false);

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
        bool result          = false;

        // GPU 待機
        if (frameData[frameIndex].waitingSignal)
        {
            result = api->WaitFence(frame.fence);
            SL_CHECK(!result, false);

            frameData[frameIndex].waitingSignal = false;
        }

        // 削除キュー実行
        DestroyPendingResources(frameIndex);

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
        frameData[frameIndex].waitingSignal = true;

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
            depth.format        = RENDERING_FORMAT_D24_UNORM_S8_UINT;

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

            sceneColorTexture = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, size.x, size.y, false);
            sceneDepthTexture = CreateTexture2D(RENDERING_FORMAT_D24_UNORM_S8_UINT,   size.x, size.y, false);

            TextureHandle* textures[] = { sceneColorTexture , sceneDepthTexture };
            sceneFramebuffer = CreateFramebuffer(scenePass, std::size(textures), textures, size.x, size.y);
        }

        InputLayout layout;
        layout.AddAttribute(0, VERTEX_BUFFER_FORMAT_R32G32B32);
        layout.AddAttribute(1, VERTEX_BUFFER_FORMAT_R32G32B32);
        layout.AddAttribute(2, VERTEX_BUFFER_FORMAT_R32G32);
        layout.AddAttribute(3, VERTEX_BUFFER_FORMAT_R32G32B32);
        layout.AddAttribute(4, VERTEX_BUFFER_FORMAT_R32G32B32);

        VertexInput* input = api->CreateInputLayout(1, &layout);

        {
            PipelineInputAssemblyState ia = {};
            PipelineRasterizationState rs = {};
            PipelineMultisampleState   ms = {};
            PipelineDepthStencilState  ds = {};
            PipelineColorBlendState    bs = {false};

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Triangle.glsl", compiledData);
            shader   = api->CreateShader(compiledData);
            pipeline = api->CreateGraphicsPipeline(shader, input, ia, rs, ms, ds, bs, scenePass);

            api->DestroyInputLayout(input);
        }

        {
            PipelineInputAssemblyState ia = {};
            PipelineRasterizationState rs = {};
            PipelineMultisampleState   ms = {};
            PipelineDepthStencilState  ds = {};
            PipelineColorBlendState    bs = {true};

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Grid.glsl", compiledData);
            gridShader   = api->CreateShader(compiledData);
            gridPipeline = api->CreateGraphicsPipeline(gridShader, nullptr, ia, rs, ms, ds, bs, scenePass);
        }

        {
            PipelineInputAssemblyState ia = {};
            PipelineRasterizationState rs = {};
            PipelineMultisampleState   ms = {};
            PipelineDepthStencilState  ds = {};
            PipelineColorBlendState    bs = { false };

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
            Test::SceneData sceneData;
            sceneData.world      = glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0));
            sceneData.view       = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
            sceneData.projection = glm::perspectiveFov(glm::radians(60.0f), (float)size.x, (float)size.y, 0.1f, 1000.0f);
        
            sceneUBO = api->CreateBuffer(sizeof(Test::SceneData), (BUFFER_USAGE_UNIFORM_BIT | BUFFER_USAGE_TRANSFER_DST_BIT), MEMORY_ALLOCATION_TYPE_CPU);
            mappedSceneData = api->MapBuffer(sceneUBO);
            std::memcpy(mappedSceneData, &sceneData, sizeof(Test::SceneData));
        }

        {
            Test::GridData gridData;
            gridData.pos        = glm::vec4(0, 0, 0, 1000.0f);
            gridData.view       = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
            gridData.projection = glm::perspectiveFov(glm::radians(60.0f), (float)size.x, (float)size.y, 0.1f, 1000.0f);

            gridUBO = api->CreateBuffer(sizeof(Test::GridData), (BUFFER_USAGE_UNIFORM_BIT | BUFFER_USAGE_TRANSFER_DST_BIT), MEMORY_ALLOCATION_TYPE_CPU);
            mappedGridData = api->MapBuffer(gridUBO);
            std::memcpy(mappedGridData, &gridData, sizeof(Test::GridData));
        }

        {
            Test::Light lightData;
            lightData.directionalLight = glm::vec4(1.0, 1.0, 0.0, 0.0);
            lightData.cameraPos        = glm::vec4(0.0, 0.0, 0.0, 0.0);

            lightUBO = api->CreateBuffer(sizeof(Test::Light), (BUFFER_USAGE_UNIFORM_BIT | BUFFER_USAGE_TRANSFER_DST_BIT), MEMORY_ALLOCATION_TYPE_CPU);
            mappedLightData = api->MapBuffer(lightUBO);
            std::memcpy(mappedLightData, &lightData, sizeof(Test::Light));
        }

        {
            TextureReader reader;
            byte* pixels   = reader.Read8bit("Assets/Textures/gray.png");
            int32 width    = reader.data.width;
            int32 height   = reader.data.height;
            int64 dataSize = reader.data.byteSize;

            textureFile = CreateTextureFromMemory(pixels, dataSize, width, height, true);
        }

        {
            DescriptorSetInfo uboinfo;
            uboinfo.AddBuffer(0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, sceneUBO);
            descriptorSet = CreateDescriptorSet(shader, 0, uboinfo);

            DescriptorSetInfo texinfo;
            texinfo.AddTexture(0, DESCRIPTOR_TYPE_IMAGE_SAMPLER, textureFile, sceneSampler);
            textureSet = CreateDescriptorSet(shader, 1, texinfo);

            DescriptorSetInfo lightinfo;
            lightinfo.AddBuffer(0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, lightUBO);
            lightSet = CreateDescriptorSet(shader, 2, lightinfo);
        }

        {
            DescriptorSetInfo gridinfo;
            gridinfo.AddBuffer(0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, gridUBO);
            gridSet = CreateDescriptorSet(gridShader, 0, gridinfo);
        }

        {
            DescriptorSetInfo blitinfo;
            blitinfo.AddTexture(0, DESCRIPTOR_TYPE_IMAGE_SAMPLER, sceneColorTexture, sceneSampler);
            blitSet = CreateDescriptorSet(blitShader, 0, blitinfo);
        }

        cubeMesh   = MeshFactory::Cube();
        sphereMesh = MeshFactory::Sphere();

        sponzaMesh = slnew(Mesh);
        sponzaMesh->Load("Assets/Models/Sponza/Sponza.fbx");
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

                SL_LOG_TRACE("Resize SceneFramebuffer: {}, {}", sceneFramebufferSize.x, sceneFramebufferSize.y);
                RESIZE(sceneFramebufferSize.x, sceneFramebufferSize.y);
            }

            // シーン描画
            ImGui::Image(((VulkanDescriptorSet*)blitSet)->descriptorSet, content);
        }

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Begin("Info", nullptr, isUsingCamera ? ImGuiWindowFlags_NoInputs : 0);
        //ImGui::Separator();
        ImGui::Text("FPS: %d", Engine::Get()->GetFrameRate());
        //ImGui::Separator();
        //ImGui::Text("ReginonMin:     %d, %d", (int)contentMin.x, (int)contentMin.y);
        //ImGui::Text("ReginonMax:     %d, %d", (int)contentMax.x, (int)contentMax.y);
        //ImGui::Text("viewportOffset: %d, %d", (int)viewportOffset.x, (int)viewportOffset.y);
        //ImGui::Text("Hover:          %s",     bHoveredViewport? "true" : "false");
        ImGui::End();

        ImGui::PopStyleVar(2);
    }

    void RenderingDevice::RESIZE(uint32 width, uint32 height)
    {
        if (width == 0 || height == 0)
            return;

        DestroyDescriptorSet(blitSet);
        DestroyFramebuffer(sceneFramebuffer);

        DestroyTexture(sceneColorTexture);
        DestroyTexture(sceneDepthTexture);

        sceneColorTexture = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height, false);
        sceneDepthTexture = CreateTexture2D(RENDERING_FORMAT_D24_UNORM_S8_UINT,   width, height, false);

        TextureHandle* textures[] = { sceneColorTexture , sceneDepthTexture };
        sceneFramebuffer = CreateFramebuffer(scenePass, std::size(textures), textures, width, height);

        DescriptorSetInfo setinfo;
        setinfo.AddTexture(0, DESCRIPTOR_TYPE_IMAGE_SAMPLER, sceneColorTexture, sceneSampler);

        blitSet = CreateDescriptorSet(blitShader, 0, setinfo);
    }

    void RenderingDevice::DRAW(Camera* camera)
    {
        FrameData& frame = frameData[frameIndex];
        const auto& windowSize   = Window::Get()->GetSize();
        const auto& viewportSize = camera->GetViewportSize();

        {
            Test::SceneData sceneData;
            sceneData.projection = camera->GetProjectionMatrix();
            sceneData.view       = camera->GetViewMatrix();
            std::memcpy(mappedSceneData, &sceneData, sizeof(Test::SceneData));
        }

        {
            Test::GridData gridData;
            gridData.projection = camera->GetProjectionMatrix();
            gridData.view       = camera->GetViewMatrix();
            gridData.pos        = glm::vec4(camera->GetPosition(), camera->GetFarPlane());
            std::memcpy(mappedGridData, &gridData, sizeof(Test::GridData));
        }

        {
            Test::Light lightData;
            lightData.directionalLight = glm::vec4(1.0, 1.0, 0.0, 0.0);
            lightData.cameraPos        = glm::vec4(camera->GetPosition(), 0.0);
            std::memcpy(mappedLightData, &lightData, sizeof(Test::Light));
        }

        api->SetViewport(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);
        api->SetScissor(frame.commandBuffer,  0, 0, viewportSize.x, viewportSize.y);

        {
            defaultClearColor.color          = {0.0f, 0.0f, 0.0f, 1.0f};
            defaultClearDepthStencil.depth   = 1.0f;
            defaultClearDepthStencil.stencil = 0;

            RenderPassClearValue clearValue[] = { defaultClearColor, defaultClearDepthStencil };
            api->BeginRenderPass(frame.commandBuffer, scenePass, sceneFramebuffer, COMMAND_BUFFER_TYPE_PRIMARY, std::size(clearValue), clearValue);
            
            {
                api->BindPipeline(frame.commandBuffer, gridPipeline);
                api->BindDescriptorSet(frame.commandBuffer, gridSet, 0);
                api->Draw(frame.commandBuffer, 6, 1, 0, 0);
            }

            {
                uint64 offset = 0;
                api->BindPipeline(frame.commandBuffer, pipeline);
                api->BindDescriptorSet(frame.commandBuffer, descriptorSet, 0);
                api->BindDescriptorSet(frame.commandBuffer, textureSet, 1);
                api->BindDescriptorSet(frame.commandBuffer, lightSet, 2);

                Buffer* cvb       = sphereMesh->GetMeshSource()->GetVertexBuffer();
                Buffer* cib       = sphereMesh->GetMeshSource()->GetIndexBuffer();
                uint32 indexCount = sphereMesh->GetMeshSource()->GetIndexCount();
                api->BindVertexBuffers(frame.commandBuffer, 1, &cvb, &offset);
                api->BindIndexBuffer(frame.commandBuffer, cib, INDEX_BUFFER_FORMAT_UINT32, 0);
                api->DrawIndexed(frame.commandBuffer, indexCount, 1, 0, 0, 0);
            }

            if (true)
            {
                for (MeshSource* source : sponzaMesh->GetMeshSources())
                {
                    uint64 offset = 0;
                    Buffer* vb        = source->GetVertexBuffer();
                    Buffer* ib        = source->GetIndexBuffer();
                    uint32 indexCount = source->GetIndexCount();
                    api->BindVertexBuffers(frame.commandBuffer, 1, &vb, &offset);
                    api->BindIndexBuffer(frame.commandBuffer, ib, INDEX_BUFFER_FORMAT_UINT32, 0);
                    api->DrawIndexed(frame.commandBuffer, indexCount, 1, 0, 0, 0);
                }
            }

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




    TextureHandle* RenderingDevice::CreateTextureFromMemory(const byte* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap)
    {
        // ステージングバッファ
        Buffer* staging = api->CreateBuffer(dataSize, BUFFER_USAGE_TRANSFER_SRC_BIT, MEMORY_ALLOCATION_TYPE_CPU);

        // ステージングにテクスチャデータをコピー
        void* mappedPtr = api->MapBuffer(staging);
        std::memcpy(mappedPtr, pixelData, dataSize);
        api->UnmapBuffer(staging);

        // テクスチャ生成
        TextureUsageFlags flags = TEXTURE_USAGE_COPY_SRC_BIT | TEXTURE_USAGE_COPY_DST_BIT;
        TextureHandle* texture = _CreateTexture(TEXTURE_TYPE_2D, RENDERING_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, genMipmap, flags);

        // データコピー
        api->ImmidiateCommands(graphicsQueue, immidiateContext.commandBuffer, immidiateContext.fence, [&](CommandBuffer* cmd)
        {
            TextureSubresourceRange range = {};
            range.aspect = TEXTURE_ASPECT_COLOR_BIT;

            TextureBarrierInfo info = {};
            info.texture      = texture;
            info.subresources = range;
            info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.oldLayout    = TEXTURE_LAYOUT_UNDEFINED;
            info.newLayout    = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;

            api->PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            TextureSubresource subresource = {};
            subresource.aspect     = TEXTURE_ASPECT_COLOR_BIT;

            BufferTextureCopyRegion region = {};
            region.bufferOffset        = 0;
            region.textureOffset       = { 0, 0, 0 };
            region.textureRegionSize   = { width, height, 1 };
            region.textureSubresources = subresource;

            api->CopyBufferToTexture(cmd, staging, texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

            if (genMipmap)
            {
                // ミップマップ生成
                _GenerateMipmaps(cmd, texture, width, height, 1, 1, TEXTURE_ASPECT_COLOR_BIT);

                // シェーダーリードに移行 (ミップマップ生成時のコピーでコピーソースに移行するため、コピーソース -> シェーダーリード)
                info.oldLayout = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                info.newLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                api->PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
            }
            else
            {
                // シェーダーリードに移行 (バッファからの転送でレイアウト変更がないため、コピー先 -> シェーダーリード)
                info.oldLayout = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
                info.newLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                api->PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
            }
        });

        api->DestroyBuffer(staging);
        return texture;
    }

    TextureHandle* RenderingDevice::CreateTexture2D(RenderingFormat format, uint32 width, uint32 height, bool genMipmap)
    {
        return _CreateTexture(TEXTURE_TYPE_2D, format, width, height, 1, 1, genMipmap, 0);
    }

    TextureHandle* RenderingDevice::CreateTexture2DArray(RenderingFormat format, uint32 width, uint32 height, uint32 array, bool genMipmap)
    {
        return _CreateTexture(TEXTURE_TYPE_2D_ARRAY, format, width, height, 1, array, genMipmap, 0);
    }

    TextureHandle* RenderingDevice::CreateTextureCube(RenderingFormat format, uint32 width, uint32 height, bool genMipmap)
    {
        return _CreateTexture(TEXTURE_TYPE_CUBE, format, width, height, 1, 1, genMipmap, 0);
    }

    void RenderingDevice::DestroyTexture(TextureHandle* texture)
    {
        FrameData& frame = frameData[frameIndex];
        frame.pendingResources.texture.push_back(texture);
    }



    void RenderingDevice::_GenerateMipmaps(CommandBuffer* cmd, TextureHandle* texture, uint32 width, uint32 height, uint32 depth, uint32 array, TextureAspectFlags aspect)
    {
        auto mipmaps = RenderingUtility::CalculateMipmap(width, height);

        // コピー回数は (ミップ数 - 1)
        for (uint32 mipLevel = 0; mipLevel < mipmaps.size() - 1; mipLevel++)
        {
            // バリア
            TextureBarrierInfo info = {};
            info.texture                    = texture;
            info.subresources.aspect        = aspect;
            info.subresources.baseMipLevel  = mipLevel;
            info.subresources.mipLevelCount = 1;
            info.subresources.baseLayer     = 0;
            info.subresources.layerCount    = 1;
            info.srcAccess                  = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess                  = BARRIER_ACCESS_MEMORY_WRITE_BIT | BARRIER_ACCESS_MEMORY_READ_BIT;
            info.oldLayout                  = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
            info.newLayout                  = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            api->PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            // Blit
            Extent srcoffset = mipmaps[mipLevel + 0];
            Extent dstoffset = mipmaps[mipLevel + 1];

            // NOTE:
            // srcImageがVK_IMAGE_TYPE_1DまたはVK_IMAGE_TYPE_2D型の場合、pRegionsの各要素について、srcOffsets[0].zは0、srcOffsets[1].zは1でなければなりません。
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdBlitImage.html#VUID-vkCmdBlitImage-srcImage-00247

            TextureBlitRegion region = {};
            region.srcOffset[0] = { 0, 0, 0 };
            region.dstOffset[0] = { 0, 0, 0 };
            region.srcOffset[1] = srcoffset;
            region.dstOffset[1] = dstoffset;

            region.srcSubresources.aspect     = aspect;
            region.srcSubresources.mipLevel   = mipLevel;
            region.srcSubresources.baseLayer  = 0;
            region.srcSubresources.layerCount = array;

            region.dstSubresources.aspect     = aspect;
            region.dstSubresources.mipLevel   = mipLevel + 1;
            region.dstSubresources.baseLayer  = 0;
            region.dstSubresources.layerCount = array;

            api->BlitTexture(cmd, texture, TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture, TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, SAMPLER_FILTER_LINEAR);
        }

        //------------------------------------------------------------------------
        // 最後のミップレベルはコピー先としか利用せず、コピーソースレイアウトに移行していない
        // 後で実行されるレイアウト移行時に、ミップレベル間でレイアウトの不一致があると不便なので
        // 最後のミップレベルのレイアウトも [コピー先 → コピーソース] に移行させておく
        //------------------------------------------------------------------------
        // バリア
        TextureSubresourceRange range = {};
        range.aspect        = TEXTURE_ASPECT_COLOR_BIT;
        range.baseMipLevel  = mipmaps.size() - 1;
        range.mipLevelCount = 1;

        TextureBarrierInfo info = {};
        info.texture      = texture;
        info.subresources = range;
        info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
        info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT | BARRIER_ACCESS_MEMORY_READ_BIT;
        info.oldLayout    = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
        info.newLayout    = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        api->PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
    }

    TextureHandle* RenderingDevice::_CreateTexture(TextureType type, RenderingFormat format, uint32 width, uint32 height, uint32 depth, uint32 array, bool genMipmap, TextureUsageFlags additionalFlags)
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



    Buffer* RenderingDevice::CreateUniformBuffer(void* data, uint64 dataByte)
    {
        return nullptr;
    }

    Buffer* RenderingDevice::CreateStorageBuffer(void* data, uint64 dataByte)
    {
        return nullptr;
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
        FrameData& frame = frameData[frameIndex];
        frame.pendingResources.buffer.push_back(buffer);
    }

    Buffer* RenderingDevice::_CreateBuffer()
    {
        return nullptr;
    }


    FramebufferHandle* RenderingDevice::CreateFramebuffer(RenderPass* renderpass, uint32 numTexture, TextureHandle** textures, uint32 width, uint32 height)
    {
        FramebufferHandle* framebuffer = api->CreateFramebuffer(renderpass, numTexture, textures, width, height);
        SL_CHECK(!framebuffer, nullptr);

        return framebuffer;
    }

    void RenderingDevice::DestroyFramebuffer(FramebufferHandle* framebuffer)
    {
        FrameData& frame = frameData[frameIndex];
        frame.pendingResources.framebuffer.push_back(framebuffer);
    }



    DescriptorSet* RenderingDevice::CreateDescriptorSet(ShaderHandle* shader, uint32 setIndex, const DescriptorSetInfo& setInfo)
    {
        DescriptorSetInfo data = setInfo;

        DescriptorSet* descriptorSet = api->CreateDescriptorSet(data.infos.size(), data.infos.data(), shader, setIndex);
        SL_CHECK(!descriptorSet, nullptr);

        return descriptorSet;
    }

    void RenderingDevice::DestroyDescriptorSet(DescriptorSet* set)
    {
        FrameData& frame = frameData[frameIndex];
        frame.pendingResources.descriptorset.push_back(set);
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



    void RenderingDevice::DestroyPendingResources(uint32 frame)
    {
        FrameData& f = frameData[frame];

        for (Buffer* buffer : f.pendingResources.buffer)
        {
            api->DestroyBuffer(buffer);
        }

        f.pendingResources.buffer.clear();

        for (TextureHandle* texture : f.pendingResources.texture)
        {
            api->DestroyTexture(texture);
        }

        f.pendingResources.texture.clear();

        for (Sampler* sampler : f.pendingResources.sampler)
        {
            api->DestroySampler(sampler);
        }

        f.pendingResources.sampler.clear();

        for (DescriptorSet* set : f.pendingResources.descriptorset)
        {
            api->DestroyDescriptorSet(set);
        }

        f.pendingResources.descriptorset.clear();

        for (FramebufferHandle* framebuffer : f.pendingResources.framebuffer)
        {
            api->DestroyFramebuffer(framebuffer);
        }

        f.pendingResources.framebuffer.clear();

        for (ShaderHandle* shader : f.pendingResources.shader)
        {
            api->DestroyShader(shader);
        }

        f.pendingResources.shader.clear();

        for (Pipeline* pipeline : f.pendingResources.pipeline)
        {
            api->DestroyPipeline(pipeline);
        }

        f.pendingResources.pipeline.clear();
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
