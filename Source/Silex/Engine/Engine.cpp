
#include "PCH.h"

#include "Engine/Engine.h"
#include "Asset/Asset.h"
#include "Editor/SplashImage.h"
#include "Renderer/Renderer.h"
#include "OpenGL/GLEditorUI.h"


namespace Silex
{
    // グローバル Engine ポインター
    static Engine* engine = nullptr;


    int32 LaunchEngine()
    {
        SplashImage::Show();

        OS::Init();
        Logger::Init();
        MemoryTracker::Init();
        PoolAllocator::Init();
        Input::Init();

        SL_LOG_INFO("***** Launch Engine *****");

        engine = Memory::Allocate<Engine>();
        if (engine)
        {
            if (engine->Init())
            {
                engine->MainLoop();
                return EXIT_SUCCESS;
            }
        }

        return EXIT_FAILURE;
    }

    void ShutdownEngine()
    {
        engine->Shutdown();
        Memory::Deallocate(engine);

        SL_LOG_INFO("***** Shutdown Engine *****");

        Input::Shutdown();
        PoolAllocator::Shutdown();
        MemoryTracker::Shutdown();
        Logger::Shutdown();
        OS::Shutdown();
    }


    //=========================================
    // Engine
    //=========================================
    Engine* Engine::Get()
    {
        SL_ASSERT(engine != nullptr);
        return engine;
    }

    bool Engine::Init()
    {
        WindowCreateInfo info = {};
        info.title             = "Silex";
        info.width             = 1280;
        info.height            = 720;
        info.vsync             = false;
        info.roundWindowCorner = true;

        // ウィンドウ
        window = Window::Create(info);
        if (!window)
        {
            SL_LOG_FATAL("FAILED: Create Window");
            return false;
        }

        // ウィンドウコールバック登録
        WindowData& data = window->GetWindowData();
        data.windowCloseEvent.Bind(this,  &Engine::OnWindowClose);
        data.windowResizeEvent.Bind(this, &Engine::OnWindowResize);
        data.mouseMoveEvent.Bind(this,    &Engine::OnMouseMove);
        data.mouseScrollEvent.Bind(this,  &Engine::OnMouseScroll);

        // レンダラー
        Renderer::Get()->Init();

        // アセットマネージャー
        AssetManager::Get()->Init();

        // エディターUI (ImGui)
        editorUI = EditorUI::Create();
        editorUI->Init();

        // エディター
        editor = Memory::Allocate<Editor>();
        editor->Init();

        // スプラッシュスクリーン終了
        window->Show();
        SplashImage::Hide();

        return true;
    }

    void Engine::MainLoop()
    {
        while (isRunning)
        {
            CalcurateFrameTime();

            window->PumpMessage();

            if (!minimized)
            {
                Renderer::Get()->BeginFrame();
                editorUI->BeginFrame();

                editor->Update(deltaTime);
                editor->Render();

                editorUI->EndFrame();
                Renderer::Get()->EndFrame();

                Input::Flush();
            }

            PerformanceProfiler::Get().GetFrameData(&performanceData, true);
        }
    }

    void Engine::Shutdown()
    {
        editor->Shutdown();
        editorUI->Shutdown();

        AssetManager::Get()->Shutdown();
        Renderer::Get()->Shutdown();

        // ウィンドウイベントへのバインドを解除
        auto& data = window->GetWindowData();
        data.windowCloseEvent.Unbind();
        data.windowResizeEvent.Unbind();
        data.mouseMoveEvent.Unbind();
        data.mouseScrollEvent.Unbind();

        window->Destroy();
    }

    // OnWindowCloseイベント（Xボタン / Alt + F4）以外で、エンジンループを終了させる場合に使用
    void Engine::Close()
    {
        isRunning = false;
    }

    void Engine::OnWindowResize(WindowResizeEvent& e)
    {
        if (e.width == 0 || e.height == 0)
        {
            minimized = true;
            return;
        }

        minimized = false;
        Renderer::Get()->Resize(e.width, e.height);
    }

    void Engine::OnWindowClose(WindowCloseEvent& e)
    {
        isRunning = false;
    }

    void Engine::OnMouseMove(MouseMoveEvent& e)
    {
        editor->OnMouseMove(e);
    }

    void Engine::OnMouseScroll(MouseScrollEvent& e)
    {
        editor->OnMouseScroll(e);
    }


    void Engine::CalcurateFrameTime()
    {
        float time = OS::GetTime();
        deltaTime     = time - lastFrameTime;
        lastFrameTime = time;

        static float  secondLeft = 0.0f;
        static uint32 frame = 0;
        secondLeft += deltaTime;
        frame++;

        if (secondLeft >= 1.0f)
        {
            frameRate  = frame;
            frame      = 0;
            secondLeft = 0.0f;
        }
    }
}
