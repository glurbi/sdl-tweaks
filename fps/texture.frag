#version 330 core

uniform sampler2D texture;

in vec2 vTexCoord;

out vec4 fColor;

void main(void)
{
	fColor = texture2D(texture, vTexCoord);
//	fColor = vec4(vTexCoord.x, vTexCoord.y, 1.0f, 1.0f);
}
