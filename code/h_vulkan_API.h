#pragma once
#include "h_win32_platform.h"
#include "vulkan/vulkan.h"
#if defined (_WIN32)
#include "vulkan/vulkan_win32.h"
#endif

#include "h_type.h"
#include "h_math.h"

#include <assert.h>
#include <vector>
#include <string>
#include <string_view>
#include <memory>

#include "glm.hpp"

#define vk_assert(p) assert(p==VK_SUCCESS)
constexpr u32 shader_counts = 2;//TODO:

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
	//TODO: clean up
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

	VkCommandPool g_cmdpool;
};


struct vk_image {
	VkImage image_handle;
	VkDeviceMemory memory;
	VkImageView view;
};


struct vk_renderpass {
	VkRenderPass renderpass_handle;
	f32 w, h;
	f32 r, g, b, a;

	f32 depth;
	u32 stencil;

	vk_renderpass_state state;
};

struct vk_cmdbuffer {
	VkCommandBuffer cmdbuffer_handle;
	vk_cmdbuffer_state cmdbuffer_state;
};

struct vk_framebuffer {
	VkFramebuffer framebuffer_handle;
	u32 attachment_counts;
	//TODO:clean up
	VkImageView* views;
	vk_renderpass* renderpass;
};

struct vk_swapchain_info
{
	vk_framebuffer* framebuffer;

	VkSurfaceFormatKHR surface_format;
	u32 max_frames_in_flight;
	VkSwapchainKHR swapchain_handle;
	u32 image_counts;
	//TODO::clean up
	VkImage* images;
	VkImageView* views;

	vk_image depth_image;
};

struct gloabal_uniform_object {
	glm::mat4 projection;
	glm::mat4 view;
	glm::mat4 temp1;
	glm::mat4 temp2;
};

struct vk_fence {
	VkFence fence_handle;
	bool is_signaled;
};

struct vk_shader_stage {
	VkShaderModule handle;
	VkShaderModuleCreateInfo create_info;
	VkPipelineShaderStageCreateInfo shader_stage_create_info;
};

struct vk_pipeline
{
	VkPipeline handle;
	VkPipelineLayout pipeline_layout;
};


struct vk_buffer {
	size_t total_size;
	VkBuffer handle;
	VkBufferUsageFlags usage;
	bool is_locked;
	VkDeviceMemory memory;
	i32 mem_index;
	u32 mem_property_flags;
};

struct vk_shader {
	vk_shader_stage stages[shader_counts];

	VkDescriptorPool global_descriptor_pool;
	VkDescriptorSetLayout  global_descriptor_set_layout;

	VkDescriptorSet global_descriptor_set[3];
	gloabal_uniform_object global_uo;
	vk_buffer global_ubo;

	vk_pipeline pipeline;
};

struct vk_context
{
	bool resize;
	u32 image_index;
	u32 current_frame;
	u32 extent_w;
	u32 extent_h;
	u64 geometry_vertex_offest;
	u64 geometry_index_offest;
	VkInstance vk_instance;
	VkAllocationCallbacks *vk_allocator;
	VkSurfaceKHR vk_surface;
	vk_device device;

	//renderpass
	vk_renderpass main_renderpass;

	vk_buffer vertex_buffer;
	vk_buffer index_buffer;

	//signal
	VkSemaphore* image_available_semphores;
	VkSemaphore* queue_complete_semphores;
	u32 in_flght_fence_counts;
	vk_fence* in_flight_fence;
	vk_fence**  images_in_flight;

	//extension
	std::vector<const char* > instance_extensions;
	std::vector<const char* > instance_layers_extensions;
	std::vector<const char* > device_extensions;
	std::vector<const char* > device_layers_extensions;

	vk_swapchain_info swapchain_info;
	vk_cmdbuffer* g_cmd_buffer;
	vk_shader shader;
};



void vk_init_extensions(vk_context* context);

void vk_create_instance(vk_context* context);

void vk_destroy_instance(vk_context* context);

void vk_create_surface(vk_context* context, win32_platform_context* win_context);

void vk_destroy_surface(vk_context* context);

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

void vk_destroy_device(vk_context* context);

void vk_destory_cmdpool(vk_context* context);

void vk_create_swapchain(vk_context* context,vk_swapchain_info* swapchain_info,u32 w,u32 h);

//@Param:iamge
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
	vk_image* image);

void vk_create_image_view(
	vk_context* context,
	VkFormat format,
	vk_image* image,
	VkImageAspectFlags aepect_flag);

void vk_image_translation_layout(
	vk_context* context,
	vk_cmdbuffer* cmdbuf,
	vk_image* image,
	VkFormat format,
	VkImageLayout old_layout,
	VkImageLayout new_layout);

void vk_image_copy_from_buffer(
	vk_context* context,
	vk_cmdbuffer* cmdbuf,
	VkBuffer buf,
	vk_image* image);

void vk_destroy_image(vk_context* context, vk_image* image);

void vk_recreate_swapchain(vk_context* context,vk_swapchain_info* swapchain_info,u32 w,u32 h);

void vk_destroy_swapchain(vk_context* context, vk_swapchain_info* swapchain_info);


bool vk_swapchain_acquire_next_image_index(
	vk_context* context,
	vk_swapchain_info* swapchain_info,
	u64 timeout_ns,
	VkSemaphore image_available_semaphore,
	VkFence fence,
	u32* out_image_index);


void vk_swapchain_present(
	vk_context* context,
	vk_swapchain_info* swapchain_info,
	VkQueue graphics_queue,
	VkQueue present_queue,
	VkSemaphore render_complete_semaphore,
	u32 present_image_index,f32,f32);


void vk_create_renderpass(
	vk_context* context,
	vk_renderpass* renderpass,
	f32 r, f32 g, f32 b, f32 a,
	f32 depth, u32 stencil);

void vk_destroy_renderpass(vk_context* context, vk_renderpass* rederpass);

void vk_renderpass_begin(
	vk_cmdbuffer* cmdbuffer,
	vk_renderpass* renderpass,
	VkFramebuffer frame_buffer);

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

void vk_create_framebuffer(
	vk_context* context,
	vk_renderpass* renderpass,
	u32 w,
	u32 h,
	u32 view_counts,
	VkImageView* views,
	vk_framebuffer* framebuffer);

void vk_destroy_framebuffer(vk_context* context, vk_framebuffer* framebuffer);

void vk_create_fence(vk_context* context,bool create_signal ,vk_fence* fence);

void vk_destroy_fence(vk_context* context, vk_fence* fence);

bool vk_fence_wait(vk_context* context, vk_fence* fence, u64 time_out_ns);

void vk_fence_reset(vk_context* context, vk_fence* fence);


//1 @Param:shader
static bool vk_create_shader_module(
	vk_context* context,
	const char* name,
	const char* type_str,
	VkShaderStageFlagBits flag,
	u32 stage_index,
	vk_shader_stage* shader_stage);//@Param: called by vk_create_shader

bool vk_create_shader(vk_context* context, vk_shader* shader);
void vk_destroy_shader(vk_context* context, vk_shader* shader);
void vk_use_shader(vk_context* context, vk_shader* shader);
void vk_shader_update_global_state(vk_context* context, vk_shader* shader);
void vk_push_const(vk_context* context, vk_shader* shader,glm::mat4 model);


//1 @Param:graphics pipeline
bool vk_create_g_pipeline(
	vk_context* context,
	vk_renderpass* renderpass,
	u32 attribute_counts,
	VkVertexInputAttributeDescription* attributes,
	u32 descriptors_set_layout_counts,
	VkDescriptorSetLayout* descriptors_set_layout,
	u32 stage_counts,
	VkPipelineShaderStageCreateInfo* stages,
	VkViewport viewport,
	VkRect2D scissor,
	bool is_wireframe,
	vk_pipeline* pipeline);

void vk_destroy_pipeline(vk_context* context, vk_pipeline* pipeline);

void vk_bind_pipeline(vk_cmdbuffer* cmdbuf, VkPipelineBindPoint bindpoint, vk_pipeline* pipeline);

bool vk_create_buffer(
	vk_context* context,
	size_t size,
	VkBufferUsageFlags usage,
	u32 mem_flag,
	bool bind_on_create,
	vk_buffer* buf);

void vk_destroy_buffer(vk_context* context, vk_buffer* buf);

void* vk_lock_buffer_mem(vk_context* context, vk_buffer* buf, size_t offest, size_t size, u32 flags);

void vk_unlock_buffer_mem(vk_context* context, vk_buffer* buf);

void vk_buffer_load_data(vk_context* context, vk_buffer* buf, size_t offest, size_t size, u32 flags, const void* data);

void vk_copy_buffer(
	vk_context* context,
	VkCommandPool pool,
	VkFence fence,
	VkQueue queue,
	VkBuffer src,
	size_t offest,
	VkBuffer dest,
	size_t dest_offest,
	size_t szie);

bool vk_resize_buffer(
	vk_context* context,
	VkCommandPool pool,
	vk_buffer* buf,
	VkQueue queue,
	size_t new_size);

void vk_bind_buffer(vk_context* context, vk_buffer* buf, size_t offest);
