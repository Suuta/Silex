
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
        renderingAPI->DestroyCommandQueue(graphicsQueue);

        for (uint32 i = 0; i < frameData.size(); i++)
        {
            renderingAPI->DestroyCommandPool(frameData[i].commandPool);
            renderingAPI->DestroySemaphore(frameData[i].semaphore);
            renderingAPI->DestroyFence(frameData[i].fence);
        }

        renderingContext->DestroyRendringAPI(renderingAPI);

        instance = nullptr;
    }

    bool RenderingDevice::Initialize(RenderingContext* context)
    {
        renderingContext = context;

        // レンダーAPI実装クラスを生成
        renderingAPI = renderingContext->CreateRendringAPI();
        SL_CHECK_RETURN(!renderingAPI->Initialize(), false);
       
        // グラフィックスとコンピュートをサポートするキューファミリを取得
        uint32 supportQueue = QUEUE_FAMILY_GRAPHICS_BIT | QUEUE_FAMILY_COMPUTE_BIT;
        graphicsQueueFamily = renderingAPI->QueryQueueFamily(supportQueue, Window::Get()->GetSurface());
        SL_CHECK_RETURN(graphicsQueueFamily == INVALID_RENDER_ID, false);

        // コマンドキュー生成
        graphicsQueue = renderingAPI->CreateCommandQueue(graphicsQueueFamily);
        SL_CHECK_RETURN(!graphicsQueue, false);
      

        // フレームデータ生成
        frameData.resize(2);
        for (uint32 i = 0; i < frameData.size(); i++)
        {
            // コマンドプール生成
            frameData[i].commandPool = renderingAPI->CreateCommandPool(graphicsQueueFamily, COMMAND_BUFFER_TYPE_PRIMARY);
            SL_CHECK_RETURN(!frameData[i].commandPool, false);

            // コマンドバッファ生成
            frameData[i].commandBuffer = renderingAPI->CreateCommandBuffer(frameData[i].commandPool);
            SL_CHECK_RETURN(!frameData[i].commandBuffer, false);

            // セマフォ生成
            frameData[i].semaphore = renderingAPI->CreateSemaphore();
            SL_CHECK_RETURN(!frameData[i].semaphore, false);

            // フェンス生成
            frameData[i].fence = renderingAPI->CreateFence();
            SL_CHECK_RETURN(!frameData[i].fence, false);
        }

        return true;
    }
}

