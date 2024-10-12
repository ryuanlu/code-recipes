include ../build.mk

TARGET:=freetype2

PKG_CONFIG_PACKAGES:=freetype2

all: $(TARGET)

clean: $(TARGET)_clean

$(TARGET)_src_files:=freetype2.c bitmap.c unicode.c
$(TARGET)_cflags:=-Wall -g $(shell pkg-config --cflags $(PKG_CONFIG_PACKAGES))
$(TARGET)_ldflags:=-Wall -g $(shell pkg-config --libs $(PKG_CONFIG_PACKAGES))
$(eval $(call define_c_targets,$(TARGET)))
