#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "xshm.h"
#include "kms.h"

#define V4L2_DEVICE_NODE	"/dev/video9"
#define NUMBER_OF_BUFFERS	(2)


typedef void* (*PFN_CONTEXT_INIT)(void);
typedef void (*PFN_FRAME_COPY)(void* context, char* dest, int size);
typedef void (*PFN_CONTEXT_DESTROY)(void* context);

PFN_CONTEXT_INIT	context_init = NULL;
PFN_FRAME_COPY		frame_copy = NULL;
PFN_CONTEXT_DESTROY	context_destroy = NULL;


int main(int argc, char** argv)
{
	int fd, i;
	struct v4l2_format fmt = {0};
	struct v4l2_requestbuffers reqbufs = {0};
	struct v4l2_buffer bufs[NUMBER_OF_BUFFERS];
	void* context = NULL;
	int64_t timestamp, delay;
	struct timespec ts;
	void* ptr[NUMBER_OF_BUFFERS];


	context_init = xshm_init;
	frame_copy = xshm_frame_copy;

	context_init = kms_init;
	frame_copy = kms_frame_copy;

	context = context_init();

	fd = open(V4L2_DEVICE_NODE, O_RDWR);

	fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	fmt.fmt.pix.width = 1920;
	fmt.fmt.pix.height = 1080;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_BGR32;

	ioctl(fd, VIDIOC_S_FMT, &fmt);

	reqbufs.count = NUMBER_OF_BUFFERS;
	reqbufs.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	reqbufs.memory = V4L2_MEMORY_MMAP;

	ioctl(fd, VIDIOC_REQBUFS, &reqbufs);

	for(i = 0;i < NUMBER_OF_BUFFERS;++i)
	{
		bufs[i].index = i;
		bufs[i].type = reqbufs.type;
		bufs[i].memory = reqbufs.memory;

		ioctl(fd, VIDIOC_QUERYBUF, &bufs[i]);
		ptr[i] = mmap(NULL, bufs[i].length, PROT_WRITE, MAP_SHARED, fd, bufs[i].m.offset);
	}

	ioctl(fd, VIDIOC_STREAMON, &fmt.type);

	i = 0;
	delay = 0;

	while(1)
	{
		struct v4l2_buffer buf = {0};
		struct timespec ts_delay = {0};

		clock_gettime(CLOCK_MONOTONIC, &ts);
		timestamp = ts.tv_sec * 1000000000 + ts.tv_nsec;
		bufs[i].timestamp.tv_sec = ts.tv_sec;
		bufs[i].timestamp.tv_usec = ts.tv_nsec / 1000;

		frame_copy(context, ptr[i], bufs[i].length);

		ioctl(fd, VIDIOC_QBUF, &bufs[i]);
		buf.type = reqbufs.type;
		buf.memory = reqbufs.memory;
		ioctl(fd, VIDIOC_DQBUF, &buf);
		i = buf.index;

		clock_gettime(CLOCK_MONOTONIC, &ts);
		delay = 1000000000 / 60 - (ts.tv_sec * 1000000000 + ts.tv_nsec - timestamp);

		ts_delay.tv_nsec = delay > 0 ? delay : 0;

		nanosleep(&ts_delay, NULL);
	}

	close(fd);

	context_destroy(context);

	return EXIT_SUCCESS;
}