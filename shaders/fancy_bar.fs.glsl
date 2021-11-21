#version 330

// From Vertex Shader
in vec2 out_local_pos;
in vec3 vcolor;
uniform float xy_ratio;

// Application data
uniform vec3 fcolor;

// Output color
layout(location = 0) out vec4 color;

const float radius = .5;

void main()
{
	vec2 dist = abs(out_local_pos);
	dist.x *= xy_ratio;
	vec2 max_size = vec2(.5 * xy_ratio, .5);
	vec2 difference = max_size - radius - dist;
	float edge_distance_squared = .25 - dist.y * dist.y;
	if(difference.x <= 0 && difference.y <= 0) {
		edge_distance_squared = radius * radius - (difference.x * difference.x + difference.y * difference.y);
		if(edge_distance_squared <= 0) {
			discard;
		}
		edge_distance_squared = edge_distance_squared;
	}
	vec3 tone = vec3(0);
	if(dist.y > .3) {
		tone = vec3(.2) * (out_local_pos.y > 0 ? -1.f : 1.f);
	}
	vec3 base_color = (vcolor == vec3(0)) ? fcolor * vec3(.2): fcolor * vcolor;
	color = vec4((base_color + tone) * (edge_distance_squared < .07 ? 0 : 1), 1.0);
}