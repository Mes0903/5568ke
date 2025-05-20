#version 330 core
layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 proj;

void main()
{
    TexCoords = aPos;
    
    // Skybox should be infinitely far away, so we set z to w
    // after perspective division this ensures depth is always 1.0 (the maximum)
    vec4 pos = proj * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}