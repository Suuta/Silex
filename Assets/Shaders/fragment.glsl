//=================================
// 頂点シェーダ
//=================================
struct FragmentOutput
{
    vec4 color;
};

struct VertexOutput
{
    vec4 pos;
    vec3 normal;
    vec2 uv;
    vec4 color;
};

layout (location = 0) in  VertexOutput outv;
layout (location = 0) out vec4         outFragColor;


FragmentOutput Fragment(VertexOutput vout);

void main()
{
    FragmentOutput fout = Fragment(outv);

    // https://docs.unity3d.com/ja/2019.4/Manual/SL-ShaderSemantics.html
    vec4 screenPos = gl_FragCoord;
    
    screenPos.xy  = floor(screenPos.xy * 0.25) * 0.5;
    float checker = fract(screenPos.x + screenPos.y);

    if (checker <= 0.0)
        discard;

    outFragColor = fout.color;
}
