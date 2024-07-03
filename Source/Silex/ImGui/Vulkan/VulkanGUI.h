
#pragma once

#include "Core/Core.h"
#include "ImGui/GUI.h"


namespace Silex
{
    class VulkanGUI : public GUI
    {
        SL_CLASS(VulkanGUI, GUI)

    public:

        void Init()     override;
        void Shutdown() override;
        void Render()   override;

        void BeginFrame() override;
        void EndFrame()   override;
    };
}
