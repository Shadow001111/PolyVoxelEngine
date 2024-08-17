#version 460 core

uniform vec2 position;
uniform vec2 scale;
uniform float aspectRatio;

layout (location = 0) in vec2 inPos;

void main()
{
    vec2 vertPos = position + inPos * scale;
    vertPos.x /= aspectRatio;
    gl_Position = vec4(vertPos, 0.0f, 1.0f);
}