
#pragma once

#include "Core/Core.h"


namespace Silex
{
    struct TextureSourceData
    {
        uint32 Width    = 0;
        uint32 Height   = 0;
        uint32 Channels = 0;
        byte*  Pixels   = nullptr;
        bool   IsHDR    = false;
    };

    // 読み込んだテクスチャデータは、変数のスコープ内のみ有効
    // 自動で解放されるが、Unloadで明示的に解放もできる
    struct TextureReader
    {
        TextureReader() = default;
        ~TextureReader();

        byte* Read(const char* path, bool flipOnRead = false);
        void Unload(void* data);

        TextureSourceData Data;
    };
}

