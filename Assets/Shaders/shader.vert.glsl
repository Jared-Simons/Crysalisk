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
	mat4 model;
	mat4 reserved;
} global_ubo;

void main()
{
	gl_Position = global_ubo.projection * global_ubo.view * global_ubo.model * vec4(inPosition, 1.0);
	outTexCoord = fragTexCoord;
}