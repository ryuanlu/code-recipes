#version 300 es

uniform sampler2D   src_texture;

in highp vec2 out_texcoord;
out highp vec4 fragmentColor;

void main()
{
	fragmentColor = texture(src_texture, out_texcoord);
}
