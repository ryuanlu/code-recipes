include ../build.mk

TARGET:=tty

all: $(TARGET)

clean: $(TARGET)_clean

$(TARGET)_src_files:=tty.c
$(TARGET)_cflags:=-Wall -g
$(TARGET)_ldflags:=-Wall -g
$(eval $(call define_c_targets,$(TARGET)))
