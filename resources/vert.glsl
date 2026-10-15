#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aTint;
layout (location = 3) in float aAlpha;
layout (location = 4) in float aNormal;//+X, -X, +Y, -Y, +Z, -Z

//comment

out vec2 TexCoord;
out vec4 Tint;
out vec3 FragPos;

uniform mat4 view;
uniform mat4 proj;

void main()
{
    gl_Position = proj * view  * vec4(aPos, 1.0);
    TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    FragPos = vec3(aPos.x, aPos.y, aPos.z);

    vec3 faceTint = vec3(0.0f,0.0f,0.0f);

    vec3 aTModif = aTint;
    if(aNormal == 0.0f || aNormal == 1.0f || aNormal == 3.0f || aNormal == 4.0f || aNormal == 5.0f) {
        aTModif = mix(aTint, vec3(0,0,0), 0.5f);
    }

    Tint = vec4(aTModif, aAlpha);
}