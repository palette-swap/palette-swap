#version 330

// From Vertex Shader
in vec3 vcolor;
in vec2 vpos;

// Application data
uniform vec4 fcolor;

// Output color
layout(location = 0) out vec4 color;

void main()
{
	vec2 vpos2 = vpos * vpos * 4;
	if(vpos2.x + vpos2.y > 1) {
		discard;
	}
	color = fcolor * vec4(vcolor, 1.0) * vec4(vec3(1), 1 - pow(vpos2.x + vpos2.y, 2));
}