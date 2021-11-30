#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D screen;
uniform sampler2D los;
uniform sampler2D lighting;

// Output color
layout(location = 0) out  vec4 color;

void main()
{
	vec4 los_sample = texture(los, vec2(texcoord.x, texcoord.y));
	vec4 lighting_sample = texture(lighting, vec2(texcoord.x, texcoord.y));
	color = texture(screen, vec2(texcoord.x, texcoord.y)) * vec4(min(los_sample.rgb, lighting_sample.rgb), 1);
}