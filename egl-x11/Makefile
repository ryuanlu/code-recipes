
EGL_X11_SOURCES:=egl-x11.c
EGL_X11_PKG_CONFIG_PACKAGES:=x11 egl gl
EGL_X11_CFLAGS:=-Wall $(shell pkg-config --cflags $(EGL_X11_PKG_CONFIG_PACKAGES))
EGL_X11_LDFLAGS:=$(shell pkg-config --libs $(EGL_X11_PKG_CONFIG_PACKAGES))
EGL_X11_BINARY:=egl-x11

all: $(EGL_X11_BINARY)

clean:
	rm -f *.o $(EGL_X11_BINARY)

EGL_X11_OBJECTS:=$(EGL_X11_SOURCES:.c=.o)
$(EGL_X11_OBJECTS): %.o: %.c
	$(CC) -c $< -o $@ $(EGL_X11_CFLAGS)
$(EGL_X11_BINARY): $(EGL_X11_OBJECTS)
	$(CC) $^ -o $@ $(EGL_X11_LDFLAGS)
