#pragma once

#include "Core/OS.h"
#include "Core/Event.h"
#include "Core/Window.h"
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
        SL_CLASS(Engine, Object)

    public:

        bool Initialize();
        void Finalize();
        bool MainLoop();
        void Close();

    public:

        static Engine* Get();

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

        RenderingDevice* renderingDevice = nullptr;

    private:

        Editor*   editor   = nullptr;
        EditorUI* editorUI = nullptr;

        bool isRunning = true;
        bool minimized = false;

        uint64 lastFrameTime = 0;
        uint32 frameRate     = 0;
        float  deltaTime     = 0.0f;

        std::unordered_map<const char*, float> performanceData;
    };
}
