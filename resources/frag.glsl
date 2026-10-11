#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Tint;

uniform sampler2D ourTexture;

void main()
{
    FragColor = texture(ourTexture, TexCoord) * vec4(Tint, 1.0f);
}