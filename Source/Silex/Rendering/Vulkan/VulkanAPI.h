
#pragma once

#include "Rendering/RenderingAPI.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_mem_alloc.h>


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

        std::vector<VkFramebuffer> framebuffers;
        std::vector<VkImage>       images;
        std::vector<VkImageView>   views;
    };


    // デバイス拡張機能関数
    struct DeviceExtensionFunctions
    {
        PFN_vkCreateSwapchainKHR    vkCreateSwapchainKHR    = nullptr;
        PFN_vkDestroySwapchainKHR   vkDestroySwapchainKHR   = nullptr;
        PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = nullptr;
        PFN_vkAcquireNextImageKHR   vkAcquireNextImageKHR   = nullptr;
        PFN_vkQueuePresentKHR       vkQueuePresentKHR       = nullptr;
        PFN_vkCreateRenderPass2KHR  vkCreateRenderPass2KHR  = nullptr;
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
        FramebufferHandle* GetSwapChainNextFramebuffer() override;
        RenderPass* GetSwapChainRenderPass(SwapChain* swapchain) override;
        RenderingFormat GetSwapChainFormat(SwapChain* swapchain) override;
        void DestroySwapChain(SwapChain* swapchain) override;

        // レンダーパス
        RenderPass* CreateRenderPass(uint32 numAttachments, Attachment* attachments, uint32 numSubpasses, Subpass* subpasses, uint32 numSubpassDependencies, SubpassDependency* subpassDependencies) override;
        void DestroyRenderPass(RenderPass* renderpass) override;

    private:

        VkSampleCountFlagBits _GetSupportedSampleCounts(TextureSamples samples);



    private:

        // デバイス拡張機能関数
        DeviceExtensionFunctions extensions;

        // レンダリングコンテキスト
        VulkanContext* context = nullptr;

        // 論理デバイス
        VkDevice device = nullptr;

        // アロケータ
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

