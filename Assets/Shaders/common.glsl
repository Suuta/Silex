
#extension GL_EXT_buffer_reference : enable

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
