#pragma VERTEX
{
    #version 450
    #include "common.glsl"

    layout (location = 0) out vec3 outNormal;
    layout (location = 1) out vec3 outColor;
    layout (location = 2) out vec2 outUV;

    void main()
    {
        // デバイスアドレスから頂点データ取得
        Vertex v = constants.vertexBuffer.vertices[gl_VertexIndex];

        // アウトプット
        outNormal = (constants.render_matrix * vec4(v.normal, 0.0)).xyz;
        outColor  = v.color.xyz * materialData.colorFactors.xyz;
        outUV.x   = v.uv_x;
        outUV.y   = v.uv_y;

        // 頂点座標
        gl_Position = sceneData.viewproj * constants.render_matrix * vec4(v.position, 1.0);
    }
}


#pragma FRAGMENT
{
    #version 450
    #include "common.glsl"

    layout (location = 0) in  vec3 inNormal;
    layout (location = 1) in  vec3 inColor;
    layout (location = 2) in  vec2 inUV;
    layout (location = 0) out vec4 outFragColor;

    layout(set = 1, binding = 1) uniform sampler2D colorTex;
    layout(set = 1, binding = 2) uniform sampler2D metalRoughTex;

    void main()
    {
        // 環境光
        float lightValue = max(dot(inNormal, sceneData.sunlightDirection.xyz), 0.1);
        vec3 color       = inColor * texture(colorTex,inUV).xyz;
        vec3 ambient     = color *  sceneData.ambientColor.xyz;

        // ピクセルカラー
        outFragColor = vec4(color * lightValue *  sceneData.sunlightColor.w + ambient ,1.0);
    }
}
