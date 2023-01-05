#include <dbus/dbus.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// For real case, use "org.freedesktop.Notifications" as bus name.

#define BUS_NAME	"org.freedesktop.Notification"
#define INTERFACE_NAME	"org.freedesktop.Notifications"
#define METHOD_NAME	"Notify"

struct context
{
	DBusConnection*	connection;
};


int main(int argc, char** argv)
{
	struct context context;
	DBusError err;
	int ret = 0;


	dbus_error_init(&err);
	context.connection = dbus_bus_get(DBUS_BUS_SESSION, &err);

	if(dbus_error_is_set(&err))
	{ 
		fprintf(stderr, "Connection Error (%s)\n", err.message); 
		dbus_error_free(&err); 
	}

	ret = dbus_bus_request_name(context.connection, BUS_NAME, DBUS_NAME_FLAG_DO_NOT_QUEUE, &err);

	if(dbus_error_is_set(&err))
	{ 
		fprintf(stderr, "Name Error (%s)\n", err.message); 
		dbus_error_free(&err);
	}

	if(ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
	{
		fprintf(stderr, "Not Primary Owner (%d)\n", ret);
		exit(1); 		
	}

	while(1)
	{
		DBusMessage* msg = NULL;

		dbus_connection_read_write(context.connection, 0);
		msg = dbus_connection_pop_message(context.connection);


		if(!msg)
		{
			usleep(10000);
			continue;
		}else
		{
			if(dbus_message_is_method_call(msg, INTERFACE_NAME, METHOD_NAME))
				fprintf(stderr, "!!!\n");
			dbus_message_unref(msg);
		}

	}


	return EXIT_SUCCESS;
}