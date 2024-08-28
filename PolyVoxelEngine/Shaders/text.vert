#version 460 core
layout (location = 0) in vec2 pos;
out vec2 uv;

uniform vec4 transform;
uniform float aspectRatio;

void main()
{
    vec2 vertPos = transform.xy + pos.xy * transform.zw;
    vertPos.x /= aspectRatio;
    gl_Position = vec4(vertPos, 0.0f, 1.0f);

    uv = pos.xy;
    uv.y = 1.0 - uv.y;
}  