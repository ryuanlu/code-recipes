#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include "vkhelper.h"

#define TITLE_STRING "Xlib test"

#define CLEAR_COLOR	0.5f, 0.5f, 0.5f, 1.0f

#define WINDOW_WIDTH	(1280)
#define WINDOW_HEIGHT	(720)

#define TEXTURE_IMAGE_FILE	"cthead.bin"
#define TEXTURE_IMAGE_WIDTH	(256)
#define TEXTURE_IMAGE_HEIGHT	(256)
#define TEXTURE_IMAGE_SIZE	(TEXTURE_IMAGE_WIDTH * TEXTURE_IMAGE_HEIGHT * 4)

#define VK_SRC_MEMORY_FLAGS	(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
#define VK_DST_MEMORY_FLAGS	(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)

extern unsigned char blit_vert_spv[];
extern unsigned int blit_vert_spv_len;
extern unsigned char blit_frag_spv[];
extern unsigned int blit_frag_spv_len;

float model_data[] =
{
	-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
	-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,

	0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f,
	-0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
	0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
};

int main(int argc, char const *argv[])
{
	int quit = False;
	Display		*X_display = NULL;
	Window		window;
	XTextProperty	title;
	XEvent		event;
	Atom		wm_delete_window;

	vkhelper_device*	vdevice;
	vkhelper_swapchain*	swapchain;

	VkDevice		device;

	vkhelper_renderpass*	renderpass;

	VkShaderModule		vshader;
	VkShaderModule		fshader;

	VkDescriptorSetLayout	setlayout;
	VkPipelineLayout	pipelinelayout;
	VkPipeline		pipeline = NULL;

	vkhelper_buffer*	vertex_buffer;
	vkhelper_image*		texture_image;

	VkSampler	sampler;

	VkDescriptorPool	desc_pool;
	VkDescriptorSet		desc_set;

	char* image_data;
	FILE* fp;

	unsigned int index;

	struct timespec	ts_a, ts_b;

	X_display = XOpenDisplay(NULL);
	window = XCreateWindow(X_display, RootWindow(X_display, 0), 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0,  CopyFromParent, CopyFromParent,  CopyFromParent, 0, NULL);
	XSetWindowBackground(X_display, window, BlackPixel(X_display, 0));
	title.value = (unsigned char*)TITLE_STRING;
	title.encoding = XA_STRING;
	title.format = 8;
	title.nitems = strlen(TITLE_STRING);
	XSetWMProperties(X_display, window, &title, &title, NULL, 0, NULL, NULL, NULL);
	wm_delete_window = XInternAtom(X_display, "WM_DELETE_WINDOW", False);
	XSetWMProtocols(X_display, window, &wm_delete_window, 1);
	XSelectInput(X_display, window, ExposureMask | KeyPressMask | StructureNotifyMask);
	XMapWindow(X_display, window);

	vdevice = vkhelper_device_create_with_xlib(X_display, window);
	swapchain = vkhelper_swapchain_create(vdevice, WINDOW_WIDTH, WINDOW_HEIGHT, 2);
	vkhelper_device_set_swapchain(vdevice, swapchain);

	device = vkhelper_device_get_vkdevice(vdevice);

	renderpass = vkhelper_renderpass_create
	(
		vdevice,
		&(VkRenderPassCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &(VkAttachmentDescription)
			{
				.format = VK_FORMAT_B8G8R8A8_UNORM,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			},
			.subpassCount = 1,
			.pSubpasses = &(VkSubpassDescription)
			{
				.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
				.colorAttachmentCount = 1,
				.pColorAttachments = &(VkAttachmentReference)
				{
					.attachment = 0,
					.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				},
			},
		}
	);


	/* Create shaders */

	vshader = vkhelper_shadermodule_create(vdevice, blit_vert_spv, blit_vert_spv_len);
	fshader = vkhelper_shadermodule_create(vdevice, blit_frag_spv, blit_frag_spv_len);

	/* Create pipeline */

	vkCreateDescriptorSetLayout
	(
		device,
		&(VkDescriptorSetLayoutCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1,
			.pBindings = &(VkDescriptorSetLayoutBinding)
			{
				.binding = 1,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
			},
		},
		NULL, &setlayout
	);

	vkCreatePipelineLayout
	(
		device,
		&(VkPipelineLayoutCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &setlayout,
		},
		NULL, &pipelinelayout
	);

	pipeline = vkhelper_create_graphics_pipeline
	(
		vdevice, vshader, fshader,
		&(VkPipelineVertexInputStateCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = (VkVertexInputBindingDescription[])
			{
				{
					.binding = 0,
					.stride = 8 * sizeof(float),
					.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
				},
			},
			.vertexAttributeDescriptionCount = 3,
			.pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[])
			{
				{
					.location = 0,
					.binding = 0,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = 0,
				},
				{
					.location = 1,
					.binding = 0,
					.format = VK_FORMAT_R32G32_SFLOAT,
					.offset = 2 * sizeof(float),
				},
				{
					.location = 2,
					.binding = 0,
					.format = VK_FORMAT_R32G32B32A32_SFLOAT,
					.offset = 4 * sizeof(float),
				},
			},
		},
		pipelinelayout,
		renderpass
	);

	/* Create vertex buffer */

	vertex_buffer = vkhelper_vertex_buffer_create(vdevice, model_data, sizeof(model_data));

	/* Create texture image */

	fp = fopen(TEXTURE_IMAGE_FILE, "r");
	image_data = calloc(1, TEXTURE_IMAGE_SIZE);
	fread(image_data, TEXTURE_IMAGE_SIZE, 1, fp);
	fclose(fp);

	texture_image = vkhelper_image_create(vdevice, image_data, TEXTURE_IMAGE_WIDTH, TEXTURE_IMAGE_HEIGHT);

	free(image_data);

	vkCreateSampler
	(
		device,
		&(VkSamplerCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = 16,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		},
		NULL, &sampler
	);

	vkCreateDescriptorPool
	(
		device,
		&(VkDescriptorPoolCreateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.maxSets = 1,
			.poolSizeCount = 1,
			.pPoolSizes = (VkDescriptorPoolSize[])
			{
				{
					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = 1
				},
			},
		},
		NULL, &desc_pool
	);

	vkAllocateDescriptorSets
	(
		device,
		&(VkDescriptorSetAllocateInfo)
		{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = desc_pool,
			.descriptorSetCount = 1,
			.pSetLayouts = &setlayout,
		},
		&desc_set
	);

	vkUpdateDescriptorSets
	(
		device,
		1,
		&(VkWriteDescriptorSet)
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = desc_set,
			.dstBinding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.pImageInfo = &(VkDescriptorImageInfo)
			{
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.imageView = vkhelper_image_get_vkimageview(texture_image),
				.sampler = sampler,
			},
		},
		0, NULL
	);

	index = 0;
	while(!quit)
	{
		float fps;
		VkCommandBuffer cmdbuf;

		clock_gettime(CLOCK_MONOTONIC, &ts_a);

		index = vkhelper_acquire_next_index(vdevice);

		/* Draw calls */

		cmdbuf = vkhelper_surface_begin_cmdbuf(vdevice, index, VK_TRUE);
		vkhelper_begin_renderpass(cmdbuf, renderpass, index);

		vkCmdSetViewport(cmdbuf, 0, 1, &(VkViewport) { .width = WINDOW_WIDTH, .height = WINDOW_HEIGHT,});
		vkCmdSetScissor(cmdbuf, 0, 1, &(VkRect2D) { .extent.width = WINDOW_WIDTH, .extent.height = WINDOW_HEIGHT,});

		vkhelper_cmd_clear(vdevice, cmdbuf, index, CLEAR_COLOR);

		vkCmdBindPipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		vkCmdBindVertexBuffers(cmdbuf, 0, 1, (VkBuffer[]) {vkhelper_buffer_get_vkbuffer(vertex_buffer)}, (VkDeviceSize[]) {0});
		vkCmdBindDescriptorSets(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelinelayout, 0, 1, &desc_set, 0, NULL);
		vkCmdDraw(cmdbuf, 6, 1, 0, 0);

		vkCmdEndRenderPass(cmdbuf);
		vkhelper_surface_end_cmdbuf(vdevice, index);

		vkhelper_queue_submit(vdevice, index);

		/* Swap buffers */

		vkhelper_queue_present(vdevice, index);

		clock_gettime(CLOCK_MONOTONIC, &ts_b);
		fps = 1000.0f / ((ts_b.tv_sec - ts_a.tv_sec) * 1000 + (float)(ts_b.tv_nsec - ts_a.tv_nsec) / 1000000);
		fprintf(stderr, "%3.2f\n", fps);

		if(XPending(X_display))
		{
			XNextEvent(X_display, &event);
			switch(event.type)
			{
			case ClientMessage:
				if(event.xclient.data.l[0] == wm_delete_window)
				{
					quit = True;
				}
				break;
			}
		}
	}

	vkDeviceWaitIdle(device);


	/* Free all resources */

	vkDestroySampler(device, sampler, NULL);

	vkDestroyDescriptorPool(device, desc_pool, NULL);
	vkDestroyDescriptorSetLayout(device, setlayout, NULL);
	vkDestroyPipeline(device, pipeline, NULL);
	vkDestroyPipelineLayout(device, pipelinelayout, NULL);

	vkhelper_renderpass_destroy(vdevice, renderpass);
	vkhelper_image_destroy(vdevice, texture_image);
	vkhelper_buffer_destroy(vdevice, vertex_buffer);
	vkDestroyShaderModule(device, vshader, NULL);
	vkDestroyShaderModule(device, fshader, NULL);
	vkhelper_swapchain_destroy(vdevice, swapchain);
	vkhelper_device_destroy(vdevice);

	XDestroyWindow(X_display, window);
	XCloseDisplay(X_display);

	return EXIT_SUCCESS;
}
