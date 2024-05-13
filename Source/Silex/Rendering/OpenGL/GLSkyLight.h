
#pragma once
#include "Rendering/SkyLight.h"


namespace Silex
{
    class GLSkyLight : public SkyLight
    {
        SL_DECLARE_CLASS(GLSkyLight, SkyLight)

    public:

        GLSkyLight(const std::string& filePath);
        ~GLSkyLight();

        uint32 GetCubeMap()       const override { return CubeMap;       }
        uint32 GetPrefilterMap()  const override { return PrefilterMap;  }
        uint32 GetBRDF()          const override { return BRDF;          }
        uint32 GetIrradianceMap() const override { return IrradianceMap; }

    private:

        uint32 CubeMap       = 0;
        uint32 PrefilterMap  = 0;
        uint32 BRDF          = 0;
        uint32 IrradianceMap = 0;

        uint32 CaptureFBO = 0;
        uint32 CaptureRBO = 0;
    };
}