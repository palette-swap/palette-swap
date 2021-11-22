#version 330

// From vertex shader
in vec2 texcoord;

// Application data
uniform sampler2D sampler0;
uniform vec3 fcolor;
uniform vec2 offset;
uniform vec2 size;


// Output color
layout(location = 0) out  vec4 color;

void main()
{
	ivec2 full_size = textureSize(sampler0, 0);
	vec2 texcoord_shifted = (texcoord * size + offset) / vec2(full_size);
	color = vec4(fcolor, 1.0) * texture(sampler0, texcoord_shifted);
}
