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
    TexCoords   = aTexCoords;
    gl_Position = vec4(aPos.x * 1.0, aPos.y * 1.0, 0.0, 1.0);
}

//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 430 core

in  vec2 TexCoords;
out vec4 PixelColor;

uniform sampler2D screenTexture;
uniform sampler2D normalTexture;
uniform sampler2D depthTexture;

uniform float lineWidth = 1.0;
uniform vec3  outlineColor;

// Simple GPU Outline Shaders
// https://io7m.com/documents/outline-glsl/#d0e239

void main()
{
    // ピクセルサイズを計算
    vec2 viewportSize = textureSize(screenTexture, 0);
    float dx = (1.0 / viewportSize.x) * lineWidth;
    float dy = (1.0 / viewportSize.y) * lineWidth;

    // サンプルポイント（ピクセル座標のオフセット）
    vec2 uvCenter   = TexCoords;
    vec2 uvRight    = vec2(uvCenter.x + dx, uvCenter.y);
    vec2 uvTop      = vec2(uvCenter.x,      uvCenter.y - dx);
    vec2 uvTopRight = vec2(uvCenter.x + dx, uvCenter.y - dx);

    // ノーマルマップから取得
    vec3 mCenter   = texture(normalTexture, uvCenter).rgb;
    vec3 mTop      = texture(normalTexture, uvTop).rgb;
    vec3 mRight    = texture(normalTexture, uvRight).rgb;
    vec3 mTopRight = texture(normalTexture, uvTopRight).rgb;

    //
    vec3 dT  = abs(mCenter - mTop);
    vec3 dR  = abs(mCenter - mRight);
    vec3 dTR = abs(mCenter - mTopRight);

    float dTmax  = max(dT.x, max(dT.y, dT.z));
    float dRmax  = max(dR.x, max(dR.y, dR.z));
    float dTRmax = max(dTR.x, max(dTR.y, dTR.z));

    float deltaRaw = 0.0;
    deltaRaw = max(deltaRaw, dTmax);
    deltaRaw = max(deltaRaw, dRmax);
    deltaRaw = max(deltaRaw, dTRmax);


    vec3 color = texture(screenTexture, TexCoords).rgb;

    //if (deltaRaw >= 0.5)
    //{
    //    color = outlineColor;
    //}

    float mask = step(0.5, deltaRaw);
    color = mix(color, outlineColor, mask);


    PixelColor = vec4(vec3(color), 1.0);
}