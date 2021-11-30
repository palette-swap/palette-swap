#version 330

in vec2 in_position;

// Application data
uniform mat3 projection;

void main()
{
    gl_Position = vec4(projection * vec3(in_position, 1), 1.0);
}
