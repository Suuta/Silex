//===================================================================================
// 頂点シェーダ
//===================================================================================
#pragma VERTEX
#version 430 core

layout (location = 0) in vec3 aPos;

out vec3 LocalPos;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    LocalPos = aPos;
    gl_Position = projection * view * vec4(LocalPos, 1.0);
}


//===================================================================================
// フラグメントシェーダ
//===================================================================================
#pragma FRAGMENT
#version 430 core

out vec4 FragColor;
in  vec3 LocalPos;

uniform samplerCube environmentMap;

const float PI = 3.14159265359;

void main()
{
    // スカイボックス同様に、頂点座標がそのまま法線として利用できる
    // 法線方向から来た光を放射輝度としてサンプリングし、PBRシェーダ―で利用できるようにする
    vec3 N = normalize(LocalPos);

    vec3 irradiance = vec3(0.0);

    // 原点からの接空間計算
    vec3 up    = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up         = normalize(cross(N, right));
       
    float sampleDelta = 0.025;
    float nrSamples   = 0.0;

    // TODO: 内容理解する
    for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            vec3 tangentSample = vec3(sin(theta) * cos(phi),  sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec     = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N; 

            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }

    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    FragColor  = vec4(irradiance * 0.5, 1.0);
}
