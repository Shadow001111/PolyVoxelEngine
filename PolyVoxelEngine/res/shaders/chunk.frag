#version 460 core

#ifdef Z_PRE_PASS
void main(){}
#else
uniform sampler2DArray diffuse0;
uniform sampler2DArray numbers;

uniform vec3 fogColor;
uniform float fogDensity;
uniform float fogGradient;
uniform int debugViewMode;

out vec4 fragColor;
in vec2 uv;
in vec2 normalizedUV;
flat in float textureID;
in flat float[4] gottenAOValues;
in float depth;
 #ifdef SMOOTH_LIGHTING
flat in float[4] gottenLightingValues;
 #endif
flat in float blockLight;
flat in float skyLight;

vec4 polygonLook(vec4 color, vec2 uv)
{
	if 
	(
		abs(uv.x - 0.5) > 0.45 ||
		abs(uv.y - 0.5) > 0.45 ||
		abs(uv.x - uv.y) < 0.05
	)
	{
		return vec4(1.0);
	}
	return color;
}

void main()
{
	vec4 color;
	if (debugViewMode == 3)
	{
		color = vec4(0.0, 0.0, 0.0, 1.0);
	}
	else
	{
		color = texture(diffuse0, vec3(uv, textureID));

		float b01 = mix(gottenAOValues[0], gottenAOValues[1], uv.x);
		float b32 = mix(gottenAOValues[3], gottenAOValues[2], uv.x);
		float brightness = mix(b01, b32, uv.y);

		 #ifdef SMOOTH_LIGHTING
		float l01 = mix(gottenLightingValues[0], gottenLightingValues[1], uv.x);
		float l32 = mix(gottenLightingValues[3], gottenLightingValues[2], uv.x);
		float smoothLighting = mix(l01, l32, uv.y);
		 #endif

		color *= brightness;

		if (debugViewMode == 0)
		{
			 #ifdef SMOOTH_LIGHTING
			color *= smoothLighting;
			 #else
			color *= max(blockLight, skyLight);
			 #endif
		}
		else if (debugViewMode == 1)
		{
			const float numberScale = 3.0;
			const float numberOffset = 0.5;
		
			vec2 transformedUv = (((mod(uv, 1.0) * 2.0 - 1.0) * numberScale) + 1.0) * 0.5;

			vec2 blockNumberUv = transformedUv - vec2(numberOffset, 0.0);
			vec2 skyNumberUv = transformedUv + vec2(numberOffset, 0.0);

			vec4 blockNumberColor = texture(numbers, vec3(blockNumberUv, blockLight * 15.0));
			vec4 skyNumberColor = texture(numbers, vec3(skyNumberUv, skyLight * 15.0));

			blockNumberColor.r = blockLight == 0.0 ? 0.0 : blockNumberColor.r;
			skyNumberColor.r = skyLight == 0.0 ? 0.0 : skyNumberColor.r;

			float fallof = min(1.0, 3.0 / max(1.0, depth));

			if (blockNumberColor.r * fallof < 0.1 && skyNumberColor.r * fallof < 0.1)
			{
				// face background
				color.xyz = vec3(blockLight, 0.0, skyLight) * brightness;
			}
			else
			{
				// numbers
				color.xyz = mix(vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), skyNumberColor.r) * 0.5;
				color.w = 1.0;
			}
		} 
		else if (debugViewMode == 2)
		{
			 #ifdef SMOOTH_LIGHTING
			color *= smoothLighting;
			 #else
			color *= max(blockLight, skyLight);
			 #endif
			float polyEffect = max(1.0, 3.0 / pow(depth, 0.8));
			color = mix(color, polygonLook(color, normalizedUV), polyEffect);
		}
	}

	float fogEffect = clamp(exp(-pow(depth * fogDensity, fogGradient)), 0.0, 1.0);
	color.xyz = mix(fogColor, color.xyz, fogEffect);

	fragColor = color;
}
#endif