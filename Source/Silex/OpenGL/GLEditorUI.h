
#pragma once

#include "Core/Core.h"
#include "Editor/EditorUI.h"


namespace Silex
{
    class GLEditorUI : public EditorUI
    {
        SL_DECLARE_CLASS(GLEditorUI, EditorUI)

    public:

        void Init()     override;
        void Shutdown() override;
        void Render()   override;

        void BeginFrame() override;
        void EndFrame()   override;
    };
}
