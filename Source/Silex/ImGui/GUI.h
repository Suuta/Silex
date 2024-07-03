
#pragma once

#include "Core/SharedPointer.h"


namespace Silex
{
    using GUICreateFunction = class GUI* (*)();


    class GUI : public Object
    {
        SL_CLASS(GUI, Object)

    public:

        // 生成
        static GUI* Create()
        {
            return createFunction();
        }

        // 生成用関数の登録
        static void ResisterCreateFunction(GUICreateFunction func)
        {
            createFunction = func;
        }

    public:

        virtual void Init();
        virtual void Shutdown();
        virtual void Render();

        virtual void BeginFrame();
        virtual void EndFrame();

    private:

        static inline GUICreateFunction createFunction = nullptr;
    };
}
