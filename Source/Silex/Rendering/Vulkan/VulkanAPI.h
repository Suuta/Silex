
#pragma once

#include "Rendering/RenderingAPI.h"
#include <Vulkan/vulkan.h>


namespace Silex
{
    class VulkanContext;

    class VulkanAPI : public RenderingAPI
    {
        SL_DECLARE_CLASS(VulkanAPI, RenderingAPI)

    public:

        VulkanAPI(VulkanContext* context);
        ~VulkanAPI();

        Result             Initialize()                                                         override;
        CommandQueueFamily GetCommandQueueFamily(uint32 flag, Surface* surface = nullptr) const override;

    private:

        VulkanContext* context = nullptr;
    };
}

