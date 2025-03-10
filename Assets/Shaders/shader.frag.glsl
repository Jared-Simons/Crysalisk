#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec3 inFragPos;

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
	mat4 projection;
	mat4 view;
	vec3 viewPos;
} global_ubo;

layout(set = 1, binding = 0) uniform InstanceUniformBuffer
{
	vec4 color;
} instance_ubo;

// Instance texture samplers
const int SAMP_DIFFUSE = 0;
layout(set = 1, binding = 1) uniform sampler2D samplers[1];

void main()
{
	// Ambient light.
	vec3 dirLightPos = vec3(-60.0, 200.0, 0.0);
	vec3 lightColor = vec3(1.0, 1.0, 1.0);
	float ambientStrength = 0.1;
	vec3 ambient = ambientStrength * lightColor;

	// Diffuse light.
	vec3 normal = normalize(inNormal);
	vec3 lightDir = normalize(dirLightPos - inFragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = diff * lightColor;

	// Specular light.
	float specularStrength = 0.5;
	vec3 viewDir = normalize(global_ubo.viewPos - inFragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
	vec3 specular = specularStrength * spec * lightColor;
	
	// Diffuse sample.
	vec4 diffuseSample = texture(samplers[SAMP_DIFFUSE], fragTexCoord);

	vec3 result = (ambient + diffuse + specular) * diffuseSample.xyz;
	outColor = vec4(result, 1.0);
}