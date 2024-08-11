
#pragma once

#include "Core/Core.h"
#include "ImGui/GUI.h"


namespace Silex
{
    class GLEditorUI : public GUI
    {
        SL_CLASS(GLEditorUI, GUI)

    public:

        GLEditorUI();
        ~GLEditorUI();

        void Init(RenderingContext* context) override;

        void BeginFrame() override;
        void Render()     override;
        void EndFrame()   override;
    };
}
