#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>

#include "vkhelper.h"

struct vkhelper_swapsurface
{
	VkImage		image;
	VkImageView	view;
	VkCommandBuffer	cmdbuf;
	VkFence		fence;
};

struct vkhelper_swapchain
{
	VkSwapchainKHR			swapchain;
	int				width;
	int				height;
	uint32_t			nr_images;
	struct vkhelper_swapsurface*	surfaces;
	VkSemaphore			semaphore;
};

struct vkhelper_device
{
	VkInstance		instance;
	VkPhysicalDevice	phydevice;
	VkDevice		device;
	VkQueue			queue;
	VkSurfaceKHR		surface;
	VkCommandPool		cmdpool;
	VkCommandBuffer		cmdbuf;

	struct vkhelper_swapchain*	swapchain;

};

struct vkhelper_buffer
{
	VkBuffer	buffer;
	VkDeviceMemory	memory;
};

struct vkhelper_image
{
	VkImage		image;
	VkDeviceMemory	memory;
	VkImageView	view;
};

struct vkhelper_renderpass
{
	VkRenderPass			renderpass;
	struct vkhelper_swapchain*	swapchain;
	int				nr_framebuffers;
	VkFramebuffer*			framebuffers;
};


struct vkhelper_device* vkhelper_device_create_with_xlib(Display* display, Window window)
{
	const char* const extensions_for_instance[] =
	{
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
	};

	struct vkhelper_device* device = NULL;

	uint32_t		nr_phydevs;
	VkPhysicalDevice*	phydevs = NULL;

	int				i;
	uint32_t			nr_queuefamily;
	int				queuefamily_index;
	VkQueueFamilyProperties*	queuefamilyprops;


	device = calloc(1, sizeof(struct vkhelper_device));


	/* Create instance */

	vkCreateInstance
	(
		&(VkInstanceCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.enabledExtensionCount = sizeof(extensions_for_instance) / sizeof(const char* const),
			.ppEnabledExtensionNames = extensions_for_instance,
		},
		NULL, &device->instance
	);


	/* Select physical device */

	vkEnumeratePhysicalDevices(device->instance, &nr_phydevs, NULL);
	phydevs = calloc(nr_phydevs, sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(device->instance, &nr_phydevs, phydevs);

	device->phydevice = phydevs[0];
	free(phydevs);


	/* Select queue family */

	vkGetPhysicalDeviceQueueFamilyProperties(device->phydevice, &nr_queuefamily, NULL);
	queuefamilyprops = calloc(nr_queuefamily, sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(device->phydevice, &nr_queuefamily, queuefamilyprops);

	for(i = 0;i < nr_queuefamily;++i)
	{
		if(queuefamilyprops[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			queuefamily_index = i;
			break;
		}
	}

	free(queuefamilyprops);


	/* Create device and get command queue */

	vkCreateDevice
	(
		device->phydevice,
		&(VkDeviceCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pQueueCreateInfos = &(VkDeviceQueueCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueCount = 1,
				.queueFamilyIndex = queuefamily_index,
			},
			.queueCreateInfoCount = 1,
			.ppEnabledExtensionNames = &(const char*)
			{
				VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			},
			.enabledExtensionCount = 1,
		},
		NULL, &device->device
	);

	vkGetDeviceQueue(device->device, queuefamily_index, 0, &device->queue);


	/* Create Xlib surface */

	vkCreateXlibSurfaceKHR
	(
		device->instance,
		&(VkXlibSurfaceCreateInfoKHR)
		{
			.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
			.dpy = display,
			.window = window,
		},
		NULL, &device->surface
	);


	/* Create command pool */

	vkCreateCommandPool
	(
		device->device,
		&(VkCommandPoolCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.queueFamilyIndex = queuefamily_index,
		},
		NULL, &device->cmdpool
	);


	/* Allocate a single-use command buffer */

	vkAllocateCommandBuffers
	(
		device->device,
		&(VkCommandBufferAllocateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandPool = device->cmdpool,
			.commandBufferCount = 1,
		},
		&device->cmdbuf
	);

	return device;
}


void vkhelper_device_destroy(struct vkhelper_device* device)
{
	vkDestroyCommandPool(device->device, device->cmdpool, NULL);
	vkDestroySurfaceKHR(device->instance, device->surface, NULL);
	vkDestroyDevice(device->device, NULL);
	free(device);
}


VkDevice vkhelper_device_get_vkdevice(struct vkhelper_device* device)
{
	return device->device;
}


void vkhelper_device_set_swapchain(vkhelper_device* device, vkhelper_swapchain* swapchain)
{
	device->swapchain = swapchain;
}


struct vkhelper_swapchain* vkhelper_device_get_swapchain(vkhelper_device* device)
{
	return device->swapchain;
}


struct vkhelper_swapchain* vkhelper_swapchain_create(struct vkhelper_device* device, int width, int height, int min_count)
{
	int i;

	VkImage*		images;

	struct vkhelper_swapchain* swapchain = NULL;

	swapchain = calloc(1, sizeof(struct vkhelper_swapchain));

	vkCreateSwapchainKHR
	(
		device->device,
		&(VkSwapchainCreateInfoKHR)
		{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
			.presentMode = VK_PRESENT_MODE_FIFO_KHR,
			.surface = device->surface,
			.imageExtent.width = width,
			.imageExtent.height = height,
			// .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
			// .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			// .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.imageArrayLayers = 1,
			.minImageCount = min_count,
		},
		NULL, &swapchain->swapchain
	);

	swapchain->width = width;
	swapchain->height = height;

	vkGetSwapchainImagesKHR(device->device, swapchain->swapchain, &swapchain->nr_images, NULL);
	swapchain->surfaces = calloc(swapchain->nr_images, sizeof(struct vkhelper_swapsurface));
	images = calloc(swapchain->nr_images, sizeof(VkImage));

	vkGetSwapchainImagesKHR(device->device, swapchain->swapchain, &swapchain->nr_images, images);

	for(i = 0;i < swapchain->nr_images;++i)
	{

		swapchain->surfaces[i].image = images[i];

		vkCreateImageView
		(
			device->device,
			&(VkImageViewCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = swapchain->surfaces[i].image,
				.format = VK_FORMAT_B8G8R8A8_UNORM,
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				// .components.r = VK_COMPONENT_SWIZZLE_R,
				// .components.g = VK_COMPONENT_SWIZZLE_G,
				// .components.b = VK_COMPONENT_SWIZZLE_B,
				// .components.a = VK_COMPONENT_SWIZZLE_A,
				.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.subresourceRange.baseMipLevel = 0,
				.subresourceRange.levelCount = 1,
				.subresourceRange.baseArrayLayer = 0,
				.subresourceRange.layerCount = 1,
			},
			NULL, &swapchain->surfaces[i].view
		);

		vkAllocateCommandBuffers
		(
			device->device,
			&(VkCommandBufferAllocateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
				.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
				.commandPool = device->cmdpool,
				.commandBufferCount = 1,
			},
			&swapchain->surfaces[i].cmdbuf
		);

		vkCreateFence
		(
			device->device,
			&(VkFenceCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				.flags = VK_FENCE_CREATE_SIGNALED_BIT,
			},
			NULL, &swapchain->surfaces[i].fence
		);
	}

	free(images);

	vkCreateSemaphore
	(
		device->device,
		&(VkSemaphoreCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		},
		NULL, &swapchain->semaphore
	);

	return swapchain;
}


void vkhelper_swapchain_destroy(struct vkhelper_device* device, struct vkhelper_swapchain* swapchain)
{
	int i;

	vkDestroySemaphore(device->device, swapchain->semaphore, NULL);

	for(i = 0;i < swapchain->nr_images;++i)
	{
		vkFreeCommandBuffers(device->device, device->cmdpool, 1, &swapchain->surfaces[i].cmdbuf);
		vkDestroyImageView(device->device, swapchain->surfaces[i].view, NULL);
		vkDestroyFence(device->device, swapchain->surfaces[i].fence, NULL);
	}

	free(swapchain->surfaces);
	vkDestroySwapchainKHR(device->device, swapchain->swapchain, NULL);
	free(swapchain);
}


VkCommandBuffer vkhelper_surface_begin_cmdbuf(struct vkhelper_device* device, int index, int reset)
{
	vkBeginCommandBuffer
	(
		device->swapchain->surfaces[index].cmdbuf,
		&(VkCommandBufferBeginInfo)
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = reset ? VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT : 0,
		}
	);

	return device->swapchain->surfaces[index].cmdbuf;
}


void vkhelper_surface_end_cmdbuf(struct vkhelper_device* device, int index)
{
	vkEndCommandBuffer(device->swapchain->surfaces[index].cmdbuf);
}


VkCommandBuffer vkhelper_begin_cmdbuf(struct vkhelper_device* device)
{
	vkBeginCommandBuffer
	(
		device->cmdbuf,
		&(VkCommandBufferBeginInfo)
		{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		}
	);

	return device->cmdbuf;
}


void vkhelper_end_cmdbuf(struct vkhelper_device* device)
{
	vkEndCommandBuffer(device->cmdbuf);
}


static VkFramebuffer* vkhelper_framebuffers_create(struct vkhelper_device* device, VkRenderPass renderpass)
{
	int i;
	VkFramebuffer* fb;

	fb = calloc(device->swapchain->nr_images, sizeof(VkFramebuffer));

	for(i = 0;i < device->swapchain->nr_images;++i)
	{
		vkCreateFramebuffer
		(
			device->device,
			&(VkFramebufferCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.pAttachments = &device->swapchain->surfaces[i].view,
				.renderPass = renderpass,
				.attachmentCount = 1,
				.width = device->swapchain->width,
				.height = device->swapchain->height,
				.layers = 1,
			},
			NULL, &fb[i]
		);
	}

	return fb;
}


VkShaderModule vkhelper_shadermodule_create(struct vkhelper_device* device, const unsigned char* code, size_t size)
{
	VkShaderModule	shader;

	vkCreateShaderModule
	(
		device->device,
		&(VkShaderModuleCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = size,
			.pCode = (uint32_t*)code,
		},
		NULL, &shader
	);

	return shader;
}


static VkDeviceMemory vkhelper_memory_allocate(struct vkhelper_device* device, int is_image, void* object, VkMemoryPropertyFlags flags)
{
	int i, memtype;
	VkImage		image;
	VkBuffer	buffer;
	VkDeviceMemory	memory;

	VkMemoryRequirements			memreq;
	VkPhysicalDeviceMemoryProperties	memprop;

	image = is_image ? object : NULL;
	buffer = is_image ? NULL : object;

	if(is_image)
		vkGetImageMemoryRequirements(device->device, image, &memreq);
	else
		vkGetBufferMemoryRequirements(device->device, buffer, &memreq);

	vkGetPhysicalDeviceMemoryProperties(device->phydevice, &memprop);

	for(i = 0;i < memprop.memoryTypeCount;i++)
	{
		if(memreq.memoryTypeBits & (1 << i) && (memprop.memoryTypes[i].propertyFlags & flags) == flags)
		{
			memtype = i;
			break;
		}
	}

	vkAllocateMemory
	(
		device->device,
		&(VkMemoryAllocateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memreq.size,
			.memoryTypeIndex = memtype,
		},
		NULL, &memory
	);

	if(is_image)
		vkBindImageMemory(device->device, image, memory, 0);
	else
		vkBindBufferMemory(device->device, buffer, memory, 0);

	return memory;
}


struct vkhelper_buffer* vkhelper_buffer_create(struct vkhelper_device* device, enum vkhelper_buffer_usage usage, size_t size)
{
	VkBufferUsageFlags	usage_flags;
	VkMemoryPropertyFlags	prop_flags;

	struct vkhelper_buffer*	buffer = NULL;

	buffer = calloc(1, sizeof(struct vkhelper_buffer));

	switch(usage)
	{
		case VKHELPER_BUFFER_USAGE_STAGING:
			usage_flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			prop_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			break;
		case VKHELPER_BUFFER_USAGE_VERTEX:
			usage_flags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			prop_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		default:
			break;
	}

	vkCreateBuffer
	(
		device->device,
		&(VkBufferCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = size,
			.usage = usage_flags,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		},
		NULL,
		&buffer->buffer
	);

	buffer->memory = vkhelper_memory_allocate(device, False, buffer->buffer, prop_flags);

	return buffer;
}


void vkhelper_buffer_destroy(struct vkhelper_device* device, struct vkhelper_buffer* buffer)
{
	vkFreeMemory(device->device, buffer->memory, NULL);
	vkDestroyBuffer(device->device, buffer->buffer, NULL);
	free(buffer);
}


VkBuffer vkhelper_buffer_get_vkbuffer(struct vkhelper_buffer* buffer)
{
	return buffer->buffer;
}


struct vkhelper_buffer* vkhelper_vertex_buffer_create(struct vkhelper_device* device, void* data, size_t size)
{
	struct vkhelper_buffer* staging = NULL;
	struct vkhelper_buffer* buffer = NULL;
	void* ptr = NULL;

	staging = vkhelper_buffer_create(device, VKHELPER_BUFFER_USAGE_STAGING, size);

	vkMapMemory(device->device, staging->memory, 0, size, 0, &ptr);
	memcpy(ptr, data, size);
	vkUnmapMemory(device->device, staging->memory);

	buffer = vkhelper_buffer_create(device, VKHELPER_BUFFER_USAGE_VERTEX, size);

	vkhelper_begin_cmdbuf(device);
	vkCmdCopyBuffer
	(
		device->cmdbuf, staging->buffer, buffer->buffer, 1,
		&(VkBufferCopy)
		{
			.size = size,
		}
	);
	vkhelper_end_cmdbuf(device);

	vkQueueSubmit
	(
		device->queue, 1,
		&(VkSubmitInfo)
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pCommandBuffers = &device->cmdbuf,
			.commandBufferCount = 1,
		},
		VK_NULL_HANDLE
	);

	vkQueueWaitIdle(device->queue);

	vkhelper_buffer_destroy(device, staging);

	return buffer;
}


struct vkhelper_image* vkhelper_image_create(struct vkhelper_device* device, void* data, int width, int height)
{
	size_t size;
	void* ptr;
	struct vkhelper_buffer* staging = NULL;
	struct vkhelper_image*	image = NULL;

	size = width * height * 4;

	image = calloc(1, sizeof(struct vkhelper_image));

	staging = vkhelper_buffer_create(device, VKHELPER_BUFFER_USAGE_STAGING, size);

	vkMapMemory(device->device, staging->memory, 0, size, 0, &ptr);
	memcpy(ptr, data, size);
	vkUnmapMemory(device->device, staging->memory);

	vkCreateImage
	(
		device->device,
		&(VkImageCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.extent.width = width,
			.extent.height = height,
			.extent.depth = 1,
			.mipLevels = 1,
			.arrayLayers = 1,
			.format = VK_FORMAT_B8G8R8A8_UNORM,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.samples = VK_SAMPLE_COUNT_1_BIT,
		},
		NULL, &image->image
	);

	image->memory = vkhelper_memory_allocate(device, True, image->image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vkhelper_begin_cmdbuf(device);
	vkCmdPipelineBarrier
	(
		device->cmdbuf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
		&(VkImageMemoryBarrier)
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.image = image->image,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.levelCount = 1,
			.subresourceRange.layerCount = 1,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		}
	);
	vkCmdCopyBufferToImage
	(
		device->cmdbuf, staging->buffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
		&(VkBufferImageCopy)
		{
			.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.imageSubresource.layerCount = 1,
			.imageExtent =
			{
				width,
				height,
				1,
			},
		}
	);
	vkCmdPipelineBarrier
	(
		device->cmdbuf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
		&(VkImageMemoryBarrier)
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.image = image->image,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.levelCount = 1,
			.subresourceRange.layerCount = 1,
			.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		}
	);
	vkhelper_end_cmdbuf(device);

	vkQueueSubmit
	(
		device->queue, 1,
		&(VkSubmitInfo)
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pCommandBuffers = &device->cmdbuf,
			.commandBufferCount = 1,
		},
		VK_NULL_HANDLE
	);
	vkQueueWaitIdle(device->queue);

	vkCreateImageView
	(
		device->device,
		&(VkImageViewCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = image->image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_A8B8G8R8_UNORM_PACK32,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.levelCount = 1,
			.subresourceRange.layerCount = 1,
		},
		NULL, &image->view
	);

	vkhelper_buffer_destroy(device, staging);
	return image;
}


void vkhelper_image_destroy(struct vkhelper_device* device, struct vkhelper_image* image)
{
	vkDestroyImageView(device->device, image->view, NULL);
	vkDestroyImage(device->device, image->image, NULL);
	vkFreeMemory(device->device, image->memory, NULL);
	free(image);
}


VkImageView vkhelper_image_get_vkimageview(struct vkhelper_image* image)
{
	return image->view;
}


uint32_t vkhelper_acquire_next_index(struct vkhelper_device* device)
{
	uint32_t index;

	vkAcquireNextImageKHR(device->device, device->swapchain->swapchain, UINT64_MAX, device->swapchain->semaphore, VK_NULL_HANDLE, &index);
	vkWaitForFences(device->device, 1, &device->swapchain->surfaces[index].fence, VK_TRUE, UINT64_MAX);
	vkResetFences(device->device, 1, &device->swapchain->surfaces[index].fence);

	return index;
}


void vkhelper_queue_submit(struct vkhelper_device* device, uint32_t index)
{
	VkPipelineStageFlags stageflag = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	vkQueueSubmit
	(
		device->queue, 1,
		&(VkSubmitInfo)
		{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.pCommandBuffers = &device->swapchain->surfaces[index].cmdbuf,
			.pWaitSemaphores = &device->swapchain->semaphore,
			.pWaitDstStageMask = &stageflag,
			.commandBufferCount = 1,
			.waitSemaphoreCount = 1,
		},
		device->swapchain->surfaces[index].fence
	);
}


void vkhelper_queue_present(struct vkhelper_device* device, uint32_t index)
{
	vkQueuePresentKHR
	(
		device->queue,
		&(VkPresentInfoKHR)
		{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pSwapchains = &device->swapchain->swapchain,
			.pImageIndices = &index,
			.swapchainCount = 1,
		}
	);
	vkQueueWaitIdle(device->queue);
}


struct vkhelper_renderpass* vkhelper_renderpass_create(struct vkhelper_device* device, VkRenderPassCreateInfo* info)
{
	struct vkhelper_renderpass*	renderpass = NULL;
	renderpass = calloc(1, sizeof(struct vkhelper_renderpass));

	vkCreateRenderPass(device->device, info, NULL, &renderpass->renderpass);
	vkhelper_renderpass_validate_swapchain(device, renderpass);
	return renderpass;
}


void vkhelper_renderpass_destroy(struct vkhelper_device* device, struct vkhelper_renderpass* renderpass)
{
	int i;

	for(i = 0;i < renderpass->nr_framebuffers;++i)
		vkDestroyFramebuffer(device->device, renderpass->framebuffers[i], NULL);
	free(renderpass->framebuffers);
	vkDestroyRenderPass(device->device, renderpass->renderpass, NULL);
	free(renderpass);
}


void vkhelper_renderpass_validate_swapchain(struct vkhelper_device* device, struct vkhelper_renderpass* renderpass)
{
	int i;

	if(renderpass->framebuffers)
	{
		for(i = 0;i < renderpass->nr_framebuffers;++i)
			vkDestroyFramebuffer(device->device, renderpass->framebuffers[i], NULL);
	}

	free(renderpass->framebuffers);

	renderpass->framebuffers = vkhelper_framebuffers_create(device, renderpass->renderpass);
	renderpass->swapchain = device->swapchain;
	renderpass->nr_framebuffers = device->swapchain->nr_images;

}


void vkhelper_begin_renderpass(VkCommandBuffer cmdbuf, struct vkhelper_renderpass* renderpass, uint32_t index)
{
	vkCmdBeginRenderPass
	(
		cmdbuf,
		&(VkRenderPassBeginInfo)
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = renderpass->renderpass,
			.framebuffer = renderpass->framebuffers[index],
			.renderArea.extent.width = renderpass->swapchain->width,
			.renderArea.extent.height = renderpass->swapchain->height,
			.clearValueCount = 1,
			.pClearValues = &(VkClearValue)
			{
				.color.float32 = {0.0, 0.0, 0.0, 1.0},
			}
		},
		VK_SUBPASS_CONTENTS_INLINE
	);
}


VkRenderPass vkhelper_renderpass_get_vkrenderpass(struct vkhelper_renderpass* renderpass)
{
	return renderpass->renderpass;
}


void vkhelper_cmd_clear(struct vkhelper_device* device, VkCommandBuffer cmdbuf, uint32_t index, float r, float g, float b, float a)
{
	vkCmdClearColorImage
	(
		cmdbuf, device->swapchain->surfaces[index].image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		&(VkClearColorValue)
		{
			.float32 = {r, g, b, a},
		},
		1,
		&(VkImageSubresourceRange)
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1,
		}
	);
}


VkPipeline vkhelper_create_graphics_pipeline
(
	struct vkhelper_device* device,
	VkShaderModule vertex_shader,
	VkShaderModule fragment_shader,
	VkPipelineVertexInputStateCreateInfo* vertex_input,
	VkPipelineLayout pipelinelayout,
	struct vkhelper_renderpass* renderpass
)
{
	VkPipeline pipeline;

	vkCreateGraphicsPipelines
	(
		device->device, VK_NULL_HANDLE, 1,
		&(VkGraphicsPipelineCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = 2,
			.pStages = (VkPipelineShaderStageCreateInfo[2])
			{
				{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = VK_SHADER_STAGE_VERTEX_BIT,
					.module = vertex_shader,
					.pName = "main",
				},
				{
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
					.module = fragment_shader,
					.pName = "main",
				},
			},
			.pVertexInputState = vertex_input,
			.pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
				.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			},
			/* pViewportState must NOT be NULL on Intel */
			.pViewportState = &(VkPipelineViewportStateCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			},
			.pRasterizationState = &(VkPipelineRasterizationStateCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
				.lineWidth = 1.0f,
			},
			.pMultisampleState = &(VkPipelineMultisampleStateCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
				.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			},
			// .pDepthStencilState = &depthinfo,
			.pColorBlendState = &(VkPipelineColorBlendStateCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
				.attachmentCount = 1,
				.pAttachments = &(VkPipelineColorBlendAttachmentState)
				{
					.blendEnable = VK_TRUE,
					.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
					.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
					.colorBlendOp = VK_BLEND_OP_ADD,
					.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
					.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
					.alphaBlendOp = VK_BLEND_OP_ADD,
					.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
				},
			},
			.pDynamicState = &(VkPipelineDynamicStateCreateInfo)
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
				.dynamicStateCount = 2,
				.pDynamicStates = (VkDynamicState[])
				{
					VK_DYNAMIC_STATE_VIEWPORT,
					VK_DYNAMIC_STATE_SCISSOR,
				},
			},
			.layout = pipelinelayout,
			.renderPass = renderpass->renderpass,
			.subpass = 0,
		},
		NULL, &pipeline
	);

	return pipeline;
}