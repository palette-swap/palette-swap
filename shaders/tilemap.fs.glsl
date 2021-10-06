#version 330

// From vertex shader
in vec2 texcoord;

in float tex_index;

// Application data
uniform sampler2D tile_textures[3];

// Output color
layout(location = 0) out  vec4 color;

void main()
{
    int index = int(tex_index);

	// Maintenance Note:
	// Sampler arrays indexed with non-constant expressions are not supported in OpenGL 3.3.
	// The following conditionals are applied to bypass this support issue.	
	if (index == 0)
	{
		color = vec4(1.0,1.0,1.0, 1.0) * texture(tile_textures[0], vec2(texcoord.x, texcoord.y));
	}
	else if (index == 1)
	{
		color = vec4(1.0,1.0,1.0, 1.0) * texture(tile_textures[1], vec2(texcoord.x, texcoord.y));
	}
	else if (index == 2)
	{
		color = vec4(1.0,1.0,1.0, 1.0) * texture(tile_textures[2], vec2(texcoord.x, texcoord.y));
	}
}
