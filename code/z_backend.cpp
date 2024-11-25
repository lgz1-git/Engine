#include "h_backend.h"
#include <assert.h>
#include "h_global_list.h"
#include "h_math.h"

static void bk_vk_create_framebuffer(vk_context* context)
{
	const u32 attachments_counts = 2;
	//TODO:memery manage
	if (context->swapchain_info.framebuffer == nullptr) {
		context->swapchain_info.framebuffer = (vk_framebuffer*)malloc(context->swapchain_info.image_counts * sizeof(vk_framebuffer));
	}
	for (i32 i = 0; i < context->swapchain_info.image_counts; i++)
	{
		VkImageView attachment[attachments_counts] = {
			context->swapchain_info.views[i] ,
			context->swapchain_info.depth_image.view };

		vk_create_framebuffer(context,
			&context->main_renderpass,
			context->extent_w,
			context->extent_h,
			attachments_counts,
			attachment,
			&context->swapchain_info.framebuffer[i]);
	}
	LTRACE("framebuffer is created!");
}
static void bk_vk_create_cmdbuffer(vk_context* context)
{
	if (!context->g_cmd_buffer)
	{
		context->g_cmd_buffer = (vk_cmdbuffer*)malloc(context->swapchain_info.image_counts * sizeof(vk_cmdbuffer));
		for (i32 i = 0; i < context->swapchain_info.image_counts; i++)
		{
			context->g_cmd_buffer[i].cmdbuffer_handle = nullptr;
		}
	}
	for (i32 i = 0; i < context->swapchain_info.image_counts; i++)
	{
		if (context->g_cmd_buffer[i].cmdbuffer_handle != nullptr)
			vk_cmdbuffer_free(context, context->device.g_cmdpool, &context->g_cmd_buffer[i]);

		vk_cmdbuffer_allocate(context, context->device.g_cmdpool, true, &context->g_cmd_buffer[i]);
	}
	LTRACE("cmd buffer is created!");
}
static void bk_vk_upload_data_range(
	vk_context* context,
	VkCommandPool pool,
	VkFence fence,
	VkQueue queue,
	vk_buffer* buffer,
	u64 offest,
	size_t size,
	void* data)
{
	VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	vk_buffer staging = {};
	vk_create_buffer(context, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true, &staging);

	vk_buffer_load_data(context, &staging, 0, size, 0, data);

	vk_copy_buffer(context, pool, fence, queue, staging.handle, 0, buffer->handle, 0, size);

	vk_destroy_buffer(context, &staging);
}


void bk_win32_init(win32_platform_context* context)
{
	win32_config config = {};//todo:: config , no local varible
	config.win_classname = "class name";
	config.win_name = "window name";
	config.w = 800.f;
	config.h = 600.f;
	create_window(context, &config);
	assert(context->win_handle != 0);

	ShowWindow(context->win_handle, SW_SHOW);
}

void bk_win32_get_size()
{
	RECT client_react;
	GetClientRect(g_context.win32_context.win_handle, &client_react);
	int32_t width = client_react.right - client_react.left;
	int32_t height = client_react.bottom - client_react.top;
	g_rect_w = width;
	g_rect_h = height;
}

void bk_vk_resize(vk_context* context)
{
	vk_assert(vkDeviceWaitIdle(context->device.logical_device));
	vk_recreate_swapchain(context, &context->swapchain_info, context->extent_w, context->extent_h);

	//@Param:destroy cmdbuffer
	for (i32 i = 0; i < context->swapchain_info.image_counts; i++) {
		if (&context->g_cmd_buffer[i].cmdbuffer_handle) {
			vk_cmdbuffer_free(
				context,
				context->device.g_cmdpool,
				&context->g_cmd_buffer[i]);
			context->g_cmd_buffer[i].cmdbuffer_handle = 0;
		}
	}

	//@Param:destroy framebuffer
	for (i32 i = 0; i < context->swapchain_info.image_counts; i++) {
		vk_destroy_framebuffer(context, &context->swapchain_info.framebuffer[i]);
	}

	//@Param:create frammebuffer and cmdbuffer
	bk_vk_create_framebuffer(context);
	bk_vk_create_cmdbuffer(context);
	LINFO("swapchian recreate!");

	context->resize = false;
}

bool bk_vk_create_buffer(vk_context* context)
{
	VkMemoryPropertyFlagBits flag = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	const u64 vertex_buf_size = sizeof(vert_3D) * 1024 * 1024;
	if (!vk_create_buffer(
		context,
		vertex_buf_size,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		flag,
		true,
		&context->vertex_buffer)) {
		LERR("fall to create vertexbuffer");
		return false;
	}
	context->geometry_vertex_offest = 0;

	const u64 index_buf_size = sizeof(f32) * 1024 * 1024;
	if (!vk_create_buffer(
		context,
		index_buf_size,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		flag,
		true,
		&context->index_buffer)) {
		LERR("fall to create indexbuffer");
		return false;
	}
	context->geometry_index_offest = 0;

	return true;
}

void bk_vk_init(vk_context* context, win32_platform_context* win32_context)
{
	//info
	vk_init_extensions(context);
	vk_create_instance(context);
	vk_create_surface(context, win32_context);
	bool result = vk_select_pdevice(context);
	if (result) {
		LTRACE("phisical device is suitable!");
	}
	vk_create_device(context);
	vk_create_swapchain(context, &context->swapchain_info, g_rect_w, g_rect_h);

	vk_create_renderpass(
		context,
		&context->main_renderpass,
		0.f, 0.f, 0.2f, 1.0f,
		1.f,
		0);

	//@Param:cmdbuffer
	bk_vk_create_cmdbuffer(context);
	//@Param:framebuffer
	bk_vk_create_framebuffer(context);

	////info::sync
	context->image_available_semphores = (VkSemaphore*)malloc(context->swapchain_info.max_frames_in_flight * sizeof(VkSemaphore));
	context->queue_complete_semphores = (VkSemaphore*)malloc(context->swapchain_info.max_frames_in_flight * sizeof(VkSemaphore));
	context->in_flight_fence = (vk_fence*)malloc(context->swapchain_info.max_frames_in_flight * sizeof(vk_fence));

	for (i32 i = 0; i < context->swapchain_info.max_frames_in_flight; i++) {
		VkSemaphoreCreateInfo create_info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		vkCreateSemaphore(
			context->device.logical_device,
			&create_info,
			context->vk_allocator,
			&context->image_available_semphores[i]);
		vkCreateSemaphore(
			context->device.logical_device,
			&create_info,
			context->vk_allocator,
			&context->queue_complete_semphores[i]);

		vk_create_fence(context, true, &context->in_flight_fence[i]);
	}
	LTRACE("sync object is created!");

	context->images_in_flight = (vk_fence**)malloc(context->swapchain_info.image_counts * sizeof(vk_fence*));
	for (i32 i = 0; i < context->swapchain_info.image_counts; i++) {
		context->images_in_flight[i] = 0;
	}

	//@Param:create shader
	if (!vk_create_shader(context, &context->shader)) {
		LERR("fail to create built_in shader!");
	}
	LTRACE("built_in shader is created!");

	bk_vk_create_buffer(context);
	LTRACE("user buffer create!");

	///TODO:clean up
	const u32 vert_count = 4;
	vert_3D verts[vert_count] = {};

	verts[0].position = { -0.5 ,-0.5 ,0.0 };
	verts[0].texcoord = {0.f , 0.f};

	verts[1].position = { 0.5,0.5,0.0 };
	verts[1].texcoord = {1 ,1};

	verts[2].position = {-0.5, 0.5, 0.0};
	verts[2].texcoord = {0,1};

	verts[3].position = { 0.5, -0.5 ,0.0 };
	verts[3].texcoord = {1,0};


	const u32 index_counts = 6;
	u32 index[index_counts] = {0,1,2,0,3,1};

	bk_vk_upload_data_range(
		context,
		context->device.g_cmdpool,
		0,
		context->device.graphics_queue,
		&context->vertex_buffer,
		0,
		sizeof(vert_3D) * vert_count,
		verts);
	bk_vk_upload_data_range(
		context,
		context->device.g_cmdpool,
		0,
		context->device.graphics_queue,
		&context->index_buffer,
		0,
		sizeof(u32) * index_counts,
		index);

	u32 obj_id = 0;
	if (!vk_shader_acquire_resource(context, &context->shader, &obj_id)) {
		LERR("failed to acquire resource");
	}
 }

void bk_vk_shutdown(vk_context* context)
{
	vkDeviceWaitIdle(context-> device.logical_device);

	vk_destroy_buffer(context, &context->vertex_buffer);
	vk_destroy_buffer(context, &context->index_buffer);

	vk_destroy_shader(context, &context->shader);
	//@Param:destroy semphores
	for (i32 i = 0; i < context-> swapchain_info.max_frames_in_flight; i++) {
		if (context-> image_available_semphores[i]) {
			vkDestroySemaphore(
				context-> device.logical_device,
				context-> image_available_semphores[i],
				context-> vk_allocator);
			context-> image_available_semphores[i] = 0;
		}
		if (context-> queue_complete_semphores[i]) {
			vkDestroySemaphore(
				context-> device.logical_device,
				context-> queue_complete_semphores[i],
				context-> vk_allocator);
			context-> queue_complete_semphores[i] = 0;
		}
		vk_destroy_fence( context, &context->in_flight_fence[i]);
	}

	//@Param:destroy cmdbuffer
	for (i32 i = 0; i < context-> swapchain_info.image_counts; i++) {
		if (&context->g_cmd_buffer[i].cmdbuffer_handle) {
			vk_cmdbuffer_free(
				context,
				context-> device.g_cmdpool, 
				&context->g_cmd_buffer[i]);
			context-> g_cmd_buffer[i].cmdbuffer_handle = 0;
		}
	}

	//@Param:destroy framebuffer
	for (i32 i = 0; i < context-> swapchain_info.image_counts; i++) {
		vk_destroy_framebuffer( context, &context->swapchain_info.framebuffer[i]);
	}
	vk_destroy_renderpass( context, &context->main_renderpass);
	vk_destroy_swapchain( context, &context->swapchain_info);
	vk_destory_cmdpool( context);
	vk_destroy_device( context);
	vk_destroy_surface( context);
	vk_destroy_instance( context);
}

bool bk_vk_begin_frame(vk_context* context)
{
	if (g_rect_w == context->extent_w && g_rect_h == context->extent_h) {
		context->resize = false;
	}
	else {
		context->resize = true;
		context->extent_w = g_rect_w;
		context->extent_h = g_rect_h;
	}
	
	vkDeviceWaitIdle(context->device.logical_device);
	/*f32 w = context->extent_w;
	f32 h = context->extent_h;*/
	if (context->resize) {
		bk_vk_resize(context);
	}
	vkDeviceWaitIdle(context->device.logical_device);
	if (!vk_fence_wait(
		context,
		&context->in_flight_fence[context->current_frame],
		UINT64_MAX))
	{
		LERR("fence wait fail!");
		return false;
	}

	if (!vk_swapchain_acquire_next_image_index(
		context,
		&context->swapchain_info,
		UINT64_MAX,
		context->image_available_semphores[context->current_frame],
		0,
		&context->image_index))
	{
		LERR("acquire next image fail!");
		return false;
	}

	f32 w = context->extent_w;
	f32 h = context->extent_h;
	//info:begin recording command
	vk_cmdbuffer* cmdbuffer = &context->g_cmd_buffer[context->image_index];
	vk_cmdbuffer_reset(cmdbuffer);
	vk_cmdbuffer_begin(cmdbuffer, false, false, false);

	if (!vk_fence_wait(
		context,
		&context->in_flight_fence[context->current_frame],
		UINT64_MAX))
	{
		LERR("fence wait fail!");
		return false;
	}

	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = h;
	viewport.width = w;
	viewport.height = -h;
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = { (u32)w,(u32)h };

	vkCmdSetViewport(cmdbuffer->cmdbuffer_handle, 0, 1, &viewport);
	vkCmdSetScissor(cmdbuffer->cmdbuffer_handle, 0, 1, &scissor);

	//@Param:update renderpass extent
	context->main_renderpass.w = w;
	context->main_renderpass.h = h;

	vk_renderpass_begin(
		cmdbuffer, 
		&context->main_renderpass, 
		context->swapchain_info.framebuffer[context->image_index].framebuffer_handle);


	return true;
}

void bk_vk_update_global_state(
	vk_context* context,
	glm::mat4 projection, 
	glm::mat4 view, 
	glm::vec3 view_pos, 
	glm::vec4 ambient_colour, 
	i32 mode)
{
	vk_cmdbuffer* cmdbuffer = &context->g_cmd_buffer[context->image_index];

	//vk_use_shader(context, &context->shader);
	context->shader.global_uo.projection = projection;
	context->shader.global_uo.view = view;

	vk_shader_update_global_state(context, &context->shader);

	vk_use_shader(context, &context->shader);
	VkDeviceSize offest[1] = { 0 };
	vkCmdBindVertexBuffers(cmdbuffer->cmdbuffer_handle, 0, 1, &context->vertex_buffer.handle, offest);
	vkCmdBindIndexBuffer(cmdbuffer->cmdbuffer_handle, context->index_buffer.handle, 0, VK_INDEX_TYPE_UINT32);

	vkCmdDrawIndexed(cmdbuffer->cmdbuffer_handle, 6, 1, 0, 0, 0);
}

void bk_vk_update_object_state(vk_context* context, vk_shader* shader, geometry_render_data data)
{
	u32 image_index = context->image_index;
	VkCommandBuffer cmdbuffer = context->g_cmd_buffer[image_index].cmdbuffer_handle;

	vkCmdPushConstants(
		cmdbuffer,
		shader->pipeline.pipeline_layout,
		VK_SHADER_STAGE_VERTEX_BIT,
		0,
		sizeof(mat4),
		&data.model);

	vk_object_state* object_state = &shader->object_state[data.object_id];
	VkDescriptorSet obj_descriptor_set = object_state->descriptor_set[image_index];

	VkWriteDescriptorSet dsecriptor_write[object_descriptor_counts];
	u32 descriptor_counts = 0;
	u32 descriptor_index = 0;

	u32 range = sizeof(object_uniform_object);
	u32 offest = sizeof(object_uniform_object) * data.object_id;
	object_uniform_object ouo;

	static f32 accumulator = 0.f;
	accumulator += 0.001f;
	f32 s = (sin(accumulator) + 1) / 2;
	ouo.diffuse_color = glm::vec4(s, s, s, 1.f);

	vk_buffer_load_data(context, &shader->object_ubo, offest, range, 0, &ouo);
	if (object_state->descriptor_state[descriptor_index].generation[image_index] == UINT32_MAX)
	{
		VkDescriptorBufferInfo buf_info = {};
		buf_info.buffer = shader->object_ubo.handle;
		buf_info.range = range;
		buf_info.offset = offest;

		VkWriteDescriptorSet write_descriptor_set = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		write_descriptor_set.descriptorCount = 1;
		write_descriptor_set.dstSet = obj_descriptor_set;
		write_descriptor_set.dstBinding = descriptor_index;
		write_descriptor_set.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write_descriptor_set.pBufferInfo = &buf_info;

		dsecriptor_write[descriptor_counts] = write_descriptor_set;
		descriptor_counts++;

		object_state->descriptor_state[descriptor_index].generation[image_index] = 1;
	}
	descriptor_index++;

	//@Param:sampler
	const u32 sampler_counts = 1;
	VkDescriptorImageInfo image_info[sampler_counts];
	for (i32 sampler_index = 0; sampler_index < sampler_counts; sampler_index++)
	{
		vk_texture* t = data.texture[sampler_index];
		u32* descriptor_generation = &object_state->descriptor_state[descriptor_index].generation[image_index];

		if (t && (*descriptor_generation != t->generation || *descriptor_generation == UINT32_MAX)) {
			vk_texture_data* t_data = (vk_texture_data*)t->internal_data;

			image_info[sampler_index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			image_info[sampler_index].imageView = t_data->image.view;
			image_info[sampler_index].sampler = t_data->sampler;

			VkWriteDescriptorSet write_set = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			write_set.dstSet = obj_descriptor_set;
			write_set.dstBinding = descriptor_index;
			write_set.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			write_set.descriptorCount = 1;
			write_set.pImageInfo = &image_info[sampler_index];

			dsecriptor_write[descriptor_counts] = write_set;
			descriptor_counts++;

			if (t->generation != UINT32_MAX) {
				*descriptor_generation = t->generation;
			}
			descriptor_index++;
		}
	}

	if (descriptor_counts > 0) {
		vkUpdateDescriptorSets(context->device.logical_device, descriptor_counts, dsecriptor_write, 0, 0);
	}
	vkCmdBindDescriptorSets(
		cmdbuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		shader->pipeline.pipeline_layout,
		1, 1, &obj_descriptor_set,
		0, 0);
}


void bk_vk_create_texture(
	vk_context* context,
	const char* name,
	bool auto_release,
	i32 w,
	i32 h,
	i32 channel_counts,
	const u8* pixels,
	bool has_transparency,
	vk_texture* texture)
{
	texture->channel_counts = channel_counts;
	texture->generation = UINT32_MAX;
	texture->w = w;
	texture->h = h;

	texture->internal_data = (vk_texture_data*)malloc(sizeof(vk_texture_data));//TODO:malloc
	vk_texture_data* data = (vk_texture_data*)texture->internal_data;
	VkDeviceSize image_size = (VkDeviceSize)(w) * h * channel_counts;

	VkFormat image_format = VK_FORMAT_R8G8B8A8_UNORM;

	VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	VkMemoryPropertyFlags  mem_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	vk_buffer stageing = {};
	vk_create_buffer(context, image_size, usage, mem_flags, true, &stageing);

	vk_buffer_load_data(context, &stageing, 0, image_size, 0, pixels);

	vk_create_image(
		context,
		VK_IMAGE_TYPE_2D,
		w, h,
		image_format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		true,
		VK_IMAGE_ASPECT_COLOR_BIT,
		&data->image);

	vk_cmdbuffer cmdbuf = {};
	VkCommandPool cmdpool = context->device.g_cmdpool;
	VkQueue queue = context->device.graphics_queue;
	vk_cmdbuffer_allocate_and_begin_single_use(context, cmdpool, &cmdbuf);

	vk_image_translation_layout(
		context,
		&cmdbuf,
		&data->image,
		image_format,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);


	vk_image_copy_from_buffer(
		context,
		&cmdbuf,
		stageing.handle,
		&data->image);

	vk_image_translation_layout(
		context,
		&cmdbuf,
		&data->image,
		image_format,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vk_cmdbuffer_free_and_end_single_use(context, cmdpool, &cmdbuf, queue);
	vk_destroy_buffer(context, &stageing);

	VkSamplerCreateInfo sampler_create_info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	sampler_create_info.minFilter = VK_FILTER_LINEAR;
	sampler_create_info.magFilter = VK_FILTER_LINEAR;
	sampler_create_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_create_info.anisotropyEnable = true;
	sampler_create_info.maxAnisotropy = 16;
	sampler_create_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_create_info.unnormalizedCoordinates = false;
	sampler_create_info.compareEnable = false;
	sampler_create_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_create_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_create_info.minLod = 0.f;
	sampler_create_info.maxLod = 0.f;
	sampler_create_info.mipLodBias = 0.f;

	vk_assert(vkCreateSampler(
		context->device.logical_device,
		&sampler_create_info,
		context->vk_allocator,
		&data->sampler));

	texture->has_transparency = has_transparency;
	texture->generation++;
}
void bk_vk_destory_texture(vk_context* context , vk_texture* texture)
{
	vkDeviceWaitIdle(context->device.logical_device);

	vk_texture_data* data = (vk_texture_data*)texture->internal_data;
	vk_destroy_image(context, &data->image);
	vkDestroySampler(context->device.logical_device, data->sampler, context->vk_allocator);
}

bool 
bk_vk_end_frame(vk_context* context)
{
	vk_cmdbuffer* cmdbuffer = &context->g_cmd_buffer[context->image_index];

	vk_renderpass_end(cmdbuffer, &context->main_renderpass);
	vk_cmdbuffer_end(cmdbuffer);

	if (context-> images_in_flight[context-> image_index] != VK_NULL_HANDLE) {
		vk_fence_wait( context,
			context-> images_in_flight[context->image_index],
			UINT64_MAX);
	}

	context->images_in_flight[context->image_index] =
		 &context->in_flight_fence[context->current_frame];

	vk_fence_reset(
		 context, 
		 &context->in_flight_fence[context-> current_frame]);
	VkSubmitInfo submit = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmdbuffer->cmdbuffer_handle;

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &context->queue_complete_semphores[context->current_frame];
	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &context->image_available_semphores[context->current_frame];

	VkPipelineStageFlags flags[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submit.pWaitDstStageMask = flags;
	vk_assert(vkQueueSubmit(
		context->device.graphics_queue,
		1,
		&submit,
		context->in_flight_fence[context->current_frame].fence_handle));

	vk_cmdbuffer_update_submitted(cmdbuffer);
	vk_swapchain_present(
		context,
		&context->swapchain_info,
		context->device.graphics_queue,
		context->device.present_queue,
		context->queue_complete_semphores[context->current_frame],
		context->image_index,context->extent_w, context->extent_w);
	return true;
}