#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>
#include <wayland-client.h>
#include <linux/input-event-codes.h>
#include "xdg-shell-client-protocol.h"

#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>

#define eglGetPlatformDisplayEXT	eglGetPlatformDisplay

#define WINDOW_WIDTH	(640)
#define WINDOW_HEIGHT	(360)

struct W_context
{
	struct wl_compositor*		compositor;
	struct wl_surface*		surface;

	struct xdg_wm_base*		xdg_shell;
	struct xdg_surface*		xdg_surface;
	struct xdg_toplevel*		xdg_toplevel;

	struct wl_egl_window*		window;
	struct wl_seat*			seat;
	struct wl_pointer*		pointer;
	int				width;
	int				height;

	EGLDisplay			EGL_display;
	EGLSurface			draw_surface;
	EGLContext			EGL_context;
	EGLConfig			config;
};

void wl_registry_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version)
{
	struct W_context* WL = (struct W_context*)data;

	if(!strcmp(interface,"wl_compositor"))
		WL->compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, version);

	if(!strcmp(interface, "xdg_wm_base"))
		WL->xdg_shell = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, version);

	if(!strcmp(interface, "wl_seat"))
	{
		WL->seat = wl_registry_bind(wl_registry, name, &wl_seat_interface, version);
		WL->pointer = wl_seat_get_pointer(WL->seat);
	}
}

void wl_registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {}

static void xdg_shell_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static void xdg_surface_configure(void* data, struct xdg_surface* surface, uint32_t serial)
{
	struct W_context* WL = (struct W_context*)data;
	xdg_surface_ack_configure(surface, serial);
	glClearColor (0.0, 1.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	eglSwapBuffers(WL->EGL_display, WL->draw_surface);
}

static void xdg_toplevel_configure(void* data, struct xdg_toplevel* toplevel, int32_t width, int32_t height, struct wl_array* states)
{
	struct W_context* WL = (struct W_context*)data;
	uint32_t *ps;

	printf("toplevel-configure: w %d, h %d / states: ", width, height);

	wl_array_for_each(ps, states)
	{
		switch(*ps)
		{
		case XDG_TOPLEVEL_STATE_MAXIMIZED:
			printf("MAXIMIZED ");
			break;
		case XDG_TOPLEVEL_STATE_FULLSCREEN:
			printf("FULLSCREEN ");
			break;
		case XDG_TOPLEVEL_STATE_RESIZING:
			printf("RESIZING %d, %d ", WL->width, WL->height);
			wl_egl_window_resize(WL->window, WL->width + width, WL->height + height, 0, 0);
			break;
		case XDG_TOPLEVEL_STATE_ACTIVATED:
			printf("ACTIVATED ");
			break;
		}
	}

	printf("\n");
}

static void xdg_toplevel_close(void* data, struct xdg_toplevel* toplevel)
{
}

void wl_pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
}

void wl_pointer_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface)
{
}

void wl_pointer_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
}

void wl_pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	struct W_context* WL = (struct W_context*)data;
	if(button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED)
	{
		fprintf(stderr, "Move\n");
		xdg_toplevel_move(WL->xdg_toplevel, WL->seat, serial);
	}

	if(button == BTN_RIGHT && state == WL_POINTER_BUTTON_STATE_PRESSED)
	{
		fprintf(stderr, "resize\n");
		xdg_toplevel_resize(WL->xdg_toplevel, WL->seat, serial, XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT);
	}
}

void wl_pointer_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	fprintf(stderr, "%d, %d\n", axis, value);
}

void wl_pointer_frame(void *data, struct wl_pointer *wl_pointer)
{
}

void wl_pointer_axis_source(void *data, struct wl_pointer *wl_pointer, uint32_t axis_source)
{
}

void wl_pointer_axis_stop(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis)
{
}

void wl_pointer_axis_discrete(void *data, struct wl_pointer *wl_pointer, uint32_t axis, int32_t discrete)
{
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

	const struct xdg_wm_base_listener xdg_shell_listener =
	{
		.ping = xdg_shell_ping,
	};

	const struct xdg_surface_listener xdg_surface_listener =
	{
		.configure = xdg_surface_configure,
	};

	const struct xdg_toplevel_listener xdg_toplevel_listener =
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
		.frame = wl_pointer_frame,
		.axis_source = wl_pointer_axis_source,
		.axis_stop = wl_pointer_axis_stop,
		.axis_discrete = wl_pointer_axis_discrete
	};

	int nr_configs;

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
	xdg_wm_base_add_listener(WL->xdg_shell, &xdg_shell_listener, NULL);

	WL->surface = wl_compositor_create_surface(WL->compositor);

	WL->xdg_surface = xdg_wm_base_get_xdg_surface(WL->xdg_shell, WL->surface);
	xdg_surface_add_listener(WL->xdg_surface, &xdg_surface_listener, WL);

	WL->xdg_toplevel = xdg_surface_get_toplevel(WL->xdg_surface);
	xdg_toplevel_add_listener(WL->xdg_toplevel, &xdg_toplevel_listener, WL);

	wl_surface_commit(WL->surface);

	wl_pointer_add_listener(WL->pointer, &pointer_listener, WL);

	WL->EGL_display = eglGetPlatformDisplayEXT(EGL_PLATFORM_WAYLAND_EXT, W_display, NULL);
	eglInitialize(WL->EGL_display, NULL, NULL);
	eglBindAPI(EGL_OPENGL_API);
	eglChooseConfig(WL->EGL_display, config_attributes, &WL->config, 1, &nr_configs);
	WL->EGL_context = eglCreateContext(WL->EGL_display, WL->config, EGL_NO_CONTEXT, context_attributes);

	WL->window = wl_egl_window_create (WL->surface, WINDOW_WIDTH, WINDOW_HEIGHT);
	WL->width = WINDOW_WIDTH;
	WL->height = WINDOW_HEIGHT;
	WL->draw_surface = eglCreatePlatformWindowSurface(WL->EGL_display, WL->config, WL->window, NULL);
	eglMakeCurrent(WL->EGL_display, WL->draw_surface, WL->draw_surface, WL->EGL_context);

	while(running)
		wl_display_dispatch(W_display);

	wl_registry_destroy(W_registry);
	wl_display_disconnect(W_display);
	free(WL);

	return EXIT_SUCCESS;
}
