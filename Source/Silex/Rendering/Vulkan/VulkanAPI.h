
#pragma once

#include "Rendering/RenderingAPI.h"
#include "Rendering/Vulkan/VulkanStructures.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_mem_alloc.h>


// TODO: VulkanAPI
// ・パイプライン
// ・シェーダー
// 
// ・RenderDeviceでラッパー実装
//   - アセットは Ref<TextureAsset> 形式で保持?
//   - 
//   - レンダーオブジェクトをメンバーに内包する形で表現
//   - class TextureAsset : public Asset
//   - { 
//   -     Texture* texture;
//   - }
//   - 
//   - アセットマネージャの対象は ○○Asset から行うようにし
//   - Texture 自体は、レンダラー側で管理できるようにする
// 
// ・メインループ実装


namespace Silex
{
    class  VulkanContext;
    struct VulkanSurface;

    //=============================================
    // Vulkan API 実装
    //=============================================
    class VulkanAPI : public RenderingAPI
    {
        SL_CLASS(VulkanAPI, RenderingAPI)

    public:

        VulkanAPI(VulkanContext* context);
        ~VulkanAPI();

        bool Initialize() override;

        //--------------------------------------------------
        // コマンドキュー
        //--------------------------------------------------
        CommandQueue* CreateCommandQueue(QueueFamily family, uint32 indexInFamily = 0) override;
        void DestroyCommandQueue(CommandQueue* queue) override;
        QueueFamily QueryQueueFamily(uint32 queueFlag, Surface* surface = nullptr) const override;
        bool ExcuteQueue(CommandQueue* queue, CommandBuffer* commandbuffer, Fence* fence, Semaphore* wait, Semaphore* signal) override;

        //--------------------------------------------------
        // コマンドプール
        //--------------------------------------------------
        CommandPool* CreateCommandPool(QueueFamily family, CommandBufferType type) override;
        void DestroyCommandPool(CommandPool* pool) override;

        //--------------------------------------------------
        // コマンドバッファ
        //--------------------------------------------------
        CommandBuffer* CreateCommandBuffer(CommandPool* pool) override;
        void DestroyCommandBuffer(CommandBuffer* commandBuffer) override;
        bool BeginCommandBuffer(CommandBuffer* commandBuffer) override;
        bool EndCommandBuffer(CommandBuffer* commandBuffer) override;

        //--------------------------------------------------
        // セマフォ
        //--------------------------------------------------
        Semaphore* CreateSemaphore() override;
        void DestroySemaphore(Semaphore* semaphore) override;

        //--------------------------------------------------
        // フェンス
        //--------------------------------------------------
        Fence* CreateFence() override;
        void DestroyFence(Fence* fence) override;
        bool WaitFence(Fence* fence) override;

        //--------------------------------------------------
        // スワップチェイン
        //--------------------------------------------------
        SwapChain* CreateSwapChain(Surface* surface, uint32 width, uint32 height, uint32 requestFramebufferCount, VSyncMode mode) override;
        bool ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, uint32 requestFramebufferCount, VSyncMode mode) override;
        FramebufferHandle* GetSwapChainNextFramebuffer(SwapChain* swapchain, Semaphore* present, Semaphore* render) override;
        RenderPass* GetSwapChainRenderPass(SwapChain* swapchain) override;
        RenderingFormat GetSwapChainFormat(SwapChain* swapchain) override;
        void DestroySwapChain(SwapChain* swapchain) override;
        bool Present(CommandQueue* queue, SwapChain* swapchain) override;

        //--------------------------------------------------
        // バッファ
        //--------------------------------------------------
        Buffer* CreateBuffer(uint64 size, BufferUsageBits usage, MemoryAllocationType memoryType) override;
        void DestroyBuffer(Buffer* buffer) override;
        byte* MapBuffer(Buffer* buffer) override;
        void UnmapBuffer(Buffer* buffer) override;

        //--------------------------------------------------
        // テクスチャ
        //--------------------------------------------------
        TextureHandle* CreateTexture(const TextureFormat& format) override;
        void DestroyTexture(TextureHandle* texture) override;

        //--------------------------------------------------
        // サンプラ
        //--------------------------------------------------
        Sampler* CreateSampler(const SamplerState& state) override;
        void DestroySampler(Sampler* sampler) override;

        //--------------------------------------------------
        // フレームバッファ
        //--------------------------------------------------
        FramebufferHandle* CreateFramebuffer(RenderPass* renderpass, TextureHandle* textures, uint32 numTexture, uint32 width, uint32 height) override;
        void DestroyFramebuffer(FramebufferHandle* framebuffer) override;

        //--------------------------------------------------
        // 頂点フォーマット
        //--------------------------------------------------
        VertexFormat* CreateVertexFormat(uint32 numattributes, VertexAttribute* attributes) override;
        void DestroyVertexFormat(VertexAttribute* attributes) override;

        //--------------------------------------------------
        // レンダーパス
        //--------------------------------------------------
        RenderPass* CreateRenderPass(uint32 numAttachments, Attachment* attachments, uint32 numSubpasses, Subpass* subpasses, uint32 numSubpassDependencies, SubpassDependency* subpassDependencies) override;
        void DestroyRenderPass(RenderPass* renderpass) override;
        
        //--------------------------------------------------
        // シェーダー
        //--------------------------------------------------
        ShaderHandle* CreateShader(const ShaderCompiledData& compiledData) override;
        void DestroyShader(ShaderHandle* shader) override;

        //--------------------------------------------------
        // デスクリプターセット
        //--------------------------------------------------
        DescriptorSet* CreateDescriptorSet(uint32 numdescriptors, DescriptorInfo* descriptors, ShaderHandle* shader, uint32 setIndex) override;
        void DestroyDescriptorSet(DescriptorSet* descriptorset) override;

        //--------------------------------------------------
        // パイプライン
        //--------------------------------------------------
        virtual Pipeline* CreateGraphicsPipeline(ShaderHandle* shader, VertexFormat* vertexFormat, PrimitiveTopology primitive, PipelineRasterizationState rasterizationState, PipelineMultisampleState multisampleState, PipelineDepthStencilState depthstencilState, PipelineColorBlendState blendState, int32* colorAttachments, uint32 numColorAttachments, PipelineDynamicStateFlags dynamicState, RenderPass* renderpass, uint32 renderSubpass) override;
        virtual Pipeline* CreateComputePipeline(ShaderHandle* shader) override;
        void DestroyPipeline(Pipeline* pipeline) override;

        //--------------------------------------------------
        // コマンド
        //--------------------------------------------------
        void PipelineBarrier(CommandBuffer* commandbuffer, PipelineStageBits srcStage, PipelineStageBits dstStage, uint32 numMemoryBarrier, MemoryBarrier* memoryBarrier, uint32 numBufferBarrier, BufferBarrier* bufferBarrier, uint32 numTextureBarrier, TextureBarrier* textureBarrier) override;
        
        void ClearBuffer(CommandBuffer* commandbuffer, Buffer* buffer, uint64 offset, uint64 size) override;
        void CopyBuffer(CommandBuffer* commandbuffer, Buffer* srcBuffer, Buffer* dstBuffer, uint32 numRegion, BufferCopyRegion* regions) override;
        void CopyTexture(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, TextureCopyRegion* regions) override;
        void ResolveTexture(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, uint32 srcLayer, uint32 srcMipmap, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 dstLayer, uint32 dstMipmap) override;
        void ClearColorTexture(CommandBuffer* commandbuffer, TextureHandle* texture, TextureLayout textureLayout, const glm::vec4& color, const TextureSubresourceRange& subresources) override;
        void CopyBufferToTexture(CommandBuffer* commandbuffer, Buffer* srcBuffer, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, BufferTextureCopyRegion* regions) override;
        void CopyTextureToBuffer(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, Buffer* dstBuffer, uint32 numRegion, BufferTextureCopyRegion* regions) override;
        
        void PushConstants(CommandBuffer* commandbuffer, ShaderHandle* shader, uint32 firstIndex, uint32* data, uint32 numData) override;
        
        void BeginRenderPass(CommandBuffer* commandbuffer, RenderPass* renderpass, FramebufferHandle* framebuffer, CommandBufferType commandBufferType, uint32 numclearValues, RenderPassClearValue* clearvalues, uint32 x, uint32 y, uint32 width, uint32 height) override;
        void EndRenderPass(CommandBuffer* commandbuffer) override;
        void NextRenderSubpass(CommandBuffer* commandbuffer, CommandBufferType commandBufferType) override;
        
        void SetViewport(CommandBuffer* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height) override;
        void SetScissor(CommandBuffer* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height) override;
        void ClearAttachments(CommandBuffer* commandbuffer, uint32 numAttachmentClear, AttachmentClear** attachmentClears, uint32 x, uint32 y, uint32 width, uint32 height) override;
        
        void BindPipeline(CommandBuffer* commandbuffer, Pipeline* pipeline) override;
        void BindDescriptorSet(CommandBuffer* commandbuffer, DescriptorSet* descriptorset, ShaderHandle* shader, uint32 setIndex) override;
        void Draw(CommandBuffer* commandbuffer, uint32 vertexCount, uint32 instanceCount, uint32 baseVertex, uint32 firstInstance) override;
        void DrawIndexed(CommandBuffer* commandbuffer, uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance) override;
        void BindVertexBuffers(CommandBuffer* commandbuffer, uint32 bindingCount, const Buffer** buffers, const uint64* offsets) override;
        void BindIndexBuffer(CommandBuffer* commandbuffer, Buffer* buffer, IndexBufferFormat format, uint64 offset) override;
        
        void SetLineWidth(CommandBuffer* commandbuffer, float width) override;

    private:

        // デスクリプターセットシグネチャから同一シグネチャのデスクリプタプールを検索、なければ新規生成
        VkDescriptorPool _FindOrCreateDescriptorPool(const DescriptorSetPoolKey& key);

        // デスクリプタプールの参照カウントを減らす（0なら破棄）
        void _DecrementPoolRefCount(VkDescriptorPool pool, DescriptorSetPoolKey& poolKey);

        // 指定されたサンプル数が、利用可能なサンプル数かどうかチェックする
        VkSampleCountFlagBits _CheckSupportedSampleCounts(TextureSamples samples);

    private:

        // デスクリプター型と個数では一意のハッシュ値を生成できないので、unordered_mapではなく、mapを採用
        std::map<DescriptorSetPoolKey, std::unordered_map<VkDescriptorPool, uint32>> descriptorsetPools;

        // デバイス拡張機能関数
        PFN_vkCreateSwapchainKHR    CreateSwapchainKHR    = nullptr;
        PFN_vkDestroySwapchainKHR   DestroySwapchainKHR   = nullptr;
        PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR = nullptr;
        PFN_vkAcquireNextImageKHR   AcquireNextImageKHR   = nullptr;
        PFN_vkQueuePresentKHR       QueuePresentKHR       = nullptr;
        PFN_vkCreateRenderPass2KHR  CreateRenderPass2KHR  = nullptr;

        // レンダリングコンテキスト
        VulkanContext* context = nullptr;

        // 論理デバイス
        VkDevice device = nullptr;

        // VMAアロケータ (VulkanMemoryAllocator: VkImage/VkBuffer の生成にともなうメモリ管理を代行)
        VmaAllocator allocator = nullptr;
    };
}
