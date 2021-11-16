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

// Fire spell constants
uniform vec2 fireball_offset = {-0.028,0.022};
uniform float fireball_darken = 0.3;
uniform float fireball_radius = 0.18;
uniform float fireball_color_gradient = 0.3;
uniform float fire_time_period = 3;
uniform float fire_r_amplitude = 0.5;
uniform float fire_g_amplitude = 0.1;

// Ice spell constants
uniform float ice_b_constant = 2;
uniform float ice_b_amplitude = 0.5;
uniform float ice_time_period = 2;
uniform float ice_opacity_amplitude = 0.3;

// Rock spell constants
uniform float rock_color_amplitude = 0.2;
uniform float rock_time_period = 3;

// Wind spell constants
uniform float wind_ball_default_color_amplitude = 3.5;
uniform vec2 wind_ball_offset = {0,0.3};
uniform float wind_xradial_amplitutde = 10;
uniform float wind_y_amplitutde = 20;

// Output color
layout(location = 0) out  vec4 color; 

vec4 fireShader(vec4 input_color)
{
	float darken = 0;
	float ball = distance(vpos, fireball_offset);
	if (ball <= fireball_radius) {
		darken = fireball_darken;
	}
	float fire_r = input_color.x * (1 + fire_r_amplitude * (fireball_color_gradient - vpos.y) * sin(time/fire_time_period)) - darken;
	float fire_g = input_color.y * (1 + fire_g_amplitude * (fireball_color_gradient - vpos.y) * sin(time/fire_time_period)) - darken;
	float fire_b = input_color.z - darken;
	float fire_opacity = input_color.w;
	return vec4(fire_r, fire_g, fire_b, fire_opacity);
}

vec4 iceShader(vec4 input_color)
{
	float ice_r = input_color.x;
	float ice_g = input_color.y;
	float ice_b = input_color.z * (ice_b_constant + ice_b_amplitude * sin(time/ice_time_period + (vpos.x) + (vpos.y)));
	float ice_opacity = input_color.w * (1 + ice_opacity_amplitude * sin(time/ice_time_period + (vpos.x) + (vpos.y)));

	return vec4(ice_r, ice_g, ice_b, ice_opacity);
}

vec4 rockShader(vec4 input_color)
{
	float rock_r = input_color.x * (1 + rock_color_amplitude * sin(time/rock_time_period));
	float rock_g = input_color.y * (1 + rock_color_amplitude * sin(time/rock_time_period));
	float rock_b = input_color.z * (1 + rock_color_amplitude * sin(time/rock_time_period));
	float rock_opacity = input_color.w;

	return vec4(rock_r, rock_g, rock_b, rock_opacity);
}

vec4 windShader(vec4 input_color)
{
	float wind_r = input_color.x;
	float wind_g = input_color.y * (wind_ball_default_color_amplitude - wind_y_amplitutde * vpos.y - wind_xradial_amplitutde * pow(vpos.x,2) - 
									2 * distance(vpos - wind_ball_offset, vec2(0,0)));
	float wind_b = input_color.z;
	float wind_opacity = input_color.w;

	return vec4(wind_r, wind_g, wind_b, wind_opacity);
}
void main()
{
	if (spelltype == 0) {
		color = fireShader(vec4(fcolor, 1)) * texture(sampler0, vec2(texcoord.x, texcoord.y));
	} else if (spelltype == 1) {
		color = iceShader(vec4(fcolor,1)) * texture(sampler0, vec2(texcoord.x, texcoord.y));
	} else if (spelltype == 2) {
		color = rockShader(vec4(fcolor,1)) * texture(sampler0, vec2(texcoord.x, texcoord.y));
	} else if (spelltype == 3) {
		color = windShader(vec4(fcolor,1)) * texture(sampler0, vec2(texcoord.x, texcoord.y));
	} else {
		color = vec4(fcolor, 1) * texture(sampler0, vec2(texcoord.x, texcoord.y));
	}
}


