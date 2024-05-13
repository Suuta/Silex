
#include "PCH.h"

#include "Core/Engine.h"
#include "Asset/Asset.h"
#include "Editor/SplashImage.h"
#include "Rendering/Renderer.h"
#include "Rendering/OpenGL/GLEditorUI.h"


namespace Silex
{
    static Engine* engine = nullptr;
    static Window* window = nullptr;


    Result LaunchEngine()
    {
        // コア機能初期化
        OS::Get()->Initialize();

        // スプラッシュイメージ表示
        SplashImage::Show();

        // コア機能初期化
        Logger::Initialize();
        Memory::Initialize();
        Input::Initialize();

        SL_LOG_INFO("***** Launch Engine *****");

        // エンジン初期化
        engine = Memory::Allocate<Engine>();
        Result result = engine->Initialize();

        // スプラッシュイメージ非表示
        SplashImage::Hide();

        return result;
    }

    void ShutdownEngine()
    {
        engine->Finalize();
        Memory::Deallocate(engine);

        SL_LOG_INFO("***** Shutdown Engine *****");

        Input::Finalize();
        Memory::Finalize();
        Logger::Finalize();

        OS::Get()->Finalize();
    }




    //=========================================
    // Engine
    //=========================================
    Engine* Engine::Get()
    {
        return engine;
    }

    Result Engine::Initialize()
    {
#if SL_DEBUG
        assimpDLL = LoadLibraryW(L"Resources/DLL/assimp-vc143-mtd.dll");
#else
        assimpDLL = LoadLibraryW(L"Resources/DLL/assimp-vc143-mt.dll");
#endif

        WindowCreateInfo info = {};
        info.title            = "Silex";
        info.width            = 1280;
        info.height           = 720;
        info.vsync            = false;

        // ウィンドウ
        window = Window::Create(info);
        if (!window)
        {
            SL_LOG_FATAL("FAILED: Create Window");
            return Result::FAIL;
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

        // ウィンドウ表示
        window->Show();

        return Result::OK;
    }

    bool Engine::MainLoop()
    {
        CalcurateFrameTime();

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

        // メインループ抜け出し確認
        return isRunning;
    }

    void Engine::Finalize()
    {
        editor->Shutdown();
        editorUI->Shutdown();

        AssetManager::Get()->Shutdown();
        Renderer::Get()->Shutdown();

        // ウィンドウイベントへのバインドを解除
        WindowData& data = window->GetWindowData();
        data.windowCloseEvent.Unbind();
        data.windowResizeEvent.Unbind();
        data.mouseMoveEvent.Unbind();
        data.mouseScrollEvent.Unbind();

        // ウィンドウ破棄
        Memory::Deallocate(window);

        FreeLibrary(assimpDLL);
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
        uint64 time = OS::Get()->GetTickCount();
        deltaTime     = (double)(time - lastFrameTime) / 1'000'000;
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


