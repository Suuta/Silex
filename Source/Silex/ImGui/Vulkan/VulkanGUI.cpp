
#include "PCH.h"

#include "Core/Engine.h"
#include "Rendering/RenderingDevice.h"
#include "ImGui/Vulkan/VulkanGUI.h"

#include <imgui/imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>



namespace Silex
{
    void VulkanGUI::Init()
    {
        Super::Init();
        GLFWwindow* window = Window::Get()->GetGLFWWindow();


        // RenderingDevice::Get()->GetImGuiInitData();

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance              = nullptr;
        initInfo.PhysicalDevice        = nullptr;
        initInfo.Device                = nullptr;
        initInfo.QueueFamily           = 0;
        initInfo.Queue                 = nullptr;
        initInfo.PipelineCache         = nullptr;
        initInfo.DescriptorPool        = nullptr;
        initInfo.Subpass               = 0;
        initInfo.MinImageCount         = 2;
        initInfo.ImageCount            = 3;
        initInfo.MSAASamples           = VK_SAMPLE_COUNT_1_BIT;
        initInfo.UseDynamicRendering   = false;
        initInfo.ColorAttachmentFormat = VK_FORMAT_B8G8R8A8_UNORM;
        initInfo.Allocator             = nullptr;
        initInfo.CheckVkResultFn       = nullptr;

        ImGui_ImplGlfw_InitForVulkan(window, true);
        ImGui_ImplVulkan_Init(&initInfo, nullptr);  // renderpass

        ImGui_ImplVulkan_CreateFontsTexture(nullptr);
    }

    void VulkanGUI::Shutdown()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();

        Super::Shutdown();
    }

    void VulkanGUI::Render()
    {
        Super::Render();

        ImDrawData* drawData = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(drawData, nullptr); // commandbuffer, pipline = nullptr
    }

    void VulkanGUI::BeginFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        Super::BeginFrame();
    }

    void VulkanGUI::EndFrame()
    {
        Render();
        Super::EndFrame();
    }
}
