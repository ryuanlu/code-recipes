#ifndef __WAYLAND_H__
#define __WAYLAND_H__

struct wl_client;
struct wl_window;

typedef int (*PFN_POINTER_BUTTON)(struct wl_window* window, const int x, const int y, const int button, const int state, void* userdata);
typedef int (*PFN_POINTER_MOTION)(struct wl_window* window, const int x, const int y, void* userdata);
typedef int (*PFN_POINTER_WHEEL)(struct wl_window* window, const int value, void* userdata);
typedef int (*PFN_KEYBOARD)(struct wl_window* window, const int key, const int state, void* userdata);
typedef int (*PFN_REDRAW)(struct wl_window* window, void* userdata);
typedef int (*PFN_RESIZE)(struct wl_window* window, const int width, const int height, void* userdata);

struct wl_client* wl_client_create(void);
int wl_client_destroy(struct wl_client* client);
int wl_client_read_events(struct wl_client* client);
int wl_client_dispatch(struct wl_client* client);

struct wl_window* wl_window_create(struct wl_client* client, const int width, const int height);
int wl_window_destroy(struct wl_window* window);

int wl_window_make_current(struct wl_window* window);
int wl_window_swapbuffers(struct wl_window* window);
void wl_window_set_title(struct wl_window* window, const char* title);
void wl_window_resize(struct wl_window* window);

void wl_window_set_userdata(struct wl_window* window, void* userdata);
void wl_window_set_keyboard_handler(struct wl_window* window, const PFN_KEYBOARD handler);
void wl_window_set_pointer_button_handler(struct wl_window* window, const PFN_POINTER_BUTTON handler);
void wl_window_set_pointer_motion_handler(struct wl_window* window, const PFN_POINTER_MOTION handler);
void wl_window_set_pointer_wheel_handler(struct wl_window* window, const PFN_POINTER_WHEEL handler);
void wl_window_set_redraw_handler(struct wl_window* window, const PFN_REDRAW handler);
void wl_window_set_resize_handler(struct wl_window* window, const PFN_RESIZE handler);


#endif /* __WAYLAND_H__ */