
#pragma once

#include "Rendering/RenderingCore.h"


namespace Silex
{
    class RenderingAPI : public Object
    {
        SL_CLASS(RenderingAPI, Object)

    public:

        RenderingAPI()  {};
        ~RenderingAPI() {};

    public:

        virtual bool Initialize() = 0;

        // コマンドキュー
        virtual CommandQueue* CreateCommandQueue(QueueFamily family, uint32 indexInFamily = 0) = 0;
        virtual void DestroyCommandQueue(CommandQueue* queue) = 0;
        virtual QueueFamily QueryQueueFamily(uint32 flag, Surface* surface = nullptr) const = 0;

        // コマンドプール
        virtual CommandPool* CreateCommandPool(QueueFamily family, CommandBufferType type) = 0;
        virtual void DestroyCommandPool(CommandPool* pool) = 0;

        // コマンドバッファ
        virtual CommandBuffer* CreateCommandBuffer(CommandPool* pool) = 0;
        virtual void DestroyCommandBuffer(CommandBuffer* commandBuffer) = 0;
        virtual bool BeginCommandBuffer(CommandBuffer* commandBuffer) = 0;
        virtual bool EndCommandBuffer(CommandBuffer* commandBuffer) = 0;

        // セマフォ
#ifdef CreateSemaphore
#undef CreateSemaphore
        virtual Semaphore* CreateSemaphore() = 0;
        virtual void DestroySemaphore(Semaphore* semaphore) = 0;
#endif
        // フェンス
        virtual Fence* CreateFence() = 0;
        virtual void DestroyFence(Fence* fence) = 0;
        virtual bool WaitFence(Fence* fence) = 0;
    };
}
