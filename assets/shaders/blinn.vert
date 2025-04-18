#version 330 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;

uniform mat4 model,view,proj;
out VS_OUT{vec3 Pos;vec3 N;vec2 UV;} vs;

void main(){
    vec4 world = model*vec4(aPos,1);
    vs.Pos = world.xyz;
    vs.N   = mat3(transpose(inverse(model)))*aNormal;
    vs.UV  = aUV;
    gl_Position = proj*view*world;
}
