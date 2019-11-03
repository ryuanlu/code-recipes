include ../build.mk

.SUFFIXES:
.ONESHELL:

TARGET:=kmsgrab

PKG_CONFIG_PACKAGES:=libdrm

all: $(TARGET)

clean: $(TARGET)_clean

$(TARGET)_src_files:=kmsgrab.c
$(TARGET)_cflags:=-Wall $(shell pkg-config --cflags $(PKG_CONFIG_PACKAGES))
$(TARGET)_ldflags:=-Wall $(shell pkg-config --libs $(PKG_CONFIG_PACKAGES))
$(eval $(call define_c_target,$(TARGET)))

-include $(TARGET).d
