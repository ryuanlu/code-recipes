include ../build.mk

TARGET:=kmsgrab

PKG_CONFIG_PACKAGES:=egl gl libdrm gbm libva-drm

all: $(TARGET)

clean: $(TARGET)_clean

$(TARGET)_src_files:=kmsgrab-egl.c
$(TARGET)_resource_files:=fb.vert fb.frag
$(TARGET)_cflags:=-Wall $(shell pkg-config --cflags $(PKG_CONFIG_PACKAGES)) -g
$(TARGET)_ldflags:=-Wall $(shell pkg-config --libs $(PKG_CONFIG_PACKAGES)) -g
$(eval $(call define_c_targets,$(TARGET)))
