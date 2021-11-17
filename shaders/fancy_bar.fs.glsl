#version 330

// From Vertex Shader
in vec2 out_local_pos;
in vec3 vcolor;
uniform float xy_ratio;

// Output color
layout(location = 0) out vec4 color;

const float radius = .5;

void main()
{
	vec2 dist = abs(out_local_pos);
	dist.x *= xy_ratio;
	vec2 max_size = vec2(.5 * xy_ratio, .5);
	vec2 difference = max_size - radius - dist;
	if(difference.x <= 0 && difference.y <= 0) {
		if(difference.x * difference.x + difference.y * difference.y >= radius * radius) {
			discard;
		}
	}
	color = vec4(vcolor, 1.0);
}