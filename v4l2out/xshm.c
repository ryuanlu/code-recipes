#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <sys/shm.h>
#include <sys/ipc.h>



struct xshm_context
{
	XShmSegmentInfo	shminfo;
	Display*	display;
	XImage*		image;



};


void* xshm_init(void)
{
	struct xshm_context* context = NULL;

	context = calloc(sizeof(struct xshm_context), 1);
	context->display = XOpenDisplay(NULL);
	context->image = XShmCreateImage(context->display, DefaultVisual(context->display, 0), DefaultDepth(context->display, 0), ZPixmap, NULL, &context->shminfo, 1920, 1080);
	context->shminfo.shmid = shmget(IPC_PRIVATE, context->image->bytes_per_line * context->image->height, IPC_CREAT|0777);
	context->shminfo.shmaddr = context->image->data = shmat(context->shminfo.shmid, 0, 0);
	context->shminfo.readOnly = False;
	XShmAttach(context->display, &context->shminfo);

	return context;
}


void xshm_frame_copy(void* context, char* dest, int size)
{
	struct xshm_context* xshm_context = (struct xshm_context*)context;

	XShmGetImage(xshm_context->display, RootWindow(xshm_context->display, 0), xshm_context->image, 0, 0, AllPlanes);
	XFlush(xshm_context->display);
	memcpy(dest, xshm_context->image->data, size);

}
