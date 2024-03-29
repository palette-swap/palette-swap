#version 330

// Input attributes
layout (location = 0) in vec2 in_position;

// Passed to fragment shader
out vec2 texcoord;

void main()
{
    gl_Position = vec4(in_position, 0, 1.0);
	texcoord = (in_position.xy + 1) / 2.f;
}