
#include "PCH.h"
#include "Asset/TextureReader.h"
#include <stb/stb_image.h>


namespace Silex
{
    TextureReader::~TextureReader()
    {
        Unload(Data.Pixels);
    }

    byte* TextureReader::Read(const char* path, bool flipOnRead)
    {
        stbi_set_flip_vertically_on_load(flipOnRead);

        int32 width, height, channels;
        byte* pixels = nullptr;

        //----------------------------------------------------------------------
        // NOTE: hdr形式の stbi_loadf() が float* を返すが、uint8* にキャストしている
        // 動作はしているが、おそらく正しい値になっていないので注意
        //----------------------------------------------------------------------
        if (stbi_is_hdr(path))
        {
            pixels     = reinterpret_cast<byte*>(stbi_loadf(path, &width, &height, &channels, 0));
            Data.IsHDR = true;
        }
        else
        {
            pixels     = stbi_load(path, &width, &height, &channels, 0);
            Data.IsHDR = false;
        }

        if (pixels)
        {
            Data.Channels = channels;
            Data.Width    = width;
            Data.Height   = height;
            Data.Pixels   = pixels;
        }
        else
        {
            SL_LOG_ERROR("{} が見つかりません", path);
        }

        return pixels;
    }

    void TextureReader::Unload(void* data)
    {
        if (data)
        {
            stbi_image_free(data);
            data = nullptr;
        }
    }
}