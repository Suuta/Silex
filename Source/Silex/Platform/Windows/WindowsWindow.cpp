
#include "PCH.h"

#include "Core/Timer.h"
#include "Core/OS.h"
#include "Asset/TextureReader.h"
#include "Platform/Windows/WindowsWindow.h"
#include "Rendering/RenderingDevice.h"
#include "Rendering/Vulkan/Windows/WindowsVulkanContext.h"

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
    
    //===========================================================
    // TODO: エラーハンドリング出来るようにする（戻り値が返せる方式にする）
    //===========================================================
    WindowsWindow::WindowsWindow(const WindowCreateInfo& createInfo)
    {
        SL_LOG_TRACE("WindowsWindow::Create");

        instanceHandle = ::GetModuleHandleW(nullptr);

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
        window       = glfwCreateWindow((int32)createInfo.width, (int32)createInfo.height, createInfo.title.c_str(), nullptr, nullptr);
        windowHandle = glfwGetWin32Window(window);

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

    WindowsWindow::~WindowsWindow()
    {
#if NEW_RENDERER
        Memory::Deallocate(renderingDevice);
        renderingContext->DestroySurface(renderingSurface);
        Memory::Deallocate(renderingContext);
#endif

        Hide();
        glfwDestroyWindow(window);
    }

    bool WindowsWindow::SetupRenderingContext()
    {
        // レンダリングコンテキスト生成
        renderingContext = RenderingContext::Create();
        
        bool result = renderingContext->Initialize(true);
        SL_CHECK_RETURN(!result, false);

        // サーフェース生成
        WindowsVulkanSurfaceData surfaceData;
        surfaceData.instance = instanceHandle;
        surfaceData.window   = windowHandle;

        renderingSurface = renderingContext->CreateSurface(&surfaceData);
        SL_CHECK_RETURN(!renderingSurface, false);

        // レンダリングデバイス生成
        renderingDevice = Memory::Allocate<RenderingDevice>();
        result = renderingDevice->Initialize(renderingContext);
        SL_CHECK_RETURN(!result, false);

        return true;
    }

    glm::ivec2 WindowsWindow::GetSize() const
    {
        return { windowData.width, windowData.height };
    }

    glm::ivec2 WindowsWindow::GetWindowPos() const
    {
        int x, y;
        glfwGetWindowPos(window, &x, &y);
        return { x, y };
    }

    void WindowsWindow::PumpMessage()
    {
        glfwPollEvents();
    }

    void WindowsWindow::Maximize()
    {
        glfwMaximizeWindow(window);
    }

    void WindowsWindow::Minimize()
    {
        glfwIconifyWindow(window);
    }

    void WindowsWindow::Restore()
    {
        glfwRestoreWindow(window);
    }

    void WindowsWindow::Show()
    {
        glfwShowWindow(window);
    }

    void WindowsWindow::Hide()
    {
        glfwHideWindow(window);
    }

    const char* WindowsWindow::GetTitle() const
    {
        return windowData.title.c_str();
    }

    void WindowsWindow::SetTitle(const char* title)
    {
        windowData.title = title;
        glfwSetWindowTitle(window, windowData.title.c_str());
    }

    GLFWwindow* WindowsWindow::GetGLFWWindow() const
    {
        return window;
    }

    void* WindowsWindow::GetWindowHandle() const
    {
        return glfwGetWin32Window(window);
    }

    WindowData& WindowsWindow::GetWindowData()
    {
        return windowData;
    }

    Surface* WindowsWindow::GetSurface() const
    {
        return renderingSurface;
    }


    namespace Callback
    {
        static void OnWindowClose(GLFWwindow* window)
        {
            WindowData* data = ((WindowData*)glfwGetWindowUserPointer(window));

            WindowCloseEvent event;
            data->callbacks.windowCloseEvent.Execute(event);
        }

        static void OnWindowSize(GLFWwindow* window, int width, int height)
        {
            WindowData* data = ((WindowData*)glfwGetWindowUserPointer(window));

            WindowResizeEvent event((uint32)width, (uint32)height);
            data->width  = width;
            data->height = height;
            data->callbacks.windowResizeEvent.Execute(event);
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
                    data->callbacks.keyPressedEvent.Execute(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    Input::ProcessKey((Keys)key, false);

                    KeyReleasedEvent event((Keys)key);
                    data->callbacks.keyReleasedEvent.Execute(event);
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
                    data->callbacks.mouseButtonPressedEvent.Execute(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    Input::ProcessButton((Mouse)button, false);

                    MouseButtonReleasedEvent event(button);
                    data->callbacks.mouseButtonReleasedEvent.Execute(event);
                    break;
                }

                default: break;
            }
        }

        static void OnScroll(GLFWwindow* window, double xOffset, double yOffset)
        {
            auto& data = *((WindowData*)glfwGetWindowUserPointer(window));

            MouseScrollEvent event((float)xOffset, (float)yOffset);
            data.callbacks.mouseScrollEvent.Execute(event);
        }

        static void OnCursorPos(GLFWwindow* window, double x, double y)
        {
            Input::ProcessMove((int16)x, (int16)y);

            auto& data = *((WindowData*)glfwGetWindowUserPointer(window));
            MouseMoveEvent event((float)x, (float)y);
            data.callbacks.mouseMoveEvent.Execute(event);
        }
    }
}
