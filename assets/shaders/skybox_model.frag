#version 330 core
out vec4 FragColor;
in VS_OUT{vec3 Pos;vec3 N;vec2 UV;} fs;
uniform sampler2D tex0;   // base
uniform sampler2D tex1;   // overlay, may be all-transparent

void main()
{
    // Sample textures
    vec4 base = texture(tex0, fs.UV);
    vec4 overlay = texture(tex1, fs.UV);
    
    // Choose overlay if it contributes color, otherwise use base
    vec4 texColor = overlay.a > 0.05 ? overlay : base;
    
    // For skybox, always use full opacity
    texColor.a = 1.0;
    
    // Check for completely black texture (possible missing texture)
    if (length(texColor.rgb) < 0.01) {
        texColor = vec4(0.7, 0.7, 0.7, 1.0); // Use light gray as fallback
    }
    
    // Apply brighter ambient lighting (increased from 0.8 to 1.5)
    float ambient = 1.0;
    vec3 finalColor = texColor.rgb * ambient;
    
    // Ensure colors don't exceed 1.0 due to brightness
    finalColor = min(finalColor, vec3(1.0));
    
    // For skybox, we output the color without normal-based lighting
    FragColor = vec4(finalColor, 1.0);
}