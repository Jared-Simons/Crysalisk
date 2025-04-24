#version 450

layout(location = 0) out vec4 outColor;

// NOTE: Must match VSOut struct in corresponding vertex shaders.
struct FSIn
{
	vec4 Color;
	vec2 TexCoord;
};
layout(location = 0) in FSIn fsInput;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
	mat4 view;
} global_ubo;

layout(set = 1, binding = 1) uniform sampler2D textMap;

void main()
{
	vec4 textSample = texture(textMap, fsInput.TexCoord);
	outColor = vec4(textSample.rgb, 1.0);
}