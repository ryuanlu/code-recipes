include ../build.mk

.SUFFIXES:
.ONESHELL:

GLSLC:=glslangValidator

TARGET:=vulkan-x11

PKG_CONFIG_PACKAGES:=x11

SHADERS:=blit.vert blit.frag
SHADER_SPVS:=$(SHADERS:%=%.spv)
SHADER_SOURCES:=$(SHADERS:%=%.c)

all: $(TARGET)

clean: $(TARGET)_clean
	@echo "\tCLEAN"
	rm -f $(SHADER_SOURCES)

$(TARGET)_src_files:=vulkan-x11.c vkhelper.c $(SHADER_SOURCES)
$(TARGET)_cflags:=-Wall $(shell pkg-config --cflags $(PKG_CONFIG_PACKAGES))
$(TARGET)_ldflags:=-Wall $(shell pkg-config --libs $(PKG_CONFIG_PACKAGES)) -lvulkan
$(eval $(call define_c_target,$(TARGET)))

$(TARGET): $(SHADER_OBJECTS)

$(SHADER_SPVS): %.spv: %
	@echo "\tGLSLC\t$@"
	$(GLSLC) -V $< -o $@ >/dev/null

$(SHADER_SOURCES): %.c: %.spv
	@echo "\tXXD\t$@"
	xxd -i $< > $@

-include $(TARGET).d
