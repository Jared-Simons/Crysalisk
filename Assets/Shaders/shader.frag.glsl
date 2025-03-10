#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;
layout(location = 5) in vec3 inFragPos;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
	mat4 view;
	vec3 viewPos;
	vec3 dirLightPos;
	int	 renderMode;
} global_ubo;

layout(set = 1, binding = 0) uniform InstanceUniformBuffer
{
	vec4 color;
} instance_ubo;

// Instance texture samplers
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
layout(set = 1, binding = 1) uniform sampler2D samplers[3];

mat3 TBN;

void main()
{
	vec3 normal = normalize(inNormal);
	TBN = mat3(inTangent, inBitangent, inNormal);

	// Sample the normal map.
	normal = texture(samplers[SAMP_NORMAL], fragTexCoord).rgb;
	normal = normalize(normal * 2.0 - 1.0);
	normal = normalize(TBN * normal);

	if (global_ubo.renderMode == 5)
	{
		normal = normalize(inNormal);
	}

	// ambient
	vec3 ambient = vec3(0.5, 0.5, 0.5) * texture(samplers[SAMP_DIFFUSE], fragTexCoord).rgb;

	// diffuse
	//vec3 lightPosition = vec3(1.2, 1.0, 2.0);
	vec3 lightDir = normalize(global_ubo.dirLightPos - inFragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 lightDiffuse = vec3(0.3, 0.5, 1.0);
	vec3 diffuse = lightDiffuse * diff * texture(samplers[SAMP_DIFFUSE], fragTexCoord).rgb;

	// specular
	vec3 viewDir = normalize(global_ubo.viewPos - inFragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 lightSpecular = vec3(1.0);
	vec3 specular = lightSpecular * spec * texture(samplers[SAMP_SPECULAR], fragTexCoord).rgb;

	vec3 result = ambient + diffuse + specular;
	if (global_ubo.renderMode == 0)
	{
		outColor = vec4(result, 1.0);
		return;
	}
	else
	{
		if (global_ubo.renderMode == 1)
		{
			outColor = vec4(ambient, 1.0);
			return;
		}
		else if(global_ubo.renderMode == 2)
		{
			outColor = vec4(diffuse, 1.0);
			return;
		}
		else if (global_ubo.renderMode == 3)
		{
			outColor = vec4(specular, 1.0);
			return;
		}
		else if (global_ubo.renderMode == 4)
		{
			outColor = vec4(abs(normal), 1.0);
			return;
		}
		else
		{
			outColor = vec4(result, 1.0);
			return;
		}
	}
}