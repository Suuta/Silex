
#pragma once

#include "Rendering/RenderingCore.h"


namespace Silex
{
    class RenderingAPI;
    class RenderingContext;

    // TODO: エディターの設定項目として扱える形にする
    const uint32 swapchainBufferCount = 3;

    // フレームデータ
    struct FrameData
    {
        CommandPool*   commandPool      = nullptr;
        CommandBuffer* commandBuffer    = nullptr;
        Semaphore*     presentSemaphore = nullptr;
        Semaphore*     renderSemaphore  = nullptr;
        Fence*         fence            = nullptr;
    };


    class RenderingDevice : public Object
    {
        SL_CLASS(RenderingDevice, Object);

    public:

        RenderingDevice();
        ~RenderingDevice();

        static RenderingDevice* Get();

    public:

        bool Initialize(RenderingContext* context);

        bool Begin();
        bool End();


        SwapChain* CreateSwapChain(Surface* surface, uint32 width, uint32 height, VSyncMode mode);
        bool       ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, VSyncMode mode);
        void       DestoySwapChain(SwapChain* swapchain);

    public:

        RenderingContext* GetContext() const;
        RenderingAPI*     GetAPI()     const;

        QueueFamily   GetGraphicsQueueFamily()  const;
        CommandQueue* GetGraphicsCommandQueue() const;

        const FrameData& GetFrameData() const;

    public:

        void DrawTriangle();
        void TEST();

        Pipeline*     pipeline;
        ShaderHandle* shader;

    private:

        std::array<FrameData, 2> frameData  = {};
        uint64                   frameIndex = 0;

        RenderingContext* renderingContext = nullptr;
        RenderingAPI*     api              = nullptr;

        QueueFamily   graphicsQueueFamily = INVALID_RENDER_ID;
        CommandQueue* graphicsQueue       = nullptr;

        RenderPassClearValue defaultClearColor = {};

    private:

        static inline RenderingDevice* instance = nullptr;
    };
}

