
#include "PCH.h"

#include "Core/Engine.h"
#include "Rendering/RenderingDevice.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/Vulkan/VulkanStructures.h"
#include "Rendering/Vulkan/VulkanContext.h"
#include "ImGui/Vulkan/VulkanGUI.h"

#include <imgui/imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_dx12.h>


namespace Silex
{
    static VkDescriptorPool CreatePool(VkDevice device)
    {
        VkDescriptorPoolSize poolSizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER,                1000},
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000},
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000},
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000},
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000},
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000},
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000},
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000},
        };

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT; // vkFreeDescriptorSets が使用可能になる
        poolInfo.maxSets       = 1000;
        poolInfo.poolSizeCount = std::size(poolSizes);
        poolInfo.pPoolSizes    = poolSizes;

        VkDescriptorPool pool = nullptr;
        VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
        SL_CHECK_VKRESULT(result, nullptr);

        return pool;
    }

    static void UploadFontTexture(VkDevice device, VkQueue queue, uint32 queueFamily)
    {
        VkResult result;

        VkCommandPoolCreateInfo commandPoolInfo = {};
        commandPoolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolInfo.queueFamilyIndex = queueFamily;

        VkCommandPool commandPool = nullptr;
        result = vkCreateCommandPool(device, &commandPoolInfo, nullptr, &commandPool);
        SL_CHECK_VKRESULT(result, SL_DONT_USE);

        VkCommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandPool        = commandPool;
        allocateInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = nullptr;
        result = vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);
        SL_CHECK_VKRESULT(result, SL_DONT_USE);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        {
            result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
            SL_CHECK_VKRESULT(result, SL_DONT_USE);

            ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

            result = vkEndCommandBuffer(commandBuffer);
            SL_CHECK_VKRESULT(result, SL_DONT_USE);
        }

        VkSubmitInfo submit = {};
        submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers    = &commandBuffer;

        result = vkQueueSubmit(queue, 1, &submit, nullptr);
        SL_CHECK_VKRESULT(result, SL_DONT_USE);

        result = vkQueueWaitIdle(queue);
        SL_CHECK_VKRESULT(result, SL_DONT_USE);

        vkDestroyCommandPool(device, commandPool, nullptr);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }



    VulkanGUI::VulkanGUI()
    {
    }

    VulkanGUI::~VulkanGUI()
    {
        vkDeviceWaitIdle(vulkanContext->GetDevice());

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        vkDestroyDescriptorPool(vulkanContext->GetDevice(), descriptorPool, nullptr);
    }

    void VulkanGUI::Init(RenderingContext* context)
    {
        vulkanContext = (VulkanContext*)context;

        GLFWwindow*         glfw      = Window::Get()->GetGLFWWindow();
        VulkanSwapChain*    swapchain = (VulkanSwapChain*)Window::Get()->GetSwapChain();
        VulkanCommandQueue* queue     = (VulkanCommandQueue*)RenderingDevice::Get()->GetGraphicsCommandQueue();

        Super::Init(vulkanContext);

        // デスクリプタープール生成
        descriptorPool = CreatePool(vulkanContext->GetDevice());

        // VulknaImGui 初期化
        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance              = vulkanContext->GetInstance();
        initInfo.PhysicalDevice        = vulkanContext->GetPhysicalDevice();
        initInfo.Device                = vulkanContext->GetDevice();
        initInfo.Queue                 = queue->queue;
        initInfo.QueueFamily           = queue->family;
        initInfo.DescriptorPool        = descriptorPool;
        initInfo.Subpass               = 0;
        initInfo.MinImageCount         = 2;
        initInfo.ImageCount            = 3;
        initInfo.MSAASamples           = VK_SAMPLE_COUNT_1_BIT;
        initInfo.ColorAttachmentFormat = swapchain->format;
        initInfo.UseDynamicRendering   = false;
        initInfo.PipelineCache         = nullptr;
        initInfo.Allocator             = nullptr;
        initInfo.CheckVkResultFn       = nullptr;

        ImGui_ImplGlfw_InitForVulkan(glfw, true);
        ImGui_ImplVulkan_Init(&initInfo, swapchain->renderpass->renderpass);

        // フォントアップロード
        UploadFontTexture(vulkanContext->GetDevice(), queue->queue, queue->family);
    }

    void VulkanGUI::BeginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        Super::BeginFrame();
    }

    void VulkanGUI::EndFrame()
    {
        const FrameData& frame = RenderingDevice::Get()->GetFrameData();
        VkCommandBuffer commandBuffer = ((VulkanCommandBuffer*)(frame.commandBuffer))->commandBuffer;
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        Super::EndFrame();
    }
}