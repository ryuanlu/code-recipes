#version 300 es
#extension GL_OES_EGL_image_external_essl3 : enable

uniform samplerExternalOES   src_texture;

in highp vec2 out_texcoord;
out highp vec4 fragmentColor;

void main()
{
	fragmentColor = texture(src_texture, out_texcoord).bgra;
}
