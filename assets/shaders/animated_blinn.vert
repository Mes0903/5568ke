#version 330 core

layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aUV;
layout(location=3) in ivec4 aBoneIds;
layout(location=4) in vec4 aBoneWeights;

const int MAX_BONES = 100;
uniform mat4 boneMatrices[MAX_BONES];
uniform mat4 model, view, proj;
uniform bool hasAnimation;

out VS_OUT{vec3 Pos; vec3 N; vec2 UV;} vs;

void main() {
    if (hasAnimation) {
        // Calculate bone transformations
        mat4 boneTransform = mat4(0.0);
        for (int i = 0; i < 4; i++) {
            if (aBoneIds[i] >= 0) {
                boneTransform += boneMatrices[aBoneIds[i]] * aBoneWeights[i];
            }
        }
        
        // Ensure the bone transform is valid
        if (boneTransform == mat4(0.0)) {
            boneTransform = mat4(1.0);
        }
        
        // Apply bone transform followed by model transform
        vec4 worldPos = model * boneTransform * vec4(aPos, 1.0);
        vs.Pos = worldPos.xyz;
        
        // Transform normal by the transpose of the inverse of the bone transform
        mat3 normalMatrix = transpose(inverse(mat3(model * boneTransform)));
        vs.N = normalMatrix * aNormal;
    } else {
        // Standard vertex transformation without animation
        vec4 worldPos = model * vec4(aPos, 1.0);
        vs.Pos = worldPos.xyz;
        vs.N = mat3(transpose(inverse(model))) * aNormal;
    }
    
    vs.UV = aUV;
    gl_Position = proj * view * vec4(vs.Pos, 1.0);
}