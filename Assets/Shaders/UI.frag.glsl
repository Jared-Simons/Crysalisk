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
	vec4 textureSample = texture(textMap, fsInput.TexCoord);
	
	// If the texture is coming in as a single channel texture, then we need to convert it to a vec4 to make it white,
	// then we can tint the text accordingly.
	vec4 textSample = vec4(textureSample.r, textureSample.r, textureSample.r, textureSample.r);
	// We use the red channel for alpha blending since we're expecting this to be a single channel texture anyways.
	outColor = vec4(textSample.rgb, textSample.r) * fsInput.Color;
}