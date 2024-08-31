#version 460 core

layout(location = 0) in vec2 vertPos;

uniform float aspectRatio;
uniform vec2 position;
uniform vec2 scale;

void main()
{
	vec2 pos = position + vertPos * scale;
	pos.x /= aspectRatio;
	gl_Position = vec4(pos, 0.0, 1.0);
}