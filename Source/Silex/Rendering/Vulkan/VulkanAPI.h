
#pragma once

#include "Rendering/RenderingAPI.h"

#include <vulkan/vulkan.h>
#include <vulkan/vk_mem_alloc.h>


namespace Silex
{
    class VulkanContext;

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
        VkFence             fence         = nullptr;
        VulkanCommandQueue* queueSignaled = nullptr;
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

    private:

        VulkanContext* context = nullptr;
        VkDevice       device  = nullptr;

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

