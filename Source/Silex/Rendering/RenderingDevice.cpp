
#include "PCH.h"

#include "Core/Window.h"
#include "Rendering/RenderingDevice.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/Camera.h"


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

            sceneTexture = api->CreateTexture(format);
        }



        {
            swapchainPass = api->GetSwapChainRenderPass(Window::Get()->GetSwapChain());
        }

        {
            Attachment attachment = {};
            attachment.format        = RENDERING_FORMAT_R16G16B16A16_SFLOAT;
            attachment.initialLayout = TEXTURE_LAYOUT_UNDEFINED;
            attachment.finalLayout   = TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachment.loadOp        = ATTACHMENT_LOAD_OP_CLEAR;
            attachment.storeOp       = ATTACHMENT_STORE_OP_STORE;
            attachment.samples       = TEXTURE_SAMPLES_1;

            AttachmentReference ref = {};
            ref.attachment = 0;
            ref.layout     = TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            Subpass subpass = {};
            subpass.colorReferences.push_back(ref);

            scenePass        = api->CreateRenderPass(1, &attachment, 1, &subpass, 0, nullptr);
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
            DescriptorInfo info = {};
            info.handles.push_back(ubo);
            info.binding = 0;
            info.type    = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        
            descriptorSet = api->CreateDescriptorSet(1, &info, shader, 0);
        }

        {
            DescriptorInfo info = {};
            info.handles.push_back(sceneSampler);
            info.handles.push_back(sceneTexture);
            info.binding = 0;
            info.type    = DESCRIPTOR_TYPE_IMAGE_SAMPLER;

            blitSet = api->CreateDescriptorSet(1, &info, blitShader, 0);
        }

    }

    void RenderingDevice::DRAW(Camera* camera)
    {
        FrameData& frame = frameData[frameIndex];
        const auto& size = Window::Get()->GetSize();

        {
            SceneData sceneData;
            sceneData.projection = camera->GetProjectionMatrix();
            sceneData.view       = camera->GetViewMatrix();
            std::memcpy(mappedSceneData, &sceneData, sizeof(SceneData));
        }

        api->SetViewport(frame.commandBuffer, 0, 0, size.x, size.y);
        api->SetScissor(frame.commandBuffer, 0, 0, size.x, size.y);

        
        {
            api->BeginRenderPass(frame.commandBuffer, scenePass, sceneFramebuffer, COMMAND_BUFFER_TYPE_PRIMARY, 1, &defaultClearColor);
            
            uint64 offset = 0;
            api->BindPipeline(frame.commandBuffer, pipeline);
            api->BindDescriptorSet(frame.commandBuffer, descriptorSet, 0);
            api->BindVertexBuffers(frame.commandBuffer, 1, &buffer, &offset);
            api->Draw(frame.commandBuffer, 3, 1, 0, 0);

            api->EndRenderPass(frame.commandBuffer);
        }

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

        api->Present(graphicsQueue, swapchain, frame.commandBuffer, frame.fence, frame.renderSemaphore, frame.presentSemaphore);
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

