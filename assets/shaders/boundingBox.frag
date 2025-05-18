#version 330 core
uniform vec3 uColor = vec3(1.0, 0.0, 1.0); // default magenta
out vec4 FragColor;

void main() {
    FragColor = vec4(uColor, 1.0);
}