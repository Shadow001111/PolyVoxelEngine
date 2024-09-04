#version 460 core
layout (location = 0) in vec2 pos;

struct CharacterData
{
    float transform[4];
    uint textureID;
};

layout(std430, binding = 2) restrict readonly buffer TextSSBO
{
	CharacterData charactersData[];
};

out vec2 uv;
flat out uint textureID;

uniform float aspectRatio;

void main()
{
    CharacterData data = charactersData[gl_InstanceID];

    const vec4 transform = {data.transform[0], data.transform[1], data.transform[2], data.transform[3]};
    vec2 vertPos = transform.xy + pos.xy * transform.zw;
    vertPos.x /= aspectRatio;
    gl_Position = vec4(vertPos, 0.0f, 1.0f);

    uv = pos.xy;
    uv.y = 1.0 - uv.y;

    textureID = data.textureID;
}