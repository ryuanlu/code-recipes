#ifndef	__VKHELPER_H__
#define	__VKHELPER_H__

#include <vulkan/vulkan.h>
#include <X11/Xlib.h>

#ifdef	__c_plusplus
extern "C"
{
#endif

enum vkhelper_buffer_usage
{
	VKHELPER_BUFFER_USAGE_STAGING,
	VKHELPER_BUFFER_USAGE_VERTEX,
};

typedef struct vkhelper_device		vkhelper_device;
typedef struct vkhelper_swapchain	vkhelper_swapchain;
typedef struct vkhelper_buffer		vkhelper_buffer;
typedef struct vkhelper_image		vkhelper_image;
typedef struct vkhelper_renderpass	vkhelper_renderpass;


vkhelper_device*	vkhelper_device_create_with_xlib	(Display* display, Window window);
void			vkhelper_device_destroy			(vkhelper_device* device);
VkDevice		vkhelper_device_get_vkdevice		(vkhelper_device* device);
void			vkhelper_device_set_swapchain		(vkhelper_device* device, vkhelper_swapchain* swapchain);
vkhelper_swapchain*	vkhelper_device_get_swapchain		(vkhelper_device* device);

vkhelper_swapchain*	vkhelper_swapchain_create	(vkhelper_device* device, int width, int height, int min_count);
void			vkhelper_swapchain_destroy	(vkhelper_device* device, vkhelper_swapchain* swapchain);


VkCommandBuffer	vkhelper_surface_begin_cmdbuf	(vkhelper_device* device, int index, int reset);
void		vkhelper_surface_end_cmdbuf	(vkhelper_device* device, int index);


VkCommandBuffer	vkhelper_begin_cmdbuf	(vkhelper_device* device);
void		vkhelper_end_cmdbuf	(vkhelper_device* device);

VkShaderModule	vkhelper_shadermodule_create(vkhelper_device* device, const unsigned char* code, size_t size);


vkhelper_buffer*	vkhelper_buffer_create		(vkhelper_device* device, enum vkhelper_buffer_usage usage, size_t size);
void			vkhelper_buffer_destroy		(vkhelper_device* device, vkhelper_buffer* buffer);
VkBuffer		vkhelper_buffer_get_vkbuffer	(vkhelper_buffer* buffer);
vkhelper_buffer*	vkhelper_vertex_buffer_create	(vkhelper_device* device, void* data, size_t size);

vkhelper_image*	vkhelper_image_create		(vkhelper_device* device, void* image, int width, int height);
void		vkhelper_image_destroy		(vkhelper_device* device, vkhelper_image* image);
VkImageView	vkhelper_image_get_vkimageview	(vkhelper_image* image);

uint32_t	vkhelper_acquire_next_index	(vkhelper_device* device);
void		vkhelper_queue_submit		(vkhelper_device* device, uint32_t index);
void		vkhelper_queue_present		(vkhelper_device* device, uint32_t index);

vkhelper_renderpass*	vkhelper_renderpass_create		(vkhelper_device* device, VkRenderPassCreateInfo* info);
void			vkhelper_renderpass_destroy		(vkhelper_device* device, vkhelper_renderpass* renderpass);
void			vkhelper_renderpass_validate_swapchain	(vkhelper_device* device, vkhelper_renderpass* renderpass);
void			vkhelper_begin_renderpass		(VkCommandBuffer cmdbuf, vkhelper_renderpass* renderpass, uint32_t index);
VkRenderPass		vkhelper_renderpass_get_vkrenderpass	(vkhelper_renderpass* renderpass);

void	vkhelper_cmd_clear	(vkhelper_device* device, VkCommandBuffer cmdbuf, uint32_t index, float r, float g, float b, float a);

VkPipeline vkhelper_create_graphics_pipeline
(
	vkhelper_device* device,
	VkShaderModule vertex_shader,
	VkShaderModule fragment_shader,
	VkPipelineVertexInputStateCreateInfo* vertex_input,
	VkPipelineLayout pipelinelayout,
	vkhelper_renderpass* renderpass
);

#ifdef	__c_plusplus
}
#endif

#endif	/* __VKHELPER_H__ */