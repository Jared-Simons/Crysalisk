#version 450

layout(location = 0) out vec4 outColor;

layout(set = 1, binding = 0) uniform InstanceUniformBuffer
{
	vec4 color;
} instance_ubo;

void main()
{
	outColor = instance_ubo.color;
}