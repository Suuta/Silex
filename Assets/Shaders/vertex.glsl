//=================================
// 頂点シェーダ
//=================================
struct VertexInput
{
    vec3 pos;
    vec3 normal;
    vec2 texcoord;
};

struct VertexOutput
{
    vec4 pos;
    vec3 normal;
    vec2 texcoord;
};

//=================================
// IN / OUT
//=================================
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;

layout (location = 0) out VertexOutput outVertex;

//=================================
// ユニフォーム
//=================================
layout (set = 0, binding = 0) uniform Transform
{
    mat4 world;
    mat4 view;
    mat4 projection;
};


VertexOutput Vertex(VertexInput vin);

void main()
{
    VertexInput vin;
    vin.pos      = inPos;
    vin.normal   = inNormal;
    vin.texcoord = inTexCoord;

    VertexOutput vout = Vertex(vin);

    gl_Position = vout.pos;
    outVertex.pos      = vout.pos;
    outVertex.normal   = vout.normal;
    outVertex.texcoord = vout.texcoord;
}
