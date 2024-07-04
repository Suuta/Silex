
#include "PCH.h"

#include "Core/Window.h"
#include "Rendering/RenderingDevice.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/RenderingAPI.h"
#include "Rendering/ShaderCompiler.h"


namespace Silex
{
    //======================================
    // シェーダーコンパイラ
    //======================================
    ShaderCompiler shaserCompiler;



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
        renderingContext->DestroyRendringAPI(api);

        instance = nullptr;
    }

    bool RenderingDevice::Initialize(RenderingContext* context)
    {
        renderingContext = context;

        // レンダーAPI実装クラスを生成
        api = renderingContext->CreateRendringAPI();
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
            frameData[i].commandPool = api->CreateCommandPool(graphicsQueueFamily, COMMAND_BUFFER_TYPE_PRIMARY);
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

#if 0
        struct SceneData
        {
            glm::mat4 view;
            glm::mat4 proj;
            glm::mat4 viewproj;
            glm::vec4 ambientColor;
            glm::vec4 sunlightDirection;
            glm::vec4 sunlightColor;
        };

        SceneData sd = {};

        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/test.glsl", compiledData);

        ShaderHandle* shader = api->CreateShader(compiledData);
        Buffer* uniform = api->CreateBuffer(sizeof(SceneData), BufferUsageBits(BUFFER_USAGE_UNIFORM_BIT | BUFFER_USAGE_TRANSFER_DST_BIT), MEMORY_ALLOCATION_TYPE_CPU);

        DescriptorInfo info = {};
        info.binding = 0;
        info.type    = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        info.handles.push_back(uniform);

        DescriptorSet* set = api->CreateDescriptorSet(1, &info, shader, 0);

        byte* mapped = api->MapBuffer(uniform);
        memcpy(mapped, &sd, sizeof(SceneData));
        api->UnmapBuffer(uniform);

        api->DestroyDescriptorSet(set);
        api->DestroyShader(shader);
        api->DestroyBuffer(uniform);
#endif

        return true;
    }

    bool RenderingDevice::Begin()
    {
        SwapChain*         swapchain   = Window::Get()->GetSwapChain();
        FrameData&         frame       = frameData[frameIndex];
        FramebufferHandle* framebuffer = nullptr;

        bool result = api->WaitFence(frame.fence);
        SL_CHECK(!result, false);

        framebuffer = api->GetSwapChainNextFramebuffer(swapchain, frame.presentSemaphore, frame.renderSemaphore);
        SL_CHECK(!framebuffer, false);

        result = api->BeginCommandBuffer(frame.commandBuffer);
        SL_CHECK(!result, false);

        {
            // api->BeginRenderPass(frame.commandBuffer, api->GetSwapChainRenderPass(swapchain), framebuffer, COMMAND_BUFFER_TYPE_PRIMARY);
            // api->EndRenderPass(frame.commandBuffer);
        }

        return true;
    }

    bool RenderingDevice::End()
    {
        SwapChain* swapchain = Window::Get()->GetSwapChain();
        FrameData& frame     = frameData[frameIndex];
        
        api->EndCommandBuffer(frame.commandBuffer);
        api->ExcuteQueue(graphicsQueue, frame.commandBuffer, frame.fence, frame.presentSemaphore, frame.renderSemaphore);
        api->Present(graphicsQueue, swapchain);

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
        return renderingContext;
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

