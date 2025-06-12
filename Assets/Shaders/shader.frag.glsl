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
vec3 lightColor = vec3(0.93, 0.94, 0.65);
vec3 lightDir = vec3(-0.577350, -0.577350, -0.577350);
// End Temp 

void main()
{
	// Build our TBN matrix.
	vec3 normal = fsInput.Normal;
	vec3 tangent = fsInput.Tangent;
	tangent = (tangent - dot(tangent, normal) * normal);
	vec3 bitangent = cross(fsInput.Normal, fsInput.Tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	// Get a normal from the normal map.
	normal = texture(samplers[SAMP_NORMAL], fsInput.TexCoord).rgb;
	normal = normalize(normal * 2.0 - 1.0);
	
	// Bring our normals into world space.
	normal = normalize(TBN * normal);

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
	float diffuseFactor = max(dot(normal, -lightDir), 0.0);
	vec3 diffuse = diffuseFactor * lightColor;
	
	if (global_ubo.renderMode == 3) // Diffuse only.
	{
		outColor = vec4(diffuse, 1.0);
		return;
	}
	
	// Specular lighting	
	vec3 viewDir = normalize(global_ubo.viewPos - fsInput.FragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	float specularFactor = pow(max(dot(viewDir, reflectDir), 0.0), instance_ubo.shininess);
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
	
	vec3 test = texture(samplers[SAMP_NORMAL], fsInput.TexCoord).rgb;
	outColor = vec4(test, 1.0);
	vec3 lightingResult = ambient + diffuse + specular;
	
	outColor = diffuseSample * vec4(lightingResult, 1.0);
}