.SUFFIXES:
.ONESHELL:
.DEFAULT_GOAL:=all

define define_c_to_o_targets

$(1)_obj_files:=$$(foreach obj,$$($(1)_src_files:.c=.o),$$(obj))
$(1)_dep_files:=$$($(1)_obj_files:.o=.o.d)

$$($(1)_obj_files): %.o: %.c
	@printf "\tCC\t$$@\n"
	$$(CC) -MMD -MF $$@.d -c $$< -o $$@ $$($(1)_cflags)

$(1)_obj_clean:
	@rm -f $$($(1)_obj_files) $$($(1)_dep_files)

sinclude $$($(1)_dep_files)

endef


define define_c_targets

$(1)_resource_obj_files:=$$(foreach obj,$$($(1)_resource_files:%=%.o),$$(obj))

$(eval $(call define_c_to_o_targets,$(1)))

$$($(1)_resource_obj_files): %.o: %
	@printf "\tLD\t$$@\n"
	$$(LD) -r -b binary $$< -o $$@

$(1): $$($(1)_obj_files) $$($(1)_extra_obj_files) $$($(1)_resource_obj_files)
	@printf "\tLD\t$$@\n"
	$$(CC) $$^ -o $$@ $$($(1)_ldflags)
	@printf "\tDEBUG\t$$@\n"
	objcopy --only-keep-debug $$@ $$@.debug
	strip --strip-debug $$@
	objcopy --add-gnu-debuglink $$@.debug $$@

$(1)_clean: $(1)_obj_clean
	@printf "\tCLEAN\t$(1)\n"
	rm -f $$($(1)_resource_obj_files) $(1) $(1).debug

endef