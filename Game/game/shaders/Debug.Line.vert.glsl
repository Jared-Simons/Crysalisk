#version 450

// Vertex input
layout(location = 0) in vec3 inPosition;

// NOTE: Must match FSIn in the corresponding fragment shaders.
struct VSOut
{
	vec4 vertColor;
};

layout(location = 0) out VSOut vsOutput;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
	mat4 view;
} global_ubo;

layout(set = 1, binding = 0) uniform InstanceUniformBuffer
{
	vec4 color;
} instance_ubo;

layout(push_constant) uniform push_constants
{
	mat4 model;
} u_PushConstants;

void main()
{
	vsOutput.vertColor = instance_ubo.color;
	gl_Position = global_ubo.projection * global_ubo.view * u_PushConstants.model * vec4(inPosition, 1.0);
}