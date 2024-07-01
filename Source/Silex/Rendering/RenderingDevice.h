
#pragma once

#include "Rendering/RenderingCore.h"


namespace Silex
{
    class RenderingAPI;
    class RenderingContext;

    // TODO: エディターの設定項目として扱える形にする
    const uint32 swapchainBufferCount = 3;


    class RenderingDevice : public Object
    {
        SL_CLASS(RenderingDevice, Object);

    public:

        RenderingDevice();
        ~RenderingDevice();

        bool Initialize(RenderingContext* context);

        SwapChain* CreateSwapChain(Surface* surface, uint32 width, uint32 height, VSyncMode mode);
        bool ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, VSyncMode mode);
        void DestoySwapChain(SwapChain* swapchain);


        static RenderingDevice* Get();

    private:

        // フレームデータ
        struct FrameData
        {
            CommandPool*   commandPool      = nullptr;
            CommandBuffer* commandBuffer    = nullptr;
            Semaphore*     presentSemaphore = nullptr;
            Semaphore*     renderSemaphore  = nullptr;
            Fence*         fence            = nullptr;
        };

        std::vector<FrameData> frameData  = {};
        uint64                 frameIndex = 0;

    private:

        RenderingContext* renderingContext = nullptr;
        RenderingAPI*     api              = nullptr;

        QueueFamily   graphicsQueueFamily = INVALID_RENDER_ID;
        CommandQueue* graphicsQueue       = nullptr;

    private:

        static inline RenderingDevice* instance = nullptr;
    };
}

