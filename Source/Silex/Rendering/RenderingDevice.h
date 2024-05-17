
#pragma once

#include "Rendering/RenderingCore.h"


namespace Silex
{
    class RenderingAPI;
    class RenderingContext;

    class RenderingDevice : public Object
    {
        SL_DECLARE_CLASS(RenderingDevice, Object);

    public:

        RenderingDevice();
        ~RenderingDevice();

        Result Initialize(RenderingContext* context);

    private:

        CommandQueueFamily graphicsQueueFamily;
        CommandQueueFamily presentQueueFamily;


        RenderingContext* renderingContext = nullptr;
        RenderingAPI*     renderingAPI     = nullptr;

        static inline RenderingDevice* instance = nullptr;
    };
}

