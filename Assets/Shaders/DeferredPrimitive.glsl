//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inTangent;
layout (location = 4) in vec3 inBitangent;

layout (location = 0) out vec3     outNormal;
layout (location = 1) out vec2     outTexCoord;
layout (location = 2) out flat int outID;
layout (location = 3) out mat3     outNormalMatrix;

//------------------------------------------------------------------
// ユニフォーム
//------------------------------------------------------------------
layout (set = 0, binding = 0) uniform Transform
{
    mat4 world;
    mat4 view;
    mat4 projection;
};


void main()
{
    vec4 worldPos = world * vec4(inPos, 1.0);

    outNormalMatrix = mat3(transpose(inverse(world)));
    outNormal       = outNormalMatrix * inNormal;
    outTexCoord     = inTexCoord;
    outID           = 1;

    gl_Position = projection * view * worldPos;
}

//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 450

layout (location = 0) in vec3     inNormal;
layout (location = 1) in vec2     inTexCoord;
layout (location = 2) in flat int inID;
layout (location = 3) in mat3     inNormalMatrix;

layout (location = 0) out vec4 outAlbedo;   // アルベド + ラフネス
layout (location = 1) out vec4 outNormal;   // 法線    + ラフネス
layout (location = 2) out vec3 outEmission; // エミッション
layout (location = 3) out int  outID;       // エンティティID + マテリアルID

//               |     R     |     G     |     B     |     A     |
// [0]: RGBA8U   |               Color               | Roughness |
// [1]: RGBA8U   |               Normal              | Metallic  |
// [2]: RG11B10F |              Emission             |
// [3]: RG32I    |   Entity  |

//------------------------------------------------------------------
// マテリアル
//------------------------------------------------------------------
layout (set = 1, binding = 0) uniform Material
{
    vec3  albedo;
    float metallic;
    vec3  emission;
    float roughness;
    vec2  textureTiling;
} u_material;

//------------------------------------------------------------------
// テクスチャ
//------------------------------------------------------------------
  layout (set = 1, binding = 1) uniform sampler2D u_albedo;
//layout (set = 1, binding = 2) uniform sampler2D u_normal;
//layout (set = 1, binding = 3) uniform sampler2D u_emission;


vec3 Ganmma(vec3 color, float ganmma)
{
    return pow(color, vec3(ganmma));
}


void main()
{
    //----------------------------------------------------------------------------
    // RT[0]
    //----------------------------------------------------------------------------

    // ベースカラー
    vec4 albedo    = vec4(u_material.albedo, 1.0);
    vec4 albedomap = texture(u_albedo, inTexCoord);

    // 透明は書き込まない (完全に0.0ではない場合があるので 0.01に)
    if (albedomap.a < 0.01)
        discard;

    // テクスチャ と 追加カラーを乗算
    albedomap.rgb *= albedo.rgb;

    // カラー・ラフネス
    outAlbedo.rgb = Ganmma(albedomap.rgb, 2.2);
    outAlbedo.a   = u_material.roughness;

    //----------------------------------------------------------------------------
    // RT[1]
    //----------------------------------------------------------------------------

    // ノーマル（テクスチャ）
    //vec4 normalmap     = texture(u_normal, inTexCoord) * inNormalMatrix;
    //vec3 convertNormal = normalmap.rgb; // テクスチャは [0~1] の範囲内なので変換しない
    //vec3 normal        = convertNormal;

    // ノーマル・メタリック
    vec4 normalmap     = vec4(inNormal, 1.0);
    vec3 convertNormal = (normalmap.rgb * 0.5) + vec3(0.5);
    vec3 normal        = convertNormal;

    outNormal.rgb = normal;
    outNormal.a   = u_material.metallic;

    //----------------------------------------------------------------------------
    // RT[2]
    //----------------------------------------------------------------------------

    // エミッション（テクスチャ）
    //vec4 emissionmap = texture(u_emission, inTexCoord);
    //vec3 emission    = emissionmap.rgb;

    // エミッション
    vec3 emission = u_material.emission;
    outEmission = Ganmma(emission, 2.2);

    //----------------------------------------------------------------------------
    // RT[3]
    //----------------------------------------------------------------------------

    outID = inID; // エンティティID
}