#version 330

uniform mat4 mvpMatrix;
uniform vec4 color;

in vec3 vpos;
out vec4 vcolor;

void main(void)
{
	gl_Position = mvpMatrix * vec4(vpos, 1.0f);
	vcolor = color;
}
