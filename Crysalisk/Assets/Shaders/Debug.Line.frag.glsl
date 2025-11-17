#version 450

layout(location = 0) out vec4 outColor;

// NOTE: Must match VSOut struct in corresponding vertex shaders.
struct FSIn
{
	int iDummyVar;
};

layout(set = 1, binding = 0) uniform InstanceUniformBuffer
{
	vec4 color;
} instance_ubo;

layout(location = 0) flat in FSIn fsInput;

void main()
{
	outColor = instance_ubo.color;
}