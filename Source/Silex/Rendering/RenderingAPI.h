
#pragma once

#include "Rendering/RenderingCore.h"


namespace Silex
{
    class RenderingContext;

    class RenderingAPI : public Object
    {
        SL_CLASS(RenderingAPI, Object)

    public:

        RenderingAPI()  {};
        ~RenderingAPI() {};

    public:

        virtual bool Initialize() = 0;

        virtual QueueFamily GetQueueFamily(uint32 flag, Surface* surface = nullptr) const = 0;
        virtual CommandQueue* CreateCommandQueue(QueueFamily family) = 0;
        virtual CommandPool* CreateCommandPool(QueueFamily family, CommandBufferType type) = 0;
        
    private:

    };
}
