//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 450

layout(location = 0) out vec2 outUV;

void main()
{
    // 三角形でフルスクリーン描画
    // https://stackoverflow.com/questions/2588875/whats-the-best-way-to-draw-a-fullscreen-quad-in-opengl-3-2

    const vec2 triangle[3] =
    {
        vec2(-1.0,  1.0), // 左上
        vec2(-1.0, -3.0), // 左下
        vec2( 3.0,  1.0), // 右上
    };

    vec4 pos = vec4(triangle[gl_VertexIndex], 0.0, 1.0);
    vec2 uv  = (0.5 * pos.xy) + vec2(0.5);

    // uv 反転
    uv.y = 1.0 - uv.y;

    outUV       = uv;
    gl_Position = pos;
}


//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 450

layout(location = 0) in  vec2 inTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D            sceneColor;
layout(set = 0, binding = 1) uniform sampler2D            sceneNormal;
layout(set = 0, binding = 2) uniform sampler2D            sceneEmission;
layout(set = 0, binding = 3) uniform sampler2D            sceneDepth;
layout(set = 0, binding = 4) uniform samplerCube          irradianceMap;
layout(set = 0, binding = 5) uniform samplerCube          prefilterMap;
layout(set = 0, binding = 6) uniform sampler2D            brdfMap;
layout(set = 0, binding = 7) uniform sampler2DArrayShadow cascadeshadowMap;

layout(set = 0, binding = 8) uniform Scene
{
    vec4  lightDir;
    vec4  lightColor;
    vec4  cameraPosition; // xyz: pos w: far
    mat4  view;
    mat4  invViewProjection;
} u_scene;


layout(set = 0, binding = 9) uniform Cascade
{
    vec4 cascadePlaneDistances[4];
};

layout(set = 0, binding = 10) uniform ShadowData
{
    mat4 lightSpaceMatrices[4];
};


//---------------------------------------------------------------------------
// 定数
//---------------------------------------------------------------------------
const int   cascadeCount    = 4;
const float shadowDepthBias = 0.01;



//---------------------------------------------------------------------------
// 深度バイアス
//---------------------------------------------------------------------------
float ShadowDepthBias(int currentLayer, vec3 normal, float farPlane)
{
    float bias     = max(shadowDepthBias * (1.0 - dot(normal, normalize(u_scene.lightDir.xyz))), shadowDepthBias);
    float mask     = 1.0 - abs(sign(currentLayer - cascadeCount));
    bias *= mix(1.0 / (cascadePlaneDistances[currentLayer].x * 0.5), 1.0 / (farPlane * 0.5), mask);

    return bias;
}

//---------------------------------------------------------------------------
// ソフトシャドウでサンプリングするピクセルオフセット
//---------------------------------------------------------------------------
float ShadowSampleOffset(vec3 shadowMapCoords, vec2 offsetPos, vec2 texelSize, int layer, float currentDepth, float bias)
{
    vec2  offset = vec2(offsetPos.x, offsetPos.y) * texelSize;
    float depth  = texture(cascadeshadowMap, vec4(shadowMapCoords.xy + offset, layer, currentDepth - bias));
    return depth;
}

//---------------------------------------------------------------------------
// ピクセルのシャドウカラーの決定
//---------------------------------------------------------------------------
float ShadowSampling(float bias, int layer, vec3 shadowMapCoords, float lightSpaceDepth)
{
    // ピクセルあたりのテクセルサイズ
    vec2  texelSize   = 1.0 / vec2(textureSize(cascadeshadowMap, 0));
    float shadowColor = 0.0;

    // 5 x 5 PCF

    //if (enableSoftShadow)
    {
        for (float x = -2.0; x <= 2.0; x += 1.0)
        {
            for (float y = -2.0; y <= 2.0; y += 1.0)
            {
                shadowColor += ShadowSampleOffset(shadowMapCoords, vec2(x, y), texelSize, layer, lightSpaceDepth, bias);
            }
        }

        shadowColor /= 25;
    }

    //else
    //{
    //    float depth = texture(cascadeshadowMap, vec4(shadowMapCoords.xy, layer, shadowMapCoords.z));
    //    shadowColor = step(lightSpaceDepth - bias, depth);
    //}

    return shadowColor;
}

//---------------------------------------------------------------------------
// ディレクショナルライト
//---------------------------------------------------------------------------
float DirectionalLightShadow(vec3 fragPosWorldSpace, vec3 normal, out int currentLayer)
{
    // カメラ空間からの距離からカスケードレイヤー選択
    vec4 fragPosViewSpace = u_scene.view * vec4(fragPosWorldSpace, 1.0);
    float fragPosDistance = abs(fragPosViewSpace.z);

    currentLayer = cascadeCount - 1;
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (fragPosDistance < cascadePlaneDistances[i].x)
        {
            currentLayer = i;
            break;
        }
    }

    // ライト空間変換
    vec4 fragPosLightSpace = lightSpaceMatrices[currentLayer] * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords        = fragPosLightSpace.xyz / fragPosLightSpace.w;

    //-------------------------------------------------------------
    // 負のビューポート使用につき、y軸反転の必要あり、復元の時は
    // vulkan側の処理が行われないので手動で反転させる
    //-------------------------------------------------------------
    projCoords.y = -projCoords.y;

    // Z座標は（OpenGLをのぞいて） 0~1 のままなので xy のみ適応
    //projCoords  = projCoords    * 0.5 + 0.5; // OpenGL
    projCoords.xy = projCoords.xy * 0.5 + 0.5; // Vulkan

    // ライト空間での深度値 (視錐台外であれば影を落とさない)
    float lightSpaceDepth = projCoords.z;
    if (lightSpaceDepth >= 1.0)
    {
        return 1.0;
    }

    // 深度値バイアス
    float bias = ShadowDepthBias(currentLayer, normal, u_scene.cameraPosition.w);

    // シャドウサンプリング
    return ShadowSampling(bias, currentLayer, projCoords, lightSpaceDepth);
}



//---------------------------------------------------------------------------
// 深度値 から ワールド座標 を計算
//---------------------------------------------------------------------------
vec3 ConstructWorldPosition(vec2 texcoord, float depthFromDepthBuffer, mat4 inverceProjectionView)
{
    // テクスチャ座標 と 深度値を使って NDC座標系[xy: -1~1] に変換 (zはそのまま)  ※openGLは z: -1~1 に変換する必要あり
    vec4 clipSpace = vec4(texcoord * vec2(2.0) - vec2(1.0), depthFromDepthBuffer, 1.0);

    //-------------------------------------------------------------
    // 負のビューポート使用につき、y軸反転の必要あり、復元の時は
    // vulkan側の処理が行われないので手動で反転させる
    //-------------------------------------------------------------
    clipSpace.y = -clipSpace.y;

    // ワールドに変換
    vec4 position = inverceProjectionView * clipSpace;

    // 透視除算
    return vec3(position.xyz / position.w);
}

//---------------------------------------------------------------------------
// フォンシェーディング
//---------------------------------------------------------------------------
vec3 BlinnPhong()
{
    //========================================================
    // 未使用（パラメータ統一のため）
    //float irr    = texture(irradianceMap, vec3(1.0)).r;
    //float pre    = textureLod(prefilterMap, vec3(1.0), 1.0).r;
    //float brdf   = texture(brdfLUT, vec2(1.0)).r;

    // コンパイル時、最適化で破棄される
    //float NO_USE = irr + pre + brdf;
    //========================================================
    vec3  ALBEDO   = texture(sceneColor,    inTexCoord).rgb;
    vec3  NORMAL   = texture(sceneNormal,   inTexCoord).rgb;
    vec3  EMISSION = texture(sceneEmission, inTexCoord).rgb;
    float DEPTH    = texture(sceneDepth,    inTexCoord).r;

    // 深度値から復元
    vec3 WORLD = ConstructWorldPosition(inTexCoord, DEPTH, u_scene.invViewProjection);

    // ノーマルを -1~1に戻す
    vec3 N = vec3(NORMAL * 2.0) - vec3(1.0);

    // 環境ベースカラー
    vec3 ambient = u_scene.lightColor.rgb * 0.01;

    // 拡散反射光
    vec3  L        = normalize(u_scene.lightDir.xyz);
    vec3  diffuse  = max(dot(N, L), 0.0) * u_scene.lightColor.rgb;

    // 鏡面反射
    vec3 V        = normalize(u_scene.cameraPosition.xyz - WORLD);
    vec3 H        = normalize(L + V);
    vec3 specular = pow(max(dot(N, H), 0.0), 64.0) * u_scene.lightColor.rgb;

    // シャドウ (Directional Light)
    int   currentLayer;
    float shadowColor = smoothstep(0.0, 1.0, DirectionalLightShadow(WORLD, N, currentLayer));

    // 最終コンポーネント
    vec3 ambientComponent  = ambient  * ALBEDO;
    vec3 diffuseComponent  = diffuse  * ALBEDO * shadowColor;
    vec3 specularComponent = specular * ALBEDO * shadowColor;
    vec3 emissionComponent = EMISSION;
    vec3 color             = ambientComponent + diffuseComponent + specularComponent + emissionComponent;

    // デバッグ: カスケード表示
    //float sc = float(showCascade);
    //color *= mix(vec3(1.0), CascadeColor(currentLayer), sc);

    return color; //+ (NO_USE * EPSILON);
}


//---------------------------------------------------------------------------
// エントリー
//---------------------------------------------------------------------------
void main()
{   
    // サンプリング関数
    // int   : isampler2D + texelFetch
    // float : sampler2D  + texture

    // 整数型のサンプリングは、テクスチャ座標ではなくピクセルを指定
    // int shadingModel = texelFetch(idMap, ivec2(fsi.TexCoords * textureSize(idMap, 0)), 0).r;

    vec3 color = BlinnPhong();
    outColor = vec4(color, 1.0);
}
