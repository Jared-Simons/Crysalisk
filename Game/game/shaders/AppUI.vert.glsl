#version 450

// Vertex input
layout(location = 0) in vec2 inPosition;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
} global_ubo;

void main()
{
	// Flip the y-coord around
	gl_Position = global_ubo.projection * vec4(inPosition.x, inPosition.y, 0.0, 1.0);
}