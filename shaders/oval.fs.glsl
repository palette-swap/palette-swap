#version 330

// From Vertex Shader
in vec3 vcolor;
in vec2 vpos;

// Application data
uniform vec4 fcolor;
uniform vec4 fcolor_fill;
uniform vec2 scale;
uniform float thickness;

// Output color
layout(location = 0) out vec4 color;

void main()
{
	float dist2 = vpos.x * vpos.x + vpos.y * vpos.y;
	if(dist2 > .25f) {
		discard;
	}
	if (dist2 < pow(.5 - thickness / sqrt(scale.x * scale.x + scale.y * scale.y), 2)) {
		color = fcolor_fill * vec4(vcolor, 1.0);
	} else {
		color = fcolor * vec4(vcolor, 1.0);
	}
}