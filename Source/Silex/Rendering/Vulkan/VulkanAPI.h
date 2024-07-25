
#pragma once

#include "Rendering/RenderingAPI.h"
#include "Rendering/Vulkan/VulkanStructures.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_mem_alloc.h>


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
        QueueFamily QueryQueueFamily(QueueFamilyFlags queueFlag, Surface* surface = nullptr) const override;
        bool SubmitQueue(CommandQueue* queue, CommandBuffer* commandbuffer, Fence* fence, Semaphore* present, Semaphore* render) override;

        //--------------------------------------------------
        // コマンドプール
        //--------------------------------------------------
        CommandPool* CreateCommandPool(QueueFamily family, CommandBufferType type = COMMAND_BUFFER_TYPE_PRIMARY) override;
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
        void DestroySwapChain(SwapChain* swapchain) override;
        bool ResizeSwapChain(SwapChain* swapchain, uint32 width, uint32 height, uint32 requestFramebufferCount, VSyncMode mode) override;
        FramebufferHandle* GetCurrentBackBuffer(SwapChain* swapchain, Semaphore* present) override;
        bool Present(CommandQueue* queue, SwapChain* swapchain, Semaphore* render) override;

        RenderPass* GetSwapChainRenderPass(SwapChain* swapchain) override;
        RenderingFormat GetSwapChainFormat(SwapChain* swapchain) override;

        //--------------------------------------------------
        // バッファ
        //--------------------------------------------------
        Buffer* CreateBuffer(uint64 size, BufferUsageFlags usage, MemoryAllocationType memoryType) override;
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
        Sampler* CreateSampler(const SamplerInfo& info) override;
        void DestroySampler(Sampler* sampler) override;

        //--------------------------------------------------
        // フレームバッファ
        //--------------------------------------------------
        FramebufferHandle* CreateFramebuffer(RenderPass* renderpass, uint32 numTexture, TextureHandle** textures, uint32 width, uint32 height) override;
        void DestroyFramebuffer(FramebufferHandle* framebuffer) override;

        //--------------------------------------------------
        // 入力レイアウト
        //--------------------------------------------------
        InputLayout* CreateInputLayout(uint32 numBindings, InputBinding* bindings) override;
        void DestroyInputLayout(InputLayout* layout) override;

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
        void UpdateDescriptorSet(DescriptorSet* descriptorSet, uint32 numdescriptor, DescriptorInfo* descriptors) override;

        //--------------------------------------------------
        // パイプライン
        //--------------------------------------------------
        Pipeline* CreateGraphicsPipeline(ShaderHandle* shader, InputLayout* inputLayout, PipelineInputAssemblyState inputAssemblyState, PipelineRasterizationState rasterizationState, PipelineMultisampleState multisampleState, PipelineDepthStencilState depthstencilState, PipelineColorBlendState blendState, RenderPass* renderpass, uint32 renderSubpass = 0, PipelineDynamicStateFlags dynamicState = DYNAMIC_STATE_NONE) override;
        Pipeline* CreateComputePipeline(ShaderHandle* shader) override;
        void DestroyPipeline(Pipeline* pipeline) override;

        //--------------------------------------------------
        // コマンド
        //--------------------------------------------------
        void PipelineBarrier(CommandBuffer* commandbuffer, PipelineStageBits srcStage, PipelineStageBits dstStage, uint32 numMemoryBarrier, MemoryBarrierInfo* memoryBarrier, uint32 numBufferBarrier, BufferBarrierInfo* bufferBarrier, uint32 numTextureBarrier, TextureBarrierInfo* textureBarrier) override;
        
        void ClearBuffer(CommandBuffer* commandbuffer, Buffer* buffer, uint64 offset, uint64 size) override;
        void CopyBuffer(CommandBuffer* commandbuffer, Buffer* srcBuffer, Buffer* dstBuffer, uint32 numRegion, BufferCopyRegion* regions) override;
        void CopyTexture(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, TextureCopyRegion* regions) override;
        void ResolveTexture(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, uint32 srcLayer, uint32 srcMipmap, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 dstLayer, uint32 dstMipmap) override;
        void ClearColorTexture(CommandBuffer* commandbuffer, TextureHandle* texture, TextureLayout textureLayout, const glm::vec4& color, const TextureSubresourceRange& subresources) override;
        void CopyBufferToTexture(CommandBuffer* commandbuffer, Buffer* srcBuffer, TextureHandle* dstTexture, TextureLayout dstTextureLayout, uint32 numRegion, BufferTextureCopyRegion* regions) override;
        void CopyTextureToBuffer(CommandBuffer* commandbuffer, TextureHandle* srcTexture, TextureLayout srcTextureLayout, Buffer* dstBuffer, uint32 numRegion, BufferTextureCopyRegion* regions) override;
        
        void PushConstants(CommandBuffer* commandbuffer, ShaderHandle* shader, uint32 firstIndex, uint32* data, uint32 numData) override;
        
        void BeginRenderPass(CommandBuffer* commandbuffer, RenderPass* renderpass, FramebufferHandle* framebuffer, CommandBufferType commandBufferType, uint32 numclearValues, RenderPassClearValue* clearvalues) override;
        void EndRenderPass(CommandBuffer* commandbuffer) override;
        void NextRenderSubpass(CommandBuffer* commandbuffer, CommandBufferType commandBufferType) override;
        
        void SetViewport(CommandBuffer* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height) override;
        void SetScissor(CommandBuffer* commandbuffer, uint32 x, uint32 y, uint32 width, uint32 height) override;
        void ClearAttachments(CommandBuffer* commandbuffer, uint32 numAttachmentClear, AttachmentClear** attachmentClears, uint32 x, uint32 y, uint32 width, uint32 height) override;
        
        void BindPipeline(CommandBuffer* commandbuffer, Pipeline* pipeline) override;
        void BindDescriptorSet(CommandBuffer* commandbuffer, DescriptorSet* descriptorset, uint32 setIndex) override;
        void Draw(CommandBuffer* commandbuffer, uint32 vertexCount, uint32 instanceCount, uint32 baseVertex, uint32 firstInstance) override;
        void DrawIndexed(CommandBuffer* commandbuffer, uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance) override;
        void BindVertexBuffers(CommandBuffer* commandbuffer, uint32 bindingCount, Buffer** buffers, uint64* offsets) override;
        void BindIndexBuffer(CommandBuffer* commandbuffer, Buffer* buffer, IndexBufferFormat format, uint64 offset) override;

        //--------------------------------------------------
        // MISC
        //--------------------------------------------------
        bool ImmidiateCommands(CommandQueue* queue, CommandBuffer* commandBuffer, Fence* fence, std::function<void(CommandBuffer*)>&& func) override;
        bool WaitDevice() override;


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

        // レンダリングコンテキスト
        VulkanContext* context = nullptr;

        // 論理デバイス
        VkDevice device = nullptr;

        // VMAアロケータ (VulkanMemoryAllocator: VkImage/VkBuffer の生成にともなうメモリ管理を代行)
        VmaAllocator allocator = nullptr;
    };
}
