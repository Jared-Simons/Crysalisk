#version 450

// Vertex input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

// NOTE: Must match FSIn in the corresponding fragment shaders.
struct VSOut
{
	vec2 TexCoord;
	vec3 Normal;
	vec3 Tangent;
	vec3 Bitangent;
	vec3 FragPos;
};

layout(location = 0) out VSOut vsOutput;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
	mat4 view;
	vec3 viewPos;
	vec3 dirLightPos;
	int	 renderMode;
} global_ubo;

layout(push_constant) uniform push_constants
{
	mat4 model;
} u_PushConstants;

void main()
{
	vsOutput.TexCoord = fragTexCoord;
	vsOutput.Normal = mat3(transpose(inverse(u_PushConstants.model))) * inNormal;
	vsOutput.Tangent = normalize(vec3(u_PushConstants.model * vec4(inTangent, 0.0)));
	vsOutput.Bitangent = normalize(vec3(u_PushConstants.model * vec4(inBitangent, 0.0)));
	vsOutput.FragPos = vec3(u_PushConstants.model * vec4(inPosition, 1.0));

	gl_Position = global_ubo.projection * global_ubo.view * u_PushConstants.model * vec4(inPosition, 1.0);
}