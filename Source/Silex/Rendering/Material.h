
#pragma once
#include "Asset/Asset.h"
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"


namespace Silex
{
    enum ShadingModelType
    {
        BlinnPhong = 0,
        BRDF       = 1,
    };

    class Material : public Asset
    {
        SL_DECLARE_CLASS(Material, Asset)

    public:

        glm::vec3         Albedo        = { 1.0f, 1.0f, 1.0f };
        Shared<Texture2D> AlbedoMap     = nullptr;
        glm::vec3         Emission      = { 0.0f, 0.0f, 0.0f };
        float             Roughness     = 1.0f;
        float             Metallic      = 1.0f;
        glm::vec2         TextureTiling = { 1.0f, 1.0f };
        ShadingModelType  ShadingModel  = BRDF;
    };
}
