
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
        if (renderingAPI)
        {
            renderingContext->DestroyRendringAPI(renderingAPI);
        }

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
        QueueFamily family = renderingAPI->GetQueueFamily(supportQueue, Window::Get()->GetSurface());
        if (family == INVALID_RENDER_ID)
        {
            SL_LOG_ERROR("GetQueueFamily return INVALID_RENDER_ID");
            return false;
        }

        graphicsQueueFamily = family;
        presentQueueFamily = family;

        // コマンドキュー生成
        CommandQueue* queue = renderingAPI->CreateCommandQueue(graphicsQueueFamily);
        if (!queue)
        {
            SL_LOG_ERROR("queue is null");
            return false;
        }

        graphicsQueue = queue;
        presentQueue = queue;

        // コマンドプール生成
        commandPool = renderingAPI->CreateCommandPool(graphicsQueueFamily, COMMAND_BUFFER_TYPE_PRIMARY);
        if (!commandPool)
        {
            SL_LOG_ERROR("commandPool is null");
            return false;
        }

        return true;
    }
}

