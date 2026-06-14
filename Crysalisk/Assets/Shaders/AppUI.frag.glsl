#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
} global_ubo;

layout(set = 1, binding = 0) uniform InstanceUniformBuffer
{
	vec4 color;
} instance_ubo;

void main()
{
	outColor = instance_ubo.color;
}