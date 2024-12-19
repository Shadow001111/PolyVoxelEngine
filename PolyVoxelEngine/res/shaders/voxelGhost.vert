#version 460 core

layout(location = 0) in vec3 vertPos;
layout(location = 1) in vec2 vertUV;

uniform mat4 camMatrix;
uniform vec3 position;
uniform vec3 scale;

out vec2 uv;

void main()
{
	uv = vertUV;
	gl_Position = camMatrix * vec4(position + ((vertPos - 0.5) * scale * 1.02 + 0.5 * scale), 1.0);
}