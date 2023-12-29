include ../build.mk

TARGET:=unshare

all: $(TARGET)

clean: $(TARGET)_clean

$(TARGET)_src_files:=unshare.c
$(TARGET)_cflags:=-Wall -g
$(TARGET)_ldflags:=-Wall -g
$(eval $(call define_c_targets,$(TARGET)))
