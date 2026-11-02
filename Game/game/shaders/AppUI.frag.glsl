#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
} global_ubo;

layout(set = 1, binding = 0) uniform InstanceUniformBuffer
{
	vec4 color;
	uint instance_uses_textures;
} instance_ubo;

layout(set = 1, binding = 1) uniform sampler samp;
layout(set = 1, binding = 2) uniform texture2D tex;

void main()
{
	if (instance_ubo.instance_uses_textures > 0)
	{
		outColor = vec4(1.0);
	}
	else
	{
		outColor = instance_ubo.color;
	}
}