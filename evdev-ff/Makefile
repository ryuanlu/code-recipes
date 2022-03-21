include ../build.mk

TARGET:=evdev-ff

all: $(TARGET)

clean: $(TARGET)_clean

$(TARGET)_src_files:=evdev-ff.c
$(TARGET)_cflags:=-Wall -g
$(TARGET)_ldflags:=-Wall -g
$(eval $(call define_c_targets,$(TARGET)))
