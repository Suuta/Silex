
#include "PCH.h"

#include "Core/Window.h"
#include "Core/Engine.h"
#include "Asset/TextureReader.h"
#include "Rendering/RHI.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/RenderingUtility.h"
#include "Rendering/Camera.h"
#include "Rendering/Mesh.h"
#include "Rendering/MeshFactory.h"

#include <imgui/imgui_internal.h>
#include <imgui/imgui.h>


namespace Silex
{
    Mesh* cubeMesh   = nullptr;
    Mesh* sponzaMesh = nullptr;

    namespace Test
    {
        struct Transform
        {
            glm::mat4 world      = glm::mat4(1.0f);
            glm::mat4 view       = glm::mat4(1.0f);
            glm::mat4 projection = glm::mat4(1.0f);
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

        struct EquirectangularData
        {
            glm::mat4 view[6];
            glm::mat4 projection;
        };

        struct MaterialUBO
        {
            glm::vec3 albedo;
            float     metallic;
            glm::vec3 emission;
            float     roughness;
            glm::vec2 textureTiling;
        };

        struct SceneUBO
        {
            glm::vec4 lightDir;
            glm::vec4 lightColor;
            glm::vec4 cameraPosition;
            glm::mat4 invViewProjection;
        };

        struct EnvironmentUBO
        {
            glm::mat4 view;
            glm::mat4 projection;
        };

        struct CompositUBO
        {
            float exposure;
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


    RHI* RHI::Get()
    {
        return instance;
    }

    RHI::RHI()
    {
        instance = this;
    }

    RHI::~RHI()
    {
        api->WaitDevice();

        sldelete(cubeMesh);
        sldelete(sponzaMesh);

        CleanupGBuffer();
        CleanupLightingBuffer();
        CleanupEnvironmentBuffer();

        api->UnmapBuffer(sceneUBO);
        api->DestroyBuffer(sceneUBO);
        api->UnmapBuffer(gridUBO);
        api->DestroyBuffer(gridUBO);
        api->UnmapBuffer(lightUBO);
        api->DestroyBuffer(lightUBO);
        api->UnmapBuffer(equirectangularUBO);
        api->DestroyBuffer(equirectangularUBO);
        api->DestroyBuffer(vb);
        api->DestroyBuffer(ib);
        api->DestroyTexture(sceneColorTexture);
        api->DestroyTexture(sceneDepthTexture);
        api->DestroyTexture(textureFile);
        api->DestroyTexture(envTexture);
        api->DestroyTexture(cubemap);
        api->DestroyTexture(compositTexture);
        api->DestroyShader(shader);
        api->DestroyShader(gridShader);
        api->DestroyShader(compositShader);
        api->DestroyShader(skyShader);
        api->DestroyShader(equirectangularShader);
        api->DestroyDescriptorSet(equirectangularSet);
        api->DestroyDescriptorSet(descriptorSet);
        api->DestroyDescriptorSet(textureSet);
        api->DestroyDescriptorSet(lightSet);
        api->DestroyDescriptorSet(gridSet);
        api->DestroyDescriptorSet(compositSet);
        api->DestroyDescriptorSet(imageSet);
        api->DestroyPipeline(pipeline);
        api->DestroyPipeline(gridPipeline);
        api->DestroyPipeline(compositPipeline);
        api->DestroyPipeline(skyPipeline);
        api->DestroyPipeline(equirectangularPipeline);
        api->DestroyRenderPass(compositPass);
        api->DestroyRenderPass(equirectangularPass);
        api->DestroyRenderPass(scenePass);
        api->DestroyFramebuffer(equirectangularFB);
        api->DestroyFramebuffer(sceneFramebuffer);
        api->DestroyFramebuffer(compositFB);
        api->DestroySampler(sceneSampler);
        api->DestroySampler(cubemapSampler);

        for (uint32 i = 0; i < frameData.size(); i++)
        {
            _DestroyPendingResources(i);

            api->DestroyCommandBuffer(frameData[i].commandBuffer);
            api->DestroyCommandPool(frameData[i].commandPool);
            api->DestroySemaphore(frameData[i].presentSemaphore);
            api->DestroySemaphore(frameData[i].renderSemaphore);
            api->DestroyFence(frameData[i].fence);

            sldelete(frameData[i].pendingResources);
        }

        api->DestroyCommandBuffer(immidiateContext.commandBuffer);
        api->DestroyCommandPool(immidiateContext.commandPool);
        api->DestroyFence(immidiateContext.fence);
        api->DestroyCommandQueue(graphicsQueue);

        context->DestroyRendringAPI(api);

        instance = nullptr;
    }

    bool RHI::Initialize(RenderingContext* renderingContext)
    {
        context = renderingContext;

        // レンダーAPI実装クラスを生成
        api = context->CreateRendringAPI();
        SL_CHECK(!api->Initialize(), false);
       
        // グラフィックスをサポートするキューファミリを取得
        graphicsQueueID = api->QueryQueueID(QUEUE_FAMILY_GRAPHICS_BIT, Window::Get()->GetSurface());
        SL_CHECK(graphicsQueueID == RENDER_INVALID_ID, false);

        // コマンドキュー生成
        graphicsQueue = api->CreateCommandQueue(graphicsQueueID);
        SL_CHECK(!graphicsQueue, false);
      
        // フレームデータ生成
        for (uint32 i = 0; i < frameData.size(); i++)
        {
            frameData[i].pendingResources = slnew(PendingDestroyResourceQueue);

            // コマンドプール生成
            frameData[i].commandPool = api->CreateCommandPool(graphicsQueueID);
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
        immidiateContext.commandPool = api->CreateCommandPool(graphicsQueueID);
        SL_CHECK(!immidiateContext.commandPool, false);

        immidiateContext.commandBuffer = api->CreateCommandBuffer(immidiateContext.commandPool);
        SL_CHECK(!immidiateContext.commandBuffer, false);

        immidiateContext.fence = api->CreateFence();
        SL_CHECK(!immidiateContext.fence, false);

        return true;
    }

    bool RHI::BeginFrame()
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
        _DestroyPendingResources(frameIndex);

        // 描画先スワップチェインバッファを取得
        swapchainFramebuffer = api->GetCurrentBackBuffer(swapchain, frame.presentSemaphore);
        SL_CHECK(!swapchainFramebuffer, false);

        // コマンドバッファ開始
        result = api->BeginCommandBuffer(frame.commandBuffer);
        SL_CHECK(!result, false);

        return true;
    }

    bool RHI::EndFrame()
    {
        SwapChain* swapchain = Window::Get()->GetSwapChain();
        FrameData& frame     = frameData[frameIndex];
  
        api->EndCommandBuffer(frame.commandBuffer);

        api->SubmitQueue(graphicsQueue, frame.commandBuffer, frame.fence, frame.presentSemaphore, frame.renderSemaphore);
        frameData[frameIndex].waitingSignal = true;

        return true;
    }

    void RHI::TEST()
    {
        auto size = Window::Get()->GetSize();
        swapchainPass = api->GetSwapChainRenderPass(Window::Get()->GetSwapChain());

        cubeMesh   = MeshFactory::Cube();
        sponzaMesh = MeshFactory::Sponza();

        InputLayout layout;
        layout.Binding(0);
        layout.Attribute(0, VERTEX_BUFFER_FORMAT_R32G32B32);
        layout.Attribute(1, VERTEX_BUFFER_FORMAT_R32G32B32);
        layout.Attribute(2, VERTEX_BUFFER_FORMAT_R32G32);
        layout.Attribute(3, VERTEX_BUFFER_FORMAT_R32G32B32);
        layout.Attribute(4, VERTEX_BUFFER_FORMAT_R32G32B32);

        SamplerInfo info = {};
        sceneSampler = api->CreateSampler(info);

        {
            TextureReader reader;
            byte* pixels = reader.Read8bit("Assets/Textures/default.png");
            textureFile = CreateTextureFromMemory(pixels, reader.data.byteSize, reader.data.width, reader.data.height, true);
        }

        {
            TextureReader reader;
            byte* pixels = reader.Read8bit("Assets/Textures/cloud.png");
            envTexture = CreateTextureFromMemory(pixels, reader.data.byteSize, reader.data.width, reader.data.height, true);

            SamplerInfo samplerInfo = {};
            cubemapSampler = api->CreateSampler(samplerInfo);
        }

        {
            // 環境マップ変換 レンダーパス
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.loadOp        = ATTACHMENT_LOAD_OP_DONT_CARE;
            color.storeOp       = ATTACHMENT_STORE_OP_STORE;
            color.samples       = TEXTURE_SAMPLES_1;
            color.format        = RENDERING_FORMAT_R8G8B8A8_UNORM;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            Subpass subpass = {};
            subpass.colorReferences.push_back(colorRef);

            RenderPassClearValue clear;
            clear.SetFloat(0, 0, 0, 1);

            equirectangularPass = api->CreateRenderPass(1, &color, 1, &subpass, 0, nullptr, 1, &clear);


            // シェーダー
            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/EquirectangularToCubeMap.glsl", compiledData);
            equirectangularShader = api->CreateShader(compiledData);

            // キューブマップ
            cubemap = CreateTextureCube(RENDERING_FORMAT_R8G8B8A8_UNORM, 1024, 1024, false);

            // キューブマップ UBO
            Test::EquirectangularData data;
            data.projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 1.0f);
            data.view[0]    = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3( 0.0f, -1.0f,  0.0f));
            data.view[1]    = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3( 0.0f, -1.0f,  0.0f));
            data.view[2]    = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3( 0.0f,  0.0f, -1.0f));
            data.view[3]    = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3( 0.0f,  0.0f, -1.0f));
            data.view[4]    = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3( 0.0f, -1.0f,  0.0f));
            data.view[5]    = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3( 0.0f, -1.0f,  0.0f));
            equirectangularUBO = CreateUniformBuffer(&data, sizeof(Test::EquirectangularData), &mappedEquirectanguler);

            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .InputLayout(1, &layout)
                .Rasterizer(POLYGON_CULL_BACK, POLYGON_FRONT_FACE_CLOCKWISE)
                .Depth(false, false)
                .Blend(false, 1)
                .Value();

            equirectangularPipeline = api->CreateGraphicsPipeline(equirectangularShader, &pipelineInfo, equirectangularPass);

            // デスクリプタ
            DescriptorSetInfo info;
            info.AddBuffer(0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, equirectangularUBO);
            info.AddTexture(1, DESCRIPTOR_TYPE_IMAGE_SAMPLER, envTexture, cubemapSampler);
            equirectangularSet = CreateDescriptorSet(equirectangularShader, 0, info);


            equirectangularFB = CreateFramebuffer(equirectangularPass, 1, &cubemap, 1024, 1024);

            // キューブマップ書き込み
            api->ImmidiateCommands(graphicsQueue, immidiateContext.commandBuffer, immidiateContext.fence, [&](CommandBuffer* cmd)
            {
                MeshSource* ms = cubeMesh->GetMeshSource();

                api->SetViewport(cmd, 0, 0, 1024, 1024);
                api->SetScissor(cmd,  0, 0, 1024, 1024);

                // layout: undef
                api->BeginRenderPass(cmd, equirectangularPass, equirectangularFB, COMMAND_BUFFER_TYPE_PRIMARY);

                {
                    api->BindPipeline(cmd, equirectangularPipeline);
                    api->BindDescriptorSet(cmd, equirectangularSet, 0);
                    api->BindVertexBuffer(cmd, ms->GetVertexBuffer(), 0);
                    api->BindIndexBuffer(cmd, ms->GetIndexBuffer(), INDEX_BUFFER_FORMAT_UINT32, 0);
                    api->DrawIndexed(cmd, ms->GetIndexCount(), 1, 0, 0, 0);
                }

                // layout: shader_read_only
                api->EndRenderPass(cmd);

#if 0
                TextureBarrierInfo info = {};
                info.texture      = cubemap;
                info.subresources = {};
                info.srcAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
                info.dstAccess    = BARRIER_ACCESS_MEMORY_WRITE_BIT;
                info.oldLayout    = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                info.newLayout    = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
                api->PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

                _GenerateMipmaps(cmd, cubemap, 1024, 1024, 1, 6, TEXTURE_ASPECT_COLOR_BIT);

                info.oldLayout = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                info.newLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                api->PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);
#endif
            });
        }

        {
            // シーンパス
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

            RenderPassClearValue clear[2];
            clear[0].SetFloat(0, 0, 0, 1);
            clear[1].SetDepthStencil(1, 0);

            Attachment attachments[] = {color, depth};
            scenePass = api->CreateRenderPass(std::size(attachments), attachments, 1, &subpass, 0, nullptr, 2, clear);

            sceneColorTexture = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, size.x, size.y);
            sceneDepthTexture = CreateTexture2D(RENDERING_FORMAT_D24_UNORM_S8_UINT,   size.x, size.y);

            TextureHandle* textures[] = { sceneColorTexture , sceneDepthTexture };
            sceneFramebuffer = CreateFramebuffer(scenePass, std::size(textures), textures, size.x, size.y);
        }

        {
            // シーンBlitパス
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            color.storeOp       = ATTACHMENT_STORE_OP_STORE;
            color.samples       = TEXTURE_SAMPLES_1;
            color.format        = RENDERING_FORMAT_R8G8B8A8_UNORM;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            Subpass subpass = {};
            subpass.colorReferences.push_back(colorRef);

            RenderPassClearValue clear = {};
            clear.SetFloat(0, 0, 0, 1);

            compositPass    = api->CreateRenderPass(1, &color, 1, &subpass, 0, nullptr, 1, &clear);
            compositTexture = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM, size.x, size.y);
            compositFB      = CreateFramebuffer(compositPass, 1, &compositTexture, size.x, size.y);
        }

        {
            // スカイ
            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .InputLayout(1, &layout)
                .Rasterizer(POLYGON_CULL_BACK, POLYGON_FRONT_FACE_CLOCKWISE)
                .Depth(false, false)
                .Stencil(true, 0, COMPARE_OP_EQUAL)
                .Blend(false, 1)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Environment.glsl", compiledData);
            skyShader   = api->CreateShader(compiledData);
            skyPipeline = api->CreateGraphicsPipeline(skyShader, &pipelineInfo, scenePass);
        }

        {
            // グリッド
            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .Depth(true, true)
                .Blend(true, 1)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Grid.glsl", compiledData);
            gridShader   = api->CreateShader(compiledData);
            gridPipeline = api->CreateGraphicsPipeline(gridShader, &pipelineInfo, scenePass);
        }

        {
            // シーン
            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .InputLayout(1, &layout)
                .Depth(true, true)
                .Stencil(true, 1, COMPARE_OP_ALWAYS)
                .Blend(false, 1)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Triangle.glsl", compiledData);
            shader   = api->CreateShader(compiledData);
            pipeline = api->CreateGraphicsPipeline(shader, &pipelineInfo, scenePass);
        }

        {
            // Blit
            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .Blend(false, 1)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Composit.glsl", compiledData);
            compositShader   = api->CreateShader(compiledData);
            compositPipeline = api->CreateGraphicsPipeline(compositShader, &pipelineInfo, compositPass);
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

        sceneUBO    = CreateUniformBuffer(nullptr, sizeof(Test::Transform),   &mappedSceneData);
        gridUBO     = CreateUniformBuffer(nullptr, sizeof(Test::GridData),    &mappedGridData);
        lightUBO    = CreateUniformBuffer(nullptr, sizeof(Test::Light),       &mappedLightData);

        {
            DescriptorSetInfo uboinfo;
            uboinfo.AddBuffer(0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, sceneUBO);
            descriptorSet = CreateDescriptorSet(shader, 0, uboinfo);

            DescriptorSetInfo texinfo;
            texinfo.AddTexture(0, DESCRIPTOR_TYPE_IMAGE_SAMPLER, cubemap, sceneSampler);
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


        // Gバッファ初期化
        PrepareGBuffer(size.x, size.y);

        // ライティングバッファ
        PrepareLightingBuffer(size.x, size.y);

        // 環境マップ
        PrepareEnvironmentBuffer(size.x, size.y);

        {
            DescriptorSetInfo blitinfo;
            blitinfo.AddTexture(0, DESCRIPTOR_TYPE_IMAGE_SAMPLER,  lighting.color, sceneSampler);
            compositSet = CreateDescriptorSet(compositShader, 0, blitinfo);
        }

        {
            DescriptorSetInfo imageinfo;
            imageinfo.AddTexture(0, DESCRIPTOR_TYPE_IMAGE_SAMPLER, compositTexture, sceneSampler);
            imageSet = CreateDescriptorSet(compositShader, 0, imageinfo);
        }

        int i = 0;
    }

    void RHI::PrepareGBuffer(uint32 width, uint32 height)
    {
        gbuffer.albedo   = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM,          width, height);
        gbuffer.normal   = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM,          width, height);
        gbuffer.emission = CreateTexture2D(RENDERING_FORMAT_B10G11R11_UFLOAT_PACK32, width, height);
        gbuffer.id       = CreateTexture2D(RENDERING_FORMAT_R32_SINT,                width, height);
        gbuffer.depth    = CreateTexture2D(RENDERING_FORMAT_D24_UNORM_S8_UINT,       width, height);

        RenderPassClearValue clearvalues[5] = {};
        Attachment           attachments[5] = {};
        Subpass              subpass        = {};

        {
            // ベースカラー
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            color.storeOp       = ATTACHMENT_STORE_OP_STORE;
            color.samples       = TEXTURE_SAMPLES_1;
            color.format        = RENDERING_FORMAT_R8G8B8A8_UNORM;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpass.colorReferences.push_back(colorRef);
            attachments[0] = color;
            clearvalues[0].SetFloat(0, 0, 0, 0);

            // ノーマル
            Attachment normal = {};
            normal.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            normal.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            normal.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            normal.storeOp       = ATTACHMENT_STORE_OP_STORE;
            normal.samples       = TEXTURE_SAMPLES_1;
            normal.format        = RENDERING_FORMAT_R8G8B8A8_UNORM;

            AttachmentReference normalRef = {};
            normalRef.attachment = 1;
            normalRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpass.colorReferences.push_back(normalRef);
            attachments[1] = normal;
            clearvalues[1].SetFloat(0, 0, 0, 1);

            // エミッション
            Attachment emission = {};
            emission.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            emission.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            emission.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            emission.storeOp       = ATTACHMENT_STORE_OP_STORE;
            emission.samples       = TEXTURE_SAMPLES_1;
            emission.format        = RENDERING_FORMAT_B10G11R11_UFLOAT_PACK32;

            AttachmentReference emissionRef = {};
            emissionRef.attachment = 2;
            emissionRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpass.colorReferences.push_back(emissionRef);
            attachments[2] = emission;
            clearvalues[2].SetFloat(0, 0, 0, 1);

            // ID
            Attachment id = {};
            id.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            id.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            id.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            id.storeOp       = ATTACHMENT_STORE_OP_STORE;
            id.samples       = TEXTURE_SAMPLES_1;
            id.format        = RENDERING_FORMAT_R32_SINT;

            AttachmentReference idRef = {};
            idRef.attachment = 3;
            idRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            subpass.colorReferences.push_back(idRef);
            attachments[3] = id;
            clearvalues[3].SetInt(0, 0, 0, 1);

            // 深度
            Attachment depth = {};
            depth.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            depth.finalLayout   = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            depth.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            depth.storeOp       = ATTACHMENT_STORE_OP_STORE;
            depth.samples       = TEXTURE_SAMPLES_1;
            depth.format        = RENDERING_FORMAT_D24_UNORM_S8_UINT;

            AttachmentReference depthRef = {};
            depthRef.attachment = 4;
            depthRef.layout     = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            subpass.depthstencilReference = depthRef;
            attachments[4] = depth;
            clearvalues[4].SetDepthStencil(1.0f, 0);

            gbuffer.pass = api->CreateRenderPass(std::size(attachments), attachments, 1, &subpass, 0, nullptr, std::size(clearvalues), clearvalues);
        }

        {
            TextureHandle* attachments[] = { gbuffer.albedo, gbuffer.normal, gbuffer.emission, gbuffer.id, gbuffer.depth };
            gbuffer.framebuffer = CreateFramebuffer(gbuffer.pass, std::size(attachments), attachments, width, height);
        }

        {
            // Gバッファ―
            InputLayout layout;
            layout.Binding(0);
            layout.Attribute(0, VERTEX_BUFFER_FORMAT_R32G32B32);
            layout.Attribute(1, VERTEX_BUFFER_FORMAT_R32G32B32);
            layout.Attribute(2, VERTEX_BUFFER_FORMAT_R32G32);
            layout.Attribute(3, VERTEX_BUFFER_FORMAT_R32G32B32);
            layout.Attribute(4, VERTEX_BUFFER_FORMAT_R32G32B32);

            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .InputLayout(1, &layout)
                .Depth(true, true)
                .Stencil(true, 1, COMPARE_OP_ALWAYS)
                .Blend(false, 4)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/DeferredPrimitive.glsl", compiledData);
            gbuffer.shader   = api->CreateShader(compiledData);
            gbuffer.pipeline = api->CreateGraphicsPipeline(gbuffer.shader, &pipelineInfo, gbuffer.pass);
        }

        // ユニフォームバッファ
        gbuffer.materialUBO  = CreateUniformBuffer(nullptr, sizeof(Test::MaterialUBO), &gbuffer.mappedMaterial);
        gbuffer.transformUBO = CreateUniformBuffer(nullptr, sizeof(Test::Transform),   &gbuffer.mappedTransfrom);

        // トランスフォーム
        DescriptorSetInfo transformInfo = {};
        transformInfo.AddBuffer(0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, gbuffer.transformUBO);
        gbuffer.transformSet = CreateDescriptorSet(gbuffer.shader, 0, transformInfo);

        // マテリアル
        DescriptorSetInfo materialInfo = {};
        materialInfo.AddBuffer(0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, gbuffer.materialUBO);
        materialInfo.AddTexture(1, DESCRIPTOR_TYPE_IMAGE_SAMPLER, textureFile, sceneSampler);
        gbuffer.materialSet  = CreateDescriptorSet(gbuffer.shader, 1, materialInfo);
    }

    void RHI::PrepareLightingBuffer(uint32 width, uint32 height)
    {
        // 初期レイアウトとサブパスレイアウトの違い
        //https://community.khronos.org/t/initial-layout-in-vkattachmentdescription/109850/5

        // パス
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

        Subpass subpass = {};
        subpass.colorReferences.push_back(colorRef);

        RenderPassClearValue clear;
        clear.SetFloat(0, 0, 0, 1);

        lighting.pass        = api->CreateRenderPass(1, &color, 1, &subpass, 0, nullptr, 1, &clear);
        lighting.color       = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);
        lighting.framebuffer = CreateFramebuffer(lighting.pass, 1, &lighting.color, width, height);

        // パイプライン
        PipelineStateInfoBuilder builder;
        PipelineStateInfo pipelineInfo = builder
            .Depth(false, false)
            .Stencil(false)
            .Blend(false, 1)
            .Value();

        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/DeferredLighting.glsl", compiledData);
        lighting.shader   = api->CreateShader(compiledData);
        lighting.pipeline = api->CreateGraphicsPipeline(lighting.shader, &pipelineInfo, lighting.pass);

        // UBO
        lighting.sceneUBO = CreateUniformBuffer(nullptr, sizeof(Test::SceneUBO), &lighting.mappedScene);

        // セット
        DescriptorSetInfo textureInfo = {};
        textureInfo.AddTexture(0, DESCRIPTOR_TYPE_IMAGE_SAMPLER, gbuffer.albedo,   sceneSampler);
        textureInfo.AddTexture(1, DESCRIPTOR_TYPE_IMAGE_SAMPLER, gbuffer.normal,   sceneSampler);
        textureInfo.AddTexture(2, DESCRIPTOR_TYPE_IMAGE_SAMPLER, gbuffer.emission, sceneSampler);
        textureInfo.AddTexture(3, DESCRIPTOR_TYPE_IMAGE_SAMPLER, gbuffer.depth,    sceneSampler);
        textureInfo.AddBuffer(4, DESCRIPTOR_TYPE_UNIFORM_BUFFER, lighting.sceneUBO);

        lighting.set = CreateDescriptorSet(lighting.shader, 0, textureInfo);
    }

    void RHI::PrepareEnvironmentBuffer(uint32 width, uint32 height)
    {
        {
            Attachment color = {};
            color.initialLayout = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            color.loadOp        = ATTACHMENT_LOAD_OP_LOAD;
            color.storeOp       = ATTACHMENT_STORE_OP_STORE;
            color.samples       = TEXTURE_SAMPLES_1;
            color.format        = RENDERING_FORMAT_R16G16B16A16_SFLOAT;

            AttachmentReference colorRef = {};
            colorRef.attachment = 0;
            colorRef.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            Attachment depth = {};
            depth.initialLayout = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            depth.finalLayout   = TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            depth.loadOp        = ATTACHMENT_LOAD_OP_LOAD;
            depth.storeOp       = ATTACHMENT_STORE_OP_STORE;
            depth.samples       = TEXTURE_SAMPLES_1;
            depth.format        = RENDERING_FORMAT_D24_UNORM_S8_UINT;

            AttachmentReference depthRef = {};
            depthRef.attachment = 1;
            depthRef.layout     = TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            Subpass subpass = {};
            subpass.colorReferences.push_back(colorRef);
            subpass.depthstencilReference = depthRef;

            RenderPassClearValue clear[2];
            clear[0].SetFloat(0, 0, 0, 1);
            clear[1].SetDepthStencil(1, 0);

            Attachment attachments[] = {color, depth};
            environment.pass = api->CreateRenderPass(std::size(attachments), attachments, 1, &subpass, 0, nullptr, 2, clear);

            TextureHandle* textures[] = { lighting.color, gbuffer.depth };
            environment.framebuffer = CreateFramebuffer(environment.pass, std::size(textures), textures, width, height);
        }

        {
            InputLayout layout;
            layout.Binding(0);
            layout.Attribute(0, VERTEX_BUFFER_FORMAT_R32G32B32);
            layout.Attribute(1, VERTEX_BUFFER_FORMAT_R32G32B32);
            layout.Attribute(2, VERTEX_BUFFER_FORMAT_R32G32);
            layout.Attribute(3, VERTEX_BUFFER_FORMAT_R32G32B32);
            layout.Attribute(4, VERTEX_BUFFER_FORMAT_R32G32B32);

            PipelineStateInfoBuilder builder;
            PipelineStateInfo pipelineInfo = builder
                .InputLayout(1, &layout)
                .Rasterizer(POLYGON_CULL_BACK, POLYGON_FRONT_FACE_CLOCKWISE)
                .Depth(false, false)
                .Stencil(true, 0, COMPARE_OP_EQUAL)
                .Blend(false, 1)
                .Value();

            ShaderCompiledData compiledData;
            ShaderCompiler::Get()->Compile("Assets/Shaders/Environment.glsl", compiledData);
            environment.shader   = api->CreateShader(compiledData);
            environment.pipeline = api->CreateGraphicsPipeline(environment.shader, &pipelineInfo, environment.pass);

            environment.ubo = CreateUniformBuffer(nullptr, sizeof(Test::EnvironmentUBO), &environment.mapped);

            DescriptorSetInfo materialInfo = {};
            materialInfo.AddBuffer(0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, environment.ubo);
            materialInfo.AddTexture(1, DESCRIPTOR_TYPE_IMAGE_SAMPLER, cubemap, sceneSampler);
            environment.set = CreateDescriptorSet(environment.shader, 0, materialInfo);
        }
    }

    void RHI::Update(Camera* camera)
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
                    if (ImGui::MenuItem("終了", "Alt+F4")) Engine::Get()->Close();

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
            GUI::Image(imageSet, content.x, content.y);
        }

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Begin("Info", nullptr, isUsingCamera ? ImGuiWindowFlags_NoInputs : 0);
        ImGui::Text("FPS: %d", Engine::Get()->GetFrameRate());
        ImGui::End();

        ImGui::PopStyleVar(2);
    }

    void RHI::CleanupGBuffer()
    {
        api->DestroyShader(gbuffer.shader);
        api->DestroyPipeline(gbuffer.pipeline);
        api->DestroyRenderPass(gbuffer.pass);
        api->DestroyFramebuffer(gbuffer.framebuffer);

        api->DestroyTexture(gbuffer.albedo);
        api->DestroyTexture(gbuffer.normal);
        api->DestroyTexture(gbuffer.emission);
        api->DestroyTexture(gbuffer.id);
        api->DestroyTexture(gbuffer.depth);

        api->DestroyDescriptorSet(gbuffer.transformSet);
        api->DestroyDescriptorSet(gbuffer.materialSet);

        api->UnmapBuffer(gbuffer.materialUBO);
        api->UnmapBuffer(gbuffer.transformUBO);
        api->DestroyBuffer(gbuffer.materialUBO);
        api->DestroyBuffer(gbuffer.transformUBO);
    }

    void RHI::CleanupLightingBuffer()
    {
        api->DestroyShader(lighting.shader);
        api->DestroyPipeline(lighting.pipeline);
        api->DestroyRenderPass(lighting.pass);
        api->DestroyFramebuffer(lighting.framebuffer);

        api->DestroyTexture(lighting.color);
        api->DestroyDescriptorSet(lighting.set);

        api->UnmapBuffer(lighting.sceneUBO);
        api->DestroyBuffer(lighting.sceneUBO);
    }

    void RHI::CleanupEnvironmentBuffer()
    {
        api->DestroyShader(environment.shader);
        api->DestroyPipeline(environment.pipeline);
        api->DestroyRenderPass(environment.pass);
        api->DestroyFramebuffer(environment.framebuffer);

        api->DestroyDescriptorSet(environment.set);

        api->UnmapBuffer(environment.ubo);
        api->DestroyBuffer(environment.ubo);
    }

    void RHI::ResizeGBuffer(uint32 width, uint32 height)
    {
        DestroyTexture(gbuffer.albedo);
        DestroyTexture(gbuffer.normal);
        DestroyTexture(gbuffer.emission);
        DestroyTexture(gbuffer.id);
        DestroyTexture(gbuffer.depth);

        DestroyFramebuffer(gbuffer.framebuffer);

        gbuffer.albedo   = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM,          width, height);
        gbuffer.normal   = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM,          width, height);
        gbuffer.emission = CreateTexture2D(RENDERING_FORMAT_B10G11R11_UFLOAT_PACK32, width, height);
        gbuffer.id       = CreateTexture2D(RENDERING_FORMAT_R32_SINT,                width, height);
        gbuffer.depth    = CreateTexture2D(RENDERING_FORMAT_D24_UNORM_S8_UINT,       width, height);

        TextureHandle* textures[] = {gbuffer.albedo, gbuffer.normal, gbuffer.emission, gbuffer.id, gbuffer.depth};
        gbuffer.framebuffer = CreateFramebuffer(gbuffer.pass, std::size(textures), textures, width, height);
    }

    void RHI::ResizeLightingBuffer(uint32 width, uint32 height)
    {
        DestroyTexture(lighting.color);
        DestroyFramebuffer(lighting.framebuffer);
        DestroyDescriptorSet(lighting.set);

        lighting.color       = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);
        lighting.framebuffer = CreateFramebuffer(lighting.pass, 1, &lighting.color, width, height);

        DescriptorSetInfo textureInfo = {};
        textureInfo.AddTexture(0, DESCRIPTOR_TYPE_IMAGE_SAMPLER, gbuffer.albedo,   sceneSampler);
        textureInfo.AddTexture(1, DESCRIPTOR_TYPE_IMAGE_SAMPLER, gbuffer.normal,   sceneSampler);
        textureInfo.AddTexture(2, DESCRIPTOR_TYPE_IMAGE_SAMPLER, gbuffer.emission, sceneSampler);
        textureInfo.AddTexture(3, DESCRIPTOR_TYPE_IMAGE_SAMPLER, gbuffer.depth,    sceneSampler);
        textureInfo.AddBuffer(4, DESCRIPTOR_TYPE_UNIFORM_BUFFER, lighting.sceneUBO);
        lighting.set = CreateDescriptorSet(lighting.shader, 0, textureInfo);
    }

    void RHI::ResizeEnvironmentBuffer(uint32 width, uint32 height)
    {
        DestroyFramebuffer(environment.framebuffer);

        TextureHandle* textures[] = { lighting.color, gbuffer.depth };
        environment.framebuffer = CreateFramebuffer(environment.pass, std::size(textures), textures, width, height);
    }

    void RHI::RESIZE(uint32 width, uint32 height)
    {
        if (width == 0 || height == 0)
            return;

        if (0)
        {
            DestroyFramebuffer(sceneFramebuffer);
            DestroyTexture(sceneColorTexture);
            DestroyTexture(sceneDepthTexture);

            sceneColorTexture = CreateTexture2D(RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height);
            sceneDepthTexture = CreateTexture2D(RENDERING_FORMAT_D24_UNORM_S8_UINT, width, height);
            TextureHandle* textures[] = { sceneColorTexture , sceneDepthTexture };
            sceneFramebuffer = CreateFramebuffer(scenePass, std::size(textures), textures, width, height);
        }

        ResizeGBuffer(width, height);
        ResizeLightingBuffer(width, height);
        ResizeEnvironmentBuffer(width, height);

        {
            DestroyDescriptorSet(compositSet);
            DestroyFramebuffer(compositFB);
            DestroyTexture(compositTexture);

            compositTexture = CreateTexture2D(RENDERING_FORMAT_R8G8B8A8_UNORM, width, height);
            compositFB      = CreateFramebuffer(compositPass, 1, &compositTexture, width, height);
            DescriptorSetInfo blitinfo;
            blitinfo.AddTexture(0, DESCRIPTOR_TYPE_IMAGE_SAMPLER, lighting.color, sceneSampler);
            compositSet = CreateDescriptorSet(compositShader, 0, blitinfo);
        }

        {
            DestroyDescriptorSet(imageSet);
            DescriptorSetInfo imageinfo;
            imageinfo.AddTexture(0, DESCRIPTOR_TYPE_IMAGE_SAMPLER, compositTexture, sceneSampler);
            imageSet = CreateDescriptorSet(compositShader, 0, imageinfo);
        }
    }

    void RHI::Render(Camera* camera, float dt)
    {
        FrameData& frame = frameData[frameIndex];
        const auto& windowSize   = Window::Get()->GetSize();
        const auto& viewportSize = camera->GetViewportSize();

        {
            Test::Transform sceneData;
            sceneData.projection = camera->GetProjectionMatrix();
            sceneData.view       = camera->GetViewMatrix();
            std::memcpy(mappedSceneData, &sceneData, sizeof(Test::Transform));
            std::memcpy(gbuffer.mappedTransfrom, &sceneData, sizeof(Test::Transform));
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

        {
            Test::MaterialUBO matData = {};
            matData.albedo        = glm::vec3(1.0);
            matData.emission      = glm::vec3(0.0);
            matData.metallic      = 0.5f;
            matData.roughness     = 0.4f;
            matData.textureTiling = glm::vec2(1.0, 1.0);
            std::memcpy(gbuffer.mappedMaterial, &matData, sizeof(Test::MaterialUBO));
        }

        {
            Test::SceneUBO sceneUBO = {};
            sceneUBO.lightColor        = glm::vec4(1.0, 1.0, 1.0, 1.0);
            sceneUBO.lightDir          = glm::vec4(1.0, 1.0, 0.0, 0.0);
            sceneUBO.cameraPosition    = glm::vec4(camera->GetPosition(), 0.0);
            sceneUBO.invViewProjection = glm::inverse(camera->GetProjectionMatrix() * camera->GetViewMatrix());
            std::memcpy(lighting.mappedScene, &sceneUBO, sizeof(Test::SceneUBO));
        }

        {
            Test::EnvironmentUBO environmentUBO = {};
            environmentUBO.view       = camera->GetViewMatrix();
            environmentUBO.projection = camera->GetProjectionMatrix();
            std::memcpy(environment.mapped, &environmentUBO, sizeof(Test::EnvironmentUBO));
        }

        api->SetViewport(frame.commandBuffer, 0, 0, viewportSize.x, viewportSize.y);
        api->SetScissor(frame.commandBuffer,  0, 0, viewportSize.x, viewportSize.y);

        if (1) // メッシュパス
        {
            api->BeginRenderPass(frame.commandBuffer, gbuffer.pass, gbuffer.framebuffer);
            
            api->BindPipeline(frame.commandBuffer, gbuffer.pipeline);
            api->BindDescriptorSet(frame.commandBuffer, gbuffer.transformSet, 0);
            api->BindDescriptorSet(frame.commandBuffer, gbuffer.materialSet,  1);

            // スポンザ
            for (MeshSource* source : sponzaMesh->GetMeshSources())
            {
                Buffer* vb        = source->GetVertexBuffer();
                Buffer* ib        = source->GetIndexBuffer();
                uint32 indexCount = source->GetIndexCount();
                api->BindVertexBuffer(frame.commandBuffer, vb, 0);
                api->BindIndexBuffer(frame.commandBuffer, ib, INDEX_BUFFER_FORMAT_UINT32, 0);
                api->DrawIndexed(frame.commandBuffer, indexCount, 1, 0, 0, 0);
            }

            api->EndRenderPass(frame.commandBuffer);
        }

        if (1) // ライティングパス
        {
            api->BeginRenderPass(frame.commandBuffer, lighting.pass, lighting.framebuffer);

            api->BindPipeline(frame.commandBuffer, lighting.pipeline);
            api->BindDescriptorSet(frame.commandBuffer, lighting.set, 0);

            api->Draw(frame.commandBuffer, 3, 1, 0, 0);

            api->EndRenderPass(frame.commandBuffer);
        }

        if (1) // 仮 フォワードパス
        {
            api->BeginRenderPass(frame.commandBuffer, environment.pass, environment.framebuffer);

            if (1) // スカイ
            {
                api->BindPipeline(frame.commandBuffer, environment.pipeline);
                api->BindDescriptorSet(frame.commandBuffer, environment.set, 0);

                MeshSource* ms = cubeMesh->GetMeshSource();
                api->BindVertexBuffer(frame.commandBuffer, ms->GetVertexBuffer(), 0);
                api->BindIndexBuffer(frame.commandBuffer, ms->GetIndexBuffer(), INDEX_BUFFER_FORMAT_UINT32, 0);
                api->DrawIndexed(frame.commandBuffer, ms->GetIndexCount(), 1, 0, 0, 0);
            }

            if (1) // グリッド
            {
                api->BindPipeline(frame.commandBuffer, gridPipeline);
                api->BindDescriptorSet(frame.commandBuffer, gridSet, 0);
                api->Draw(frame.commandBuffer, 6, 1, 0, 0);
            }

            api->EndRenderPass(frame.commandBuffer);
        }

        if (1) // コンポジットパス
        {
            api->BeginRenderPass(frame.commandBuffer, compositPass, compositFB);

            api->BindPipeline(frame.commandBuffer, compositPipeline);
            api->BindDescriptorSet(frame.commandBuffer, compositSet, 0);
            api->Draw(frame.commandBuffer, 3, 1, 0, 0);

            api->EndRenderPass(frame.commandBuffer);
        }
    }




    TextureHandle* RHI::CreateTextureFromMemory(const uint8* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap)
    {
        // RGBA8_UNORM フォーマットテクスチャ
        TextureHandle* gpuTexture = _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_2D, RENDERING_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, genMipmap, 0);
        _SubmitTextureData(gpuTexture, width, height, genMipmap, pixelData, dataSize);

        return gpuTexture;
    }

    TextureHandle* RHI::CreateTextureFromMemory(const float* pixelData, uint64 dataSize, uint32 width, uint32 height, bool genMipmap)
    {
        // RGBA16_SFLOAT フォーマットテクスチャ
        TextureHandle* gpuTexture = _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_2D, RENDERING_FORMAT_R16G16B16A16_SFLOAT, width, height, 1, 1, genMipmap, 0);
        _SubmitTextureData(gpuTexture, width, height, genMipmap, pixelData, dataSize);

        return gpuTexture;
    }

    TextureHandle* RHI::CreateTexture2D(RenderingFormat format, uint32 width, uint32 height, bool genMipmap)
    {
        return _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_2D, format, width, height, 1, 1, genMipmap, 0);
    }

    TextureHandle* RHI::CreateTexture2DArray(RenderingFormat format, uint32 width, uint32 height, uint32 array, bool genMipmap)
    {
        return _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_2D_ARRAY, format, width, height, 1, array, genMipmap, 0);
    }

    TextureHandle* RHI::CreateTextureCube(RenderingFormat format, uint32 width, uint32 height, bool genMipmap)
    {
        return _CreateTexture(TEXTURE_DIMENSION_2D, TEXTURE_TYPE_CUBE, format, width, height, 1, 6, genMipmap, 0);
    }

    void RHI::DestroyTexture(TextureHandle* texture)
    {
        FrameData& frame = frameData[frameIndex];
        frame.pendingResources->texture.push_back(texture);
    }

    TextureHandle* RHI::_CreateTexture(TextureDimension dimension, TextureType type, RenderingFormat format, uint32 width, uint32 height, uint32 depth, uint32 array, bool genMipmap, TextureUsageFlags additionalFlags)
    {
        int32 usage = TEXTURE_USAGE_SAMPLING_BIT;
        usage |= RenderingUtility::IsDepthFormat(format)? TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : TEXTURE_USAGE_COLOR_ATTACHMENT_BIT;
        usage |= genMipmap? TEXTURE_USAGE_COPY_SRC_BIT | TEXTURE_USAGE_COPY_DST_BIT : TEXTURE_USAGE_COPY_DST_BIT;
        usage |= additionalFlags;

        auto mipmaps = RenderingUtility::CalculateMipmap(width, height);

        TextureInfo texformat = {};
        texformat.format    = format;
        texformat.width     = width;
        texformat.height    = height;
        texformat.dimension = dimension;
        texformat.type      = type;
        texformat.usageBits = usage;
        texformat.samples   = TEXTURE_SAMPLES_1;
        texformat.array     = array;
        texformat.depth     = depth;
        texformat.mipLevels = genMipmap? mipmaps.size() : 1;

        TextureHandle* texture = api->CreateTexture(texformat);
        SL_CHECK(!texture, nullptr);

        return texture;
    }

    void RHI::_SubmitTextureData(TextureHandle* texture, uint32 width, uint32 height, bool genMipmap, const void* pixelData, uint64 dataSize)
    {
        // ステージングバッファ
        Buffer* staging = api->CreateBuffer(dataSize, BUFFER_USAGE_TRANSFER_SRC_BIT, MEMORY_ALLOCATION_TYPE_CPU);

        // ステージングにテクスチャデータをコピー
        void* mappedPtr = api->MapBuffer(staging);
        std::memcpy(mappedPtr, pixelData, dataSize);
        api->UnmapBuffer(staging);

        // コピーコマンド
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

        // ステージング破棄
        api->DestroyBuffer(staging);
    }
    
    void RHI::_GenerateMipmaps(CommandBuffer* cmd, TextureHandle* texture, uint32 width, uint32 height, uint32 depth, uint32 array, TextureAspectFlags aspect)
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
            info.subresources.layerCount    = array;
            info.srcAccess                  = BARRIER_ACCESS_MEMORY_WRITE_BIT;
            info.dstAccess                  = BARRIER_ACCESS_MEMORY_WRITE_BIT | BARRIER_ACCESS_MEMORY_READ_BIT;
            info.oldLayout                  = TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL;
            info.newLayout                  = TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL;

            api->PipelineBarrier(cmd, PIPELINE_STAGE_ALL_COMMANDS_BIT, PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, nullptr, 0, nullptr, 1, &info);

            // Blit
            Extent srcExtent = mipmaps[mipLevel + 0];
            Extent dstExtent = mipmaps[mipLevel + 1];

            // NOTE:
            // srcImageがVK_IMAGE_TYPE_1DまたはVK_IMAGE_TYPE_2D型の場合、pRegionsの各要素について、srcOffsets[0].zは0、srcOffsets[1].zは1でなければなりません。
            // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdBlitImage.html#VUID-vkCmdBlitImage-srcImage-00247

            TextureBlitRegion region = {};
            region.srcOffset[0] = { 0, 0, 0 };
            region.dstOffset[0] = { 0, 0, 0 };
            region.srcOffset[1] = srcExtent;
            region.dstOffset[1] = dstExtent;

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


    Buffer* RHI::CreateUniformBuffer(void* data, uint64 size, void** outMappedAdress)
    {
        return _CreateAndMapBuffer(BUFFER_USAGE_UNIFORM_BIT, data, size, outMappedAdress);
    }

    Buffer* RHI::CreateStorageBuffer(void* data, uint64 size, void** outMappedAdress)
    {
        return _CreateAndMapBuffer(BUFFER_USAGE_STORAGE_BIT, data, size, outMappedAdress);
    }

    Buffer* RHI::CreateVertexBuffer(void* data, uint64 size)
    {
        return _CreateAndSubmitBufferData(BUFFER_USAGE_VERTEX_BIT, data, size);
    }

    Buffer* RHI::CreateIndexBuffer(void* data, uint64 size)
    {
        return _CreateAndSubmitBufferData(BUFFER_USAGE_INDEX_BIT, data, size);
    }

    void RHI::DestroyBuffer(Buffer* buffer)
    {
        FrameData& frame = frameData[frameIndex];
        frame.pendingResources->buffer.push_back(buffer);
    }

    Buffer* RHI::_CreateAndMapBuffer(BufferUsageBits type, const void* data, uint64 dataSize, void** outMappedAdress)
    {
        Buffer* buffer   = api->CreateBuffer(dataSize, type | BUFFER_USAGE_TRANSFER_DST_BIT, MEMORY_ALLOCATION_TYPE_CPU);
        *outMappedAdress = api->MapBuffer(buffer);

        if (data != nullptr)
            std::memcpy(*outMappedAdress, data, dataSize);

        return buffer;
    }

    Buffer* RHI::_CreateAndSubmitBufferData(BufferUsageBits type, const void* data, uint64 dataSize)
    {
        Buffer* buffer  = api->CreateBuffer(dataSize, type | BUFFER_USAGE_TRANSFER_DST_BIT, MEMORY_ALLOCATION_TYPE_GPU);
        Buffer* staging = api->CreateBuffer(dataSize, type | BUFFER_USAGE_TRANSFER_SRC_BIT, MEMORY_ALLOCATION_TYPE_CPU);
        
        void* mapped = api->MapBuffer(staging);
        std::memcpy(mapped, data, dataSize);
        api->UnmapBuffer(staging);

        api->ImmidiateCommands(graphicsQueue, immidiateContext.commandBuffer, immidiateContext.fence, [&](CommandBuffer* cmd)
        {
            BufferCopyRegion region = {};
            region.size = dataSize;

            api->CopyBuffer(cmd, staging, buffer, 1, &region);
        });

        api->DestroyBuffer(staging);

        return buffer;
    }



    FramebufferHandle* RHI::CreateFramebuffer(RenderPass* renderpass, uint32 numTexture, TextureHandle** textures, uint32 width, uint32 height)
    {
        FramebufferHandle* framebuffer = api->CreateFramebuffer(renderpass, numTexture, textures, width, height);
        SL_CHECK(!framebuffer, nullptr);

        return framebuffer;
    }

    void RHI::DestroyFramebuffer(FramebufferHandle* framebuffer)
    {
        FrameData& frame = frameData[frameIndex];
        frame.pendingResources->framebuffer.push_back(framebuffer);
    }



    DescriptorSet* RHI::CreateDescriptorSet(ShaderHandle* shader, uint32 setIndex, DescriptorSetInfo& setInfo)
    {
        DescriptorSet* descriptorSet = api->CreateDescriptorSet(setInfo.infos.size(), setInfo.infos.data(), shader, setIndex);
        SL_CHECK(!descriptorSet, nullptr);

        return descriptorSet;
    }

    void RHI::DestroyDescriptorSet(DescriptorSet* set)
    {
        FrameData& frame = frameData[frameIndex];
        frame.pendingResources->descriptorset.push_back(set);
    }



    SwapChain* RHI::CreateSwapChain(Surface* surface, uint32 width, uint32 height, VSyncMode mode)
    {
        SwapChain* swapchain = api->CreateSwapChain(surface, width, height, swapchainBufferCount, mode);
        SL_CHECK(!swapchain, nullptr);

        return swapchain;
    }

    void RHI::DestoySwapChain(SwapChain* swapchain)
    {
        api->DestroySwapChain(swapchain);
    }

    bool RHI::ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, VSyncMode mode)
    {
        bool result = api->ResizeSwapChain(swapchain, width, height, swapchainBufferCount, mode);
        SL_CHECK(!result, false);

        return false;
    }

    bool RHI::Present()
    {
        SwapChain* swapchain = Window::Get()->GetSwapChain();
        FrameData& frame = frameData[frameIndex];

        api->Present(graphicsQueue, swapchain, frame.renderSemaphore);
        frameIndex = (frameIndex + 1) % 2;

        return true;
    }

    void RHI::BeginSwapChainPass()
    {
        FrameData& frame = frameData[frameIndex];
        api->BeginRenderPass(frame.commandBuffer, swapchainPass, swapchainFramebuffer, COMMAND_BUFFER_TYPE_PRIMARY);
    }

    void RHI::EndSwapChainPass()
    {
        FrameData& frame = frameData[frameIndex];
        api->EndRenderPass(frame.commandBuffer);
    }



    void RHI::_DestroyPendingResources(uint32 frame)
    {
        FrameData& f = frameData[frame];

        for (Buffer* buffer : f.pendingResources->buffer)
        {
            api->DestroyBuffer(buffer);
        }

        f.pendingResources->buffer.clear();

        for (TextureHandle* texture : f.pendingResources->texture)
        {
            api->DestroyTexture(texture);
        }

        f.pendingResources->texture.clear();

        for (Sampler* sampler : f.pendingResources->sampler)
        {
            api->DestroySampler(sampler);
        }

        f.pendingResources->sampler.clear();

        for (DescriptorSet* set : f.pendingResources->descriptorset)
        {
            api->DestroyDescriptorSet(set);
        }

        f.pendingResources->descriptorset.clear();

        for (FramebufferHandle* framebuffer : f.pendingResources->framebuffer)
        {
            api->DestroyFramebuffer(framebuffer);
        }

        f.pendingResources->framebuffer.clear();

        for (ShaderHandle* shader : f.pendingResources->shader)
        {
            api->DestroyShader(shader);
        }

        f.pendingResources->shader.clear();

        for (Pipeline* pipeline : f.pendingResources->pipeline)
        {
            api->DestroyPipeline(pipeline);
        }

        f.pendingResources->pipeline.clear();
    }

    const DeviceInfo& RHI::GetDeviceInfo() const
    {
        return context->GetDeviceInfo();
    }

    RenderingContext* RHI::GetContext() const
    {
        return context;
    }

    RenderingAPI* RHI::GetAPI() const
    {
        return api;
    }

    FrameData& RHI::GetFrameData()
    {
        return frameData[frameIndex];
    }

    const FrameData& RHI::GetFrameData() const
    {
        return frameData[frameIndex];
    }
    
    CommandQueue* RHI::GetGraphicsCommandQueue() const
    {
        return graphicsQueue;
    }

    QueueID RHI::GetGraphicsQueueID() const
    {
        return graphicsQueueID;
    }
}
