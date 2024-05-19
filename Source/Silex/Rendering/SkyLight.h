
#pragma once

#include "Core/SharedPointer.h"
#include "Asset/Asset.h"


namespace Silex
{
    class SkyLight : public Asset
    {
        SL_CLASS(SkyLight, Asset)

    public:

        static Shared<SkyLight> Create(const std::string& filePath);

    public:

        virtual uint32 GetCubeMap()       const = 0;
        virtual uint32 GetPrefilterMap()  const = 0;
        virtual uint32 GetBRDF()          const = 0;
        virtual uint32 GetIrradianceMap() const = 0;
    };
}