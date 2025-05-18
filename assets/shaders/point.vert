#version 330 core
layout(location=0) in vec3 aPos;
uniform mat4 view, proj;
uniform float pointSize;
void main(){
    gl_Position = proj * view * vec4(aPos,1.0);
    gl_PointSize = pointSize;
}