
#include "PCH.h"

#include "Core/Window.h"
#include "Core/Timer.h"
#include "Core/OS.h"
#include "Asset/TextureReader.h"

#ifndef GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif

#include <glfw/glfw3.h>
#include <GLFW/glfw3native.h>


namespace Silex
{
    namespace Callback
    {
        static void OnWindowSize(GLFWwindow* window, int32 width, int32 height);
        static void OnWindowClose(GLFWwindow* window);
        static void OnKey(GLFWwindow* window, int32 key, int32 scancode, int32 action, int32 mods);
        static void OnMouseButton(GLFWwindow* window, int32 button, int32 action, int32 mods);
        static void OnScroll(GLFWwindow* window, double xOffset, double yOffset);
        static void OnCursorPos(GLFWwindow* window, double x, double y);
    }

    Window* Window::Create(const WindowCreateInfo& createInfo)
    {
        return createFunction(createInfo);
    }

    Window* Window::Get()
    {
        return instance;
    }

    void Window::RegisterCreateFunction(WindowCreateFunction createFunc)
    {
        if (!createFunc)
        {
            SL_LOG_ERROR("RegisterWindowCreateFunction: param [createFunc] is null");
        }

        createFunction = createFunc;
    }

    Window::Window(const WindowCreateInfo& createInfo)
    {
        SL_LOG_TRACE("Window::Create");

        instance = this;


#if SL_PLATFORM_OPENGL
        glfwWindowHint(GLFW_VISIBLE, FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#else
        glfwWindowHint(GLFW_VISIBLE, FALSE);
        glfwWindowHint(GLFW_CLIENT_API , GLFW_NO_API); // Vulkan / D3D は GLFW_NO_API を指定する
#endif

        // glfwWindow 生成
        window = glfwCreateWindow((int32)createInfo.width, (int32)createInfo.height, createInfo.title.c_str(), nullptr, nullptr);

        // リサイズはレンダラー生成後はレンダラーに依存するので、初期化前にリサイズしておく
        glfwMaximizeWindow(window);

        // ウィンドウコンテキストに登録するデータを設定 (※ glfwGetWindowUserPointerで取り出し)
        glfwSetWindowUserPointer(window, &windowData);

        glfwSetWindowSizeCallback(window,  Callback::OnWindowSize);  // リサイズ
        glfwSetWindowCloseCallback(window, Callback::OnWindowClose); // ウィンドウクローズ
        glfwSetKeyCallback(window,         Callback::OnKey);         // キー入力
        glfwSetMouseButtonCallback(window, Callback::OnMouseButton); // マウスボタン入力
        glfwSetScrollCallback(window,      Callback::OnScroll);      // マウススクロール
        glfwSetCursorPosCallback(window,   Callback::OnCursorPos);   // マウス位置

        // ウィンドウサイズ同期（既に最大化で変化しているので、同期が必要）
        int32 w, h;
        glfwGetWindowSize(window, &w, &h);
        windowData.width  = w;
        windowData.height = h;
        windowData.title  = createInfo.title;

        // アイコン設定
        GLFWimage icon;
        TextureReader reader;
        icon.pixels = reader.Read("Assets/Editor/Logo.png", false);
        icon.width  = reader.Data.Width;
        icon.height = reader.Data.Height;
        glfwSetWindowIcon(window, 1, &icon);

        // ウィンドウコンテキストを更新（指定されたウィンドウのコンテキストを、このスレッドに対して生成する）
        // 1度に1つのスレッド上でのみ更新することが可能で、スレッドごとに1つだけ持つことが出来る
        glfwMakeContextCurrent(window);

        // NOTE: 垂直同期の設定は、glfwMakeContextCurrent呼び出し後に行う必要がある(GLコマンドだからか？)
        glfwSwapInterval(createInfo.vsync);
    }

    Window::~Window()
    {
        SL_LOG_TRACE("Window::Destroy");
        Hide();

        // glfwTerminateで呼び出されるが、明示的に呼び出し
        glfwDestroyWindow(window);
    }

    glm::ivec2 Window::GetSize() const
    {
        return { windowData.width, windowData.height };
    }

    glm::ivec2 Window::GetWindowPos() const
    {
        int x, y;
        glfwGetWindowPos(window, &x, &y);
        return { x, y };
    }

    void Window::PumpMessage()
    {
        glfwPollEvents();
    }

    void Window::Maximize()
    {
        glfwMaximizeWindow(window);
    }

    void Window::Minimize()
    {
        glfwIconifyWindow(window);
    }

    void Window::Restore()
    {
        glfwRestoreWindow(window);
    }

    void Window::Show()
    {
        glfwShowWindow(window);
    }

    void Window::Hide()
    {
        glfwHideWindow(window);
    }

    const char* Window::GetTitle() const
    {
        return windowData.title.c_str();
    }

    void Window::SetTitle(const char* title)
    {
        windowData.title = title;
        glfwSetWindowTitle(window, windowData.title.c_str());
    }

    GLFWwindow* Window::GetGLFWWindow() const
    {
        return window;
    }

    HWND Window::GetWindowHandle() const
    {
        return glfwGetWin32Window(window);
    }

    WindowData& Window::GetWindowData()
    {
        return windowData;
    }

    namespace Callback
    {
        static void OnWindowClose(GLFWwindow* window)
        {
            WindowData* data = ((WindowData*)glfwGetWindowUserPointer(window));

            WindowCloseEvent event;
            data->windowCloseEvent.Execute(event);
        }

        static void OnWindowSize(GLFWwindow* window, int width, int height)
        {
            WindowData* data = ((WindowData*)glfwGetWindowUserPointer(window));

            WindowResizeEvent event((uint32)width, (uint32)height);
            data->width  = width;
            data->height = height;
            data->windowResizeEvent.Execute(event);
        }

        static void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            WindowData* data = ((WindowData*)glfwGetWindowUserPointer(window));

            switch (action)
            {
                case GLFW_PRESS:
                {
                    Input::ProcessKey((Keys)key, true);

                    KeyPressedEvent event((Keys)key);
                    data->keyPressedEvent.Execute(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    Input::ProcessKey((Keys)key, false);

                    KeyReleasedEvent event((Keys)key);
                    data->keyReleasedEvent.Execute(event);
                    break;
                }

                default: break;
            }
        }

        static void OnMouseButton(GLFWwindow* window, int button, int action, int mods)
        {
            WindowData* data = ((WindowData*)glfwGetWindowUserPointer(window));

            switch (action)
            {
                case GLFW_PRESS:
                {
                    Input::ProcessButton((Mouse)button, true);

                    MouseButtonPressedEvent event(button);
                    data->mouseButtonPressedEvent.Execute(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    Input::ProcessButton((Mouse)button, false);

                    MouseButtonReleasedEvent event(button);
                    data->mouseButtonReleasedEvent.Execute(event);
                    break;
                }

                default: break;
            }
        }

        static void OnScroll(GLFWwindow* window, double xOffset, double yOffset)
        {
            auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

            MouseScrollEvent event((float)xOffset, (float)yOffset);
            data.mouseScrollEvent.Execute(event);
        }

        static void OnCursorPos(GLFWwindow* window, double x, double y)
        {
            Input::ProcessMove((int16)x, (int16)y);

            auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
            MouseMoveEvent event((float)x, (float)y);
            data.mouseMoveEvent.Execute(event);
        }
    }
}
