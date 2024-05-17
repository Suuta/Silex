
#include "PCH.h"
#include "Rendering/Vulkan/VulkanAPI.h"
#include "Rendering/Vulkan/VulkanContext.h"


namespace Silex
{
    VulkanAPI::VulkanAPI(VulkanContext* context)
        : context(context)
    {
    }

    VulkanAPI::~VulkanAPI()
    {
    }

    Result VulkanAPI::Initialize()
    {
        return Result::OK;
    }

    CommandQueueFamily VulkanAPI::GetCommandQueueFamily(uint32 flag, Surface* surface) const
    {
        CommandQueueFamily familyIndex = INVALID_RENDER_ID;

        const auto& queueFamilyProperties = context->GetQueueFamilyProperties();
        for (uint32 i = 0; i < queueFamilyProperties.size(); i++)
        {
            // サーフェースが有効であれば、プレゼントキューが有効なキューが必須となる
            if (surface != nullptr && !context->HasPresentSupport(surface, i))
            {
                continue;
            }

            // TODO: 専用キューを選択
            // 独立したキューがあればそのキューの方が性能が良いとされるので、flag値が低いものが専用キューになる。


            // 全フラグが立っていたら（全てのキューをサポートしている場合）
            const bool includeAll = (queueFamilyProperties[i].queueFlags & flag) == flag;
            if (includeAll)
            {
                familyIndex = i;
                break;
            }
        }

        return familyIndex;
    }
}
