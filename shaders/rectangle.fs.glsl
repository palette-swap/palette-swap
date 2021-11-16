#version 330

// From Vertex Shader
in vec3 vcolor;
in vec2 vpos;

// Application data
uniform vec3 fcolor;
uniform vec2 scale;
uniform float thickness;

// Output color
layout(location = 0) out vec4 color;

void main()
{
	if(abs(vpos.x) < .5f - thickness / scale.x && abs(vpos.y) < .5f - thickness / scale.y) {
		discard;
	}
	color = vec4(fcolor * vcolor, 1.0);
}