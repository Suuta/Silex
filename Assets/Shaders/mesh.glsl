
#pragma VERTEX

#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference     : require


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


layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;

struct Vertex
{
	vec3  position;
	float uv_x;
	vec3  normal;
	float uv_y;
	vec4  color;
};

// バッファアドレス使用
layout(buffer_reference, std430) readonly buffer VertexBuffer
{
	Vertex vertices[];
};

// プッシュ定数
layout(push_constant) uniform PushConstants
{
	mat4         render_matrix;
	VertexBuffer vertexBuffer;
} constants;

void main()
{
	// デバイスアドレスから頂点データ取得
	Vertex v = constants.vertexBuffer.vertices[gl_VertexIndex];

	outNormal = (constants.render_matrix * vec4(v.normal, 0.0)).xyz;
	outColor  = v.color.xyz * materialData.colorFactors.xyz;
	outUV.x   = v.uv_x;
	outUV.y   = v.uv_y;

	gl_Position = sceneData.viewproj * constants.render_matrix * vec4(v.position, 1.0);
}





#pragma FRAGMENT
#version 450
#extension GL_GOOGLE_include_directive : require

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

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;


void main() 
{
	float lightValue = max(dot(inNormal, sceneData.sunlightDirection.xyz), 0.1);
	vec3 color       = inColor * texture(colorTex,inUV).xyz;
	vec3 ambient     = color *  sceneData.ambientColor.xyz;

	outFragColor = vec4(color * lightValue *  sceneData.sunlightColor.w + ambient ,1.0);
}