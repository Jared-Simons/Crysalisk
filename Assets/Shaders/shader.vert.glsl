#version 450

vec2 positions[3] = vec2[](
	vec2(0.0, -0.5),
	vec2(0.5, 0.5),
	vec2(-0.5, 0.5)
);

// Vertex input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outNormal;
layout(location = 3) out vec3 outTangent;
layout(location = 4) out vec3 outBitangent;
layout(location = 5) out vec3 outFragPos;

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
	outTexCoord = fragTexCoord;
	outNormal = mat3(transpose(inverse(u_PushConstants.model))) * inNormal;
	outTangent = normalize(vec3(u_PushConstants.model * vec4(inTangent, 0.0)));
	outBitangent = normalize(vec3(u_PushConstants.model * vec4(inBitangent, 0.0)));
	outFragPos = vec3(u_PushConstants.model * vec4(inPosition, 1.0));

	gl_Position = global_ubo.projection * global_ubo.view * u_PushConstants.model * vec4(inPosition, 1.0);
}