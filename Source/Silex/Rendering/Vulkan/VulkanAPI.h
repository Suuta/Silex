
#pragma once

#include "Rendering/RenderingAPI.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_mem_alloc.h>


//----------------------------------------------------
// memo
//----------------------------------------------------
// アセットは Ref<TextureAsset> 形式で保持?
// 
// レンダーオブジェクトをメンバーに内包する形で表現
// class TextureAsset : public Asset
// { 
//     Texture* texture;
// }
//----------------------------------------------------

//============
// TODO:
//============
// 
// --- オブジェクト ---
// Buffer
// パイプライン・バリア
// コマンド
// 
// --- クリアスクリーンにむけて --- 
// シェーダ・デスクリプターセット（必須なら）
// データ転送: 他の実装参考に...
// フレーム同期
// 
// --- 各種操作 ---
// クリアスクリーンテスト
// トライアングル表示
// ImGui::Image 反映
// 


namespace Silex
{
    class  VulkanContext;
    struct VulkanSurface;

    //=============================================
    // Vulkan 構造体
    //=============================================
    struct VulkanCommandQueue : public CommandQueue
    {
        VkQueue queue  = nullptr;
        uint32  family = INVALID_RENDER_ID;
        uint32  index  = INVALID_RENDER_ID;
    };

    struct VulkanCommandPool : public CommandQueue
    {
        VkCommandPool     commandPool = nullptr;
        CommandBufferType type        = COMMAND_BUFFER_TYPE_PRIMARY;
    };

    struct VulkanCommandBuffer : public CommandBuffer
    {
        VkCommandBuffer commandBuffer = nullptr;
    };

    struct VulkanSemaphore : public Semaphore
    {
        VkSemaphore semaphore = nullptr;
    };

    struct VulkanFence : public Fence
    {
        VkFence fence = nullptr;
    };

    struct VulkanRenderPass : public RenderPass
    {
        VkRenderPass renderpass = nullptr;
    };

    struct VulkanFramebuffer : public FramebufferHandle
    {
        VkFramebuffer framebuffer = nullptr;
    };

    struct VulkanSwapChain : public SwapChain
    {
        VulkanSurface*    surface    = nullptr;
        VulkanRenderPass* renderpass = nullptr;

        VkSwapchainKHR  swapchain  = nullptr;
        VkFormat        format     = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR colorspace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        std::vector<FramebufferHandle*> framebuffers;
        std::vector<VkImage>            images;
        std::vector<VkImageView>        views;

        uint32 imageIndex = 0;
    };

    struct VulkanBuffer : public Buffer
    {
        VkBuffer      buffer           = nullptr;
        VkBufferView  view             = nullptr;
        uint64        bufferSize       = 0;
        VmaAllocation allocationHandle = nullptr;
        uint64        allocationSize   = 0;
    };

    struct VulkanTexture : public TextureHandle
    {
        VkImage     image    = nullptr;
        VkImageView imageView= nullptr;

        VmaAllocation     allocationHandle = nullptr;
        VmaAllocationInfo allocationInfo   = {};
    };

    struct VulkanSampler : public Sampler
    {
        VkSampler sampler = nullptr;
    };

    struct VulkanVertexFormat : public VertexFormat
    {
        std::vector<VkVertexInputBindingDescription>   bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
        VkPipelineVertexInputStateCreateInfo           createInfo = {};
    };


    // デバイス拡張機能関数
    struct DeviceExtensionFunctions
    {
        PFN_vkCreateSwapchainKHR    vkCreateSwapchainKHR    = nullptr;
        PFN_vkDestroySwapchainKHR   vkDestroySwapchainKHR   = nullptr;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
        PFN_vkAcquireNextImageKHR   vkAcquireNextImageKHR   = nullptr;
        PFN_vkQueuePresentKHR       vkQueuePresentKHR       = nullptr;
    };


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

        // コマンドキュー
        CommandQueue* CreateCommandQueue(QueueFamily family, uint32 indexInFamily = 0) override;
        void DestroyCommandQueue(CommandQueue* queue) override;
        QueueFamily QueryQueueFamily(uint32 queueFlag, Surface* surface = nullptr) const override;

        // コマンドプール
        CommandPool* CreateCommandPool(QueueFamily family, CommandBufferType type) override;
        void DestroyCommandPool(CommandPool* pool) override;

        // コマンドバッファ
        CommandBuffer* CreateCommandBuffer(CommandPool* pool) override;
        void DestroyCommandBuffer(CommandBuffer* commandBuffer) override;
        bool BeginCommandBuffer(CommandBuffer* commandBuffer) override;
        bool EndCommandBuffer(CommandBuffer* commandBuffer) override;

        // セマフォ
        Semaphore* CreateSemaphore() override;
        void DestroySemaphore(Semaphore* semaphore) override;

        // フェンス
        Fence* CreateFence() override;
        void DestroyFence(Fence* fence) override;
        bool WaitFence(Fence* fence) override;

        // スワップチェイン
        SwapChain* CreateSwapChain(Surface* surface) override;
        bool ResizeSwapChain(SwapChain* swapchain, uint32 requestFramebufferCount, VSyncMode mode) override;
        FramebufferHandle* GetSwapChainNextFramebuffer(SwapChain* swapchain, Semaphore* semaphore) override;
        RenderPass* GetSwapChainRenderPass(SwapChain* swapchain) override;
        RenderingFormat GetSwapChainFormat(SwapChain* swapchain) override;
        void DestroySwapChain(SwapChain* swapchain) override;

        // バッファ
        Buffer* CreateBuffer(uint64 size, BufferUsageBits usage, MemoryAllocationType memoryType) override;
        void DestroyBuffer(Buffer* buffer) override;
        byte* MapBuffer(Buffer* buffer) override;
        void UnmapBuffer(Buffer* buffer) override;

        // テクスチャ
        TextureHandle* CreateTexture(const TextureFormat& format) override;
        void DestroyTexture(TextureHandle* texture) override;

        // サンプラ
        Sampler* CreateSampler(const SamplerState& state) override;
        void DestroySampler(Sampler* sampler) override;

        // フレームバッファ
        FramebufferHandle* CreateFramebuffer(RenderPass* renderpass, TextureHandle* textures, uint32 numTexture, uint32 width, uint32 height) override;
        void DestroyFramebuffer(FramebufferHandle* framebuffer) override;

        // 頂点フォーマット
        VertexFormat* CreateVertexFormat(uint32 numattributes, VertexAttribute* attributes) override;
        void DestroyVertexFormat(VertexAttribute* attributes) override;

        // レンダーパス
        RenderPass* CreateRenderPass(uint32 numAttachments, Attachment* attachments, uint32 numSubpasses, Subpass* subpasses, uint32 numSubpassDependencies, SubpassDependency* subpassDependencies) override;
        void DestroyRenderPass(RenderPass* renderpass) override;

        // バリア
        void PipelineBarrier(CommandBuffer* commanddBuffer, PipelineStageBits srcStage, PipelineStageBits dstStage, uint32 numMemoryBarrier, MemoryBarrier* memoryBarrier, uint32 numBufferBarrier, BufferBarrier* bufferBarrier, uint32 numTextureBarrier, TextureBarrier* textureBarrier) override;

    private:

        // デバイス拡張機能関数
        DeviceExtensionFunctions extensions;

        // レンダリングコンテキスト
        VulkanContext* context = nullptr;

        // 論理デバイス
        VkDevice device = nullptr;

        // VMAアロケータ (VulkanMemoryAllocator: VkImage/VkBuffer の生成にともなうメモリ管理を代行)
        VmaAllocator allocator = nullptr;
    };


    // VkInstance
    // VkPhysicalDevice
    // VkDevice

    // VkQueue
    // VkCommandPool
    // VkCommandBuffer
    // VkSemaphore
    // VkFence

    // VkBuffer
    // VkImage
    // VkBufferView
    // VkImageView
    // VkSampler

    // VkFramebuffer
    // VkRenderPass

    // VkShaderModule
    // VkPipelineCache
    // VkPipelineLayout
    // VkPipeline
    // VkDescriptorSetLayout
    // VkDescriptorSet
    // VkDescriptorPool

    // VkDeviceMemory
    // VkEvent
    // VkQueryPool
}

