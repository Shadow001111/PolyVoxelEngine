#version 460 core
layout (location = 0) in vec2 pos;

layout(binding = 2) restrict readonly buffer TransformsSSBO
{
	vec4 transforms[];
};

out vec2 uv;
flat out uint index;

uniform float aspectRatio;

void main()
{
    const vec4 transform = transforms[gl_InstanceID];
    vec2 vertPos = transform.xy + pos.xy * transform.zw;
    vertPos.x /= aspectRatio;
    gl_Position = vec4(vertPos, 0.0f, 1.0f);

    uv = pos.xy;
    uv.y = 1.0 - uv.y;

    index = gl_InstanceID;
}  