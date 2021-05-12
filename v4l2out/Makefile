include ../build.mk

TARGET:=v4l2out

PKG_CONFIG_PACKAGES:=x11 xext

all: $(TARGET)

clean: $(TARGET)_clean

$(TARGET)_src_files:=v4l2out.c
$(TARGET)_cflags:=-Wall -g
$(TARGET)_ldflags:=-Wall $(shell pkg-config --libs $(PKG_CONFIG_PACKAGES)) -g
$(eval $(call define_c_targets,$(TARGET)))
