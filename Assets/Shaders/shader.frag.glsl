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

// Temp lighting values
float ambientStrength = 0.3;
vec3 lightColor = vec3(0.93, 0.94, 0.65);
vec3 lightPosition = vec3(0, 500, -500);
// End Temp 

void main()
{
	vec3 normal = normalize(fsInput.Normal);
	
	vec3 lightDir = normalize(lightPosition - fsInput.FragPos);

	vec4 diffuseSample = texture(samplers[SAMP_DIFFUSE], fsInput.TexCoord);
	
	if (global_ubo.renderMode == 1)	// Fullbright.
	{
		// Do no lighting calculations for fullbright rendering, boot out now.
		outColor = diffuseSample;
		return;
	}
	
	// Ambient lighting
	vec3 ambient = ambientStrength * lightColor;
	
	if (global_ubo.renderMode == 2) // Ambient only.
	{
		outColor = vec4(ambient, 1.0);
		return;
	}
	
	// Diffuse lighting 
	float diffuseFactor = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diffuseFactor * lightColor;
	
	if (global_ubo.renderMode == 3) // Diffuse only.
	{
		outColor = vec4(diffuse, 1.0);
		return;
	}
	
	// Specular lighting
	vec3 viewDir = normalize(global_ubo.viewPos - fsInput.FragPos);
	vec3 halfwayVector = normalize(lightDir + viewDir);
	//float specularFactor = pow(max(dot(normal, halfwayVector), 0.0), instance_ubo.shininess);
	float specularFactor = pow(max(dot(normal, halfwayVector), 0.0), 10);
	vec3 lightColorSpecular = vec3(1.0);
	vec3 specular = lightColorSpecular * specularFactor * texture(samplers[SAMP_SPECULAR], fsInput.TexCoord).rgb;
	
	if (global_ubo.renderMode == 4) // Specular only.
	{
		outColor = vec4(specular, 1.0);
		return;
	}
	
	// Other debug modes.
	if (global_ubo.renderMode == 5) // Normals - tangent space.
	{
		// TODO: Make this work with normal maps.
	}
	else if (global_ubo.renderMode == 6) // Normals - world space .
	{
		// TODO: Make this work with normal maps.
	}
	else if (global_ubo.renderMode == 7) // Geometric normal - world space.
	{
		outColor = vec4(abs(normal), 1.0);
		return;
	}
	
	vec3 test = texture(samplers[SAMP_NORMAL], fsInput.TexCoord).rgb;
	outColor = vec4(test, 1.0);
	vec3 lightingResult = ambient + diffuse + specular;
	
	outColor = diffuseSample * vec4(lightingResult, 1.0);
}