#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform global_object
{
	mat4 view;
	mat4 proj;
} global_ubo;

layout(set = 1, binding = 0) uniform local_object
{
    mat4 model;
} instance_ubo;

void main() 
{
	gl_Position = global_ubo.proj * global_ubo.view * instance_ubo.model * vec4(inPosition, 1.0);
	fragColor = inColor;
	fragTexCoord = inTexCoord;
}