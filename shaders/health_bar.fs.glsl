#version 330

// From Vertex Shader
in vec3 vcolor;
in vec3 pos;

// Application data
uniform sampler2D lighting;
uniform bool use_lighting;

// Output color
layout(location = 0) out  vec4 color;

void main()
{
	color = vec4(vcolor * vec3(.8, .1, .1), 1.0);
	if(use_lighting) {
		vec4 light_level = texture(lighting, (pos.xy / 2.f + .5f));
		float lightness = max(max(light_level.r, light_level.g), light_level.b);
		if(lightness <= .25) {
			color *= lightness * 4.f;
		}
	}
}