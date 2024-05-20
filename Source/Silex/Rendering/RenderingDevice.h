
#pragma once

#include "Rendering/RenderingCore.h"


namespace Silex
{
    class RenderingAPI;
    class RenderingContext;


    class RenderingDevice : public Object
    {
        SL_CLASS(RenderingDevice, Object);

    public:

        RenderingDevice();
        ~RenderingDevice();

        bool Initialize(RenderingContext* context);

    private:

        QueueFamily graphicsQueueFamily = INVALID_RENDER_ID;
        QueueFamily presentQueueFamily  = INVALID_RENDER_ID;

        CommandQueue* graphicsQueue = nullptr;
        CommandQueue* presentQueue  = nullptr;

        CommandPool* commandPool = nullptr;

        RenderingContext* renderingContext = nullptr;
        RenderingAPI*     renderingAPI     = nullptr;

        static inline RenderingDevice* instance = nullptr;
    };
}

