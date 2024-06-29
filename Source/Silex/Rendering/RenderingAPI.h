
#pragma once

#include "Rendering/RenderingCore.h"
#include "Core/Window.h"


namespace Silex
{
    struct ShaderCompiledData;

    class RenderingAPI : public Object
    {
        SL_CLASS(RenderingAPI, Object)

    public:

        RenderingAPI()  {};
        ~RenderingAPI() {};

    public:

        virtual bool Initialize() = 0;

        //--------------------------------------------------
        // コマンドキュー
        //--------------------------------------------------
        virtual CommandQueue* CreateCommandQueue(QueueFamily family, uint32 indexInFamily = 0) = 0;
        virtual void DestroyCommandQueue(CommandQueue* queue) = 0;
        virtual QueueFamily QueryQueueFamily(uint32 flag, Surface* surface = nullptr) const = 0;

        //--------------------------------------------------
        // コマンドプール
        //--------------------------------------------------
        virtual CommandPool* CreateCommandPool(QueueFamily family, CommandBufferType type) = 0;
        virtual void DestroyCommandPool(CommandPool* pool) = 0;

        //--------------------------------------------------
        // コマンドバッファ
        //--------------------------------------------------
        virtual CommandBuffer* CreateCommandBuffer(CommandPool* pool) = 0;
        virtual void DestroyCommandBuffer(CommandBuffer* commandBuffer) = 0;
        virtual bool BeginCommandBuffer(CommandBuffer* commandBuffer) = 0;
        virtual bool EndCommandBuffer(CommandBuffer* commandBuffer) = 0;

        //--------------------------------------------------
        // セマフォ
        //--------------------------------------------------
        virtual Semaphore* CreateSemaphore() = 0;
        virtual void DestroySemaphore(Semaphore* semaphore) = 0;

        //--------------------------------------------------
        // フェンス
        //--------------------------------------------------
        virtual Fence* CreateFence() = 0;
        virtual void DestroyFence(Fence* fence) = 0;
        virtual bool WaitFence(Fence* fence) = 0;

        //--------------------------------------------------
        // スワップチェイン
        //--------------------------------------------------
        virtual SwapChain* CreateSwapChain(Surface* surface) = 0;
        virtual bool ResizeSwapChain(SwapChain* swapchain, uint32 requestFramebufferCount, VSyncMode mode) = 0;
        virtual FramebufferHandle* GetSwapChainNextFramebuffer(SwapChain* swapchain, Semaphore* semaphore) = 0;
        virtual RenderPass* GetSwapChainRenderPass(SwapChain* swapchain) = 0;
        virtual RenderingFormat GetSwapChainFormat(SwapChain* swapchain) = 0;
        virtual void DestroySwapChain(SwapChain* swapchain) = 0;

        //--------------------------------------------------
        // バッファ
        //--------------------------------------------------
        virtual Buffer* CreateBuffer(uint64 size, BufferUsageBits usage, MemoryAllocationType memoryType) = 0;
        virtual void DestroyBuffer(Buffer* buffer) = 0;
        virtual byte* MapBuffer(Buffer* buffer) = 0;
        virtual void UnmapBuffer(Buffer* buffer) = 0;

        //--------------------------------------------------
        // テクスチャ
        //--------------------------------------------------
        virtual TextureHandle* CreateTexture(const TextureFormat& format) = 0;
        virtual void DestroyTexture(TextureHandle* texture) = 0;

        //--------------------------------------------------
        // サンプラ
        //--------------------------------------------------
        virtual Sampler* CreateSampler(const SamplerState& state) = 0;
        virtual void DestroySampler(Sampler* sampler) = 0;

        //--------------------------------------------------
        // フレームバッファ
        //--------------------------------------------------
        virtual FramebufferHandle* CreateFramebuffer(RenderPass* renderpass, TextureHandle* textures, uint32 numTexture, uint32 width, uint32 height) = 0;
        virtual void DestroyFramebuffer(FramebufferHandle* framebuffer) = 0;

        //--------------------------------------------------
        // 頂点フォーマット
        //--------------------------------------------------
        virtual VertexFormat* CreateVertexFormat(uint32 numattributes, VertexAttribute* attributes) = 0;
        virtual void DestroyVertexFormat(VertexAttribute* attributes) = 0;

        //--------------------------------------------------
        // レンダーパス
        //--------------------------------------------------
        virtual RenderPass* CreateRenderPass(uint32 numAttachments, Attachment* attachments, uint32 numSubpasses, Subpass* subpasses, uint32 numSubpassDependencies, SubpassDependency* subpassDependencies) = 0;
        virtual void DestroyRenderPass(RenderPass* renderpass) = 0;

        //--------------------------------------------------
        // シェーダー
        //--------------------------------------------------
        virtual ShaderHandle* CreateShader(const ShaderCompiledData& compiledData) = 0;
        virtual void DestroyShader(ShaderHandle* shader) = 0;

        //--------------------------------------------------
        // デスクリプターセット
        //--------------------------------------------------
        virtual DescriptorSet* CreateDescriptorSet(uint32 numdescriptors, DescriptorInfo* descriptors, ShaderHandle* shader, uint32 setIndex) = 0;
        virtual void DestroyDescriptorSet(DescriptorSet* descriptorset) = 0;

        //--------------------------------------------------
        // パイプライン
        //--------------------------------------------------
        virtual Pipeline* CreateGraphicsPipeline(ShaderHandle* shader, VertexFormat* vertexFormat, PrimitiveTopology primitive, PipelineRasterizationState rasterizationState, PipelineMultisampleState multisampleState, PipelineDepthStencilState depthstencilState, PipelineColorBlendState blendState, int32* colorAttachments, uint32 numColorAttachments, PipelineDynamicStateFlags dynamicState, RenderPass* renderpass, uint32 renderSubpass) = 0;
        virtual Pipeline* CreateComputePipeline(ShaderHandle* shader) = 0;
        virtual void DestroyPipeline(Pipeline* pipeline) = 0;

        //--------------------------------------------------
        // コマンド
        //--------------------------------------------------
        virtual void PipelineBarrier(CommandBuffer* commandbuffer, PipelineStageBits srcStage, PipelineStageBits dstStage, uint32 numMemoryBarrier, MemoryBarrier* memoryBarrier, uint32 numBufferBarrier, BufferBarrier* bufferBarrier, uint32 numTextureBarrier, TextureBarrier* textureBarrier) = 0;
        virtual void ClearBuffer(CommandBuffer* commandbuffer, Buffer* buffer, uint64 offset, uint64 size) = 0;
        virtual void CopyBuffer(CommandBuffer* commandbuffer, Buffer* srcBuffer, Buffer* dstBuffer, uint32 numRegion, BufferCopyRegion* regions) = 0;
        virtual void CopyTexture(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, TextureCopyRegion* regions) = 0;
        virtual void ResolveTexture(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, uint32 srcLayer, uint32 srcMipmap, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 dstLayer, uint32 dstMipmap) = 0;
        virtual void ClearColorTexture(CommandBuffer* commandbuffer, TextureHandle* texture, TextureLayout textureLayout, const glm::vec4& color, const TextureSubresourceRange& subresources) = 0;
        virtual void CopyBufferToTexture(CommandBuffer* commandbuffer, Buffer* srcBuffer, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, BufferTextureCopyRegion* regions) = 0;
        virtual void CopyTextureToBuffer(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, Buffer* dstBuffer, uint32 numRegion, BufferTextureCopyRegion* regions) = 0;
        virtual void PushConstants(CommandBuffer* commandbuffer, ShaderHandle* shader, uint32 firstIndex, uint32* data, uint32 numData) = 0;
        virtual void BeginRenderPass(CommandBuffer* commandbuffer, RenderPass* renderpass, FramebufferHandle* framebuffer, CommandBufferType commandBufferType, uint32 numclearValues, RenderPassClearValue* clearvalues, uint32 x, uint32 y, uint32 width, uint32 height) = 0;
        virtual void EndRenderPass(CommandBuffer* commandbuffer) = 0;
        virtual void NextRenderSubpass(CommandBuffer* commandbuffer, CommandBufferType commandBufferType) = 0;
        virtual void SetViewport(CommandBuffer* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height) = 0;
        virtual void SetScissor(CommandBuffer* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height) = 0;
        virtual void ClearAttachments(CommandBuffer* commandbuffer, uint32 numAttachmentClear, AttachmentClear** attachmentClears, uint32 x, uint32 y, uint32 width, uint32 height) = 0;
        virtual void BindPipeline(CommandBuffer* commandbuffer, Pipeline* pipeline) = 0;
        virtual void BindDescriptorSet(CommandBuffer* commandbuffer, DescriptorSet* descriptorset, ShaderHandle* shader, uint32 setIndex) = 0;
        virtual void Draw(CommandBuffer* commandbuffer, uint32 vertexCount, uint32 instanceCount, uint32 baseVertex, uint32 firstInstance) = 0;
        virtual void DrawIndexed(CommandBuffer* commandbuffer, uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance) = 0;
        virtual void BindVertexBuffers(CommandBuffer* commandbuffer, uint32 bindingCount, const Buffer** buffers, const uint64* offsets) = 0;
        virtual void BindIndexBuffer(CommandBuffer* commandbuffer, Buffer* buffer, IndexBufferFormat format, uint64 offset) = 0;
        virtual void SetLineWidth(CommandBuffer* commandbuffer, float width) = 0;
    };
}
