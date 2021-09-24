#version 330 
#extension GL_ARB_explicit_uniform_location : require
#extension GL_ARB_explicit_attrib_location : require

// Input attributes
in vec3 in_position;
in vec3 in_color;

out vec3 vcolor;
out vec2 vpos;

// Application data
layout(location = 0) uniform mat3 transform;
layout(location = 1) uniform mat3 projection;

void main()
{
	vpos = in_position.xy; // local coordinated before transform
	vcolor = in_color;
	vec3 pos = projection * transform * vec3(in_position.xy, 1.0); // why not simply *in_position.xyz ?
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}