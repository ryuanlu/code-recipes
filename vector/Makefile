include ../build.mk

.SUFFIXES:
.ONESHELL:

TARGET:=test

all: $(TARGET)

clean: $(TARGET)_clean

$(TARGET)_src_files:=test.c vector.c
$(TARGET)_cflags:=-Wall
$(TARGET)_ldflags:=-Wall
$(eval $(call define_c_target,$(TARGET)))

-include $(TARGET).d
