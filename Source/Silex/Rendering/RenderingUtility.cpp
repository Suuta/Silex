
#include "PCH.h"
#include "Rendering/RenderingUtility.h"


namespace Silex
{
    namespace RenderingUtility
    {
        bool IsDepthFormat(RenderingFormat format)
        {
            return (format == RENDERING_FORMAT_D32_SFLOAT_S8_UINT)
                or (format == RENDERING_FORMAT_D32_SFLOAT)
                or (format == RENDERING_FORMAT_D24_UNORM_S8_UINT)
                or (format == RENDERING_FORMAT_D16_UNORM_S8_UINT)
                or (format == RENDERING_FORMAT_D16_UNORM);
        }

        std::vector<Extent> CalculateMipmap(uint32 width, uint32 height)
        {
            // ミップレベル0(ミップマップなし) を含めると + 1
            uint32 mipCount = uint32(std::floor(std::log2(std::max(width, height)))) + 1;

            std::vector<Extent> resolutions(mipCount);
            Extent res = {width, height};

            for (uint32 i = 0; i < mipCount; i++)
            {
                resolutions[i] = { res.width, res.height };
                res = { resolutions[i].width / 2, resolutions[i].height / 2 };
            }

            return resolutions;
        }
    }
}