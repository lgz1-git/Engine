#pragma once
#include "h_win32_platform.h"
#include "vulkan/vulkan.h"
#if defined (_WIN32)
#include <vulkan/vulkan_win32.h>
#endif

#include "h_type.h"

#include <assert.h>
#include <vector>
#include <string>
#include <string_view>
#include <memory>

#define vk_assert(p) assert(p==VK_SUCCESS)

struct vk_select_queuefamily
{
	u32 graphics_family_index;
	u32 present_family_index;
	u32 compute_family_index;
	u32 transfer_family_index;
};

enum vk_renderpass_state {
	//todo:fill this structure
	READY,
	RECORDING,
	IN_RENDER_PASS,
	RECORDING_ENDED,
	SUBMITTED,
	NOT_ALLOCATED
};

enum vk_cmdbuffer_state {
	//todo:fill this
	CMD_READY,
	CMD_RECORDING,
	CMD_IN_RENDER_PASS,
	CMD_RECORDING_ENDED,
	CMD_SUBMITTED,
	CMD_NOT_ALLOCATED
};
struct vk_pdevice_queue_requirements
{
	//queue requirements
	bool graphics;
	bool present;
	bool compute;
	bool transfer;

	bool sampler_anisotropy;
	bool discrete_gpu;
};

struct vk_pdevice_swapchain_potency
{
	VkSurfaceCapabilitiesKHR capabilities;
	u32 format_counts;
	u32 present_modes_counts;
	//TODO::to do
	VkSurfaceFormatKHR* surface_formats;
	VkPresentModeKHR* present_modes;

};

struct vk_device
{
	VkDevice logical_device;
	VkPhysicalDevice physical_device;
	vk_select_queuefamily queue_family_info;
	vk_pdevice_swapchain_potency pdevice_swapchain_potency;

	VkQueue graphics_queue;
	VkQueue present_queue;
	VkQueue transfer_queue;

	VkFormat depth_format;
};


struct vk_depth_image {
	VkImage image_handle;
	VkDeviceMemory memory;
	VkImageView view;
	u32 w;
	u32 h;
};
struct vk_swapchain_info
{
	VkSurfaceFormatKHR surface_format;
	u8 max_frames_in_flight;
	VkSwapchainKHR swapchain_handle;
	u32 image_counts;
	//TODO::clean up
	VkImage* images;
	VkImageView* views;

	vk_depth_image depth_image;
};

struct vk_renderpass {
	VkRenderPass renderpass_handle;
	f32 x, y, w, h;
	f32 r, g, b, a;

	f32 depth;
	u32 stencil;

	vk_renderpass_state state;
};

struct vk_cmdbuffer {
	VkCommandBuffer cmdbuffer_handle;

	vk_cmdbuffer_state cmdbuffer_state;
};
struct vk_context
{

	VkInstance vk_instance;

	VkAllocationCallbacks *vk_allocator;

	VkSurfaceKHR vk_surface;

	vk_device device;

	vk_renderpass main_renderpass;

	std::vector<const char* > instance_extensions;
	std::vector<const char* > instance_layers_extensions;
	std::vector<const char* > device_extensions;
	std::vector<const char* > device_layers_extensions;

	vk_swapchain_info swapchain_info;

	u32 image_index;
	u32 current_frame;

	u32 frame_buffer_w;
	u32 frame_buffer_h;
};



void vk_init_extensions(vk_context* context);

void vk_create_instance(vk_context* context);

void vk_create_surface(vk_context* context, win32_platform_context* win_context);

bool vk_select_pdevice(vk_context* context);


bool vk_pdevice_meets_required(
	VkPhysicalDevice device,
	VkSurfaceKHR surface,
	const VkPhysicalDeviceProperties* properties,
	const VkPhysicalDeviceFeatures* features,
	const vk_pdevice_queue_requirements* requirements,
	vk_select_queuefamily* queue_family_info,
	vk_pdevice_swapchain_potency* device_swapchain_potency);

void vk_query_pdevice_swapchain_potency(VkPhysicalDevice pdevice,
	VkSurfaceKHR surface,
	vk_pdevice_swapchain_potency* device_swapchain_potency);

bool vk_query_pdevice_depth_format(vk_device* device);


void vk_create_device(vk_context* context);

void vk_create_swapchain(vk_context* context,vk_swapchain_info* swapchain_info,u32 w,u32 h);

void vk_create_image(
	vk_context* context,
	VkImageType image_type,
	u32 w,
	u32 h,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlagBits usage,
	VkMemoryPropertyFlags memory_flags,
	bool create_view,
	VkImageAspectFlags view_aspect_flags,
	vk_depth_image* image);

void vk_create_image_view(
	vk_context* context,
	VkFormat format,
	vk_depth_image* image,
	VkImageAspectFlags aepect_flag);

void vk_recreate_swapchain(vk_context* context,vk_swapchain_info* swapchain_info,u32 w,u32 h);

void vk_destory_swapchain(vk_context* context, vk_swapchain_info* swapchain_info);


bool vk_swapchain_acquire_next_image_index(
	vk_context* context,
	vk_swapchain_info* swapchain_info,
	u64 timeout_ns,
	VkSemaphore image_available_semaphore,
	VkFence fence,
	u32 out_image_index);


void vk_swapchain_present(
	vk_context* context,
	vk_swapchain_info* swapchain_info,
	VkQueue graphics_queue,
	VkQueue present_queue,
	VkSemaphore render_complete_semaphore,
	u32 present_image_index);


void vk_create_renderpass(
	vk_context* context,
	vk_renderpass* renderpass,
	f32 x, f32 y, f32 w, f32 h,
	f32 r, f32 g, f32 b, f32 a,
	f32 depth, u32 stencil);

void vk_destory_renderpass(vk_context* context, vk_renderpass* rederpass);

void vk_renderpass_begin(vk_cmdbuffer* cmdbuffer,vk_renderpass* renderpass,VkFramebuffer frame_buffer);

void vk_renderpass_end(vk_cmdbuffer* cmdbuffer, vk_renderpass* renderpass);

void vk_cmdbuffer_allocate(
	vk_context* context,
	VkCommandPool pool,
	bool is_primary,
	vk_cmdbuffer* cmdbuffer);

void vk_cmdbuffer_free(
	vk_context* context,
	VkCommandPool pool,
	vk_cmdbuffer* cmdbuffer);

void vk_cmdbuffer_begin(
	vk_cmdbuffer* cmdbuffer,
	bool is_single_use,
	bool is_renderpass_continue_use,
	bool is_simultaneous_use);

void vk_cmdbuffer_end(vk_cmdbuffer* cmdbuffer);
void vk_cmdbuffer_update_submitted(vk_cmdbuffer* cmdbuffer);
void vk_cmdbuffer_reset(vk_cmdbuffer* cmdbuffer);

void vk_cmdbuffer_allocate_and_begin_single_use(
	vk_context* context,
	VkCommandPool pool,
	vk_cmdbuffer* cmdbuffer);


void vk_cmdbuffer_free_and_end_single_use(
	vk_context* context,
	VkCommandPool pool,
	vk_cmdbuffer* cmdbuffer,
	VkQueue queue);


