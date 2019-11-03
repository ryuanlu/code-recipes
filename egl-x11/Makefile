include ../build.mk

.SUFFIXES:
.ONESHELL:

TARGET:=egl-x11

PKG_CONFIG_PACKAGES:=x11 egl gl

all: $(TARGET)

clean: $(TARGET)_clean

$(TARGET)_src_files:=egl-x11.c
$(TARGET)_cflags:=-Wall $(shell pkg-config --cflags $(PKG_CONFIG_PACKAGES))
$(TARGET)_ldflags:=-Wall $(shell pkg-config --libs $(PKG_CONFIG_PACKAGES))
$(eval $(call define_c_target,$(TARGET)))

-include $(TARGET).d
