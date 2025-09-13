#version 450

layout(location = 0) out vec4 outColor;

// TODO: Store these in a shared header or something.
struct DirectionalLight_t
{
	vec3	vecDirection;
	vec3	lightColor;
	vec3	ambientLightColor;
	float	flAmbientStrength;
};

struct PointLight_t
{
	vec3 vecPosition;
	vec3 lightColor;

	float flConstant;
	float flLinear;
	float flQuadratic;
};

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
	DirectionalLight_t dirLight;
	PointLight_t pointLight1;
	PointLight_t pointLight2;
	PointLight_t pointLight3;
	PointLight_t pointLight4;
	int	 renderMode;
} global_ubo;

layout(set = 1, binding = 0) uniform InstanceUniformBuffer
{
	float shininess;
	vec3 color;
} instance_ubo;

// Instance texture samplers
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
layout(set = 1, binding = 1) uniform sampler samplers[3];
layout(set = 1, binding = 2) uniform texture2D textures[3];

// Light calculations.
vec3 CalculateDirectionalLight(vec3 normal, vec3 viewDir, vec3 diffuseSample, vec3 specularSample);
vec3 CalculatePointLight(PointLight_t pLight, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseSample, vec3 specularSample);

void main()
{
	// Build our TBN matrix.
	vec3 normal = fsInput.Normal;
	vec3 tangent = fsInput.Tangent;
	tangent = (tangent - dot(tangent, normal) * normal);
	vec3 bitangent = cross(fsInput.Normal, fsInput.Tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	vec3 vecViewDir = normalize(global_ubo.viewPos - fsInput.FragPos);

	// Get a normal from the normal map.
	vec3 normalMapSamp = 2.0 * texture(sampler2D(textures[SAMP_NORMAL], samplers[SAMP_NORMAL]), fsInput.TexCoord).rgb - 1.0;
	normal = normalize(TBN * normalMapSamp);

	vec4 diffuseSample = texture(sampler2D(textures[SAMP_DIFFUSE], samplers[SAMP_DIFFUSE]), fsInput.TexCoord);
	vec4 specularSample = texture(sampler2D(textures[SAMP_SPECULAR], samplers[SAMP_SPECULAR]), fsInput.TexCoord);

	vec3 lightingResult = CalculateDirectionalLight(normal, vecViewDir, diffuseSample.rgb, specularSample.rgb);

	// Point light tests.
	{
		lightingResult += CalculatePointLight(global_ubo.pointLight1, normal, fsInput.FragPos, vecViewDir, diffuseSample.rgb, specularSample.rgb);
		lightingResult += CalculatePointLight(global_ubo.pointLight2, normal, fsInput.FragPos, vecViewDir, diffuseSample.rgb, specularSample.rgb);
		lightingResult += CalculatePointLight(global_ubo.pointLight3, normal, fsInput.FragPos, vecViewDir, diffuseSample.rgb, specularSample.rgb);
		lightingResult += CalculatePointLight(global_ubo.pointLight4, normal, fsInput.FragPos, vecViewDir, diffuseSample.rgb, specularSample.rgb);
	}
	
	if (global_ubo.renderMode == 1)	// Fullbright.
	{
		// Do no lighting calculations for fullbright rendering, boot out now.
		outColor = diffuseSample;
		return;
	}
	
	// Other debug modes.
	if (global_ubo.renderMode == 5) // Normals - tangent space.
	{
		// Use a sample from the normal map.
		vec4 normal4 = texture(sampler2D(textures[SAMP_NORMAL], samplers[SAMP_NORMAL]), fsInput.TexCoord);
		outColor = vec4(normal4.xyz, 1.0);
		return;
	}
	else if (global_ubo.renderMode == 6) // Normals - world space.
	{
		outColor = vec4(abs(normal), 1.0);
		return;
	}
	else if (global_ubo.renderMode == 7) // Geometric normal - world space.
	{
		// Use the normal from the vertex data (duh!)
		outColor = vec4(abs(fsInput.Normal), 1.0);
		return;
	}
	else if (global_ubo.renderMode == 8) // Tangents - world space.
	{
		outColor = vec4(abs(tangent), 1.0);
		return;
	}
	else if (global_ubo.renderMode == 9) // Bitangents - world space.
	{
		outColor = vec4(abs(bitangent), 1.0);
		return;
	}
	
	outColor = vec4(lightingResult, diffuseSample.a);	// Use the diffuse sample for alpha-blending.
}

vec3 CalculateDirectionalLight(vec3 normal, vec3 viewDir, vec3 diffuseSample, vec3 specularSample)
{
	vec3 vecLightDir = normalize(-global_ubo.dirLight.vecDirection);

	// Diffuse shading
	float flDiffuseFactor = max(dot(normal, vecLightDir), 0.0);

	// Specular shading
	vec3 vecHalfway = normalize(vecLightDir + viewDir);
	float flSpecularFactor = pow(max(dot(normal, vecHalfway), 0.0), instance_ubo.shininess);

	// Composite results.
	vec3 ambientLight = global_ubo.dirLight.ambientLightColor * global_ubo.dirLight.flAmbientStrength;
	vec3 diffuseLight = global_ubo.dirLight.lightColor * flDiffuseFactor;
	vec3 specularLight = global_ubo.dirLight.lightColor * flSpecularFactor;
	
	// Don't include diffuse texture data when using debug-lighting modes.
	if (global_ubo.renderMode == 2 || global_ubo.renderMode == 3)
	{
		diffuseSample = vec3(1.0f);
	}

	ambientLight *= diffuseSample;
	diffuseLight *= diffuseSample;
	specularLight *= specularSample;

	if (global_ubo.renderMode == 3)
	{
		return diffuseLight;
	}
	else if (global_ubo.renderMode == 4)
	{
		return specularLight;
	}

	return ambientLight + diffuseLight + specularLight;
}

vec3 CalculatePointLight(PointLight_t pLight, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 diffuseSample, vec3 specularSample)
{
	vec3 vecLightDir = normalize(pLight.vecPosition - fragPos);

	// Diffuse shading
	float flDiffuseFactor = max(dot(normal, vecLightDir), 0.0);

	// Specular shading
	vec3 vecHalfway = normalize(vecLightDir + viewDir);
	float flSpecularFactor = pow(max(dot(normal, vecHalfway), 0.0), instance_ubo.shininess);

	// Compute attenuation term.
	float flDistance = length(pLight.vecPosition - fragPos);
	float flAttenuation = 1.0 / (pLight.flConstant + pLight.flLinear * flDistance + pLight.flQuadratic * (flDistance * flDistance));

	// Composite results.
	vec3 diffuseLight = pLight.lightColor * flDiffuseFactor;
	vec3 specularLight = pLight.lightColor * flSpecularFactor;

	// Apply attenuation.
	diffuseLight *= flAttenuation;
	specularLight *= flAttenuation;

	// Don't include diffuse texture data when using debug-lighting modes.
	if (global_ubo.renderMode == 2 || global_ubo.renderMode == 3)
	{
		diffuseSample = vec3(1.0f);
	}

	diffuseLight *= diffuseSample;
	specularLight *= specularSample;

	if (global_ubo.renderMode == 3)
	{
		return diffuseLight;
	}
	else if (global_ubo.renderMode == 4)
	{
		return specularLight;
	}

	return diffuseLight + specularLight;
}