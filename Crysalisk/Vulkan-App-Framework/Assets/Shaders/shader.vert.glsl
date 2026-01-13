#version 450

// Vertex input
layout(location = 0) in vec2 inPosition;

// Vertex output
layout(location = 0) out vec4 vertColor;

layout(push_constant) uniform pushConstants_t
{
	int	iHovered;
} pushConstants;

void main() 
{
	if (pushConstants.iHovered == 1)
		vertColor = vec4(0.0, 1.0, 0.0, 0.0);
	else
		vertColor = vec4(1.0);

	gl_Position = vec4(inPosition, 0.0, 1.0f);
}
