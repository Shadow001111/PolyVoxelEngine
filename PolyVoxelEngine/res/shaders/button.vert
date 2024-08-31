#version 460 core

uniform float aspectRatio;

layout (location = 0) in vec2 inPos;

void main()
{
    vec2 vertPos = inPos;
    vertPos.x /= aspectRatio;
    gl_Position = vec4(vertPos, 0.0f, 1.0f);
}