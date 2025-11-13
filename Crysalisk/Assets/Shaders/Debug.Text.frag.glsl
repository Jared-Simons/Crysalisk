#version 450

layout(location = 0) out vec4 outColor;

// NOTE: Must match VSOut struct in corresponding vertex shaders.
struct FSIn
{
	vec2 TexCoord;
};

layout(set = 1, binding = 0) uniform InstanceUniformBuffer
{
	vec4 color;
} instance_ubo;

layout(set = 1, binding = 1) uniform sampler inSampler;
layout(set = 1, binding = 2) uniform texture2D inTexture;

layout(location = 0) in FSIn fsInput;

void main()
{
	vec4 textureSample = texture(sampler2D(inTexture, inSampler), fsInput.TexCoord);

	outColor = vec4(textureSample.r, textureSample.r, textureSample.r, textureSample.r);
}