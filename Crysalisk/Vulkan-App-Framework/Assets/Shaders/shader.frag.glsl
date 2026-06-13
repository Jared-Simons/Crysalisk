#version 450

layout (location = 0) out vec4 outFragColor;

// Fragment input
layout(location = 0) in vec4 vertColor;
 
void main() 
{
	outFragColor = vertColor;
}
