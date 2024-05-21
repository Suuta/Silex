
#pragma once

#include "Rendering/RenderingCore.h"


namespace Silex
{
    class RenderingAPI;
    class RenderingContext;


    class RenderingDevice : public Object
    {
        SL_CLASS(RenderingDevice, Object);

    public:

        RenderingDevice();
        ~RenderingDevice();

        bool Initialize(RenderingContext* context);

    private:

        // フレームデータ
        struct FrameData
        {
            CommandPool*   commandPool   = nullptr;
            CommandBuffer* commandBuffer = nullptr;
            Semaphore*     semaphore     = nullptr;
            Fence*         fence         = nullptr;
        };

        std::vector<FrameData> frameData  = {};
        uint64                 frameIndex = 0;

    private:

        RenderingContext* renderingContext = nullptr;
        RenderingAPI*     renderingAPI     = nullptr;

        QueueFamily   graphicsQueueFamily = INVALID_RENDER_ID;
        CommandQueue* graphicsQueue       = nullptr;

        SwapChain* swapchain = nullptr;

    private:

        static inline RenderingDevice* instance = nullptr;
    };
}

