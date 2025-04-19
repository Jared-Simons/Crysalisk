#version 450

layout(location = 0) out vec4 outColor;

// NOTE: Must match VSOut struct in corresponding vertex shaders.
struct FSIn
{
	vec2 TexCoord;
	vec3 Normal;
	vec3 Tangent;
	vec3 Bitangent;
	vec3 FragPos;
	mat3 TBN;
};
layout(location = 0) in FSIn fsInput;

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
	int shininess;
	vec3 color;
} instance_ubo;

// Instance texture samplers
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
layout(set = 1, binding = 1) uniform sampler2D samplers[3];

mat3 TBN;

void main()
{
	vec3 normal = normalize(fsInput.Normal);
	vec3 tangentLightPos = fsInput.TBN * global_ubo.dirLightPos;
	vec3 tangentViewPos = fsInput.TBN * global_ubo.viewPos;
	vec3 tangentFragPos = fsInput.TBN * fsInput.FragPos;
	TBN = mat3(fsInput.Tangent, fsInput.Bitangent, normal);

	// Sample the normal map.
	normal = texture(samplers[SAMP_NORMAL], fsInput.TexCoord).rgb;
	normal = normalize(normal * 2.0 - 1.0);
	normal = normalize(fsInput.TBN * normal);

	if (global_ubo.renderMode == 5)
	{
		normal = normalize(fsInput.Normal);
	}

	// ambient
	vec3 ambient = vec3(0.5, 0.5, 0.5) * texture(samplers[SAMP_DIFFUSE], fsInput.TexCoord).rgb;

	// diffuse
	//vec3 lightPosition = vec3(1.2, 1.0, 2.0);
	vec4 diffuseSample = texture(samplers[SAMP_DIFFUSE], fsInput.TexCoord);
	vec3 lightDir = normalize(tangentLightPos - tangentFragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 lightDiffuse = vec3(0.3, 0.5, 1.0);
	vec3 diffuse = lightDiffuse * diff * diffuseSample.rgb;

	// specular
	vec3 viewDir = normalize(tangentViewPos - tangentFragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), instance_ubo.shininess);
	vec3 lightSpecular = vec3(1.0);
	vec3 specular = lightSpecular * spec * texture(samplers[SAMP_SPECULAR], fsInput.TexCoord).rgb;

	vec3 result = ambient + diffuse + specular;
	if (global_ubo.renderMode == 0)
	{
		outColor = vec4(result, diffuseSample.a);
		return;
	}
	else
	{
		if (global_ubo.renderMode == 1)
		{
			outColor = texture(samplers[SAMP_DIFFUSE], fsInput.TexCoord);
			return;
		}
		else if (global_ubo.renderMode == 2)
		{
			outColor = vec4(diffuse, diffuseSample.a);
			return;
		}
		else if (global_ubo.renderMode == 3)
		{
			outColor = vec4(specular, diffuseSample.a);
			return;
		}
		else if (global_ubo.renderMode == 4)
		{
			outColor = texture(samplers[SAMP_NORMAL], fsInput.TexCoord);
			outColor.a = diffuseSample.a;
			return;
		}
		else if (global_ubo.renderMode == 5)
		{
			outColor = vec4(abs(normal), diffuseSample.a);
			return;
		}
	}
}