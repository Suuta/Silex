//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 430 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}


//===================================================================================
// フラグメントシェーダ
//-----------------------------------------------------------------------------------
// webgl-meincraft / FXAA shader
// https://github.com/mitsuhiko/webgl-meincraft/blob/master/assets/shaders/fxaa.glsl
//===================================================================================
#pragma FRAGMENT
#version 430 core

in  vec2 TexCoords;
out vec4 PixelColor;

uniform sampler2D screenTexture;

#define FXAA_REDUCE_MIN   1.0 / 128.0
#define FXAA_REDUCE_MUL   1.0 / 8.0
#define FXAA_SPAN_MAX     8.0


// NW | N | NE
// ___ ___ ___
//  W | M | E
// ___ ___ ___
// SW | S | SE

// AAするピクセルの判別にピクセルの輝度差をとってるので
// 輝度差が少ない部分では、AAの効果が薄くなる

void main()
{
    // ピクセルサイズ
    vec2 pixelsize = 1.0 / textureSize(screenTexture, 0);

    // サンプリングピクセルから　1px 斜め4方向を 取得
    vec3 rgbNW = texture(screenTexture, TexCoords + pixelsize * vec2(-1, -1)).xyz;
    vec3 rgbNE = texture(screenTexture, TexCoords + pixelsize * vec2( 1, -1)).xyz;
    vec3 rgbSW = texture(screenTexture, TexCoords + pixelsize * vec2(-1,  1)).xyz;
    vec3 rgbSE = texture(screenTexture, TexCoords + pixelsize * vec2( 1,  1)).xyz;
    vec3 rgbM  = texture(screenTexture, TexCoords).xyz;

    // 輝度勾配を求める？
    vec3 luma = vec3(0.299, 0.587, 0.114);
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

    dir = min(vec2(FXAA_SPAN_MAX, FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * pixelsize;

    // エッジラインと近傍ピクセルをブレンド
    vec3 rgbA = 0.5 * (
        texture(screenTexture, TexCoords.xy + dir * (1.0/3.0 - 0.5)).xyz +
        texture(screenTexture, TexCoords.xy + dir * (2.0/3.0 - 0.5)).xyz);

    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(screenTexture, TexCoords.xy + dir * -0.5).xyz +
        texture(screenTexture, TexCoords.xy + dir *  0.5).xyz);

    float lumaB = dot(rgbB, luma);


    //if((lumaB < minLuma) || (lumaB > maxLuma))
    //{
    //    PixelColor = vec4(rgbA, 1.0);
    //}
    //else
    //{
    //    PixelColor = vec4(rgbB, 1.0);
    //}

    float boolX = step(lumaB, minLuma);
    float boolY = step(maxLuma, lumaB);
    float mask = step(0.5, boolX + boolY);

    PixelColor = mix(vec4(rgbA, 1.0), vec4(rgbB, 1.0), mask);
}