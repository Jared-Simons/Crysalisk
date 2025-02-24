#version 450

vec2 positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

// Vertex input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 1) out vec2 outTexCoord;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer 
{
	mat4 projection;
	mat4 view;
	mat4 reserved0;
	mat4 reserved1;
} global_ubo;

layout(push_constant) uniform push_constants
{
	mat4 model;
} u_PushConstants;

void main()
{
	gl_Position = global_ubo.projection * global_ubo.view * u_PushConstants.model * vec4(inPosition, 1.0);
	outTexCoord = fragTexCoord;
}