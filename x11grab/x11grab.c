#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

int main(int argc, char** argv)
{
	unsigned int width, height;
	void* unused = NULL;
	Window window;
	FILE* fp = NULL;
	Display* display = NULL;
	XImage* image = NULL;
	XShmSegmentInfo shminfo = {0};

	display = XOpenDisplay(NULL);

	window = RootWindow(display, 0);
	XGetGeometry(display, window, (Window*)&unused, (int*)&unused, (int*)&unused, &width, &height, (unsigned int*)&unused, (unsigned int*)&unused);


	/* XGetImage method */

	image = XGetImage(display, window, 0, 0, width, height, AllPlanes, ZPixmap);

	fp = fopen("output.bin", "w");
	fwrite(image->data, width * height * 4, 1, fp);
	fclose(fp);

	XDestroyImage(image);


	/* XShmGetImage method */

	image = XShmCreateImage(display, DefaultVisual(display, 0), DefaultDepth(display, 0), ZPixmap, NULL, &shminfo, width, height);

	shminfo.shmid = shmget(IPC_PRIVATE, image->bytes_per_line * image->height, IPC_CREAT|0777);
	shminfo.shmaddr = image->data = shmat(shminfo.shmid, 0, 0);
	shminfo.readOnly = False;

	XShmAttach(display, &shminfo);
	XShmGetImage(display, window, image, 0, 0, AllPlanes);

	fp = fopen("output_shm.bin", "w");
	fwrite(image->data, width * height * 4, 1, fp);
	fclose(fp);

	XShmDetach(display, &shminfo);
	XDestroyImage(image);

	XCloseDisplay(display);

	return EXIT_SUCCESS;
}