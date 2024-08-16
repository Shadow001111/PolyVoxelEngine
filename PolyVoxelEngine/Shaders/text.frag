#version 330 core
in vec2 uv;
out vec4 fragColor;

uniform sampler2D texture_;
uniform vec3 textColor;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(texture_, uv).r);
    fragColor = vec4(textColor, 1.0) * sampled;
}  