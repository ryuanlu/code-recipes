#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#define DEVTMPFS_PATH	"/dev/input"
#define EVENT_DEV_NAME	"event"


int open_first_device(void)
{
	int fd = -1;
	DIR* dir = NULL;
	struct dirent* entry = NULL;
	int index = -1;
	char filename[24] = {0};
	struct input_id info = {0};

	const char* bus_types[] =
	{
		"BUS_UNKNOWN",
		"BUS_PCI",
		"BUS_ISAPNP",
		"BUS_USB",
		"BUS_HIL",
		"BUS_BLUETOOTH",
		"BUS_VIRTUAL",
	};


	dir = opendir(DEVTMPFS_PATH);

	while(1)
	{
		entry = readdir(dir);
		if(!entry) break;

		index = -1;
		sscanf(entry->d_name,"event%d", &index);
		if(index < 0) continue;

		snprintf(filename, sizeof(filename), "%s/%s%d", DEVTMPFS_PATH, EVENT_DEV_NAME, index);
		fd = open(filename, O_RDWR | O_CLOEXEC);
		ioctl(fd, EVIOCGID, &info);

		if(info.vendor == 0x045e && info.product == 0x0b13)
		{
			fprintf(stderr, "%s\t%04X:%04X\t%s\n", filename, info.vendor, info.product, bus_types[info.bustype]);
			closedir(dir);
			return fd;
		}

		close(fd);
	}

	closedir(dir);

	return fd;
}


int main(int argc, char** argv)
{
	int fd = -1;
	struct input_event event = {0};
	struct ff_effect effect = {0};
	int weak_effect_id = -1;
	int strong_effect_id = -1;

	fd = open_first_device();

	effect.id = -1;
	effect.type = FF_RUMBLE;
	effect.u.rumble.strong_magnitude = 0xffff;
	effect.u.rumble.weak_magnitude = 0x0000;
	effect.replay.length = 1000;
	ioctl(fd, EVIOCSFF, &effect);
	strong_effect_id = effect.id;

	effect.id = -1;
	effect.type = FF_RUMBLE;
	effect.u.rumble.strong_magnitude = 0x0000;
	effect.u.rumble.weak_magnitude = 0xffff;
	effect.replay.length = 1000;
	ioctl(fd, EVIOCSFF, &effect);
	weak_effect_id = effect.id;

	event.type = EV_FF;
	event.value = 1;

	event.code = weak_effect_id;
	write(fd, &event, sizeof(event));

	usleep(200000);

	event.code = strong_effect_id;
	write(fd, &event, sizeof(event));


	sleep(2);
	close(fd);

	return EXIT_SUCCESS;
}