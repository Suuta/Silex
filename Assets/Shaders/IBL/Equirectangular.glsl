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
in vec3  LocalPos;

uniform sampler2D equirectangularMap;


vec2 SampleSphericalMap(vec3 v)
{
    const vec2 invAtan = vec2(0.1591, 0.3183);

    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(LocalPos));
    vec3 color = texture(equirectangularMap, uv).rgb;
    
    FragColor = vec4(color, 1.0);
}