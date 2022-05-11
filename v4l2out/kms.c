#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#include <gbm.h>
#include <sys/mman.h>
#include <drm_fourcc.h>
#include <va/va.h>
#include <va/va_drm.h>
#include <va/va_drmcommon.h>

#define DRI_DEVICE_NODE "/dev/dri/card0"

uint32_t get_fb(const int drm_fd)
{
	int i;
	drmModeResPtr res;
	drmModeCrtcPtr crtc;
	uint32_t fb;

	res = drmModeGetResources(drm_fd);

	for(i = res->count_crtcs;i >= 0;--i)
	{
		crtc = drmModeGetCrtc(drm_fd, res->crtcs[i]);
		if(!crtc || !crtc->buffer_id)
			continue;
		
		fb = crtc->buffer_id;

		if(fb)
			break;
	}

	drmModeFreeCrtc(crtc);
	drmModeFreeResources(res);

	return fb;
}


struct kms_context
{
	int			drm_fd;
	VADisplay		va_display;
	struct gbm_device	*gbm;
};


void write_with_vaapi(struct kms_context* context, int dma_fd, drmModeFB2Ptr fb, char* dest)
{
	VASurfaceAttrib va_surface_attrs[2];
	VASurfaceID va_surface;
	VAImage va_image;
	VAImageFormat va_format;
	void* ptr = NULL;

#ifdef USE_TYPE_DRM_PRIME_V1
	VASurfaceAttribExternalBuffers buffer_desc = {0};
	unsigned long buffer_handle;

	va_surface_attrs[0].value.value.i	= VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
	va_surface_attrs[0].type		= VASurfaceAttribMemoryType;
	va_surface_attrs[0].flags		= VA_SURFACE_ATTRIB_SETTABLE;
	va_surface_attrs[0].value.type		= VAGenericValueTypeInteger;
	va_surface_attrs[0].value.value.i	= VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME;
	va_surface_attrs[1].type		= VASurfaceAttribExternalBufferDescriptor;
	va_surface_attrs[1].flags		= VA_SURFACE_ATTRIB_SETTABLE;
	va_surface_attrs[1].value.type		= VAGenericValueTypePointer;
	va_surface_attrs[1].value.value.p	= &buffer_desc;

	buffer_handle = dma_fd;
	buffer_desc.pixel_format = VA_FOURCC_RGBA;
	buffer_desc.width = fb->width;
	buffer_desc.height = fb->height;
	buffer_desc.num_buffers = 1;
	buffer_desc.buffers = &buffer_handle;
	buffer_desc.num_planes =1 ;
	buffer_desc.pitches[0] = fb->pitches[0];
	buffer_desc.offsets[0] = 0;
#else
	VADRMPRIMESurfaceDescriptor drm_surface_desc;

	va_surface_attrs[0].type		= VASurfaceAttribMemoryType;
	va_surface_attrs[0].flags		= VA_SURFACE_ATTRIB_SETTABLE;
	va_surface_attrs[0].value.type		= VAGenericValueTypeInteger;
	va_surface_attrs[0].value.value.i	= VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2;
	va_surface_attrs[1].type		= VASurfaceAttribExternalBufferDescriptor;
	va_surface_attrs[1].flags		= VA_SURFACE_ATTRIB_SETTABLE;
	va_surface_attrs[1].value.type		= VAGenericValueTypePointer;
	va_surface_attrs[1].value.value.p	= &drm_surface_desc;

	drm_surface_desc.fourcc = VA_FOURCC_RGBA;
	drm_surface_desc.width = fb->width;
	drm_surface_desc.height = fb->height;
	drm_surface_desc.num_objects = 1;
	drm_surface_desc.objects[0].fd = dma_fd;
	drm_surface_desc.objects[0].size = fb->pitches[0] * fb->height;
	drm_surface_desc.objects[0].drm_format_modifier = fb->modifier;

	drm_surface_desc.num_layers = 1;
	drm_surface_desc.layers[0].drm_format = fb->pixel_format;
	drm_surface_desc.layers[0].num_planes = 1;

	drm_surface_desc.layers[0].pitch[0] = fb->pitches[0];
	drm_surface_desc.layers[0].offset[0] = 0;
	drm_surface_desc.layers[0].object_index[0] = 0;
#endif

	vaCreateSurfaces(context->va_display, VA_RT_FORMAT_RGB32, fb->width, fb->height, &va_surface, 1, va_surface_attrs, 2);

	va_image.image_id = VA_INVALID_ID;
	va_image.buf = VA_INVALID_ID;

	va_format.fourcc = VA_FOURCC_RGBA;
	va_format.bits_per_pixel = 32;
	va_format.byte_order = VA_LSB_FIRST;

	vaCreateImage(context->va_display, &va_format, fb->width, fb->height, &va_image);
	vaGetImage(context->va_display, va_surface, 0, 0, fb->width, fb->height, va_image.image_id);
	vaMapBuffer(context->va_display, va_image.buf, &ptr);

	memcpy(dest, ptr, fb->pitches[0] * fb->height);

	vaUnmapBuffer(context->va_display, va_image.buf);
	vaDestroyImage(context->va_display, va_image.image_id);
	vaDestroySurfaces(context->va_display, &va_surface, 1);
}


void write_with_gbm(struct kms_context* context, int dma_fd, drmModeFB2Ptr fb, char* dest)
{
	struct gbm_bo* bo = NULL;
	struct gbm_import_fd_modifier_data data = {0};
	void* ptr = NULL;
	void* mapdata = NULL;

	data.fds[0] = dma_fd;
	data.width = fb->width;
	data.height = fb->height;
	data.format = fb->pixel_format;
	data.modifier = fb->modifier;
	data.num_fds = 1;
	data.strides[0] = fb->pitches[0];
	data.offsets[0] = 0;

	bo = gbm_bo_import(context->gbm, GBM_BO_IMPORT_FD_MODIFIER, &data, GBM_BO_USE_RENDERING);
	ptr = gbm_bo_map(bo, 0, 0, fb->width, fb->height, GBM_BO_TRANSFER_READ, (uint32_t*)&data.strides[0], &mapdata);

	memcpy(dest, ptr, fb->pitches[0] * fb->height);

	gbm_bo_unmap(bo, mapdata);
	gbm_bo_destroy(bo);
}


void* kms_init(void)
{
	struct kms_context* context = NULL;

	context = calloc(sizeof(struct kms_context), 1);

	context->drm_fd = open(DRI_DEVICE_NODE, O_RDWR | O_CLOEXEC);
	drmSetClientCap(context->drm_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);

#ifdef USE_VAAPI
	{
		int major, minor;
		context->va_display = vaGetDisplayDRM(context->drm_fd);
		vaInitialize(context->va_display, &major, &minor);
	}
#else
	context->gbm = gbm_create_device(context->drm_fd);
#endif

	return context;
}


void kms_frame_copy(void* context, char* dest, int size)
{
	struct kms_context* kms_context = (struct kms_context*)context;
	drmModeFB2Ptr fb;
	uint32_t fb_id = 0;
	int fd = 0;

	fb_id = get_fb(kms_context->drm_fd);
	fb = drmModeGetFB2(kms_context->drm_fd, fb_id);
	drmPrimeHandleToFD(kms_context->drm_fd, fb->handles[0], O_RDONLY, &fd);

#ifdef USE_VAAPI
	write_with_vaapi(kms_context, fd, fb, dest);
#else
	write_with_gbm(kms_context, fd, fb, dest);
#endif
	close(fd);

	drmModeFreeFB2(fb);
}


void kms_destroy(void** context)
{
	struct kms_context* kms_context = (struct kms_context*)*context;

#ifdef USE_VAAPI
	vaTerminate(kms_context->va_display);
#else
	gbm_device_destroy(kms_context->gbm);
#endif
	close(kms_context->drm_fd);
	free(kms_context);
	*context = NULL;
}
