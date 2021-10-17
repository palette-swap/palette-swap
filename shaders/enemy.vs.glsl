#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// Passed to fragment shader
out vec2 texcoord;

// Application data
uniform mat3 transform;
uniform mat3 projection;

// Value used for sprite size, used for calculating spritesheet offset
uniform float sprite_size = 32;
uniform int frame;
uniform int state;

void main()
{
	texcoord = in_texcoord;
	vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}