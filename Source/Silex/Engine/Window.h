#pragma once

#include "Core/Core.h"
#include "Engine/Event.h"

#include <string>


struct GLFWwindow;


namespace Silex
{
    SL_DECLARE_DELEGATE(WindowCloseDelegate,         void, WindowCloseEvent&)
    SL_DECLARE_DELEGATE(WindowResizeDelegate,        void, WindowResizeEvent&)
    SL_DECLARE_DELEGATE(KeyPressedDelegate,          void, KeyPressedEvent&)
    SL_DECLARE_DELEGATE(KeyReleasedDelegate,         void, KeyReleasedEvent&)
    SL_DECLARE_DELEGATE(MouseButtonPressedDelegate,  void, MouseButtonPressedEvent&)
    SL_DECLARE_DELEGATE(MouseButtonReleasedDelegate, void, MouseButtonReleasedEvent&)
    SL_DECLARE_DELEGATE(MouseScrollDelegate,         void, MouseScrollEvent&)
    SL_DECLARE_DELEGATE(MouseMoveDelegate,           void, MouseMoveEvent&)


    // GLFW に登録するためのユーザーデータ
    struct WindowData
    {
        std::string title;
        uint32      width;
        uint32      height;

        Delegate<void(WindowCloseEvent&)>         windowCloseEvent;
        Delegate<void(WindowResizeEvent&)>        windowResizeEvent;
        Delegate<void(KeyPressedEvent&)>          keyPressedEvent;
        Delegate<void(KeyReleasedEvent&)>         keyReleasedEvent;
        Delegate<void(MouseButtonPressedEvent&)>  mouseButtonPressedEvent;
        Delegate<void(MouseButtonReleasedEvent&)> mouseButtonReleasedEvent;
        Delegate<void(MouseScrollEvent&)>         mouseScrollEvent;
        Delegate<void(MouseMoveEvent&)>           mouseMoveEvent;
    };


    struct WindowCreateInfo
    {
        std::string title             = {};
        uint32      width             = 1280;
        uint32      height            = 720;
        bool        vsync             = true;
        bool        roundWindowCorner = true;  // Windows 11 のみサポート
    };


    class Window : public Object
    {
        SL_DECLARE_CLASS(Window, Object)

    public:

        static Window* Create(const WindowCreateInfo& createInfo);

    public:

        Window(const WindowCreateInfo& createInfo);
        ~Window();

        void PumpMessage();
        void Destroy();

        glm::ivec2 GetSize()      const;
        glm::ivec2 GetWindowPos() const;

        void Maximize();
        void Minimize();
        void Restore();
        void Show();
        void Hide();

        const char* GetTitle() const;
        void SetTitle(const char* title);

        GLFWwindow* GetGLFWWindow()   const;
        HWND        GetWindowHandle() const;

        WindowData& GetWindowData() { return windowData; }

    private:

        WindowData  windowData;
        GLFWwindow* window = nullptr;
    };

}