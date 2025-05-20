#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;

uniform mat4 model, view, proj;
out VS_OUT{vec3 Pos;vec3 N;vec2 UV;} vs;

void main() {
    // Transform position
    vec4 world = model * vec4(aPos, 1.0);
    vs.Pos = world.xyz;
    
    // Just pass the normal through without transforming it
    // This helps avoid any darkening from normal-based lighting calculations
    vs.N = normalize(aNormal);
    
    // Pass UVs directly
    vs.UV = aUV;
    
    // Standard transformation for position
    gl_Position = proj * view * world;
}