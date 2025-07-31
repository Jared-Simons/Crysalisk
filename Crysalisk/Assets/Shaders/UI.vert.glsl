#version 450

// Vertex input
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;

// NOTE: Must match FSIn in the corresponding fragment shaders.
struct VSOut
{
	vec4 Color;
	vec2 TexCoord;
};

layout(location = 0) out VSOut vsOutput;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
	mat4 view;
} global_ubo;

layout(push_constant) uniform push_constants
{
	mat4 model;
} u_PushConstants;

void main()
{
	vsOutput.Color = inColor;
	vsOutput.TexCoord = inTexCoord;
	
	// Flip the y-coord around
	gl_Position = vec4(inPosition.x, -inPosition.y, 0.0, 1.0);
}