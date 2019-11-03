include ../build.mk

.SUFFIXES:
.ONESHELL:

TARGET:=kmsgrab

PKG_CONFIG_PACKAGES:=egl gl libdrm gbm

all: $(TARGET)

clean: $(TARGET)_clean

RESOURCES:=fb.vert fb.frag
RESOURCE_SOURCES:=$(RESOURCES:%=%.c)

$(TARGET)_src_files:=kmsgrab-egl.c $(RESOURCE_SOURCES)
$(TARGET)_cflags:=-Wall $(shell pkg-config --cflags $(PKG_CONFIG_PACKAGES))
$(TARGET)_ldflags:=-Wall $(shell pkg-config --libs $(PKG_CONFIG_PACKAGES))
$(eval $(call define_c_target,$(TARGET)))

$(RESOURCE_SOURCES): %.c: %
	xxd -i $< $@

-include $(TARGET).d
