include ../build.mk

TARGET:=cipher

PKG_CONFIG_PACKAGES:=libcrypto

all: $(TARGET)

clean: $(TARGET)_clean

$(TARGET)_src_files:=cipher.c
$(TARGET)_cflags:=-Wall $(shell pkg-config --cflags $(PKG_CONFIG_PACKAGES))
$(TARGET)_ldflags:=-Wall $(shell pkg-config --libs $(PKG_CONFIG_PACKAGES))
$(eval $(call define_c_targets,$(TARGET)))
