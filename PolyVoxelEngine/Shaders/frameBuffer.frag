#version 460 core

out vec4 fragColor;
in vec2 texCoords;

uniform sampler2D screenTexture;
uniform float aspectRatio;

//const float offset_x = 1.0f / 800.0f;
//const float offset_y = 1.0f / 800.0f;
//
//const vec2 offsets[9] = vec2[]
//(
//    vec2(-offset_x,  offset_y), vec2( 0.0f,    offset_y), vec2( offset_x,  offset_y),
//    vec2(-offset_x,  0.0f),     vec2( 0.0f,    0.0f),     vec2( offset_x,  0.0f),
//    vec2(-offset_x, -offset_y), vec2( 0.0f,   -offset_y), vec2( offset_x, -offset_y) 
//);
//
//const float BlurKernel[9] = float[]
//(
//    1.0 / 9,  1.0 / 9,  1.0 / 9,
//    1.0 / 9,  1.0 / 9,  1.0 / 9,
//    1.0 / 9,  1.0 / 9,  1.0 / 9
//);
//
//const float GaussianBlurKernel[9] = float[]
//(
//    1.0 / 16,  2.0 / 16,  1.0 / 16,
//    2.0 / 16,  4.0 / 16,  2.0 / 16,
//    1.0 / 16,  2.0 / 16,  1.0 / 16
//);

//vec2 GetGishEyeUV(vec2 uv0)
//{
//    float aperture = 172.0;
//    float apertureHalf = 0.5 * aperture * (3.1415926535 / 180.0);
//    float maxFactor = sin(apertureHalf);
//  
//    vec2 xy = 2.0 * uv0.xy - 1.0;
//    float d = length(xy);
//    if (d < (2.0-maxFactor))
//    {
//        d = length(xy * maxFactor);
//        float z = sqrt(1.0 - d * d);
//        float r = atan(d, z) / 3.1415926535;
//        float phi = atan(xy.y, xy.x);
//    
//        return vec2(r * cos(phi) + 0.5, r * sin(phi) + 0.5);
//    }
//    else
//    {
//        return uv0;
//    }
//}
//
//vec3 GetColorApplKernel(vec2 uv, float kernel[9], vec2 offsets[9])
//{
//    vec3 color = vec3(0.0f);
//    for(int i = 0; i < 9; i++)
//        color += vec3(texture(screenTexture, uv.st + offsets[i])) * kernel[i];
//    return color;
//}
//
//vec3 ToneMapping(vec3 color)
//{
//    float exposure = 2;
//
//    vec3 mapped = vec3(1.0f) - exp(-color * exposure);
//    return mapped;
//}

void main()
{
	if ((abs(texCoords.x - 0.5) * aspectRatio < 0.001 && abs(texCoords.y - 0.5) < 0.015) || (abs(texCoords.y - 0.5) < 0.001 && abs(texCoords.x - 0.5) * aspectRatio < 0.015))
	{
		fragColor = vec4(1.0, 1.0, 1.0, 1.0);
	}
	else
	{
		fragColor = texture(screenTexture, texCoords);
	}
}