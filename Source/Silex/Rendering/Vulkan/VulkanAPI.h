
#pragma once

#include "Rendering/RenderingAPI.h"
#include <Vulkan/vulkan.h>


namespace Silex
{
    class VulkanContext;

    //=============================================
    // Vulkan 構造体
    //=============================================
    struct VulkanCommandQueue : public CommandQueue
    {
        uint32 queueIndex = INVALID_RENDER_ID;
    };

    struct VulkanCommandPool : public CommandQueue
    {
        VkCommandPool     commandPool = nullptr;
        CommandBufferType type        = COMMAND_BUFFER_TYPE_PRIMARY;
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

        QueueFamily GetQueueFamily(uint32 flag, Surface* surface = nullptr) const override;
        CommandQueue* CreateCommandQueue(QueueFamily family) override;
        CommandPool* CreateCommandPool(QueueFamily family, CommandBufferType type) override;

    private:

        VulkanContext* context = nullptr;
        VkDevice       device  = nullptr;
    };
}

