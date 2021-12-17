#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform float opacity = 1;
uniform float time;
uniform float time_period = 4;
uniform float color_amplitude = 0.3;
// Output color
layout(location = 0) out  vec4 color;

void main()
{
	vec3 color_variation = vec3(1,1,1) * (1 + color_amplitude * sin(time/time_period));
	color = vec4(color_variation,1.0) * texture(sampler0, vec2(texcoord.x, texcoord.y));
}
