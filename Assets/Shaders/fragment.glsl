//=================================
// 頂点シェーダ
//=================================
struct FragmentOutput
{
    vec4 color;
    vec4 pos;
    vec3 normal;
    vec2 texcoord;
};

struct VertexOutput
{
    vec4 pos;
    vec3 normal;
    vec2 texcoord;
};

layout (location = 0) in  VertexOutput outv;
layout (location = 0) out vec4         outFragColor;

layout (set = 1, binding = 0) uniform sampler2D mainTexture;
layout (set = 2, binding = 0) uniform SceneInfo
{
    vec4 light;
    vec4 cameraPos;
};


FragmentOutput Fragment(VertexOutput vout);

void main()
{
    FragmentOutput fout = Fragment(outv);

    // https://docs.unity3d.com/ja/2019.4/Manual/SL-ShaderSemantics.html
    //vec4 screenPos = gl_FragCoord;
    
    //screenPos.xy  = floor(screenPos.xy * 0.25) * 0.5;
    //float checker = fract(screenPos.x + screenPos.y);

    //if (checker <= 0.0)
        //discard;

    // 環境光
    vec3 ambient = fout.color.rgb * 0.05;

    // 拡散反射光
    vec3 N        = fout.normal;
    vec3 L        = normalize(light.xyz);
    vec3 diffuse  = max(dot(L, N), 0.0) * fout.color.rgb;

    // 鏡面反射強度
    vec3 V        = normalize(cameraPos.xyz - fout.pos.xyz);
    vec3 H        = normalize(L + V);
    vec3 specular = vec3(pow(max(dot(N, H), 0.0), 256.0)) * 0.3;

    // 最終カラー
    vec4 color = vec4(vec3(ambient + diffuse + specular), fout.color.a);

    if (color.a <= 0.001)
        discard;

    outFragColor = color;
}
