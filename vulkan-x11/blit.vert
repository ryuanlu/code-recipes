#version 450
// #extension GL_ARB_separate_shader_objects : enable

// vec2 positions[3] = vec2[]
// (
// 	vec2(0.0, -0.5),
// 	vec2(0.5, 0.5),
// 	vec2(-0.5, 0.5)
// );

// vec3 colors[3] = vec3[]
// (
// 	vec3(1.0, 0.0, 0.0),
// 	vec3(0.0, 1.0, 0.0),
// 	vec3(0.0, 0.0, 1.0)
// );

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec4 color;

layout(location = 0) out vec2 frag_texcoord;
layout(location = 1) out vec4 frag_color;


void main()
{
	frag_texcoord = texcoord;
	frag_color = color;
	gl_Position = vec4(position, 0.0, 1.0);
}