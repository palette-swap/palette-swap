#version 330 
#extension GL_ARB_explicit_uniform_location : require
#extension GL_ARB_explicit_attrib_location : require

// Input attributes
in vec3 in_color;
in vec3 in_position;

out vec3 vcolor;

// Application data
layout(location = 0) uniform mat3 transform;
layout(location = 1) uniform mat3 projection;

void main()
{
	vcolor = in_color;
	vec3 pos = projection * transform * vec3(in_position.xy, 1.0);
	gl_Position = vec4(pos.xy, in_position.z, 1.0);
}