#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 Tint;
in vec3 FragPos;

uniform sampler2D ourTexture;
uniform vec3 camPosition;

vec3 fogColor = vec3(0.52f, 0.81f, 0.92f);
float fdensity = .006f;
bool doFog = true;

void main()
{
    vec4 tex = texture(ourTexture, TexCoord);
    vec4 col = tex * Tint;

    if(col.a<0.01) discard;

    if(doFog){
        float distToCam = length(FragPos-camPosition);

        float factor = exp(-pow((distToCam * fdensity), 5.0f));
        factor = clamp(factor, 0.0f, 1.0f);

        vec3 finalCol = mix(fogColor, col.rgb, factor);

        FragColor=vec4(finalCol, col.a);
    } else {
        FragColor=col;
    }
}