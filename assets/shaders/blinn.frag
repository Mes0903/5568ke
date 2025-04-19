#version 330 core
out vec4 FragColor;
in VS_OUT{vec3 Pos;vec3 N;vec2 UV;} fs;
uniform sampler2D tex0;   // base
uniform sampler2D tex1;   // overlay, may be all‑transparent
uniform vec3 lightPos, viewPos;

void main()
{
    // Sample textures
    vec4 base = texture(tex0, fs.UV);
    vec4 eye = texture(tex1, fs.UV);
    
    // Choose overlay if it contributes color, otherwise use base
    vec4 texColor = eye.a > 0.05 ? eye : base;
    
    // If texture is completely transparent, discard the fragment
    if (texColor.a < 0.05) discard;
    
    // Check for completely black texture (possible missing texture)
    if (length(texColor.rgb) < 0.01) {
        texColor = vec4(0.7, 0.7, 0.7, 1.0); // Use light gray as fallback
    }
    
    // Prepare lighting variables
    vec3 N = normalize(fs.N);
    vec3 L = normalize(lightPos - fs.Pos);
    vec3 V = normalize(viewPos - fs.Pos);
    vec3 H = normalize(L + V);
    
    // Calculate lighting components
    float ambient = 0.2;
    float diffuse = max(dot(N, L), 0.0) * 0.8;
    float specular = pow(max(dot(N, H), 0.0), 32.0) * 0.4;
    
    // Apply lighting to texture color
    vec3 finalColor = texColor.rgb * (ambient + diffuse) + vec3(1.0) * specular;
    
    // Output final color
    FragColor = vec4(finalColor, 1.0);
}