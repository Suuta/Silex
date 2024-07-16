//=================================
// 頂点シェーダ
//=================================
struct VertexInput
{
    vec3 pos;
    vec3 normal;
    vec2 uv;
    vec4 color;
};

struct VertexOutput
{
    vec4 pos;
    vec3 normal;
    vec2 uv;
    vec4 color;
};

//=================================
// IN / OUT
//=================================
layout (location = 0) in vec3 inPos;
layout (location = 1) in vec4 inColor;

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
    vin.pos   = inPos;
    vin.color = inColor;

    VertexOutput vout = Vertex(vin);

    gl_Position     = vout.pos;
    outVertex.color = vout.color;
}
