#version 300 es

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_texcoord;

out vec2 out_texcoord;

void main()
{
	gl_Position = vec4(in_position, 0, 1);
	out_texcoord = in_texcoord;
}
