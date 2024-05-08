#pragma once

#include "Core/OS.h"
#include "Engine/Event.h"
#include "Engine/Window.h"
#include "Core/Timer.h"
#include "Editor/Editor.h"
#include "Editor/EditorUI.h"


namespace Silex
{
    //==================================
    // エンジンクラス
    //==================================
    class Engine : public Object
    {
        SL_DECLARE_CLASS(Engine, Object)

    public:

        bool Init();
        void Shutdown();
        void MainLoop();
        void Close();

    public:

        static Engine* Get();

        Window* GetWindow()          { return window;    }
        Editor* GetEditor()          { return editor;    }
        float   GetDeltaTime() const { return deltaTime; }
        uint32  GetFrameRate() const { return frameRate; }

        const std::unordered_map<const char*, float>& GetPerformanceData() const
        {
            return performanceData;
        }

    private:

        void OnWindowResize(WindowResizeEvent& e);
        void OnWindowClose(WindowCloseEvent& e);
        void OnMouseMove(MouseMoveEvent& e);
        void OnMouseScroll(MouseScrollEvent& e);

        void CalcurateFrameTime();

    private:

        Window*   window;
        Editor*   editor;
        EditorUI* editorUI;

        bool isRunning   = true;
        bool minimized   = false;

        float  lastFrameTime = 0.0f;
        float  deltaTime     = 0.0f;
        uint32 frameRate     = 0;

        std::unordered_map<const char*, float> performanceData;
    };
}
