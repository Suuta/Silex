
#pragma once
#include "Rendering/RenderingCore.h"


namespace Silex
{
    class DescriptorSet
    {
    public:

        DescriptorSetHandle* Get(uint32 index)
        {
            return descriptorSets[index];
        }

    private:

        // フレーム毎
        std::vector<DescriptorSetHandle*> descriptorSets;
    };

}
