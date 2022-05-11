include ../build.mk

TARGET:=v4l2out

PKG_CONFIG_PACKAGES:=x11 xext libdrm libva-drm gbm

all: $(TARGET)

clean: $(TARGET)_clean

$(TARGET)_src_files:=v4l2out.c xshm.c kms.c
$(TARGET)_cflags:=-Wall $(shell pkg-config --cflags $(PKG_CONFIG_PACKAGES)) -g -DUSE_VAAPI
$(TARGET)_ldflags:=-Wall $(shell pkg-config --libs $(PKG_CONFIG_PACKAGES)) -g
$(eval $(call define_c_targets,$(TARGET)))
