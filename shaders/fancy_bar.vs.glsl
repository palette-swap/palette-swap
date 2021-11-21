#version 330

// Input attributes
in vec3 in_position;
in vec3 in_color;

out vec2 out_local_pos;
out vec3 vcolor;

// Application data
uniform mat3 transform;
uniform mat3 projection;
uniform float health;

void main()
{
	vcolor = in_color;
	vec2 local_position = in_position.xy;
	if(in_color != vec3(0, 0, 0)) {
		local_position *= vec2(health, 1.0);
	}
	out_local_pos = local_position - vec2(.5, 0);
	vec3 pos = projection * transform * vec3(local_position, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}