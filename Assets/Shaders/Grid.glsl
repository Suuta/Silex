#pragma VERTEX
{
    // How to make an infinite grid.
    // https://asliceofrendering.com/scene%20helper/2020/01/05/InfiniteGrid/


    // TODO: LOD対応
    //https://www.shadertoy.com/view/7tGBDK


    #version 450

    layout(location = 0) out vec3 nearPoint;
    layout(location = 1) out vec3 farPoint;
    layout(location = 2) out mat4 fragView;
    layout(location = 7) out mat4 fragProj;

    layout(set = 0, binding = 0) uniform CameraData
    {
        mat4 view;
        mat4 proj;
        vec3 pos;
    } cameraView;

    vec3 UnprojectPoint(float x, float y, float z, mat4 view, mat4 projection)
    {
        mat4 viewInv          = inverse(view);
        mat4 projInv          = inverse(projection);
        vec4 unprojectedPoint = viewInv * projInv * vec4(x, y, z, 1.0);

        return unprojectedPoint.xyz / unprojectedPoint.w;
    }

    void main()
    {
        // トライアングルストリップで矩形描画
        vec3 gridPlane[6] =
        {
            vec3(-1.0,  1.0, 0.0), vec3(-1.0, -1.0, 0.0), vec3( 1.0,  1.0, 0.0),
            vec3( 1.0,  1.0, 0.0), vec3(-1.0, -1.0, 0.0), vec3( 1.0, -1.0, 0.0),
        };

        vec3 p      = gridPlane[gl_VertexIndex];
        nearPoint   = UnprojectPoint(p.x, p.y, 0.0, cameraView.view, cameraView.proj).xyz; // unprojecting on the near plane
        farPoint    = UnprojectPoint(p.x, p.y, 1.0, cameraView.view, cameraView.proj).xyz; // unprojecting on the far plane
        gl_Position = vec4(p, 1.0);                                                        // using directly the clipped coordinates

        fragView = cameraView.view;
        fragProj = cameraView.proj;
    }
}


#pragma FRAGMENT
{
    #version 450

    layout(location = 0) out vec4 outColor;

    layout(location = 0) in  vec3 nearPoint; // nearPoint calculated in vertex shader
    layout(location = 1) in  vec3 farPoint;  // farPoint  calculated in vertex shader
    layout(location = 2) in  mat4 fragView;
    layout(location = 7) in  mat4 fragProj;


    vec4 grid(vec3 fragPos3D, float lineWidth)
    {
        vec2  coord      = fragPos3D.xz * lineWidth; // use the scale variable to set the distance between the lines
        vec2  derivative = fwidth(coord);
        vec2  grid       = abs(fract(coord - 0.5) - 0.5) / derivative;
        float line       = min(grid.x, grid.y);
        float minimumz   = min(derivative.y, 1);
        float minimumx   = min(derivative.x, 1);
        vec4  color      = vec4(0.2, 0.2, 0.2, 1.0 - min(line, 1.0));

        // z axis
        if(fragPos3D.x > -0.1 * minimumx && fragPos3D.x < 0.1 * minimumx)
            color.z = 1.0;
        // x axis
        if(fragPos3D.z > -0.1 * minimumz && fragPos3D.z < 0.1 * minimumz)
            color.x = 1.0;

        return color;
    }

    float computeDepth(vec3 pos)
    {
        vec4 clip_space_pos = fragProj * fragView * vec4(pos.xyz, 1.0);
        return (clip_space_pos.z / clip_space_pos.w);
    }

    void main()
    {
        float t        = -nearPoint.y / (farPoint.y - nearPoint.y);
        vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);

        gl_FragDepth = computeDepth(fragPos3D);

        // t > 0 の場合は不透明度 = 1、それ以外の場合は不透明度 = 0
        vec4 color = grid(fragPos3D, 1.5 * float(t > 0));

        if (color.a <= 0.01)
            discard;

        outColor = color;
    }
}
