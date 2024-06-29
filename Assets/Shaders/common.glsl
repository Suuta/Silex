
// SET(0) シーンデータ
layout(set = 0, binding = 0) uniform  SceneData
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
	vec4 ambientColor;
	vec4 sunlightDirection;
	vec4 sunlightColor;
} sceneData;

// SET(1) マテリアル
layout(set = 1, binding = 0) uniform GLTFMaterialData
{
	vec4 colorFactors;
	vec4 metal_rough_factors;
} materialData;


layout(set = 1, binding = 1) uniform sampler2D colorTex;
layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;
