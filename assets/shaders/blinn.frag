#version 330 core
out vec4 FragColor;
in VS_OUT{vec3 Pos;vec3 N;vec2 UV;} fs;

uniform sampler2D tex0;   // base
uniform sampler2D tex1;   // overlay, may be all‑transparent
uniform vec3 lightPos, viewPos;

void main()
{
    vec4 base = texture(tex0, fs.UV);
    vec4 eye  = texture(tex1, fs.UV);

    // choose overlay if it contributes colour
    vec4 tex = eye.a > 0.05 ? eye : base;
    if (tex.a < 0.05) discard;     // fully transparent pixel? skip

    vec3 N = normalize(fs.N);
    vec3 L = normalize(lightPos - fs.Pos);
    vec3 V = normalize(viewPos  - fs.Pos);
    vec3 H = normalize(L+V);

    float diff = max(dot(N,L),0);
    float spec = pow(max(dot(N,H),0),32);

    vec3 color = tex.rgb * (0.1 + 0.8*diff) + vec3(0.4)*spec;
    FragColor  = vec4(color,1.0);
}
