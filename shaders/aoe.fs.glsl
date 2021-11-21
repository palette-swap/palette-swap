#version 330

// From vertex shader
in vec2 texcoord;
in vec2 vpos;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform float time;
uniform int spelltype = 0;
uniform vec2 texcoord_rel;
uniform bool active = false;

// aoe constants
uniform vec3 aoe_color = {200,200,200};
uniform float aoe_opacity_amplitude = 0.2;
uniform float aoe_opacity_mean = 0.4;
uniform float aoe_time_period = 3.f;

// Output color
layout(location = 0) out  vec4 color; 

void main()
{
		if (active == true) {
			color = vec4(fcolor, 1) * texture(sampler0, vec2(texcoord.x, texcoord.y));
		} else {
			color = vec4(aoe_color, (aoe_opacity_mean + aoe_opacity_amplitude * sin(time/aoe_time_period)));
		}
}


