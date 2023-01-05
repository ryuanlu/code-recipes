#include <stdio.h>
#include <stdlib.h>
#include <gio/gio.h>

// For real case, use "org.freedesktop.Notifications" as bus name.

#define BUS_NAME	"org.freedesktop.Notifications"


extern char _binary_notifyd_xml_start;
extern char _binary_notifyd_xml_end;


struct context
{
	unsigned int	id;
	GDBusNodeInfo*	introspection_data;
};

static void handle_method_call(GDBusConnection* connection, const gchar* sender, const gchar* object_path, const gchar* interface_name, const gchar* method_name, GVariant* parameters, GDBusMethodInvocation *invocation, gpointer user_data)
{
	char* app_name = NULL;
	unsigned int replaces_id = 0;
	char* app_icon = NULL;
	char* summary = NULL;
	char* body = NULL;
	char* actions = NULL;
	char* hints = NULL;
	int expire_timeout = 0;

	fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);

	fprintf(stderr, "sender = %s\nobject_path = %s\ninterface_name = %s\nmethod_name = %s\n", sender, object_path, interface_name, method_name);

	g_variant_get(parameters, "(susssasa{sv}i)", &app_name, replaces_id, &app_icon, &summary, &body, &actions, &hints, &expire_timeout);

	fprintf(stderr, "app_name = %s\nreplaces_id = %u\napp_icon = %s\nsummary = %s\nbody = %s\nactions = %s\nhints = %s\nexpire_timeout = %d\n", app_name, replaces_id, app_icon, summary, body, actions, hints, expire_timeout);

	g_dbus_method_invocation_return_value(invocation, g_variant_new("(u)", 0));
}

static const GDBusInterfaceVTable interface_vtable =
{
	handle_method_call,
	NULL,
	NULL,
	{0},
};

static void on_bus_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
	struct context* context = user_data;

	g_dbus_connection_register_object(connection, "/org/freedesktop/Notifications", context->introspection_data->interfaces[0], &interface_vtable, user_data, NULL, NULL);
	fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

static void on_name_acquired(GDBusConnection* connection, const gchar* name, gpointer user_data)
{
	fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
}

static void on_name_lost(GDBusConnection *connection, const gchar* name, gpointer user_data)
{
	fprintf(stderr, "%s: %s\n", __PRETTY_FUNCTION__, name);
}


int main(int argc, char** argv)
{
	struct context context;
	GMainLoop* loop = NULL;

	context.introspection_data = g_dbus_node_info_new_for_xml(&_binary_notifyd_xml_start, NULL);
	context.id = g_bus_own_name(G_BUS_TYPE_SESSION, BUS_NAME, G_BUS_NAME_OWNER_FLAGS_NONE, on_bus_acquired, on_name_acquired, on_name_lost, &context, NULL);

	loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);

	g_bus_unown_name(context.id);
	g_dbus_node_info_unref(context.introspection_data);

	return EXIT_SUCCESS;
}
