
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
        if (!renderingAPI)
        {
            SL_LOG_ERROR("renderingAPI is null");
            return false;
        }

        if (!renderingAPI->Initialize())
        {
            SL_LOG_ERROR("fail to initialize renderAPI");
            return false;
        }
       
        // グラフィックスとコンピュートをサポートするキューファミリを取得
        uint32 supportQueue = QUEUE_FAMILY_GRAPHICS_BIT | QUEUE_FAMILY_COMPUTE_BIT;
        QueueFamily family = renderingAPI->QueryQueueFamily(supportQueue, Window::Get()->GetSurface());
        if (family == INVALID_RENDER_ID)
        {
            SL_LOG_ERROR("GetQueueFamily return INVALID_RENDER_ID");
            return false;
        }

        graphicsQueueFamily = family;

        // コマンドキュー生成
        CommandQueue* queue = renderingAPI->CreateCommandQueue(graphicsQueueFamily);
        if (!queue)
        {
            SL_LOG_ERROR("queue is null");
            return false;
        }

        graphicsQueue = queue;

        // フレームデータ生成
        frameData.resize(2);
        for (uint32 i = 0; i < frameData.size(); i++)
        {
            // コマンドプール生成
            frameData[i].commandPool = renderingAPI->CreateCommandPool(graphicsQueueFamily, COMMAND_BUFFER_TYPE_PRIMARY);
            if (!frameData[i].commandPool)
            {
                SL_LOG_ERROR("commandPool is null");
                return false;
            }

            // コマンドバッファ生成
            frameData[i].commandBuffer = renderingAPI->CreateCommandBuffer(frameData[i].commandPool);
            if (!frameData[i].commandBuffer)
            {
                SL_LOG_ERROR("commandBuffer is null");
                return false;
            }

            // セマフォ生成
            frameData[i].semaphore = renderingAPI->CreateSemaphore();
            if (!frameData[i].semaphore)
            {
                SL_LOG_ERROR("semaphore is null");
                return false;
            }

            // フェンス生成
            frameData[i].fence = renderingAPI->CreateFence();
            if (!frameData[i].fence)
            {
                SL_LOG_ERROR("fence is null");
                return false;
            }
        }

        return true;
    }
}

