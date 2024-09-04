#version 460 core

out vec4 fragColor;

in vec2 uv;
flat in uint textureID;

uniform sampler2DArray textures;
uniform vec3 textColor;

void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(textures, vec3(uv, textureID)).r);
    fragColor = vec4(textColor, 1.0) * sampled;
}  