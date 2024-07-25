
#include "Rendering/RenderingCore.h"


namespace Silex
{
    namespace RenderingUtility
    {
        // 深度値フォーマットチェック
        bool IsDepthFormat(RenderingFormat format);

        // ミップマップレベル取得
        std::vector<Extent> CalculateMipmap(uint32 width, uint32 height);
    }
}