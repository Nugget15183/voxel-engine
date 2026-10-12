#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 Tint;

uniform sampler2D ourTexture;

void main()
{
    vec4 tex = texture(ourTexture, TexCoord);
    vec4 col = tex * Tint;
    if(col.a<0.01) discard;
    FragColor=col;
}