//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 430 core

layout (location = 0) in vec3 aPos;       // 頂点: 座標
layout (location = 1) in vec3 aNormal;    // 頂点: ノーマル
layout (location = 2) in vec2 aTexCoords; // 頂点: UV

//layout (location = 3) in mat4 aTransform0; // インスタンス: トランスフォーム0
//layout (location = 4) in vec4 aTransform1; // インスタンス: トランスフォーム1
//layout (location = 5) in vec4 aTransform2; // インスタンス: トランスフォーム2

//layout (location = N) in vec3 aTangent;
//layout (location = N) in vec3 aBitangent;

out VS_OUT
{
    vec3     FragPos;
    vec2     TexCoords;
    vec3     Normal;
    flat int EntityID;
    flat int ShadingID;
} vso;


struct InstanceParameter
{
    mat4  transformMatrix;
    mat4  normalMatrix;
    ivec4 pixelID;
};

// インスタンスバッファ
layout (std430, binding = 0) buffer InstanceParameterStorage
{
    InstanceParameter parameter[];
};

uniform mat4 projection;
uniform mat4 view;
uniform int  instanceOffset;

void main()
{
    InstanceParameter instance = parameter[instanceOffset + gl_InstanceID];
    vec4 worldPos  = vec4(instance.transformMatrix * vec4(aPos, 1.0));

    vso.Normal     = mat3(instance.normalMatrix) * aNormal;
    vso.FragPos    = worldPos.xyz;
    vso.TexCoords  = aTexCoords;
    vso.EntityID   = instance.pixelID.x;
    vso.ShadingID  = instance.pixelID.y;

    gl_Position = projection * view * worldPos;
}

//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 430 core

layout (location = 0) out vec4  gAlbedo;   // アルベド
layout (location = 1) out vec4  gNormal;   // 法線 + ラフネス
layout (location = 2) out vec4  gPosition; // 座標 + 深度
layout (location = 3) out vec4  gEmission; // エミッション + メタリック
layout (location = 4) out ivec4 gID;       // マテリアルID + エンティティID（エディター用）

in VS_OUT
{
    vec3     FragPos;
    vec2     TexCoords;
    vec3     Normal;
    flat int EntityID;
    flat int ShadingID;
} fsi;


// マテリアル
uniform sampler2D albedoMap;

//layout (std140, binding = 1) uniform MaterialUBO
//{
//    vec3  albedo;
//    float metallic;
//    vec3  emission;
//    float roughness;
//    vec2  textureTiling;
//};

uniform vec3  albedo;
uniform float metallic;
uniform vec3  emission;
uniform float roughness;
uniform vec2  textureTiling;


void main()
{
    // アルベドマップ
    vec4 color = texture(albedoMap, fsi.TexCoords * textureTiling);

    // 透明は書き込まない (完全に0.0ではない場合があるので 0.01に)
    if (color.a < 0.01)
        discard;

    gAlbedo = vec4(color.rgb * albedo, color.a);

    // 座標 + 深度　TODO: 深度から座標求めるようにする
    gPosition.rgb = fsi.FragPos;
    gPosition.a   = gl_FragCoord.z;

    // ノーマル + ラフネス
    gNormal.rgb = normalize(fsi.Normal);
    gNormal.a   = roughness;

    // エミッション + メタリック
    gEmission.rgb = emission * 4.0; // 倍率はパラメータ化する
    gEmission.a   = metallic;

    // エンティティID + シェーディングID (PBR or BlinnPhong)
    gID.r = fsi.ShadingID;
    gID.g = fsi.EntityID;
}
