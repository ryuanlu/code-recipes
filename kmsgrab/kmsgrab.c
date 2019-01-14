#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <sys/mman.h>

#define DRI_DEVICE_NODE		"/dev/dri/card0"
#define INTEL_XTILE_WIDTH	(512)
#define INTEL_XTILE_HEIGHT	(8)

drmModeFBPtr get_fb(const int drm_fd)
{
	int i;
	drmModeResPtr res;
	drmModeCrtcPtr crtc;
	drmModeFBPtr fb;
	int fb_dmafd = 0;

	drmSetClientCap(drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	res = drmModeGetResources(drm_fd);

	for(i = res->count_crtcs;i >= 0;--i)
	{
		crtc = drmModeGetCrtc(drm_fd, res->crtcs[i]);
		if(!crtc || !crtc->buffer_id)
			continue;
		fb = drmModeGetFB(drm_fd, crtc->buffer_id);
		drmModeFreeCrtc(crtc);
		if(fb)
			break;
	}

	drmPrimeHandleToFD(drm_fd, fb->handle, O_RDONLY, &fb_dmafd);
	drmModeFreeResources(res);

	return fb;
}

void tiled_to_linear(char *input, char *output, int image_width, int image_height, int xtile_width, int xtile_height)
{
	int xtile_bytes = xtile_width * xtile_height;
	int image_bytes = image_width * image_height;
	int i, x, y, ntile;

	for(i = 0;i < image_bytes;++i)
	{
		ntile = i / xtile_bytes;

		x = (ntile % (image_width / xtile_width)) * xtile_width + (i % xtile_bytes) % xtile_width;
		y = (ntile / (image_width / xtile_width)) * xtile_height + (i % xtile_bytes) / xtile_width;

		output[y * image_width + x] = input[i];
	}
}

int main(int argc, char const *argv[])
{
	drmModeFBPtr fb = NULL;
	int drm_fd = 0;
	int fb_dmafd = 0;
	char* ptr = NULL;
	char* image = NULL;
	FILE* fp = NULL;

	drm_fd = open(DRI_DEVICE_NODE, O_RDWR | FD_CLOEXEC);
	fb = get_fb(drm_fd);

	drmPrimeHandleToFD(drm_fd, fb->handle, O_RDONLY, &fb_dmafd);
	fprintf(stderr, "drm framebuffer export dmabuf as fd %d\n", fb_dmafd);

	ptr = mmap(NULL, fb->pitch * fb->height, PROT_READ, MAP_SHARED, fb_dmafd, 0);
	fprintf(stderr, "mmap fd %d: %p\n", fb_dmafd, ptr);

	image = calloc(fb->pitch * fb->height, 1);
	tiled_to_linear(ptr, image, fb->pitch, fb->height, INTEL_XTILE_WIDTH, INTEL_XTILE_HEIGHT);
	munmap(ptr, fb->pitch * fb->height);

	fp = fopen("output.bin", "w");
	fwrite(image, fb->pitch * fb->height, 1, fp);
	fclose(fp);
	free(image);

	close(fb_dmafd);
	drmModeFreeFB(fb);
	close(drm_fd);

	return EXIT_SUCCESS;
}
