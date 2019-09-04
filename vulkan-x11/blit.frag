#version 450

layout(binding = 1) uniform sampler2D teximage;

layout(location = 0) in vec2 frag_texcoord;
layout(location = 1) in vec4 frag_color;

layout(location = 0) out vec4 output_color;

void main() {
	vec4 tex_color = texture(teximage, frag_texcoord);
	output_color = vec4(tex_color.rgb * tex_color.a + vec3(frag_color.rgb * (1.0 - tex_color.a)), frag_color.a);
}