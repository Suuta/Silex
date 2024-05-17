
#pragma once

#include "Rendering/RenderingCore.h"


namespace Silex
{
    class RenderingContext;

    class RenderingAPI : public Object
    {
        SL_DECLARE_CLASS(RenderingAPI, Object)

    public:

        RenderingAPI()  {};
        ~RenderingAPI() {};

    public:

        virtual Result             Initialize()                                                         = 0;
        virtual CommandQueueFamily GetCommandQueueFamily(uint32 flag, Surface* surface = nullptr) const = 0;
        
    private:

    };
}
