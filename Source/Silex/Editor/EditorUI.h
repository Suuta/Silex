
#pragma once

#include "Core/SharedPointer.h"


namespace Silex
{
    class EditorUI : public Object
    {
        SL_DECLARE_CLASS(EditorUI, Object)

    public:

        static EditorUI* Create();

        virtual void Init();
        virtual void Shutdown();
        virtual void Render();

        virtual void BeginFrame();
        virtual void EndFrame();
    };
}
