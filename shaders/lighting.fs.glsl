#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D screen;
uniform sampler2D lighting;

// Output color
layout(location = 0) out  vec4 color;

void main()
{
	color = texture(screen, vec2(texcoord.x, texcoord.y)) * texture(lighting, vec2(texcoord.x, texcoord.y));
}