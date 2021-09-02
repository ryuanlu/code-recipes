#include <stdio.h>
#include <string.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>

#define TITLE_STRING "Xlib test"

#define eglGetPlatformDisplayEXT	eglGetPlatformDisplay

int main(int argc, char const *argv[])
{
	int quit = False;
	Display*	X_display = NULL;
	Window		window;
	XTextProperty	title;
	XEvent		event;
	Atom		wm_delete_window;

	EGLDisplay	EGL_display = NULL;
	EGLSurface	draw_surface = NULL;
	EGLContext	context = NULL;
	int nr_configs;
	EGLConfig	config;

	struct timespec	ts_a, ts_b;

	EGLint config_attributes[] =
	{
		EGL_RENDERABLE_TYPE,		EGL_OPENGL_BIT,
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_NONE,
	};

	EGLint context_attributes[] = {
		EGL_CONTEXT_MAJOR_VERSION_KHR, 4,
		EGL_CONTEXT_MINOR_VERSION_KHR, 5,
		EGL_NONE
	};

	X_display = XOpenDisplay(NULL);
	window = XCreateWindow(X_display, RootWindow(X_display, 0), 0, 0, 1280, 720, 0,  CopyFromParent, CopyFromParent,  CopyFromParent, 0, NULL);
	XSetWindowBackground(X_display, window, BlackPixel(X_display, 0));
	title.value = (unsigned char*)TITLE_STRING;
	title.encoding = XA_STRING;
	title.format = 8;
	title.nitems = strlen(TITLE_STRING);
	XSetWMProperties(X_display, window, &title, &title, NULL, 0, NULL, NULL, NULL);
	wm_delete_window = XInternAtom(X_display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(X_display, window, &wm_delete_window, 1);
	XSelectInput(X_display, window, ExposureMask | KeyPressMask | StructureNotifyMask);
	XMapWindow(X_display, window);

	EGL_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_X11_EXT, X_display, NULL);
	eglInitialize(EGL_display, NULL, NULL);
	eglBindAPI(EGL_OPENGL_API);
	eglChooseConfig(EGL_display, config_attributes, &config, 1, &nr_configs);
	draw_surface = eglCreatePlatformWindowSurface(EGL_display, config, &window, NULL);
	context = eglCreateContext(EGL_display, config, EGL_NO_CONTEXT, context_attributes);

	eglMakeCurrent(EGL_display, draw_surface, draw_surface, context);
	glClearColor(1.0, 1.0, 1.0, 1.0);
	
	while(!quit)
	{
		float fps;
		clock_gettime(CLOCK_MONOTONIC, &ts_a);
		glClearColor(1.0, 1.0, 1.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		eglSwapBuffers(EGL_display, draw_surface);
		clock_gettime(CLOCK_MONOTONIC, &ts_b);
		fps = 1000.0 / ((ts_b.tv_sec - ts_a.tv_sec) * 1000 + (float)(ts_b.tv_nsec - ts_a.tv_nsec) / 1000000);
		fprintf(stderr, "%.2f\n", fps);


		if(XPending(X_display))
		{
			XNextEvent(X_display, &event);
			switch(event.type)
			{
			case ClientMessage:
				if(event.xclient.data.l[0] == wm_delete_window)
				{
					XDestroyWindow(X_display, window);
					quit = True;
				}
				break;
			}
		}
	}

	XCloseDisplay(X_display);

	return 0;
}
