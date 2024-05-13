#pragma once

#include "Core/Core.h"
#include "Core/Event.h"

#include <string>


struct GLFWwindow;


namespace Silex
{
    class Window;
    class RenderingContext;
    class RenderingDevice;


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

        WindowCloseDelegate         windowCloseEvent;
        WindowResizeDelegate        windowResizeEvent;
        KeyPressedDelegate          keyPressedEvent;
        KeyReleasedDelegate         keyReleasedEvent;
        MouseButtonPressedDelegate  mouseButtonPressedEvent;
        MouseButtonReleasedDelegate mouseButtonReleasedEvent;
        MouseScrollDelegate         mouseScrollEvent;
        MouseMoveDelegate           mouseMoveEvent;
    };

    struct WindowCreateInfo
    {
        std::string title             = {};
        uint32      width             = 1280;
        uint32      height            = 720;
        bool        vsync             = true;

        // Windows 11 のみサポート
        //bool roundWindowCorner = true;
    };

    using WindowCreateFunction = Window*(*)(const WindowCreateInfo&);


    class Window : public Object
    {
        SL_DECLARE_CLASS(Window, Object)

    public:

        static Window* Create(const WindowCreateInfo& createInfo);
        static Window* Get();

        static void RegisterCreateFunction(WindowCreateFunction createFunc);

    public:

        Window(const WindowCreateInfo& createInfo);
        ~Window();

        void PumpMessage();

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
        WindowData& GetWindowData();

    private:

        // レンダリング
        RenderingContext* renderingContext = nullptr;
        RenderingDevice*  renderingDevice = nullptr;

    private:

        static inline Window*              instance;
        static inline WindowCreateFunction createFunction;

        // ウィンドウデータ
        void*       userData;
        WindowData  windowData;
        GLFWwindow* window;
    };
}