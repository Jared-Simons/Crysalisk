#version 450

// TODO: Store these in a shared header or something.
struct DirectionalLight_t
{
	vec3	vecDirection;
	vec3	lightColor;
	float	flBrightness;
	vec3	ambientLightColor;
	float	flAmbientStrength;
};

struct PointLight_t
{
	vec3 vecPosition;
	vec3 lightColor;
	float flBrightness;

	float flConstant;
	float flLinear;
	float flQuadratic;
};

struct SpotLight_t
{
	vec3	vecPosition;
	vec3	vecDirection;
	vec3	lightColor;
	float	flBrightness;

	float	flConstant;
	float	flLinear;
	float	flQuadratic;
	float	flInnerCutoff;
	float	flOuterCutoff;
};

// Vertex input
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in vec3 inTangent;

// NOTE: Must match FSIn in the corresponding fragment shaders.
struct VSOut
{
	vec2 TexCoord;
	vec3 Normal;
	vec3 Tangent;
	vec3 Bitangent;
	vec3 FragPos;
	mat3 TBN;
};

layout(location = 0) out VSOut vsOutput;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
	mat4 view;
	vec3 viewPos;
	DirectionalLight_t dirLight;
	PointLight_t pointLight1;
	PointLight_t pointLight2;
	PointLight_t pointLight3;
	PointLight_t pointLight4;
	SpotLight_t	 spotlight;
	int	 renderMode;
} global_ubo;

layout(push_constant) uniform push_constants
{
	mat4 model;
} u_PushConstants;

void main()
{
	vsOutput.TexCoord = fragTexCoord;
	
	mat3 modelMat = mat3(u_PushConstants.model);
	vsOutput.Normal = normalize(modelMat * inNormal);
	vsOutput.Tangent = normalize(modelMat * inTangent);
	vsOutput.Bitangent = cross(vsOutput.Normal, vsOutput.Tangent);
	vsOutput.FragPos = vec3(u_PushConstants.model * vec4(inPosition, 1.0));

	gl_Position = global_ubo.projection * global_ubo.view * u_PushConstants.model * vec4(inPosition, 1.0);
}