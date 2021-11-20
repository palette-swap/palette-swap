#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;

const float border = 16;

// Output color
layout(location = 0) out  vec4 color;

void main()
{
	vec2 dist = abs(texcoord - vec2(.5)) * (textureSize(sampler0, 0) + vec2(2 * border));
	vec2 max_size = textureSize(sampler0, 0) / 2.f;
	vec2 difference = max_size - dist;
	if(difference.x >= 0 && difference.y >= 0) {
		vec4 text_color = texture(sampler0, (texcoord * (textureSize(sampler0, 0) + vec2(2 * border)) - (textureSize(sampler0, 0) + vec2(border))) / (textureSize(sampler0, 0)));
		// Alpha blending the transparent text over a black background
		color = vec4(vec3(text_color)*text_color.a + vec3(0)*(1-text_color.a), 1);
	} else if (difference.x <= 0 && difference.y <= 0 &&
			difference.x * difference.x + difference.y * difference.y >= border * border) {
		discard;
	} else {
		color = vec4(0, 0, 0, 1.0);
	}
	color *= vec4(fcolor, 1.0);
}
