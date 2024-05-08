#pragma once

#include "Core/Timer.h"
#include "Engine/Event.h"
#include "Engine/Window.h"


namespace Silex
{
    class Application
    {
    public:

        Application()          = default;
        virtual ~Application() = default;

        virtual void OnInit()                  = 0;
        virtual void OnShutdown()              = 0;
        virtual void OnUpdate(float deltaTime) = 0;
        virtual void OnRender()                = 0;
        virtual void OnEvent(Event& event)     = 0;
    };
}