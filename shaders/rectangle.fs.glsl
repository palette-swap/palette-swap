#version 330

// From Vertex Shader
in vec3 vcolor;
in vec2 vpos;

// Application data
uniform vec4 fcolor;
uniform vec4 fcolor_fill = vec4(0);
uniform vec2 scale;
uniform float thickness;

// Output color
layout(location = 0) out vec4 color;

void main()
{
	if (abs(vpos.x) < .5f - thickness / scale.x && abs(vpos.y) < .5f - thickness / scale.y) {
		color = fcolor_fill * vec4(vcolor, 1.0);
	} else {
		color = fcolor * vec4(vcolor, 1.0);
	}
}