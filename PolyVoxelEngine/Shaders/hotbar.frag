#version 460 core

uniform sampler2DArray diffuse0;
uniform int textureID;
uniform bool drawSlot;
uniform float brightness;

out vec4 fragColor;
in vec2 uv;

void main()
{
	if (drawSlot)
	{
		float border = 1.0 / 16.0;
		if (uv.x < border || uv.x >= 1.0 - border || uv.y < border || uv.y >= 1.0 - border)
		{
			fragColor = vec4(brightness, brightness, brightness, 1.0);
		}
		else
		{
			fragColor = vec4(0.0, 0.0, 0.0, 0.5);
		}
	}
	else
	{
		fragColor = texture(diffuse0, vec3(uv, textureID));
	}
}