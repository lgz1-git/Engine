#pragma once
#include "h_win32_platform.h"
#include "h_vulkan_API.h"
#include "h_math.h"


struct backend_context
{
	vk_context vk_context;
	win32_platform_context win32_context;
};

void bk_win32_init(win32_platform_context* context);
void bk_win32_get_size();//TODO:clean up this function
void bk_vk_resize(vk_context* context);
static void bk_vk_create_framebuffer(vk_context* context);
static void bk_vk_create_cmdbuffer(vk_context* context);
bool bk_vk_create_buffer(vk_context* context);

void bk_vk_init(vk_context* context , win32_platform_context*);
bool bk_vk_begin_frame(vk_context* context);
void bk_vk_update_global_state(
	vk_context* context,
	glm::mat4 projection ,
	glm::mat4 view,
	glm::vec3 view_pos,
	glm::vec4 ambient_colour,
	i32 mode);//TODO:delete glm
void bk_vk_update_object_state(vk_context* context, vk_shader* shader, geometry_render_data data);
bool bk_vk_end_frame(vk_context* context);
void bk_vk_shutdown(vk_context* context);

void bk_vk_create_texture(
	vk_context* context,
	const char* name,
	bool auto_release,
	i32 w,
	i32 h,
	i32 channel_counts,
	const u8* pixels,
	bool has_transparency,
	vk_texture* texture);

void bk_vk_destory_texture(vk_context* context, vk_texture* texture);



