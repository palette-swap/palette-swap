#version 330

// From vertex shader
in vec2 texcoord;

in float tex_index;

// Application data
uniform sampler2D tile_textures[2];

// Output color
layout(location = 0) out  vec4 color;

void main()
{
    int index = int(tex_index);
	color = vec4(1.0,1.0,1.0, 1.0) * texture(tile_textures[index], vec2(texcoord.x, texcoord.y));
}
