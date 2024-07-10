
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

        return true;
    }


    void RenderingDevice::TEST()
    {
        ShaderCompiledData compiledData;
        ShaderCompiler::Get()->Compile("Assets/Shaders/test.glsl", compiledData);
        shader = api->CreateShader(compiledData);

        PipelineDynamicStateFlags  dynamic = {};
        PipelineInputAssemblyState ia      = {};
        PipelineRasterizationState rs      = {};
        PipelineMultisampleState   ms      = {};
        PipelineDepthStencilState  ds      = {};
        PipelineColorBlendState    bs      = {};
        bs.AddDisabled();

        RenderPass* renderpass = api->GetSwapChainRenderPass(Window::Get()->GetSwapChain());
        uint32      subpass    = 0;

        pipeline = api->CreateGraphicsPipeline(shader, nullptr, ia, rs, ms, ds, bs, dynamic, renderpass, subpass);
    }

    void RenderingDevice::DrawTriangle()
    {
        FrameData& frame = frameData[frameIndex];
        const auto& size = Window::Get()->GetSize();

        api->SetViewport(frame.commandBuffer, 0, 0, size.x, size.y);
        api->SetScissor(frame.commandBuffer, 0, 0, size.x, size.y);

        api->BindPipeline(frame.commandBuffer, pipeline);
        api->Draw(frame.commandBuffer, 3, 1, 0, 0);
    }

    bool RenderingDevice::Begin()
    {
        SwapChain*         swapchain   = Window::Get()->GetSwapChain();
        FrameData&         frame       = frameData[frameIndex];
        FramebufferHandle* framebuffer = nullptr;
        RenderPass*        renderpass  = api->GetSwapChainRenderPass(swapchain);

        bool result = api->WaitFence(frame.fence);
        SL_CHECK(!result, false);

        framebuffer = api->GetCurrentBackBuffer(swapchain, frame.presentSemaphore);
        SL_CHECK(!framebuffer, false);

        result = api->BeginCommandBuffer(frame.commandBuffer);
        SL_CHECK(!result, false);

        api->BeginRenderPass(frame.commandBuffer, renderpass, framebuffer, COMMAND_BUFFER_TYPE_PRIMARY, 1, &defaultClearColor);

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

