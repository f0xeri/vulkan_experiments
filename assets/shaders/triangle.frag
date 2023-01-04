#version 450

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 outFragColor;

layout(set = 1, binding = 0) uniform sampler2D tex1;
layout(set = 1, binding = 1) uniform sampler2D tex2;
void main()
{
    vec3 color = texture(tex1, texCoord).xyz;
    //vec3 color = inColor;
    outFragColor = vec4(color, 1.0f);
}