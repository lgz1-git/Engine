#pragma once
#include "h_win32_platform.h"
#include "h_vulkan_API.h"


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
void bk_vk_update_
bool bk_vk_end_frame(vk_context* context);
void bk_vk_shutdown(vk_context* context);

