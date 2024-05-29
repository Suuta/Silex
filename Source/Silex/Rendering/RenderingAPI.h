
#pragma once

#include "Rendering/RenderingCore.h"
#include "Core/Window.h"


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
        virtual Semaphore* CreateSemaphore() = 0;
        virtual void DestroySemaphore(Semaphore* semaphore) = 0;

        // フェンス
        virtual Fence* CreateFence() = 0;
        virtual void DestroyFence(Fence* fence) = 0;
        virtual bool WaitFence(Fence* fence) = 0;

        // スワップチェイン
        virtual SwapChain* CreateSwapChain(Surface* surface) = 0;
        virtual bool ResizeSwapChain(SwapChain* swapchain, uint32 requestFramebufferCount, VSyncMode mode) = 0;
        virtual FramebufferHandle* GetSwapChainNextFramebuffer(SwapChain* swapchain, Semaphore* semaphore) = 0;
        virtual RenderPass* GetSwapChainRenderPass(SwapChain* swapchain) = 0;
        virtual RenderingFormat GetSwapChainFormat(SwapChain* swapchain) = 0;
        virtual void DestroySwapChain(SwapChain* swapchain) = 0;


        // バッファ
        virtual Buffer* CreateBuffer(uint64 size, BufferUsageBits usage, MemoryAllocationType memoryType) = 0;
        virtual void DestroyBuffer(Buffer* buffer) = 0;
        virtual byte* MapBuffer(Buffer* buffer) = 0;
        virtual void UnmapBuffer(Buffer* buffer) = 0;

        // テクスチャ
        virtual TextureHandle* CreateTexture(const TextureFormat& format) = 0;
        virtual void DestroyTexture(TextureHandle* texture) = 0;

        // サンプラ
        virtual Sampler* CreateSampler(const SamplerState& state) = 0;
        virtual void DestroySampler(Sampler* sampler) = 0;

        // フレームバッファ
        virtual FramebufferHandle* CreateFramebuffer(RenderPass* renderpass, TextureHandle* textures, uint32 numTexture, uint32 width, uint32 height) = 0;
        virtual void DestroyFramebuffer(FramebufferHandle* framebuffer) = 0;

        // 頂点フォーマット
        virtual VertexFormat* CreateVertexFormat(uint32 numattributes, VertexAttribute* attributes) = 0;
        virtual void DestroyVertexFormat(VertexAttribute* attributes) = 0;

        // レンダーパス
        virtual RenderPass* CreateRenderPass(uint32 numAttachments, Attachment* attachments, uint32 numSubpasses, Subpass* subpasses, uint32 numSubpassDependencies, SubpassDependency* subpassDependencies) = 0;
        virtual void DestroyRenderPass(RenderPass* renderpass) = 0;

        // バリア
        virtual void PipelineBarrier(CommandBuffer* commanddBuffer, PipelineStageBits srcStage, PipelineStageBits dstStage, uint32 numMemoryBarrier, MemoryBarrier* memoryBarrier, uint32 numBufferBarrier, BufferBarrier* bufferBarrier, uint32 numTextureBarrier, TextureBarrier* textureBarrier) = 0;
    };
}
