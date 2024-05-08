#pragma once

#include "Engine/Event.h"


namespace Silex
{
    class Module : public Object
    {
        SL_DECLARE_CLASS(Module, Object)

    public:

        virtual void Init()                  = 0;
        virtual void Shutdown()              = 0;
        virtual void Update(float deltaTime) = 0;
        virtual void Render()                = 0;
    };
}