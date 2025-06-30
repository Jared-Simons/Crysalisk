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
vec3 lightDir = vec3(-0.577350, -0.577350, 0.577350);
vec3 dirLightAmbient = vec3(0.05, 0.05f, 0.05f);
vec3 lightColor = vec3(0.4f, 0.4f, 0.3f);

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
		light2.vecPosition = vec3(133, 115, 165);
		light2.lightColor = vec3(1.0f, 0.0f, 0.0f);
		light2.flConstant = 1.0f;
		light2.flLinear = 0.35f;
		light2.flQuadratic = 0.44f;

		lightingResult += CalculatePointLight(light2, normal, fsInput.FragPos, vecViewDir);
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
	vec3 diffuseSample = vec3(texture(samplers[SAMP_DIFFUSE], fsInput.TexCoord));
	vec3 specularSample = vec3(texture(samplers[SAMP_SPECULAR], fsInput.TexCoord));

	vec3 vecFragToLight = normalize(-lightDir);

	// Diffuse lighting.
	float diffuseFactor = (max(dot(normal, vecFragToLight), 0.0));

	// Specular lighting.
	vec3 vecHalfwayDir = normalize(lightDir + viewDir);
	float specularFactor = pow(max(dot(normal, vecHalfwayDir), 0.0), instance_ubo.shininess);

	vec3 ambientLight = dirLightAmbient;
	vec3 diffuseLight = lightColor * diffuseFactor;
	vec3 specularLight = vec3(specularFactor);

	if (global_ubo.renderMode == 2)
	{
		return ambientLight + diffuseLight + specularLight;
	}
	else if (global_ubo.renderMode == 3)
	{
		return diffuseLight;
	}
	else if (global_ubo.renderMode == 4)
	{
		return specularLight;
	}

	ambientLight *= diffuseSample;
	diffuseLight *= diffuseSample;
	specularLight *= specularSample;

	return vec3(ambientLight + diffuseLight + specularLight);
}

vec3 CalculatePointLight(PointLight_t pLight, vec3 normal, vec3 fragPos, vec3 viewDir)
{
	vec3 diffuseSample = vec3(texture(samplers[SAMP_DIFFUSE], fsInput.TexCoord));
	vec3 specularSample = vec3(texture(samplers[SAMP_SPECULAR], fsInput.TexCoord));

	vec3 vecLightDir = normalize(pLight.vecPosition - fragPos);

	// Diffuse lighting.
	float diffuseFactor = max(dot(normal, vecLightDir), 0.0);

	// Specular lighting.
	vec3 vecHalfwayDir = normalize(lightDir + viewDir);
	float specularFactor = pow(max(dot(normal, vecHalfwayDir), 0.0), instance_ubo.shininess);

	// Calculate attenuation factors.
	float distance = length(pLight.vecPosition - fragPos);
	float attenuation = 1.0 / (pLight.flConstant + pLight.flLinear * distance + pLight.flQuadratic * (distance * distance));

	// Light results.
	vec3 ambient = dirLightAmbient;
	vec3 diffuse = pLight.lightColor * diffuseFactor;
	vec3 specular = vec3(specularFactor);

	// Attenuate results
	ambient *= attenuation;
	diffuse *= attenuation;
	specular *= attenuation;

	if (global_ubo.renderMode == 2)
	{
		return ambient + diffuse + specular;
	}
	else if (global_ubo.renderMode == 3)
	{
		vec3 lightVal = diffuse; // Return this WITHOUT the texture data, just lighting data.
	}
	else if (global_ubo.renderMode == 4)
	{
		return specular;
	}

	ambient *= diffuseSample;
	diffuse *= diffuseSample;
	specular *= specularSample;

	return vec3(ambient + diffuse + specular);
}