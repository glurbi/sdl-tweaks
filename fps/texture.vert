#version 330 core

uniform mat4 mvpMatrix;
uniform sampler2D texture;

in vec3 pos;
in vec2 texCoord;

out vec2 vTexCoord;

void main(void)
{
	gl_Position = mvpMatrix * vec4(pos, 1.0f);
    vTexCoord = texCoord;
}
