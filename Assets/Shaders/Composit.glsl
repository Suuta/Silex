
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
    vec2 uv = (0.5 * pos.xy) + vec2(0.5);

    // uv 反転
    uv.y = 1.0 - uv.y;

    outUV       = uv;
    gl_Position = pos;
}



#pragma FRAGMENT
#version 450

layout (location = 0) in  vec2 uv;
layout (location = 0) out vec4 piexl;

layout (set = 0, binding = 0) uniform sampler2D inputAttachment;


// webgl-meincraft / FXAA shader
// https://github.com/mitsuhiko/webgl-meincraft/blob/master/assets/shaders/fxaa.glsl
vec4 FXAA(vec2 pixelSize)
{
    const float FXAA_REDUCE_MIN = 1.0 / 128.0;
    const float FXAA_REDUCE_MUL = 1.0 / 8.0;
    const float FXAA_SPAN_MAX   = 8.0;

    // サンプリングピクセルから　1px 斜め4方向を 取得
    vec3 rgbNW = texture(inputAttachment, uv + pixelSize * vec2(-1, -1)).xyz;
    vec3 rgbNE = texture(inputAttachment, uv + pixelSize * vec2( 1, -1)).xyz;
    vec3 rgbSW = texture(inputAttachment, uv + pixelSize * vec2(-1,  1)).xyz;
    vec3 rgbSE = texture(inputAttachment, uv + pixelSize * vec2( 1,  1)).xyz;
    vec3 rgbM  = texture(inputAttachment, uv).xyz;

    // 輝度勾配を求める？
    vec3  luma   = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);

    float maxLuma = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    float minLuma = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));

    // 輝度勾配からエッジラインを取得　
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * pixelSize;

    // エッジラインと近傍ピクセルをブレンド
    vec3 rgbA = 0.5 * (
        texture(inputAttachment, uv.xy + dir * (1.0/3.0 - 0.5)).xyz +
        texture(inputAttachment, uv.xy + dir * (2.0/3.0 - 0.5)).xyz);

    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(inputAttachment, uv.xy + dir * -0.5).xyz +
        texture(inputAttachment, uv.xy + dir *  0.5).xyz);

    float lumaB = dot(rgbB, luma);


    if((lumaB < minLuma) || (lumaB > maxLuma))
    {
        return vec4(vec3(rgbA), 1.0);
    }
    else
    {
        return vec4(vec3(rgbB), 1.0);
    }
}

// SEGA ACES Filmic トーンマップ
// https://techblog.sega.jp/entry/ngs_hdr_techblog_202211

// HDR: 最大輝度を指定する
vec3 ACESFilmic_Luminance(vec3 color, float luminance)
{
    const float a = 3.0;
    const float b = 0.03;
    const float c = a / luminance;
    const float d = 1.0;
    const float e = 0.14;

    return (color * (a * color + b)) / (color * (c * color + d) + e);
}

// SDR
vec3 ACESFilmic(vec3 color)
{
    const float a = 3.0;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;

    return (color * (a * color + b)) / (color * (c * color + d) + e);
}

vec3 Ganmma(vec3 color, float ganmma)
{
    return pow(color, vec3(ganmma));
}

void main()
{
    // FXAA
    vec2 pixelSize = 1.0 / textureSize(inputAttachment, 0);
    vec4 color = FXAA(pixelSize);

    // 露光？
    //color.rgb = vec3(1.0) - exp(-color.rgb * 0.5);

    // トーンマップ
    color.rgb = ACESFilmic(color.rgb).rgb;

    // ガンマ補正
    color.rgb = Ganmma(color.rgb, 1 / 2.2);

    piexl = color;
}