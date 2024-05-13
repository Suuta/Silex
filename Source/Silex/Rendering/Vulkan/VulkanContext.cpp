
#include "PCH.h"
#include "Rendering/Vulkan/VulkanContext.h"


namespace Silex
{
    void VulkanContext::Initialize()
    {
        // RenderAPIクラスの初期化でデバイス生成時に扱うため
        // instance device queue 情報を取得し、保存しておく
    }

    const DeviceInfo& VulkanContext::GetDeviceInfo() const
    {
        return deviceInfo;
    }
}
