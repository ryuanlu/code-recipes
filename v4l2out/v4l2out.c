#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>
#include <sys/ipc.h>

#define V4L2_DEVICE_NODE	"/dev/video9"
#define NUMBER_OF_BUFFERS	(2)

int main(int argc, char** argv)
{
	int fd, i;
	struct v4l2_format fmt = {0};
	struct v4l2_requestbuffers reqbufs = {0};
	struct v4l2_buffer bufs[NUMBER_OF_BUFFERS];
	XShmSegmentInfo shminfo = {0};
	Display* display = NULL;
	XImage* image = NULL;
	struct timeval tv;


	display = XOpenDisplay(NULL);
	image = XShmCreateImage(display, DefaultVisual(display, 0), DefaultDepth(display, 0), ZPixmap, NULL, &shminfo, 1920, 1080);
	shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height, IPC_CREAT|0777);
	shminfo.shmaddr = image->data = shmat(shminfo.shmid, 0, 0);
	shminfo.readOnly = False;
	XShmAttach(display, &shminfo);

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
	}

	ioctl(fd, VIDIOC_STREAMON, &fmt.type);

	i = 0;

	while(1)
	{
		struct v4l2_buffer buf = {};
		char* ptr = NULL;

		ptr = mmap(NULL, bufs[i].length, PROT_WRITE, MAP_SHARED, fd, bufs[i].m.offset);
		XShmGetImage(display, RootWindow(display, 0), image, 0, 0, AllPlanes);
		memcpy(ptr, image->data, bufs[i].length);
		munmap(ptr, bufs[i].length);

		gettimeofday(&tv, 0);

		/* TODO: We have to calculate the accurate sleep time */
		// usleep(16000);

		bufs[i].timestamp = tv;
		ioctl(fd, VIDIOC_QBUF, &bufs[0]);
		buf.type = reqbufs.type;
		buf.memory = reqbufs.memory;
		ioctl(fd, VIDIOC_DQBUF, &buf);
		i = buf.index;
	}

	close(fd);
	return EXIT_SUCCESS;
}