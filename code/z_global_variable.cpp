#include "h_backend.h"

///@Param:global variable file 
/// /////////////////////////////////////////////

bool g_running;
f32 g_rect_w;
f32 g_rect_h;
backend_context  g_context;

/// /////////////////////////////////////////////

void global_variable_init()
{
	g_running                               = false;
	g_rect_w                                = 640.f;
	g_rect_h                                = 480.f;
	g_context.vk_context.vk_allocator       = nullptr;
	g_context.vk_context.current_frame      = 0;
}
