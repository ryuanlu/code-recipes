GLSLC:=glslangValidator

VULKAN_X11_SOURCES:=vulkan-x11.c vkhelper.c
VULKAN_X11_PKG_CONFIG_PACKAGES:=x11
VULKAN_X11_CFLAGS:=-Wall $(shell pkg-config --cflags $(VULKAN_X11_PKG_CONFIG_PACKAGES))
VULKAN_X11_LDFLAGS:=$(shell pkg-config --libs $(VULKAN_X11_PKG_CONFIG_PACKAGES)) -lvulkan
VULKAN_X11_BINARY:=vulkan-x11

SHADERS:=blit.vert blit.frag
SHADER_SPVS:=$(SHADERS:%=%.spv)
SHADER_SOURCES:=$(SHADERS:%=%.c)
SHADER_OBJECTS:=$(SHADERS:%=%.o)

.ONESHELL:
.SUFFIXES:

all: $(VULKAN_X11_BINARY)

clean:
	rm -f *.o $(VULKAN_X11_BINARY) $(SHADER_SOURCES) $(SHADER_SPVS)

$(SHADER_SPVS): %.spv: %
	@echo "	GLSLC	$@"
	$(GLSLC) -V $< -o $@ >/dev/null

$(SHADER_SOURCES): %.c: %.spv
	@echo "	XXD	$@"
	xxd -i $< > $@


VULKAN_X11_OBJECTS:=$(VULKAN_X11_SOURCES:.c=.o) $(SHADER_OBJECTS)

$(VULKAN_X11_OBJECTS): %.o: %.c
	@echo "	CC	$@"
	$(CC) -c $< -o $@ $(VULKAN_X11_CFLAGS)

$(VULKAN_X11_BINARY): $(VULKAN_X11_OBJECTS)
	@echo "	LD	$@"
	$(CC) $^ -o $@ $(VULKAN_X11_LDFLAGS)
