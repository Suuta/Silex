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

const float zNear = 0.1;
const float zFar  = 10.0;


float linearDepth(float depthSample)
{
    depthSample = 2.0 * depthSample - 1.0;
    float zLinear = 2.0 * zNear * zFar / (zFar + zNear - depthSample * (zFar - zNear));
    return zLinear;
}

void main()
{
    vec2 viewoport     = textureSize(screenTexture, 0);
    vec2 texelSize     = vec2(1.0 / viewoport.x, 1.0 / viewoport.y);
    vec4 originalColor = texture(screenTexture, TexCoords);

    float offset = lineWidth   * 0.5;
    float left   = texelSize.x * -offset;
    float right  = texelSize.x *  offset;
    float top    = texelSize.y * -offset;
    float bottom = texelSize.y *  offset;

    vec2 center = TexCoords;
    vec2 tr = center + vec2(right, top);
    vec2 tl = center + vec2(left,  top);
    vec2 br = center + vec2(right, bottom);
    vec2 bl = center + vec2(left,  bottom);

    //======================================================
    // 深度
    //======================================================
    float depthCurrent = linearDepth(texture(depthTexture, center).a);

    float depthAverage = 0.0;
    depthAverage += linearDepth(texture(depthTexture, tr).a);
    depthAverage += linearDepth(texture(depthTexture, tl).a);
    depthAverage += linearDepth(texture(depthTexture, br).a);
    depthAverage += linearDepth(texture(depthTexture, bl).a);
    depthAverage *= 0.25;

    if (abs(depthCurrent - depthAverage) > 0.05)
    {
        // 深度平均値との差が一定以上ならアウトラインピクセル
        PixelColor.rgb = outlineColor;
    }
    else
    {
        //======================================================
        // Simple GPU Outline Shaders
        // https://io7m.com/documents/outline-glsl/#d0e239
        //======================================================

        // ノーマルマップから取得
        vec3 nCenter = texture(normalTexture, center).rgb;

        vec3 nTR = texture(normalTexture, center + vec2(right, top)    * 2.0).rgb;
        vec3 nTL = texture(normalTexture, center + vec2(left,  top)    * 2.0).rgb;
        vec3 nBR = texture(normalTexture, center + vec2(right, bottom) * 2.0).rgb;
        vec3 nBL = texture(normalTexture, center + vec2(left,  bottom) * 2.0).rgb;

        vec3 dTR = abs(nCenter - nTR);
        vec3 dTL = abs(nCenter - nTL);
        vec3 dBR = abs(nCenter - nBR);
        vec3 dBL = abs(nCenter - nBL);

        float dTRmax = max(dTR.x, max(dTR.y, dTR.z));
        float dTLmax = max(dTL.x, max(dTL.y, dTL.z));
        float dBRmax = max(dBR.x, max(dBR.y, dBR.z));
        float dBLmax = max(dBL.x, max(dBL.y, dBL.z));

        float deltaRaw = 0.0;
        deltaRaw = max(deltaRaw, dTRmax);
        deltaRaw = max(deltaRaw, dTLmax);
        deltaRaw = max(deltaRaw, dBRmax);
        deltaRaw = max(deltaRaw, dBLmax);

        if (deltaRaw >= 0.5)
        {
            PixelColor.rgb = outlineColor;
        }
        else
        {
            PixelColor.rgb = originalColor.rgb;
        }
    }
}