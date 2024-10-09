#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform local_object
{
    vec4 color;
    vec4 reserved1;
    vec4 reserved2;
    vec4 reserved3;
} instance_ubo;

// Instance texture uniforms
const int SAMP_DIFFUSE = 0;
layout(set = 1, binding = 1) uniform sampler2D samplers[1];

void main() 
{
    //outColor = instance_ubo.color;
    outColor = texture(samplers[SAMP_DIFFUSE], fragTexCoord);
}