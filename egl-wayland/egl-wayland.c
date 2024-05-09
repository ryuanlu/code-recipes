#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/input-event-codes.h>
#include <wayland-client.h>
#include <GLES3/gl32.h>
#include "wayland.h"

struct context
{
	int	quit;
	int	drag;
	int	resize;
	int	init;
	int	prev_x;
	int	prev_y;

	struct wl_window* window;
};

static int redraw(struct wl_window* window, void* userdata)
{
	glClearColor(0.0f, 1.0f, 0.0f, 0.5f);
	glClear(GL_COLOR_BUFFER_BIT);
	wl_window_swapbuffers(window);

	return 0;
}

static int mouse_button(struct wl_window* window, const int x, const int y, const int button, const int state, void* userdata)
{
	struct context* context = (struct context*)userdata;

	if(button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED)
	{
		if(context->resize)
			wl_window_resize(window);

		context->drag = context->resize ? 0 : 1;
		context->prev_x = x;
		context->prev_y = y;
	}

	if(button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_RELEASED)
	{
		context->drag = 0;
	}

	return 0;
}

static int mouse_motion(struct wl_window* window, const int x, const int y, void* userdata)
{
	struct context* context = (struct context*)userdata;

	if(!context->drag)
		return 0;

	if(y - context->prev_y == 0.0f && x - context->prev_x == 0.0f)
		return 0;

	redraw(window, userdata);
	context->prev_x = x;
	context->prev_y = y;

	return 0;
}

static int mouse_wheel(struct wl_window* window, const int value, void* userdata)
{
	return 0;
}

static int keyboard(struct wl_window* window, const int key, const int state, void* userdata)
{
	struct context* context = (struct context*)userdata;

	if(key == KEY_Q && state == WL_KEYBOARD_KEY_STATE_PRESSED)
		context->quit = 1;

	if(key == KEY_LEFTCTRL)
		context->resize = state;

	return 0;
}

static int resize(struct wl_window* window, const int width, const int height, void* userdata)
{
	return 0;
}

int main(int argc, char** argv)
{
	struct wl_client* client = NULL;
	struct wl_window* window = NULL;
	struct context context;


	client = wl_client_create();
	window = wl_window_create(client, 800, 800);

	memset(&context, 0, sizeof(struct context));

	context.window = window;
	wl_window_set_userdata(window, &context);

	wl_window_set_redraw_handler(window, redraw);
	wl_window_set_resize_handler(window, resize);
	wl_window_set_pointer_button_handler(window, mouse_button);
	wl_window_set_pointer_motion_handler(window, mouse_motion);
	wl_window_set_pointer_wheel_handler(window, mouse_wheel);
	wl_window_set_keyboard_handler(window, keyboard);

	wl_window_make_current(window);

	while(!context.quit)
	{
		int r = 0;

		if(wl_client_read_events(client) > 0)
			r = wl_client_dispatch(client);

		if(!r)
			usleep(2000);
	}

	wl_window_destroy(window);
	wl_client_destroy(client);

	return EXIT_SUCCESS;
}