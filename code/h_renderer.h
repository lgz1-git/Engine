#pragma once
#include "h_win32_platform.h"
#include "h_vulkan_API.h"

struct renderer
{
	
	
};

void _renderer_init();
void _renderer_shutdown();
void _renderer_resize_swapchain();
void _renderer_draw_frame();

void _renderer_create_texture();
void _renderer_destroy_texture();