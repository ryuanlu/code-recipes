#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-egl.h>
#include <wayland-client.h>
#include <linux/input-event-codes.h>
#include "xdg-shell-unstable-v6-client-protocol.h"

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>

#define eglGetPlatformDisplayEXT	eglGetPlatformDisplay

struct W_context
{
	struct wl_compositor*		compositor;
	struct wl_surface*		surface;

	struct zxdg_shell_v6*		xdg_shell;
	struct zxdg_surface_v6*		xdg_surface;
	struct zxdg_toplevel_v6*	xdg_toplevel;

	struct wl_egl_window*		window;
	struct wl_seat*			seat;
	struct wl_pointer*		pointer;
	int				x, y;
};

void wl_registry_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version)
{
	struct W_context* WL = (struct W_context*)data;

	if(!strcmp(interface,"wl_compositor"))
		WL->compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, 1);

	if(!strcmp(interface, "zxdg_shell_v6"))
		WL->xdg_shell = wl_registry_bind(wl_registry, name, &zxdg_shell_v6_interface, 1);

	if(!strcmp(interface, "wl_seat"))
	{
		WL->seat = wl_registry_bind(wl_registry, name, &wl_seat_interface, 1);
		WL->pointer = wl_seat_get_pointer(WL->seat);
	}
}

void wl_registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {}

static void xdg_shell_ping(void* data, struct zxdg_shell_v6* xdg_shell, uint32_t serial)
{
	zxdg_shell_v6_pong(xdg_shell, serial);
}

static void xdg_surface_configure(void* data, struct zxdg_surface_v6* surface, uint32_t serial)
{
	zxdg_surface_v6_ack_configure(surface, serial);
}

static void xdg_toplevel_configure(void* data, struct zxdg_toplevel_v6* toplevel, int32_t width, int32_t height, struct wl_array* states)
{
}

static void xdg_toplevel_close(void* data, struct zxdg_toplevel_v6* toplevel)
{
}

void wl_pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	struct W_context* WL = (struct W_context*)data;
	WL->x = wl_fixed_to_double(surface_x);
	WL->y = wl_fixed_to_double(surface_y);
}

void wl_pointer_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface) {}

void wl_pointer_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	struct W_context* WL = (struct W_context*)data;
	fprintf(stderr, "%f, %f\n", wl_fixed_to_double(surface_x) - WL->x, wl_fixed_to_double(surface_y) - WL->y);
	WL->x = wl_fixed_to_double(surface_x);
	WL->y = wl_fixed_to_double(surface_y);
}

void wl_pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	struct W_context* WL = (struct W_context*)data;
	if(button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED)
	{
		fprintf(stderr, "Move\n");
		zxdg_toplevel_v6_move(WL->xdg_toplevel, WL->seat, serial);
	}
}

void wl_pointer_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	fprintf(stderr, "%d, %d\n", axis, value);
}

int main(int argc, char const *argv[])
{
	int running = 1;
	struct W_context* WL = NULL;
	struct wl_display* W_display;
	struct wl_registry* W_registry;

	const struct wl_registry_listener registry_listener =
	{
		.global = wl_registry_global,
		.global_remove = wl_registry_global_remove
	};

	const struct zxdg_shell_v6_listener xdg_shell_listener =
	{
		.ping = xdg_shell_ping,
	};

	const struct zxdg_surface_v6_listener xdg_surface_listener =
	{
		.configure = xdg_surface_configure,
	};

	const struct zxdg_toplevel_v6_listener xdg_toplevel_listener =
	{
		.configure = xdg_toplevel_configure,
		.close = xdg_toplevel_close,
	};

	const struct wl_pointer_listener pointer_listener =
	{
		.enter = wl_pointer_enter,
		.leave = wl_pointer_leave,
		.motion = wl_pointer_motion,
		.button = wl_pointer_button,
		.axis = wl_pointer_axis,
		.frame = NULL,
		.axis_source = NULL,
		.axis_stop = NULL,
		.axis_discrete = NULL
	};

	EGLDisplay	EGL_display = NULL;
	EGLSurface	draw_surface = NULL;
	EGLContext	EGL_context = NULL;
	int nr_configs;
	EGLConfig	config;

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

	WL = calloc(sizeof(struct W_context), 1);

	W_display = wl_display_connect(NULL);
	W_registry = wl_display_get_registry(W_display);
	wl_registry_add_listener(W_registry, &registry_listener, WL);
	wl_display_roundtrip(W_display);
	zxdg_shell_v6_add_listener(WL->xdg_shell, &xdg_shell_listener, NULL);

	WL->surface = wl_compositor_create_surface(WL->compositor);

	WL->xdg_surface = zxdg_shell_v6_get_xdg_surface(WL->xdg_shell, WL->surface);
	zxdg_surface_v6_add_listener(WL->xdg_surface, &xdg_surface_listener, WL);

	WL->xdg_toplevel = zxdg_surface_v6_get_toplevel(WL->xdg_surface);
	zxdg_toplevel_v6_add_listener(WL->xdg_toplevel, &xdg_toplevel_listener, WL);

	wl_surface_commit(WL->surface);

	WL->window = wl_egl_window_create (WL->surface, 640, 360);
	wl_pointer_add_listener(WL->pointer, &pointer_listener, WL);

	EGL_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_WAYLAND_EXT, W_display, NULL);
	eglInitialize(EGL_display, NULL, NULL);
	eglBindAPI(EGL_OPENGL_API);
	eglChooseConfig(EGL_display, config_attributes, &config, 1, &nr_configs);
	draw_surface = eglCreatePlatformWindowSurfaceEXT(EGL_display, config, WL->window, NULL);
	EGL_context = eglCreateContext(EGL_display, config, EGL_NO_CONTEXT, context_attributes);
	eglMakeCurrent(EGL_display, draw_surface, draw_surface, EGL_context);

	while(running)
	{
		wl_display_dispatch_pending(W_display);
		glClearColor (0.0, 1.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		eglSwapBuffers(EGL_display, draw_surface);
	}

	wl_registry_destroy(W_registry);
	wl_display_disconnect(W_display);
	free(WL);

	return EXIT_SUCCESS;
}
