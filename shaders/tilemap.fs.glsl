#version 330

// From vertex shader
in vec2 texcoord;
in vec2 world_pos;

// Application data
uniform sampler2D sampler0;
uniform vec2 start_tile;
uniform float max_show_distance;
uniform bool appearing;

// Output color
layout(location = 0) out vec4 color;

void main()
{
    if(appearing && distance(start_tile, world_pos) > max_show_distance) {
        discard;
    }
    color = texture(sampler0, vec2(texcoord.x, texcoord.y));
}
