#version 460 core

uniform vec3 color;
uniform float borderScale;

out vec4 fragColor;
in vec2 uv;

void main()
{
	float border = 1.0 / 16.0 * borderScale;
	if (uv.x < border || uv.x >= 1.0 - border || uv.y < border || uv.y >= 1.0 - border)
	{
		fragColor = vec4(color, 1.0);
	}
	else
	{
		fragColor = vec4(0.0, 0.0, 0.0, 0.0);
	}
}