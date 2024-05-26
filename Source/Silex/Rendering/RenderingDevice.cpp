
#include "PCH.h"

#include "Core/Window.h"
#include "Rendering/RenderingDevice.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/RenderingAPI.h"


namespace Silex
{
    RenderingDevice::RenderingDevice()
    {
        instance = this;
    }

    RenderingDevice::~RenderingDevice()
    {
        api->DestroySwapChain(swapchain);

        for (uint32 i = 0; i < frameData.size(); i++)
        {
            api->DestroyCommandPool(frameData[i].commandPool);
            api->DestroySemaphore(frameData[i].semaphore);
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
        frameData.resize(2);
        for (uint32 i = 0; i < frameData.size(); i++)
        {
            // コマンドプール生成
            frameData[i].commandPool = api->CreateCommandPool(graphicsQueueFamily, COMMAND_BUFFER_TYPE_PRIMARY);
            SL_CHECK(!frameData[i].commandPool, false);

            // コマンドバッファ生成
            frameData[i].commandBuffer = api->CreateCommandBuffer(frameData[i].commandPool);
            SL_CHECK(!frameData[i].commandBuffer, false);

            // セマフォ生成
            frameData[i].semaphore = api->CreateSemaphore();
            SL_CHECK(!frameData[i].semaphore, false);

            // フェンス生成
            frameData[i].fence = api->CreateFence();
            SL_CHECK(!frameData[i].fence, false);
        }

        return true;
    }


    bool RenderingDevice::CreateSwapChain()
    {
        bool result = false;
        Surface* surface = Window::Get()->GetSurface();

        swapchain = api->CreateSwapChain(surface);
        SL_CHECK(!swapchain, false);

        result = api->ResizeSwapChain(swapchain, swapchainBufferCount, VSYNC_MODE_DISABLED);
        SL_CHECK(!result, false);

        return true;
    }
}

