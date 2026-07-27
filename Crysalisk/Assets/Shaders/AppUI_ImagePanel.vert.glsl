#version 450

// Vertex input
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexcoord;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
} global_ubo;

struct VSOutput
{
	vec2 texCoord;
};

layout(location = 0) out VSOutput vsOut;

void main()
{
	// Flip the y-coord around
	vsOut.texCoord = vec2(inTexcoord.x, 1.0 - inTexcoord.y);

	gl_Position = global_ubo.projection * vec4(inPosition.x, inPosition.y, 0.0, 1.0);
}