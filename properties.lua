--==================================================
-- ソリューション
--==================================================
workspace "Silex"

    architecture "x64"
    flags        "MultiProcessorCompile"
    startproject "Silex"

    configurations
    {
        "Debug",
        "Release",
    }

--==================================================
-- プロジェクト
--==================================================
project "Silex"

    location      "Source"
    language      "C++"
    cppdialect    "C++20"
    staticruntime "on"
    characterset  "Unicode"

    debugdir   "%{wks.location}"
    targetdir  "Binary/%{cfg.buildcfg}/"
    objdir     "Binary/%{cfg.buildcfg}/Intermediate"

    pchheader "PCH.h"
    pchsource "Source/Silex/Core/PCH/PCH.cpp"

    -- 追加ファイル
    files
    {
        -----------------------------------
        -- Resources
        -----------------------------------
        "Resources/Resource.h",
        "Resources/Resource.rc",

        -----------------------------------
        -- Source
        -----------------------------------
        "Source/%{prj.name}/**.c",
        "Source/%{prj.name}/**.h",
        "Source/%{prj.name}/**.cpp",
        "Source/%{prj.name}/**.hpp",

        -----------------------------------
        -- External
        -----------------------------------
        -- glad
        "Source/External/glad/src/**.c",

        -- GLFW
        "Source/External/glfw/src/context.c",
        "Source/External/glfw/src/egl_context.c",
        "Source/External/glfw/src/init.c",
        "Source/External/glfw/src/input.c",
        "Source/External/glfw/src/monitor.c",
        "Source/External/glfw/src/window.c",
        "Source/External/glfw/src/null_init.c",
        "Source/External/glfw/src/null_joystick.c",
        "Source/External/glfw/src/null_monitor.c",
        "Source/External/glfw/src/null_window.c",
        "Source/External/glfw/src/osmesa_context.c",
        "Source/External/glfw/src/platform.c",
        "Source/External/glfw/src/vulkan.c",
        "Source/External/glfw/src/wgl_context.c",

        "Source/External/glfw/src/win32_init.c",
        "Source/External/glfw/src/win32_joystick.c",
        "Source/External/glfw/src/win32_module.c",
        "Source/External/glfw/src/win32_monitor.c",
        "Source/External/glfw/src/win32_time.c",
        "Source/External/glfw/src/win32_thread.c",
        "Source/External/glfw/src/win32_window.c",

        -- YAML-cpp
        "Source/External/yaml-cpp/src/**.cpp",

        -- ImGui
        "Source/External/imgui/imgui.cpp",
        "Source/External/imgui/imgui_draw.cpp",
        "Source/External/imgui/imgui_widgets.cpp",
        "Source/External/imgui/imgui_tables.cpp",
        "Source/External/imgui/imgui_demo.cpp",
        "Source/External/imgui/backends/imgui_impl_glfw.cpp",
        "Source/External/imgui/backends/imgui_impl_opengl3.cpp",
        "Source/External/imgui/backends/imgui_impl_win32.cpp",
        "Source/External/imgui/backends/imgui_impl_vulkan.cpp",

        -- ImGuizmo
        "Source/External/imguizmo/ImGuizmo.cpp",
        
        -- Vulkan Memory Allocator
        "Source/External/vulkan/vk_mem_alloc.cpp"
    }

    includedirs
    {
        "Source/%{prj.name}/",
        "Source/%{prj.name}/Core/PCH",
        "Resources",
        "Source/External",
        "Source/External/vulkan/include",
        "Source/External/yaml-cpp/include",
        "Source/External/glad/include",
        "Source/External/glfw/include",
        "Source/External/glm",
        "Source/External/imgui",
        "Source/External/assimp/include",
    }

    links
    {
        "Source/External/vulkan/Lib/vulkan-1.lib",
    }

    defines
    {
        "YAML_CPP_STATIC_DEFINE",
    }

    buildoptions
    {
        "/wd4244",
        "/wd4267",
        "/wd4312",
        "/wd4305",
        "/wd4244",
        "/wd4291", -- 初期化により例外がスローされると、メモリが解放されません
        "/wd6011", -- NULLポインターの逆参照

        "/utf-8",  -- 文字列リテラルを utf8 として認識する (ImGuiが utf8 のみ対応しているため)

        --=======================================================================================================
        -- C++20 可変長マクロ __VA_OPT__() を サポートしていない MSVC バージョンのために
        -- /Zc:preprocessor オプションを有効にすることで回避する
        -- https://stackoverflow.com/questions/68484818/function-like-macros-with-c20-va-opt-error-in-manual-code
        -- https://learn.microsoft.com/ja-jp/cpp/preprocessor/preprocessor-experimental-overview?view=msvc-170#comma-elision-in-variadic-macros
        -- 
        -- 有効にしないと、展開部分のみならず、プロジェクト全体からエラーが出るので注意
        --=======================================================================================================
        "/Zc:preprocessor", -- C++20 __VA_OPT__ を MSVCがデフォルトで正しく展開しないため
    }

    postbuildcommands
    {
        '{COPY} "%{cfg.targetdir}/*.exe" "%{wks.location}"',
    }

    -- プリコンパイルヘッダー 無視リスト
    ----------------------------------------------------
    filter "files:Source/External/yaml-cpp/src/**.cpp" flags { "NoPCH" }
    filter "files:Source/External/imgui/**.cpp"        flags { "NoPCH" }
    filter "files:Source/External/imguizmo/**.cpp"     flags { "NoPCH" }
    filter "files:Source/External/glfw/src/**.c"       flags { "NoPCH" }
    filter "files:Source/External/glad/src/**.c"       flags { "NoPCH" }
    filter "files:Source/External/vulkan/**.cpp"       flags { "NoPCH" }

    -- Windows
    ----------------------------------------------------
    filter "system:windows"
    
        systemversion "latest"
        
        defines
        { 
            "SL_PLATFORM_WINDOWS",
            "NOMINMAX",
            "_CRT_SECURE_NO_WARNINGS",
            "_GLFW_WIN32",
        }

        links
        {
            "Dwmapi.lib",
            "Winmm.lib",
            "delayimp.lib", -- 遅延 DLL 読み込み
        }

    -- デバッグ
    ----------------------------------------------------
    filter "configurations:Debug"
    
        defines    "SL_DEBUG"
        symbols    "On"
        kind       "WindowedApp" --"ConsoleApp"
        targetname "%{prj.name}d"

        links
        {
            -- assimp
            "Source/External/assimp/lib/Debug/assimp-vc143-mtd.lib",

            -- spirv_cross
            "Source/External/vulkan/lib/spirv-cross-cored.lib",

            -- shaderc
            "Source/External/vulkan/lib/shaderc_sharedd.lib",
            "Source/External/vulkan/lib/shaderc_utild.lib",
        }

        linkoptions
        {
            -- 遅延 DLL 読み込み
            "/DELAYLOAD:assimp-vc143-mtd.dll",
            "/DELAYLOAD:shaderc_sharedd.dll",
        }

    -- リリース
    ----------------------------------------------------
    filter "configurations:Release"

        defines    "SL_RELEASE"
        optimize   "On"
        kind       "WindowedApp"
        targetname "%{prj.name}"

        links
        {
            -- assimp
            "Source/External/assimp/lib/Release/assimp-vc143-mt.lib",

            -- spirv_cross
            "Source/External/vulkan/lib/spirv-cross-core.lib",

            -- shaderc
            "Source/External/vulkan/lib/shaderc_shared.lib",
            "Source/External/vulkan/lib/shaderc_util.lib",
        }

        linkoptions
        { 
            -- 遅延 DLL 読み込み
            "/DELAYLOAD:assimp-vc143-mt.dll",
            "/DELAYLOAD:shaderc_shared.dll",
        }
