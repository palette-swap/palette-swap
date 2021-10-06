#version 330

// Input attributes
in vec3 in_position;
in vec2 in_texcoord;

// identifies tile texture
in float tile_index;

// Passed to fragment shader
out vec2 texcoord;
out float tex_index;

// Application data
uniform mat3 transform;
uniform mat3 projection;

void main()
{
    tex_index = tile_index;
	texcoord = in_texcoord;
	vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}