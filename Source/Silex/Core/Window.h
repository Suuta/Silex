#pragma once

#include "Core/Event.h"
#include "Rendering/RenderingCore.h"

#include <string>


struct GLFWwindow;


namespace Silex
{
    class  Window;
    class  RenderingContext;
    class  RenderingDevice;
    struct WindowCreateInfo;

    using WindowCreateFunction = Window* (*)(const WindowCreateInfo&);


    // ウィンドウコールバックイベント
    struct WindowCallbackEvents
    {
        SL_DECLARE_DELEGATE(WindowCloseDelegate, void, WindowCloseEvent&)
        WindowCloseDelegate windowCloseEvent;

        SL_DECLARE_DELEGATE(WindowResizeDelegate, void, WindowResizeEvent&)
        WindowResizeDelegate windowResizeEvent;

        SL_DECLARE_DELEGATE(KeyPressedDelegate, void, KeyPressedEvent&)
        KeyPressedDelegate keyPressedEvent;

        SL_DECLARE_DELEGATE(KeyReleasedDelegate, void, KeyReleasedEvent&)
        KeyReleasedDelegate keyReleasedEvent;

        SL_DECLARE_DELEGATE(MouseButtonPressedDelegate, void, MouseButtonPressedEvent&)
        MouseButtonPressedDelegate mouseButtonPressedEvent;

        SL_DECLARE_DELEGATE(MouseButtonReleasedDelegate, void, MouseButtonReleasedEvent&)
        MouseButtonReleasedDelegate mouseButtonReleasedEvent;

        SL_DECLARE_DELEGATE(MouseScrollDelegate, void, MouseScrollEvent&)
        MouseScrollDelegate mouseScrollEvent;

        SL_DECLARE_DELEGATE(MouseMoveDelegate, void, MouseMoveEvent&)
        MouseMoveDelegate mouseMoveEvent;
    };

    // ウィンドウデータ
    struct WindowData
    {
        std::string          title;
        uint32               width;
        uint32               height;
        WindowCallbackEvents callbacks;
    };

    // ウィンドウ生成情報
    struct WindowCreateInfo
    {
        std::string title  = {};
        uint32      width  = 1280;
        uint32      height = 720;
        bool        vsync  = true;
    };

    // ウィンドウインターフェース
    class Window : public Object
    {
        SL_CLASS(Window, Object)

    public:

        static Window* Create(const WindowCreateInfo& createInfo)
        {
            return createFunction(createInfo);
        }

        static Window* Get()
        {
            return instance;
        }

        static void RegisterCreateFunction(WindowCreateFunction createFunc)
        {
            createFunction = createFunc;
        }

    public:

        Window()
        {
            instance = this;
        }

        ~Window()
        {
            instance = nullptr;
        }

        // レンダリングコンテキスト
        virtual bool SetupRenderingContext() = 0;

        // ウィンドウメッセージ
        virtual void PumpMessage() = 0;

        // ウィンドウサイズ
        virtual glm::ivec2 GetSize()      const = 0;
        virtual glm::ivec2 GetWindowPos() const = 0;

        // ウィンドウ属性
        virtual void Maximize() = 0;
        virtual void Minimize() = 0;
        virtual void Restore()  = 0;
        virtual void Show()     = 0;
        virtual void Hide()     = 0;

        // タイトル
        virtual const char* GetTitle() const            = 0;
        virtual void        SetTitle(const char* title) = 0;

        // ウィンドウデータ
        virtual void*       GetWindowHandle() const = 0;
        virtual GLFWwindow* GetGLFWWindow() const   = 0; 
        virtual WindowData& GetWindowData()         = 0;
        virtual Surface*    GetSurface() const      = 0;

    protected:

        static inline Window*              instance;
        static inline WindowCreateFunction createFunction;
    };
}
