
#include "PCH.h"
#include "Asset/TextureReader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


namespace Silex
{
    static void* _Read(const char* path, bool hdr, bool flipOnRead, TextureReader* reader)
    {
        bool isHDR = stbi_is_hdr(path);
        if (isHDR != hdr)
        {
            SL_LOG_ERROR("{} は破損しているか、期待しているファイル形式ではありません", path);
            return nullptr;
        }

        stbi_set_flip_vertically_on_load(flipOnRead);

        reader->data.pixels = isHDR?
            (void*)stbi_loadf(path, &reader->data.width, &reader->data.height, &reader->data.channels, 4):
            (void*)stbi_load(path,  &reader->data.width, &reader->data.height, &reader->data.channels, 4);

        if (!reader->data.pixels)
        {
            SL_LOG_ERROR("{} が見つからなかったか、データが破損しています", path);
        }

        reader->data.byteSize = reader->data.width * reader->data.height * reader->data.channels * (isHDR? sizeof(float) : sizeof(byte));
        return reader->data.pixels;
    }


    TextureReader::TextureReader()
    {
    }

    TextureReader::~TextureReader()
    {
        Unload(data.pixels);
    }

    byte* TextureReader::Read8bit(const char* path, bool flipOnRead)
    {
        return (byte*)_Read(path, false, flipOnRead, this);
    }

    float* TextureReader::ReadHDR(const char* path, bool flipOnRead)
    {
        return (float*)_Read(path, true, flipOnRead, this);
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