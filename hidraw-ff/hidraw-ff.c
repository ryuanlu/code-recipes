#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/hidraw.h>

#define XB1S_FF_REPORT	(3)
#define DEVTMPFS_PATH	"/dev/"
#define HIDRAW_NAME	"hidraw"

enum __attribute__((__packed__)) motor
{
	FF_RUMBLE_NONE = 0x00,
	FF_RUMBLE_WEAK = 0x01,
	FF_RUMBLE_STRONG = 0x02,
	FF_RUMBLE_MAIN = FF_RUMBLE_WEAK | FF_RUMBLE_STRONG,
	FF_RUMBLE_RIGHT = 0x04,
	FF_RUMBLE_LEFT = 0x08,
	FF_RUMBLE_TRIGGERS = FF_RUMBLE_LEFT | FF_RUMBLE_RIGHT,
	FF_RUMBLE_ALL = 0x0F
};

struct __attribute__((__packed__)) ff_data
{
	u_int8_t	report_id;
        enum motor	enable;
	u_int8_t	magnitude_left;
	u_int8_t	magnitude_right;
	u_int8_t	magnitude_strong;
	u_int8_t	magnitude_weak;
	u_int8_t	pulse_sustain_10ms;
	u_int8_t	pulse_release_10ms;
	u_int8_t	loop_count;
};

int open_first_device(void)
{
	int fd = -1;
	DIR* dir = NULL;
	struct dirent* entry = NULL;
	int index = -1;
	char filename[16] = {0};
	struct hidraw_devinfo info;
	int r;

	dir = opendir("/dev/");

	while(1)
	{
		entry = readdir(dir);
		if(!entry) break;

		index = -1;
		sscanf(entry->d_name,"hidraw%d", &index);
		if(index < 0) continue;

		snprintf(filename, sizeof(filename), "%s/%s%d", DEVTMPFS_PATH, HIDRAW_NAME, index);
		fd = open(filename, O_RDWR | O_CLOEXEC);
		r = ioctl(fd, HIDIOCGRAWINFO, &info);

		if(info.vendor == 0x045e && info.product == 0x0b13)
		{
			closedir(dir);
			return fd;
		}

		r = close(fd);
	}

	closedir(dir);

	return fd;
}


int main(int argc, char** argv)
{
	int fd = -1;
	struct ff_data data = {0};

	fd = open_first_device();

	data.report_id = XB1S_FF_REPORT;
	data.enable = FF_RUMBLE_ALL;

	data.magnitude_left = 0;
	data.magnitude_right = 50;
	data.magnitude_strong = 0;
	data.magnitude_weak = 100;
	data.loop_count = 0;
	data.pulse_sustain_10ms = 50;
	data.pulse_release_10ms = 0;

	write(fd, &data, sizeof(data));
	close(fd);

	return EXIT_SUCCESS;
}