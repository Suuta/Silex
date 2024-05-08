
#pragma once

#include "Core/SharedPointer.h"
#include "Scene/Entity.h"


namespace Silex
{
    class EditorPanel
    {
    public:

        virtual ~EditorPanel() = default;

    public:

        virtual void Initialize()       = 0;
        virtual void Finalize()         = 0;
        virtual void Render(bool* show) = 0;
    };
}
