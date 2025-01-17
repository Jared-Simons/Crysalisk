#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) in vec2 fragTexCoord;

// Instance texture samplers
const int SAMP_DIFFUSE = 0;
layout(set = 0, binding = 1) uniform sampler2D samplers[1];

void main()
{
	//outColor = vec4(1.0, 0.0, 0.0, 1.0);
	outColor = texture(samplers[SAMP_DIFFUSE], fragTexCoord);
}