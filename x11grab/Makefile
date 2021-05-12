include ../build.mk

TARGET:=x11grab

PKG_CONFIG_PACKAGES:=x11 xext

all: $(TARGET)

clean: $(TARGET)_clean

$(TARGET)_src_files:=x11grab.c
$(TARGET)_cflags:=-Wall $(shell pkg-config --cflags $(PKG_CONFIG_PACKAGES)) -g
$(TARGET)_ldflags:=-Wall $(shell pkg-config --libs $(PKG_CONFIG_PACKAGES)) -g
$(eval $(call define_c_targets,$(TARGET)))
