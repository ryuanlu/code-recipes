include ../build.mk

.SUFFIXES:
.ONESHELL:

TARGET:=egl-wayland

XDG_SHELL_PROTOCOL:=xdg-shell-unstable-v6
XDG_SHELL_PROTOCOL_SOURCE:=$(XDG_SHELL_PROTOCOL)-protocol.c
XDG_SHELL_PROTOCOL_HEADER:=$(XDG_SHELL_PROTOCOL)-client-protocol.h

XDG_SHELL_PROTOCOL_XML:=/usr/share/wayland-protocols/unstable/xdg-shell/$(XDG_SHELL_PROTOCOL).xml

PKG_CONFIG_PACKAGES:=wayland-client wayland-egl egl gl

all: $(TARGET)

clean: $(TARGET)_clean
	@echo "\tCLEAN"
	rm -f $(XDG_SHELL_PROTOCOL_SOURCE) $(XDG_SHELL_PROTOCOL_HEADER)

$(TARGET)_src_files:=egl-wayland.c $(XDG_SHELL_PROTOCOL_SOURCE)
$(TARGET)_cflags:=-Wall $(shell pkg-config --cflags $(PKG_CONFIG_PACKAGES))
$(TARGET)_ldflags:=-Wall $(shell pkg-config --libs $(PKG_CONFIG_PACKAGES))
$(eval $(call define_c_target,$(TARGET)))

$(TARGET).d: $(XDG_SHELL_PROTOCOL_HEADER)

$(XDG_SHELL_PROTOCOL_SOURCE): $(XDG_SHELL_PROTOCOL_XML)
	@echo "\tGEN\t$@"
	wayland-scanner private-code $< $@
$(XDG_SHELL_PROTOCOL_HEADER): $(XDG_SHELL_PROTOCOL_XML)
	@echo "\tGEN\t$@"
	wayland-scanner client-header $< $@

-include $(TARGET).d
