#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aTint;
layout (location = 3) in float aAlpha;
layout (location = 4) in float aNormal;//+X, -X, +Y, -Y, +Z, -Z
layout (location = 5) in float aLightLevel;//1-15

//comment

out vec2 TexCoord;
out vec4 Tint;
out vec3 FragPos;

uniform mat4 view;
uniform mat4 proj;

const float shades[6] = float[6](0.5, 0.5, 0, 0.5, 0.5, 0.5);

float map(float val, float imax, float imin, float omax, float omin) {
    return clamp(omin+((omax-omin)/(imax-imin))*(val-imin),omin,omax);
}

void main()
{
    gl_Position = proj * view  * vec4(aPos, 1.0);
    TexCoord = vec2(aTexCoord.x, aTexCoord.y);
    FragPos = vec3(aPos.x, aPos.y, aPos.z);

    float factint = shades[int(aNormal)];
    float lightLevelTint = map(aLightLevel, 15.0f, 0.0f, 1.0f, 0.0f);//convert from 15 - 1 to 1 - 0

    float finalTint = lightLevelTint * (1 - factint);
    finalTint=max(finalTint, 0.05f);//so its not pitch black

    vec3 aTModif = aTint*finalTint;
    Tint = vec4(aTModif, aAlpha);
}