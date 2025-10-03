#version 450

layout(location = 0) out vec4 outColor;

// NOTE: Must match VSOut struct in corresponding vertex shaders.
struct FSIn
{
	int iDummyVar;
};

layout(location = 0) flat in FSIn fsInput;

void main()
{
	outColor = vec4(1.0, 0.0, 0.0, 1.0);
}