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

#define SIZEOF_BINARY(name) (&_binary_ ## name ## _end - &_binary_ ## name ## _start)
#define DECLARE_SHADER_SOURCE(name) \
extern unsigned char _binary_ ## name ## _start; \
extern unsigned char _binary_ ## name ## _end;

DECLARE_SHADER_SOURCE(fb_vert)
DECLARE_SHADER_SOURCE(fb_frag)

#define DRI_DEVICE_NODE "/dev/dri/card0"

#define OUTPUT_FORMAT	GBM_FORMAT_ABGR8888

drmModeFB2Ptr get_fb(const int drm_fd)
{
	int i;
	drmModeResPtr res;
	drmModeCrtcPtr crtc;
	drmModeFB2Ptr fb;

	drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	res = drmModeGetResources(drm_fd);

	for(i = res->count_crtcs;i >= 0;--i)
	{
		crtc = drmModeGetCrtc(drm_fd, res->crtcs[i]);
		if(!crtc || !crtc->buffer_id)
			continue;
		fb = drmModeGetFB2(drm_fd, crtc->buffer_id);
		drmModeFreeCrtc(crtc);
		if(fb)
			break;
	}

	drmModeFreeResources(res);

	return fb;
}

EGLImage create_eglimage(EGLDisplay display, int dmafd, int width, int height, int fourcc)
{
	EGLAttrib attr[] = {
		EGL_WIDTH, width,
		EGL_HEIGHT, height,
		EGL_LINUX_DRM_FOURCC_EXT, fourcc,
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


int main(int argc, char const *argv[])
{
	drmModeFB2Ptr fb = NULL;
	int drm_fd = 0;
	int fb_dmafd = 0;
	int fbo_dmafd = 0;
	int nr_configs;
	struct gbm_device* gbm = NULL;
	struct gbm_bo* bo = NULL;
	EGLDisplay egl_display;
	EGLContext context;
	EGLConfig config;
	EGLImage image;
	EGLImage fbo_image;
	GLuint texture;
	GLuint vs, fs, program;
	GLuint src_texture_loc;
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

	gbm = gbm_create_device(drm_fd);
	egl_display = eglGetPlatformDisplay(EGL_PLATFORM_GBM_KHR, gbm, NULL);
	eglInitialize(egl_display, NULL, NULL);
	eglBindAPI(EGL_OPENGL_ES_API);
	eglChooseConfig(egl_display, config_attributes, &config, 1, &nr_configs);
	context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, context_attributes);
	eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, context);


	fb = get_fb(drm_fd);
	drmPrimeHandleToFD(drm_fd, fb->handles[0], O_RDONLY, &fb_dmafd);
	image = create_eglimage(egl_display, fb_dmafd, fb->width, fb->height, fb->pixel_format);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


	vs = create_shader(&_binary_fb_vert_start, SIZEOF_BINARY(fb_vert), GL_VERTEX_SHADER);
	fs = create_shader(&_binary_fb_frag_start, SIZEOF_BINARY(fb_frag), GL_FRAGMENT_SHADER);

	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	src_texture_loc = glGetUniformLocation(program, "src_texture");

	glDeleteShader(vs);
	glDeleteShader(fs);
	glUseProgram(program);


	bo = gbm_bo_create(gbm, fb->width, fb->height, OUTPUT_FORMAT, GBM_BO_USE_LINEAR|GBM_BO_USE_RENDERING);
	fbo_dmafd = gbm_bo_get_fd(bo);

	glGenTextures(1, &fbo_texture);
	glBindTexture(GL_TEXTURE_2D, fbo_texture);
	fbo_image = create_eglimage(egl_display, fbo_dmafd, fb->width, fb->height, OUTPUT_FORMAT);
	glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, (GLeglImageOES)fbo_image);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);


	glViewport(0, 0, fb->width, fb->height);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(src_texture_loc, 0);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glFinish();


	data = mmap(NULL, fb->pitches[0] * fb->height, PROT_READ, MAP_PRIVATE, fbo_dmafd, 0);
	fp = fopen("output.bin", "w");
	fwrite(data, fb->pitches[0] * fb->height, 1, fp);
	fclose(fp);
	munmap(data, fb->pitches[0] * fb->height);


	glDeleteFramebuffers(1, &fbo);
	glDeleteProgram(program);
	glDeleteTextures(1, &fbo_texture);
	glDeleteTextures(1, &texture);
	eglDestroyImage(egl_display, fbo_image);
	eglDestroyImage(egl_display, image);
	eglDestroyContext(egl_display, context);
	eglTerminate(egl_display);
	drmModeFreeFB2(fb);
	gbm_bo_destroy(bo);
	gbm_device_destroy(gbm);
	close(drm_fd);

	return EXIT_SUCCESS;
}
