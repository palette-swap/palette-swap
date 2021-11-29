#version 330

uniform sampler2D screen_texture;
uniform sampler2D lighting_texture;
uniform float time;
uniform float darken_screen_factor;

in vec2 texcoord;

layout(location = 0) out vec4 color;

vec2 distort(vec2 uv) 
{

	return uv;
}

vec4 color_shift(vec4 in_color) 
{
	return in_color;
}

vec4 fade_color(vec4 in_color) 
{
	return in_color;
}

void main()
{
	vec2 coord = distort(texcoord);

    vec4 in_color = texture(screen_texture, coord) * texture(lighting_texture, coord);
    color = color_shift(in_color);
    color = fade_color(color);
}