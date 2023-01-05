include ../build.mk

PKG_CONFIG_PACKAGES:=dbus-1 glib-2.0 gio-2.0

NOTIFYD_GIO:=notifyd-gio
NOTIFYD_GIO_PKG_CONFIG:=dbus-1 glib-2.0 gio-2.0

$(NOTIFYD_GIO)_src_files:=notifyd-gio.c
$(NOTIFYD_GIO)_resource_files:=notifyd.xml
$(NOTIFYD_GIO)_cflags:=-Wall $(shell pkg-config --cflags $(NOTIFYD_GIO_PKG_CONFIG)) -g
$(NOTIFYD_GIO)_ldflags:=-Wall $(shell pkg-config --libs $(NOTIFYD_GIO_PKG_CONFIG)) -g
$(eval $(call define_c_targets,$(NOTIFYD_GIO)))

NOTIFYD_DBUS:=notifyd-dbus
NOTIFYD_DBUS_PKG_CONFIG:=dbus-1

$(NOTIFYD_DBUS)_src_files:=notifyd-dbus.c
$(NOTIFYD_DBUS)_cflags:=-Wall $(shell pkg-config --cflags $(NOTIFYD_DBUS_PKG_CONFIG)) -g
$(NOTIFYD_DBUS)_ldflags:=-Wall $(shell pkg-config --libs $(NOTIFYD_DBUS_PKG_CONFIG)) -g
$(eval $(call define_c_targets,$(NOTIFYD_DBUS)))

all: $(NOTIFYD_GIO) $(NOTIFYD_DBUS)

clean: $(NOTIFYD_GIO)_clean $(NOTIFYD_DBUS)_clean
