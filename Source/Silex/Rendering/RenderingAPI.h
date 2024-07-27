
#pragma once

#include "Rendering/RenderingCore.h"


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
        virtual QueueFamily QueryQueueFamily(QueueFamilyFlags flag, Surface* surface = nullptr) const = 0;
        virtual bool SubmitQueue(CommandQueue* queue, CommandBuffer* commandbuffer, Fence* fence, Semaphore* present, Semaphore* render) = 0;

        //--------------------------------------------------
        // コマンドプール
        //--------------------------------------------------
        virtual CommandPool* CreateCommandPool(QueueFamily family, CommandBufferType type = COMMAND_BUFFER_TYPE_PRIMARY) = 0;
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
#ifdef CreateSemaphore
#undef CreateSemaphore // マクロを使わずに CreateSemaphoreA/W を使えばよい
#endif
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
        virtual SwapChain* CreateSwapChain(Surface* surface, uint32 width, uint32 height, uint32 requestFramebufferCount, VSyncMode mode) = 0;
        virtual void DestroySwapChain(SwapChain* swapchain) = 0;
        virtual bool ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, uint32 requestFramebufferCount, VSyncMode mode) = 0;
        virtual FramebufferHandle* GetCurrentBackBuffer(SwapChain* swapchain, Semaphore* present) = 0;
        virtual bool Present(CommandQueue* queue, SwapChain* swapchain, Semaphore* render) = 0;
        
        virtual RenderPass* GetSwapChainRenderPass(SwapChain* swapchain) = 0;
        virtual RenderingFormat GetSwapChainFormat(SwapChain* swapchain) = 0;

        //--------------------------------------------------
        // バッファ
        //--------------------------------------------------
        virtual Buffer* CreateBuffer(uint64 size, BufferUsageFlags usage, MemoryAllocationType memoryType) = 0;
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
        virtual Sampler* CreateSampler(const SamplerInfo& info) = 0;
        virtual void DestroySampler(Sampler* sampler) = 0;

        //--------------------------------------------------
        // フレームバッファ
        //--------------------------------------------------
        virtual FramebufferHandle* CreateFramebuffer(RenderPass* renderpass, uint32 numTexture, TextureHandle** textures, uint32 width, uint32 height) = 0;
        virtual void DestroyFramebuffer(FramebufferHandle* framebuffer) = 0;

        //--------------------------------------------------
        // 入力レイアウト
        //--------------------------------------------------
        virtual InputLayout* CreateInputLayout(uint32 numBindings, InputBinding* bindings) = 0;
        virtual void DestroyInputLayout(InputLayout* layout) = 0;

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
        virtual void UpdateDescriptorSet(DescriptorSet* descriptorSet, uint32 numdescriptor, DescriptorInfo* descriptors) = 0;

        //--------------------------------------------------
        // パイプライン
        //--------------------------------------------------
        virtual Pipeline* CreateGraphicsPipeline(ShaderHandle* shader, InputLayout* inputLayout, PipelineInputAssemblyState inputAssemblyState, PipelineRasterizationState rasterizationState, PipelineMultisampleState multisampleState, PipelineDepthStencilState depthstencilState, PipelineColorBlendState blendState, RenderPass* renderpass, uint32 renderSubpass = 0, PipelineDynamicStateFlags dynamicState = DYNAMIC_STATE_NONE) = 0;
        virtual Pipeline* CreateComputePipeline(ShaderHandle* shader) = 0;
        virtual void DestroyPipeline(Pipeline* pipeline) = 0;

        //--------------------------------------------------
        // コマンド
        //--------------------------------------------------
        virtual void PipelineBarrier(CommandBuffer* commandbuffer, PipelineStageBits srcStage, PipelineStageBits dstStage, uint32 numMemoryBarrier, MemoryBarrierInfo* memoryBarrier, uint32 numBufferBarrier, BufferBarrierInfo* bufferBarrier, uint32 numTextureBarrier, TextureBarrierInfo* textureBarrier) = 0;
        virtual void ClearBuffer(CommandBuffer* commandbuffer, Buffer* buffer, uint64 offset, uint64 size) = 0;
        virtual void CopyBuffer(CommandBuffer* commandbuffer, Buffer* srcBuffer, Buffer* dstBuffer, uint32 numRegion, BufferCopyRegion* regions) = 0;
        virtual void CopyTexture(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, TextureCopyRegion* regions) = 0;
        virtual void ResolveTexture(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, uint32 srcLayer, uint32 srcMipmap, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 dstLayer, uint32 dstMipmap) = 0;
        virtual void ClearColorTexture(CommandBuffer* commandbuffer, TextureHandle* texture, TextureLayout textureLayout, const glm::vec4& color, const TextureSubresourceRange& subresources) = 0;
        virtual void CopyBufferToTexture(CommandBuffer* commandbuffer, Buffer* srcBuffer, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, BufferTextureCopyRegion* regions) = 0;
        virtual void CopyTextureToBuffer(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, Buffer* dstBuffer, uint32 numRegion, BufferTextureCopyRegion* regions) = 0;
        virtual void BlitTexture(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, TextureBlitRegion* regions, SamplerFilter filter = SAMPLER_FILTER_LINEAR) = 0;

        virtual void PushConstants(CommandBuffer* commandbuffer, ShaderHandle* shader, uint32 firstIndex, uint32* data, uint32 numData) = 0;
        virtual void BeginRenderPass(CommandBuffer* commandbuffer, RenderPass* renderpass, FramebufferHandle* framebuffer, CommandBufferType commandBufferType, uint32 numclearValues, RenderPassClearValue* clearvalues) = 0;
        virtual void EndRenderPass(CommandBuffer* commandbuffer) = 0;
        virtual void NextRenderSubpass(CommandBuffer* commandbuffer, CommandBufferType commandBufferType) = 0;
        virtual void SetViewport(CommandBuffer* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height) = 0;
        virtual void SetScissor(CommandBuffer* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height) = 0;
        virtual void ClearAttachments(CommandBuffer* commandbuffer, uint32 numAttachmentClear, AttachmentClear** attachmentClears, uint32 x, uint32 y, uint32 width, uint32 height) = 0;
        virtual void BindPipeline(CommandBuffer* commandbuffer, Pipeline* pipeline) = 0;
        virtual void BindDescriptorSet(CommandBuffer* commandbuffer, DescriptorSet* descriptorset, uint32 setIndex) = 0;
        virtual void Draw(CommandBuffer* commandbuffer, uint32 vertexCount, uint32 instanceCount, uint32 baseVertex, uint32 firstInstance) = 0;
        virtual void DrawIndexed(CommandBuffer* commandbuffer, uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance) = 0;
        virtual void BindVertexBuffers(CommandBuffer* commandbuffer, uint32 bindingCount, Buffer** buffers, uint64* offsets) = 0;
        virtual void BindIndexBuffer(CommandBuffer* commandbuffer, Buffer* buffer, IndexBufferFormat format, uint64 offset) = 0;

        //--------------------------------------------------
        // MISC
        //--------------------------------------------------
        virtual bool ImmidiateCommands(CommandQueue* queue, CommandBuffer* commandBuffer, Fence* fence, std::function<void(CommandBuffer*)>&& func) = 0;
        virtual bool WaitDevice() = 0;
    };
}
