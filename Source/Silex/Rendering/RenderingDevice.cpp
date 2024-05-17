
#include "PCH.h"

#include "Core/Window.h"
#include "Rendering/RenderingDevice.h"
#include "Rendering/RenderingContext.h"
#include "Rendering/RenderingAPI.h"


namespace Silex
{
    RenderingDevice::RenderingDevice()
    {
        instance = this;
    }

    RenderingDevice::~RenderingDevice()
    {
        instance = nullptr;
    }

    Result RenderingDevice::Initialize(RenderingContext* context)
    {
        renderingContext = context;
        Surface* surface = Window::Get()->GetSurface();

        // レンダーAPI実装クラスを生成
        renderingAPI = renderingContext->CreateRendringAPI();
        renderingAPI->Initialize();

        // グラフィックスとコンピュートをサポートするキューファミリを取得
        CommandQueueFamily family = renderingAPI->GetCommandQueueFamily(QUEUE_FAMILY_GRAPHICS_BIT | QUEUE_FAMILY_COMPUTE_BIT, surface);
        if (family == INVALID_RENDER_ID)
        {
            return Result::FAIL;
        }

        graphicsQueueFamily = family;
        presentQueueFamily  = family;



        return Result::OK;
    }
}

