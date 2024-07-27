
#pragma once

#include "Core/Core.h"


namespace Silex
{
    struct TextureSourceData
    {
        int32 width    = 0;
        int32 height   = 0;
        int32 channels = 0;

        void* pixels  = nullptr;

        bool isHDR = false;
    };

    // 読み込んだテクスチャデータは、変数のスコープ内のみ有効
    // 自動で解放されるが、Unloadで明示的に解放もできる
    struct TextureReader
    {
        TextureReader();
        ~TextureReader();

        byte*  Read8bit(const char* path, bool flipOnRead = false);
        float* ReadHDR(const char* path, bool flipOnRead = false);

        void Unload(void* data);

        TextureSourceData data;
    };
}

