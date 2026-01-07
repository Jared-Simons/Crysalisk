#version 450

layout (location = 0) out vec4 outFragColor;

layout(push_constant) uniform pushConstants_t
{
	int	iHovered;
} pushConstants;
 
void main() 
{
	if (pushConstants.iHovered == 1)
		outFragColor = vec4(0.0, 1.0, 0.0, 0.0);
	else
		outFragColor = vec4(1.0);
}
