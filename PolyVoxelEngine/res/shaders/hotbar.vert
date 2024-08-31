#version 460 core

layout(location = 0) in vec2 vertPos;
layout(location = 1) in vec2 vertUV;

uniform vec2 position;
uniform vec2 scale;

out vec2 uv;

void main()
{
	uv = vertUV;
	gl_Position = vec4(position + vertPos * scale, 0.0, 1.0);
}