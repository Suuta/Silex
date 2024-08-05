//=================================
// 頂点シェーダ
//=================================
#pragma VERTEX
#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;

layout (location = 0) out vec3 outPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outTexCoord;

//=================================
// ユニフォーム
//=================================
layout (set = 0, binding = 0) uniform Transform
{
    mat4 world;
    mat4 view;
    mat4 projection;
};

void main()
{
    vec4 worldPos = world * vec4(inPos, 1.0);

    outPos      = worldPos.xyz;
    outNormal   = inNormal;
    outTexCoord = inTexCoord;

    gl_Position = projection * view * worldPos;
}


//=================================
// フラグメントシェーダ
//=================================
#pragma FRAGMENT
#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout (location = 0) out vec4 outFragColor;

layout (set = 1, binding = 0) uniform sampler2D mainTexture;
layout (set = 2, binding = 0) uniform SceneInfo
{
    vec4 light;
    vec4 cameraPos;
};

void main()
{
    // ノーマル書き込み用変換 [-1~1 to 0~1]
    vec3 norm = (inNormal * 0.5) + vec3(0.5);

    // https://docs.unity3d.com/ja/2019.4/Manual/SL-ShaderSemantics.html
    //vec4 screenPos = gl_FragCoord;
    //screenPos.xy  = floor(screenPos.xy * 0.25) * 0.5;
    //float checker = fract(screenPos.x + screenPos.y);

    // アルベド
    vec4 albedo = texture(mainTexture, inTexCoord);

    // ライトカラー
    //vec3 lightcolor = vec3(1.0);

    // 環境光
    vec3 ambient = albedo.rgb * 0.01;

    // 拡散反射光
    vec3 N        = normalize(inNormal);
    vec3 L        = normalize(light.xyz);
    vec3 diffuse  = max(dot(L, N), 0.0) * albedo.rgb;

    // 鏡面反射強度
    vec3 V        = normalize(cameraPos.xyz - inPos);
    vec3 H        = normalize(L + V);
    vec3 specular = vec3(1.0) * pow(max(dot(N, H), 0.0), 64.0);

    // 最終カラー
    vec4 color = vec4(ambient + diffuse + specular, albedo.a);

    if (color.a <= 0.001)
        discard;

    outFragColor = color;
}
