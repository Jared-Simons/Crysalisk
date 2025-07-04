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
	float shininess;
	vec3 color;
} instance_ubo;

// Instance texture samplers
const int SAMP_DIFFUSE = 0;
const int SAMP_SPECULAR = 1;
const int SAMP_NORMAL = 2;
layout(set = 1, binding = 1) uniform sampler2D samplers[3];

// Temp lighting values
float ambientStrength = 0.3;
vec3 vecDirLightDirection = vec3(-0.2f, -1.0f, -0.3f);
vec3 dirLightAmbient = vec3(0.592, 0.679, 0.812);
vec3 lightColor = vec3(1.0f, 0.785f, 0.534f);

// Point light test
// 605 130 -748
struct PointLight_t
{
	vec3 vecPosition;
	vec3 lightColor;

	float flConstant;
	float flLinear;
	float flQuadratic;
};
// End Temp 

// Light calculations.
vec3 CalculateDirectionalLight(vec3 normal, vec3 viewDir);
vec3 CalculatePointLight(PointLight_t pLight, vec3 normal, vec3 fragPos, vec3 viewDir);

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
	vec3 normalMapSamp = 2.0 * texture(samplers[SAMP_NORMAL], fsInput.TexCoord).rgb - 1.0;
	normal = normalize(TBN * normalMapSamp);

	vec4 diffuseSample = texture(samplers[SAMP_DIFFUSE], fsInput.TexCoord);

	vec3 lightingResult = CalculateDirectionalLight(normal, vecViewDir);

	// Point light tests.
	{
		PointLight_t light1;
		light1.vecPosition = vec3(0.7f, 0.2f, 2.0f);
		light1.lightColor = vec3(0.8f, 0.8f, 0.8f);
		light1.flConstant = 1.0f;
		light1.flLinear = 0.09f;
		light1.flQuadratic = 0.032f;

		lightingResult += CalculatePointLight(light1, normal, fsInput.FragPos, vecViewDir);

		PointLight_t light2;
		light2.vecPosition = vec3(2.3f, 0.2f, -4.0f);
		light2.lightColor = vec3(1.0f, 0.0f, 0.0f);
		light2.flConstant = 1.0f;
		light2.flLinear = 0.09f;
		light2.flQuadratic = 0.032f;

		lightingResult += CalculatePointLight(light2, normal, fsInput.FragPos, vecViewDir);

		PointLight_t light3;
		light3.vecPosition = vec3(-4.0f, 0.2f, -12.0f);
		light3.lightColor = vec3(0.0f, 1.0f, 0.0f);
		light3.flConstant = 1.0f;
		light3.flLinear = 0.09f;
		light3.flQuadratic = 0.032f;

		lightingResult += CalculatePointLight(light3, normal, fsInput.FragPos, vecViewDir);

		PointLight_t light4;
		light4.vecPosition = vec3(0.0f, 0.0f, -3.0f);
		light4.lightColor = vec3(0.0f, 0.0f, 1.0f);
		light4.flConstant = 1.0f;
		light4.flLinear = 0.09f;
		light4.flQuadratic = 0.032f;

		lightingResult += CalculatePointLight(light4, normal, fsInput.FragPos, vecViewDir);
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
		vec4 normal4 = texture(samplers[SAMP_NORMAL], fsInput.TexCoord);
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
	
	outColor = vec4(lightingResult, 1.0);
}

vec3 CalculateDirectionalLight(vec3 normal, vec3 viewDir)
{
	vec3 vecLightDir = normalize(-vecDirLightDirection);

	// Diffuse shading
	float flDiffuseFactor = max(dot(normal, vecLightDir), 0.0);

	// Specular shading
	vec3 vecHalfway = normalize(vecLightDir + viewDir);
	float flSpecularFactor = pow(max(dot(normal, vecHalfway), 0.0), instance_ubo.shininess);

	// Composite results.
	vec3 ambientLight = dirLightAmbient * ambientStrength;
	vec3 diffuseLight = lightColor * flDiffuseFactor;
	vec3 specularLight = lightColor * flSpecularFactor;

	// Multiply in texture samples.
	vec3 diffuseSample = vec3(texture(samplers[SAMP_DIFFUSE], fsInput.TexCoord));
	vec3 specularSample = vec3(texture(samplers[SAMP_SPECULAR], fsInput.TexCoord));
	
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

vec3 CalculatePointLight(PointLight_t pLight, vec3 normal, vec3 fragPos, vec3 viewDir)
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

	// Multiply in texture samples.
	vec3 diffuseSample = vec3(texture(samplers[SAMP_DIFFUSE], fsInput.TexCoord));
	vec3 specularSample = vec3(texture(samplers[SAMP_SPECULAR], fsInput.TexCoord));

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