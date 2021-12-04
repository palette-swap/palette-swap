#version 330

// From vertex shader
in vec2 texcoord;
in vec3 pos;

// Application data
uniform sampler2D sampler0;
uniform sampler2D lighting;
uniform bool use_lighting;
uniform vec3 fcolor;
uniform float opacity = 1;

// Output color
layout(location = 0) out  vec4 color;

void main()
{
	color = vec4(fcolor, opacity) * texture(sampler0, vec2(texcoord.x, texcoord.y));
	if(use_lighting) {
		vec4 light_level = texture(lighting, (pos.xy / 2.f + .5f));
		float lightness = max(max(light_level.r, light_level.g), light_level.b);
		if(lightness <= .25) {
			color *= lightness * 4.f;
		}
	}
}
