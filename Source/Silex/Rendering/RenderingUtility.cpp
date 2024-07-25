
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
    }
}