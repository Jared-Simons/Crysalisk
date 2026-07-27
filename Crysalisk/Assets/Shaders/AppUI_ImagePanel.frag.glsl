#version 450

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
} global_ubo;

layout(set = 1, binding = 0) uniform InstanceUniformBuffer
{
	vec4 color;
	uint instance_uses_textures;
} instance_ubo;

layout(set = 1, binding = 1) uniform sampler samp;
layout(set = 1, binding = 2) uniform texture2D tex;

struct FSInput
{
	vec2 texCoord;
};

layout(location = 0) in FSInput fsIn;

void main()
{
	outColor = texture(sampler2D(tex, samp), fsIn.texCoord);
}