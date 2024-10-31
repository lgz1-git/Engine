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


struct vk_swapchain_info
{
	VkSurfaceFormatKHR surface_format;
	u8 max_frames_in_flight;
	VkSwapchainKHR swapchain_handle;
	u32 image_counts;
	//TODO::clean up
	VkImage* images;
	VkImageView* views;
};

struct vk_image {
	VkImage iamge_handle;
	VkDeviceMemory memory;
	VkImageView view;
	u32 w;
	u32 h;
};

struct vk_context
{

	VkInstance vk_instance;

	VkAllocationCallbacks *vk_allocator;

	VkSurfaceKHR vk_surface;

	vk_device device;

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
	VkImageUsageFlagBits memory_flags,
	bool create_view,
	VkImageAspectFlags view_aspect_flags,
	vk_image* image);

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


