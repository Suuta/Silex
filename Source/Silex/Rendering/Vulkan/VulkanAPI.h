
#pragma once

#include "Rendering/RenderingAPI.h"
#include <Vulkan/vulkan.h>


namespace Silex
{
    class VulkanContext;


    class VulkanCommandQueue : public CommandQueue
    {
    public:
        uint32 queueIndex = INVALID_RENDER_ID;
    };

    struct VulkanCommandPool : public CommandQueue
    {
    public:
        VkCommandPool     commandPool = nullptr;
        CommandBufferType type        = COMMAND_BUFFER_TYPE_PRIMARY;
    };


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

