#pragma once

#include "Core/Event.h"
#include "Rendering/RenderingCore.h"
#include <string>


struct GLFWwindow;

namespace Silex
{
    enum WindowMode
    {
        WINDOW_MODE_WINDOWED,
        WINDOW_MODE_MINIMIZED,
        WINDOW_MODE_MAXIMIZED,
        WINDOW_MODE_FULLSCREEN,
        WINDOW_MODE_EXCLUSIVE_FULLSCREEN,
    };

    enum VSyncMode 
    {
        VSYNC_MODE_DISABLED,
        VSYNC_MODE_MAILBOX,
        VSYNC_MODE_ENABLED,
        VSYNC_MODE_ADAPTIVE,
    };

    // VK_PRESENT_MODE_IMMEDIATE_KHR    = 0,
    // VK_PRESENT_MODE_MAILBOX_KHR      = 1,
    // VK_PRESENT_MODE_FIFO_KHR         = 2,
    // VK_PRESENT_MODE_FIFO_RELAXED_KHR = 3,


    class  Window;
    class  RenderingContext;
    class  RenderingDevice;
    struct WindowCreateInfo;

    using WindowCreateFunction = Window* (*)(const WindowCreateInfo&);

    SL_DECLARE_DELEGATE(WindowCloseDelegate,         void, WindowCloseEvent&)
    SL_DECLARE_DELEGATE(WindowResizeDelegate,        void, WindowResizeEvent&)
    SL_DECLARE_DELEGATE(KeyPressedDelegate,          void, KeyPressedEvent&)
    SL_DECLARE_DELEGATE(KeyReleasedDelegate,         void, KeyReleasedEvent&)
    SL_DECLARE_DELEGATE(MouseButtonPressedDelegate,  void, MouseButtonPressedEvent&)
    SL_DECLARE_DELEGATE(MouseButtonReleasedDelegate, void, MouseButtonReleasedEvent&)
    SL_DECLARE_DELEGATE(MouseScrollDelegate,         void, MouseScrollEvent&)
    SL_DECLARE_DELEGATE(MouseMoveDelegate,           void, MouseMoveEvent&)

    // ウィンドウデータ
    struct WindowData
    {
        std::string                 title;
        uint32                      width;
        uint32                      height;

        WindowCloseDelegate         windowCloseEvent;
        WindowResizeDelegate        windowResizeEvent;
        KeyPressedDelegate          keyPressedEvent;
        KeyReleasedDelegate         keyReleasedEvent;
        MouseButtonPressedDelegate  mouseButtonPressedEvent;
        MouseButtonReleasedDelegate mouseButtonReleasedEvent;
        MouseScrollDelegate         mouseScrollEvent;
        MouseMoveDelegate           mouseMoveEvent;
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

        Window()  { instance = this;    }
        ~Window() { instance = nullptr; }

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

    public:

        // WindowClose バインディング
        template <typename T, typename F>
        void BindWindowCloseEvent(T* instance, F memberFunc);
        template <typename T>
        void BindWindowCloseEvent(T&& func);

        // WindowResize バインディング
        template <typename T, typename F>
        void BindWindowResizeEvent(T* instance, F memberFunc);
        template <typename T>
        void BindWindowResizeEvent(T&& func);

        // KeyPressed バインディング
        template <typename T, typename F>
        void BindKeyPressedEvent(T* instance, F memberFunc);
        template <typename T>
        void BindKeyPressedEvent(T&& func);

        // keyReleased バインディング
        template <typename T, typename F>
        void BindKeyReleasedEvent(T* instance, F memberFunc);
        template <typename T>
        void BindKeyReleasedEvent(T&& func);

        // MouseButtonPressed バインディング
        template <typename T, typename F>
        void BindMouseButtonPressedEvent(T* instance, F memberFunc);
        template <typename T>
        void BindMouseButtonPressedEvent(T&& func);

        // MouseButtonReleased バインディング
        template <typename T, typename F>
        void BindMouseButtonReleasedEvent(T* instance, F memberFunc);
        template <typename T>
        void BindMouseButtonReleasedEvent(T&& func);

        // MouseScroll バインディング
        template <typename T, typename F>
        void BindMouseScrollEvent(T* instance, F memberFunc);
        template <typename T>
        void BindMouseScrollEvent(T&& func);

        // MouseMove バインディング
        template <typename T, typename F>
        void BindMouseMoveEvent(T* instance, F memberFunc);
        template <typename T>
        void BindMouseMoveEvent(T&& func);


    protected:

        WindowData windowData;

        static inline Window*              instance;
        static inline WindowCreateFunction createFunction;
    };




    template <typename T, typename F>
    void Window::BindWindowCloseEvent(T* instance, F memberFunc)
    {
        static_assert(Traits::IsSame<F, void(T::*)(WindowCloseEvent&)>());
        windowData.windowCloseEvent.Bind(SL_BIND_MEMBER_FN(instance, memberFunc));
    }

    template<typename T>
    inline void Window::BindWindowCloseEvent(T&& func)
    {
        static_assert(Traits::IsSame<T, void(*)(WindowCloseEvent&)>());
        windowData.windowCloseEvent.Bind(Traits::Forward<T>(func));
    }

    template <typename T, typename F>
    void Window::BindWindowResizeEvent(T* instance, F memberFunc)
    {
        static_assert(Traits::IsSame<F, void(T::*)(WindowResizeEvent&)>());
        windowData.windowResizeEvent.Bind(SL_BIND_MEMBER_FN(instance, memberFunc));
    }

    template<typename T>
    inline void Window::BindWindowResizeEvent(T&& func)
    {
        static_assert(Traits::IsSame<T, void(*)(WindowResizeEvent&)>());
        windowData.windowResizeEvent.Bind(Traits::Forward<T>(func));
    }

    template <typename T, typename F>
    void Window::BindKeyPressedEvent(T* instance, F memberFunc)
    {
        static_assert(Traits::IsSame<F, void(T::*)(KeyPressedEvent&)>());
        windowData.keyPressedEvent.Bind(SL_BIND_MEMBER_FN(instance, memberFunc));
    }

    template<typename T>
    inline void Window::BindKeyPressedEvent(T&& func)
    {
        static_assert(Traits::IsSame<T, void(*)(KeyPressedEvent&)>());
        windowData.keyPressedEvent.Bind(Traits::Forward<T>(func));
    }

    template <typename T, typename F>
    void Window::BindKeyReleasedEvent(T* instance, F memberFunc)
    {
        static_assert(Traits::IsSame<F, void(T::*)(KeyReleasedEvent&)>());
        windowData.keyReleasedEvent.Bind(SL_BIND_MEMBER_FN(instance, memberFunc));
    }

    template<typename T>
    inline void Window::BindKeyReleasedEvent(T&& func)
    {
        static_assert(Traits::IsSame<T, void(*)(KeyReleasedEvent&)>());
        windowData.keyReleasedEvent.Bind(Traits::Forward<T>(func));
    }

    template<typename T, typename F>
    inline void Window::BindMouseButtonPressedEvent(T* instance, F memberFunc)
    {
        static_assert(Traits::IsSame<F, void(T::*)(MouseButtonPressedEvent&)>());
        windowData.mouseButtonPressedEvent.Bind(SL_BIND_MEMBER_FN(instance, memberFunc));
    }

    template<typename T>
    inline void Window::BindMouseButtonPressedEvent(T&& func)
    {
        static_assert(Traits::IsSame<T, void(*)(MouseButtonPressedEvent&)>());
        windowData.mouseButtonPressedEvent.Bind(Traits::Forward<T>(func));
    }

    template<typename T, typename F>
    inline void Window::BindMouseButtonReleasedEvent(T* instance, F memberFunc)
    {
        static_assert(Traits::IsSame<F, void(T::*)(MouseButtonReleasedEvent&)>());
        windowData.mouseButtonReleasedEvent.Bind(SL_BIND_MEMBER_FN(instance, memberFunc));
    }

    template<typename T>
    inline void Window::BindMouseButtonReleasedEvent(T&& func)
    {
        static_assert(Traits::IsSame<T, void(*)(MouseButtonReleasedEvent&)>());
        windowData.mouseButtonReleasedEvent.Bind(Traits::Forward<T>(func));
    }

    template<typename T, typename F>
    inline void Window::BindMouseScrollEvent(T* instance, F memberFunc)
    {
        static_assert(Traits::IsSame<F, void(T::*)(MouseScrollEvent&)>());
        windowData.mouseScrollEvent.Bind(SL_BIND_MEMBER_FN(instance, memberFunc));
    }

    template<typename T>
    inline void Window::BindMouseScrollEvent(T&& func)
    {
        static_assert(Traits::IsSame<T, void(*)(MouseScrollEvent&)>());
        windowData.mouseScrollEvent.Bind(Traits::Forward<T>(func));
    }

    template<typename T, typename F>
    inline void Window::BindMouseMoveEvent(T* instance, F memberFunc)
    {
        static_assert(Traits::IsSame<F, void(T::*)(MouseMoveEvent&)>());
        windowData.mouseMoveEvent.Bind(SL_BIND_MEMBER_FN(instance, memberFunc));
    }

    template<typename T>
    inline void Window::BindMouseMoveEvent(T&& func)
    {
        static_assert(Traits::IsSame<T, void(*)(MouseMoveEvent&)>());
        windowData.mouseMoveEvent.Bind(Traits::Forward<T>(func));
    }
}
