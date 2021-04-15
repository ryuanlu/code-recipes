#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#include <gbm.h>
#include <sys/mman.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#define DECLARE_SHADER_SOURCE(name)	extern unsigned char name[];extern unsigned int name ## _len;

DECLARE_SHADER_SOURCE(fb_vert)
DECLARE_SHADER_SOURCE(fb_frag)

#define DRI_DEVICE_NODE "/dev/dri/card0"


drmModeFBPtr get_fb(const int drm_fd)
{
	int i;
	drmModeResPtr res;
	drmModeCrtcPtr crtc;
	drmModeFBPtr fb;
	int fb_dmafd = 0;

	drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	res = drmModeGetResources(drm_fd);

	for(i = res->count_crtcs;i >= 0;--i)
	{
		crtc = drmModeGetCrtc(drm_fd, res->crtcs[i]);
		if(!crtc || !crtc->buffer_id)
			continue;
		fb = drmModeGetFB(drm_fd, crtc->buffer_id);
		drmModeFreeCrtc(crtc);
		if(fb)
			break;
	}

	drmPrimeHandleToFD(drm_fd, fb->handle, O_RDONLY, &fb_dmafd);
	drmModeFreeResources(res);

	return fb;
}

EGLImage create_eglimage(EGLDisplay display, int dmafd, int width, int height)
{
	EGLAttrib attr[] = {
		EGL_WIDTH, width,
		EGL_HEIGHT, height,
		EGL_LINUX_DRM_FOURCC_EXT, GBM_FORMAT_XBGR8888,
		EGL_DMA_BUF_PLANE0_FD_EXT, dmafd,
		EGL_DMA_BUF_PLANE0_OFFSET_EXT, 0,
		EGL_DMA_BUF_PLANE0_PITCH_EXT, width * 4,
		EGL_NONE
	};
	
	return eglCreateImage(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer)NULL, attr);
}

GLuint create_shader(const unsigned char* src, const int length, GLenum type)
{
	GLuint shader;
	shader = glCreateShader(type);
	glShaderSource(shader, 1, (const char* const *)&src, &length);
	glCompileShader(shader);
	return shader;
}

GLuint create_vbo(GLuint position_loc, GLuint texcoord_loc)
{
	GLuint vbo;
	float vertices[] = 
	{
		-1.0, -1.0, 0.0, 0.0,
		1.0, 1.0, 1.0, 1.0,
		-1.0, 1.0, 0.0, 1.0,
		-1.0, -1.0, 0.0, 0.0,
		1.0, -1.0, 1.0, 0.0,
		1.0, 1.0, 1.0, 1.0,
	};

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(position_loc);
	glVertexAttribPointer(position_loc, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)0);
	glEnableVertexAttribArray(texcoord_loc);
	glVertexAttribPointer(texcoord_loc, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));

	return vbo;
}


int main(int argc, char const *argv[])
{
	drmModeFBPtr fb = NULL;
	int drm_fd = 0;
	int fb_dmafd = 0;
	int nr_configs;
	struct gbm_device* gbm = NULL;
	EGLDisplay egl_display;
	EGLContext context;
	EGLConfig config;
	EGLImage image;
	GLuint texture;
	GLuint vs, fs, program;
	GLuint vbo, in_position_loc, in_texcoord_loc, src_texture_loc;
	GLuint fbo, fbo_texture;
	char* data = NULL;
	FILE* fp = NULL;

	EGLint config_attributes[] =
	{
		EGL_RENDERABLE_TYPE,		EGL_OPENGL_ES3_BIT,
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_NONE,
	};

	EGLint context_attributes[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	drm_fd = open(DRI_DEVICE_NODE, O_RDWR | O_CLOEXEC);
	fb = get_fb(drm_fd);
	drmPrimeHandleToFD(drm_fd, fb->handle, O_RDONLY, &fb_dmafd);
	fprintf(stderr, "drm framebuffer export dmabuf as fd %d\n", fb_dmafd);


	gbm = gbm_create_device(drm_fd);
	egl_display = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, gbm, NULL);
	eglInitialize(egl_display, NULL, NULL);
	eglBindAPI(EGL_OPENGL_ES_API);
	eglChooseConfig(egl_display, config_attributes, &config, 1, &nr_configs);
	context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, context_attributes);
	eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);

	image = create_eglimage(egl_display, fb_dmafd, fb->width, fb->height);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
	glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, (GLeglImageOES)image);

	vs = create_shader(fb_vert, fb_vert_len, GL_VERTEX_SHADER);
	fs = create_shader(fb_frag, fb_frag_len, GL_FRAGMENT_SHADER);

	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	in_position_loc = glGetAttribLocation(program, "in_position");
	in_texcoord_loc = glGetAttribLocation(program, "in_texcoord");
	src_texture_loc = glGetUniformLocation(program, "src_texture");

	glDeleteShader(vs);
	glDeleteShader(fs);
	glUseProgram(program);

	vbo = create_vbo(in_position_loc, in_texcoord_loc);

	glGenFramebuffers(1, &fbo);
	glGenTextures(1, &fbo_texture);
	glBindTexture(GL_TEXTURE_2D, fbo_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fb->width, fb->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);

	glViewport(0, 0, fb->width, fb->height);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
	glUniform1i(src_texture_loc, 0);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	data = calloc(fb->pitch * fb->height, 1);

	glDrawArrays(GL_TRIANGLES, 0, 6);
	glReadPixels(0, 0, fb->width, fb->height, GL_RGBA, GL_UNSIGNED_BYTE, data);

	fp = fopen("output.bin", "w");
	fwrite(data, fb->pitch * fb->height, 1, fp);
	fclose(fp);
	free(data);

	glDeleteFramebuffers(1, &fbo);
	glDeleteTextures(1, &fbo_texture);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(program);
	glDeleteTextures(1, &texture);
	eglDestroyImage(egl_display, image);
	eglDestroyContext(egl_display, context);
	eglTerminate(egl_display);
	drmModeFreeFB(fb);
	close(drm_fd);

	return EXIT_SUCCESS;
}
