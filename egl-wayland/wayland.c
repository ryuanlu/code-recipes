#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <linux/input-event-codes.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl32.h>

#include "wayland.h"
#include "xdg-shell-client-protocol.h"


#define USE_WL_COMPOSITOR_VERSION	(1)
#define USE_XDG_WM_BASE_VERSION		(1)
#define USE_WL_SEAT_VERSION		(1)

struct wl_client
{
	struct wl_display*	display;
	struct wl_registry*	registry;
	struct wl_compositor*	compositor;
	struct xdg_wm_base*	xdg_shell;

	struct wl_seat*		seat;
	struct wl_pointer*	pointer;
	struct wl_keyboard*	keyboard;

	struct wl_window*	focused;

	EGLDisplay		egl_display;
	EGLContext		egl_context;
	EGLConfig		egl_config;

	uint32_t		pointer_serial;
};


struct wl_window
{
	struct wl_client*		client;
	struct wl_surface*		surface;

	struct xdg_surface*		xdg_surface;
	struct xdg_toplevel*		xdg_toplevel;

	struct wl_egl_window*		egl_window;
	EGLSurface			draw_surface;

	int	cursor_x;
	int	cursor_y;

	PFN_REDRAW		redraw_handler;
	PFN_RESIZE		resize_handler;
	PFN_POINTER_BUTTON	pointer_button_handler;
	PFN_POINTER_MOTION	pointer_motion_handler;
	PFN_POINTER_WHEEL	pointer_wheel_handler;
	PFN_KEYBOARD		keyboard_handler;

	void*			userdata;
};


static void wl_registry_global(void *data, struct wl_registry *wl_registry, uint32_t name, const char *interface, uint32_t version)
{
	struct wl_client* client = (struct wl_client*)data;

	if(!strcmp(interface,"wl_compositor"))
		client->compositor = wl_registry_bind(wl_registry, name, &wl_compositor_interface, USE_WL_COMPOSITOR_VERSION);

	if(!strcmp(interface, "xdg_wm_base"))
		client->xdg_shell = wl_registry_bind(wl_registry, name, &xdg_wm_base_interface, USE_XDG_WM_BASE_VERSION);

	if(!strcmp(interface, "wl_seat"))
	{
		client->seat = wl_registry_bind(wl_registry, name, &wl_seat_interface, USE_WL_SEAT_VERSION);
		client->pointer = wl_seat_get_pointer(client->seat);
		client->keyboard = wl_seat_get_keyboard(client->seat);
	}

}

static void wl_registry_global_remove(void *data, struct wl_registry *wl_registry, uint32_t name) {}

static void xdg_shell_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32_t serial)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static void xdg_surface_configure(void* data, struct xdg_surface* surface, uint32_t serial)
{
	struct wl_window* window = (struct wl_window*)data;
	xdg_surface_ack_configure(surface, serial);

	if(window->redraw_handler)
		window->redraw_handler(window, window->userdata);
}

static void xdg_toplevel_configure(void* data, struct xdg_toplevel* toplevel, int32_t width, int32_t height, struct wl_array* states)
{
	struct wl_window* window = (struct wl_window*)data;
	uint32_t *ps;

	eglMakeCurrent(window->client->egl_display, window->draw_surface, window->draw_surface, window->client->egl_context);

	wl_array_for_each(ps, states)
	{
		switch(*ps)
		{
		case XDG_TOPLEVEL_STATE_MAXIMIZED:
			break;
		case XDG_TOPLEVEL_STATE_FULLSCREEN:
			break;
		case XDG_TOPLEVEL_STATE_RESIZING:
			wl_egl_window_resize(window->egl_window, width, height, 0, 0);
			if(window->resize_handler)
				window->resize_handler(window, width, height, window->userdata);
			break;
		case XDG_TOPLEVEL_STATE_ACTIVATED:
			break;
		}
	}
}

static void xdg_toplevel_close(void* data, struct xdg_toplevel* toplevel)
{
}

static void wl_pointer_enter(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	struct wl_client* client = (struct wl_client*)data;
	struct wl_window* window = (struct wl_window*)wl_surface_get_user_data(surface);
	client->focused = window;
	window->cursor_x = surface_x >> 8;
	window->cursor_y = surface_y >> 8;
}

static void wl_pointer_leave(void *data, struct wl_pointer *wl_pointer, uint32_t serial, struct wl_surface *surface)
{
}

static void wl_pointer_motion(void *data, struct wl_pointer *wl_pointer, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
	static uint32_t prev_time = 0;
	struct wl_client* client = (struct wl_client*)data;
	struct wl_window* window = client->focused;

	window->cursor_x = surface_x >> 8;
	window->cursor_y = surface_y >> 8;

	if(window->pointer_motion_handler && (time - prev_time > 16))
	{
		window->pointer_motion_handler(window, window->cursor_x, window->cursor_y, window->userdata);
		prev_time = time;
	}

}

static void wl_pointer_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
	struct wl_client* client = (struct wl_client*)data;
	struct wl_window* window = client->focused;

	client->pointer_serial = serial;

	if(window->pointer_button_handler)
		window->pointer_button_handler(window, window->cursor_x, window->cursor_y, button, state, window->userdata);

}

static void wl_pointer_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	struct wl_client* client = (struct wl_client*)data;
	struct wl_window* window = client->focused;

	if(window->pointer_wheel_handler)
		window->pointer_wheel_handler(window, value >> 8, window->userdata);

}

static void wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard, uint32_t format, int32_t fd, uint32_t size)
{
}

static void wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys)
{
}

static void wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, struct wl_surface *surface)
{
}

static void wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
	struct wl_client* client = (struct wl_client*)data;
	struct wl_window* window = client->focused;

	if(window && window->keyboard_handler)
		window->keyboard_handler(window, key, state, window->userdata);

}

static void wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
}

struct wl_client* wl_client_create(void)
{
	static struct wl_registry_listener registry_listener =
	{
		.global = wl_registry_global,
		.global_remove = wl_registry_global_remove
	};

	static struct xdg_wm_base_listener xdg_shell_listener =
	{
		.ping = xdg_shell_ping,
	};

	static struct wl_pointer_listener pointer_listener =
	{
		.enter = wl_pointer_enter,
		.leave = wl_pointer_leave,
		.motion = wl_pointer_motion,
		.button = wl_pointer_button,
		.axis = wl_pointer_axis,
	};

	static struct wl_keyboard_listener keyboard_listener =
	{
		.keymap = wl_keyboard_keymap,
		.enter = wl_keyboard_enter,
		.leave = wl_keyboard_leave,
		.key = wl_keyboard_key,
		.modifiers = wl_keyboard_modifiers,
	};

	const EGLint config_attributes[] =
	{
		EGL_RENDERABLE_TYPE,		EGL_OPENGL_ES3_BIT,
		EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
		EGL_NONE,
	};

	const EGLint context_attributes[] =
	{
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 2,
		EGL_NONE
	};

	int nr_configs = 0;
	struct wl_client* client = NULL;

	client = calloc(1, sizeof(struct wl_client));

	client->display = wl_display_connect(NULL);
	client->registry = wl_display_get_registry(client->display);
	wl_registry_add_listener(client->registry, &registry_listener, client);
	wl_display_roundtrip(client->display);

	xdg_wm_base_add_listener(client->xdg_shell, &xdg_shell_listener, client);
	wl_pointer_add_listener(client->pointer, &pointer_listener, client);
	wl_keyboard_add_listener(client->keyboard, &keyboard_listener, client);

	client->egl_display = eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, client->display, NULL);
	eglInitialize(client->egl_display, NULL, NULL);
	eglBindAPI(EGL_OPENGL_ES_API);
	eglChooseConfig(client->egl_display, config_attributes, &client->egl_config, 1, &nr_configs);
	client->egl_context = eglCreateContext(client->egl_display, client->egl_config, EGL_NO_CONTEXT, context_attributes);

	return client;
}

int wl_client_destroy(struct wl_client* client)
{
	eglMakeCurrent(client->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(client->egl_display, client->egl_context);
	eglTerminate(client->egl_display);
	xdg_wm_base_destroy(client->xdg_shell);
	wl_pointer_destroy(client->pointer);
	wl_keyboard_destroy(client->keyboard);
	wl_seat_destroy(client->seat);
	wl_compositor_destroy(client->compositor);
	wl_registry_destroy(client->registry);
	wl_display_disconnect(client->display);
	free(client);
	return 0;
}

int wl_client_read_events(struct wl_client* client)
{
	int ret;
	struct pollfd pollfd;

	pollfd.fd = wl_display_get_fd(client->display);
	pollfd.events = POLLIN;
	pollfd.revents = 0;

	if(wl_display_prepare_read(client->display))
	{
		wl_client_dispatch(client);
		return 0;
	}

	wl_display_flush(client->display);

	ret = poll(&pollfd, 1, 0);

	if(ret <= 0)
	{
		wl_display_cancel_read(client->display);
		return 0;
	}

	if(pollfd.revents & POLLIN)
		return !wl_display_read_events(client->display);

	return 0;
}

int wl_client_dispatch(struct wl_client* client)
{
	return wl_display_dispatch_pending(client->display);
}

struct wl_window* wl_window_create(struct wl_client* client, const int width, const int height)
{
	static struct xdg_surface_listener xdg_surface_listener =
	{
		.configure = xdg_surface_configure,
	};

	static struct xdg_toplevel_listener xdg_toplevel_listener =
	{
		.configure = xdg_toplevel_configure,
		.close = xdg_toplevel_close,
	};

	struct wl_window* window = NULL;

	window = calloc(1, sizeof(struct wl_window));

	window->client = client;
	window->surface = wl_compositor_create_surface(client->compositor);
	window->xdg_surface = xdg_wm_base_get_xdg_surface(client->xdg_shell, window->surface);
	window->xdg_toplevel = xdg_surface_get_toplevel(window->xdg_surface);
	wl_surface_set_user_data(window->surface, window);
	xdg_surface_add_listener(window->xdg_surface, &xdg_surface_listener, window);
	xdg_toplevel_add_listener(window->xdg_toplevel, &xdg_toplevel_listener, window);
	wl_surface_commit(window->surface);

	window->egl_window = wl_egl_window_create(window->surface, width, height);
	window->draw_surface = eglCreatePlatformWindowSurface(client->egl_display, client->egl_config, window->egl_window, NULL);

	return window;
}

int wl_window_destroy(struct wl_window* window)
{
	eglMakeCurrent(window->client->egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, window->client->egl_context);
	eglDestroySurface(window->client->egl_display, window->draw_surface);
	wl_egl_window_destroy(window->egl_window);
	xdg_toplevel_destroy(window->xdg_toplevel);
	xdg_surface_destroy(window->xdg_surface);
	wl_surface_destroy(window->surface);
	free(window);
	return 0;
}

int wl_window_make_current(struct wl_window* window)
{
	return eglMakeCurrent(window->client->egl_display, window->draw_surface, window->draw_surface, window->client->egl_context);
}

int wl_window_swapbuffers(struct wl_window* window)
{
	return eglSwapBuffers(window->client->egl_display, window->draw_surface);
}

void wl_window_set_title(struct wl_window* window, const char* title)
{
	xdg_toplevel_set_title(window->xdg_toplevel, title);
}

void wl_window_resize(struct wl_window* window)
{
	xdg_toplevel_resize(window->xdg_toplevel, window->client->seat, window->client->pointer_serial, XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT);
}

void wl_window_set_userdata(struct wl_window* window, void* userdata)
{
	window->userdata = userdata;
}

void wl_window_set_keyboard_handler(struct wl_window* window, const PFN_KEYBOARD handler)
{
	window->keyboard_handler = handler;
}

void wl_window_set_pointer_button_handler(struct wl_window* window, const PFN_POINTER_BUTTON handler)
{
	window->pointer_button_handler = handler;
}

void wl_window_set_pointer_motion_handler(struct wl_window* window, const PFN_POINTER_MOTION handler)
{
	window->pointer_motion_handler = handler;
}

void wl_window_set_pointer_wheel_handler(struct wl_window* window, const PFN_POINTER_WHEEL handler)
{
	window->pointer_wheel_handler = handler;
}

void wl_window_set_redraw_handler(struct wl_window* window, const PFN_REDRAW handler)
{
	window->redraw_handler = handler;
}

void wl_window_set_resize_handler(struct wl_window* window, const PFN_RESIZE handler)
{
	window->resize_handler = handler;
}
