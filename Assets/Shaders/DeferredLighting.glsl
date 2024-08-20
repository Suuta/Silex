//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 450

layout (location = 0) out vec2 outUV;

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

layout(set = 0, binding = 0) uniform sampler2D sceneColor;
layout(set = 0, binding = 1) uniform sampler2D sceneNormal;
layout(set = 0, binding = 2) uniform sampler2D sceneEmission;
//layout(set = 0, binding = 3) uniform sampler2D sceneDepth;

layout(set = 1, binding = 0) uniform Scene
{
    vec4 lightDir;
    vec4 lightColor;
    vec4 cameraPosition;

} u_scene;


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
    vec3  N        = texture(sceneNormal,   inTexCoord).rgb;
    vec3  EMISSION = texture(sceneEmission, inTexCoord).rgb;
    //float DEPTH    = texture(sceneDepth,    inTexCoord).r;

    // TODO: 深度値から復元
    vec3 WORLD = vec3(0.0);

    // TODO: ノーマルを -1~1に戻す
    vec3 nN = vec3(N);


    // 環境光 (PBRシェーダー側では輝度を10倍した値を1.0として扱っているため)
    //vec3 ambient = (lightColor / 10.0) * (iblIntencity / 10.0);
    vec3 ambient = u_scene.lightColor.rgb;

    // 拡散反射光
    vec3  L        = normalize(u_scene.lightDir.xyz);
    vec3  diffuse  = max(dot(N, L), 0.0) * u_scene.lightColor.rgb;

    // 鏡面反射
    vec3 V        = normalize(u_scene.cameraPosition.xyz - WORLD);
    vec3 H        = normalize(L + V);
    vec3 specular = pow(max(dot(N, H), 0.0), 64.0) * u_scene.lightColor.rgb;

    // シャドウ (Directional Light)
    //int   currentLayer;
    //float shadowColor = smoothstep(0.0, 1.0, DirectionalLightShadow(WORLD_POS, N, currentLayer));

    // 最終コンポーネント
    vec3 ambientComponent  = ambient  * ALBEDO;
    vec3 diffuseComponent  = diffuse  * ALBEDO; //* shadowColor;
    vec3 specularComponent = specular * ALBEDO; //* shadowColor;
    vec3 emissionComponent = EMISSION;
    vec3 color             = ambientComponent + diffuseComponent + specularComponent + emissionComponent;

    // デバッグ: カスケード表示
    //float sc = float(showCascade);
    //color *= mix(vec3(1.0), CascadeColor(currentLayer), sc);

    return color; //+ (NO_USE * EPSILON);
}

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

