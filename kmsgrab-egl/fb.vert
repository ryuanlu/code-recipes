#version 300 es

vec2 position[6] = vec2[]
(
	vec2(-1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, -1.0),

	vec2(1.0, -1.0),
	vec2(-1.0, 1.0),
	vec2(1.0, 1.0)
);

vec2 texcoord[6] = vec2[]
(
	vec2(0.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 0.0),

	vec2(1.0, 0.0),
	vec2(0.0, 1.0),
	vec2(1.0, 1.0)
);

out vec2 out_texcoord;

void main()
{
	gl_Position = vec4(position[gl_VertexID], 0.0, 1.0);
	out_texcoord = texcoord[gl_VertexID];
}