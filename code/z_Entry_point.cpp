#include "h_win32_platform.h"

#include "h_clogger.h"
#include "h_clock.h"

#include "h_backend.h"

#include "h_global_list.h"
#include "h_math.h"
#include "h_events.h"
#include "h_inputs.h"

static bool app_on_event(u32 code, void* sender, void* listener_inst, event_context data)
{
	if (code == EVENT_CODE_KEY_PRESSED) {
		u32 key_code = data.data.u32[0];
		if (key_code == KEY_A)
		{
			LINFO("A is down");
		}
	}
	else if (code == EVENT_CODE_KEY_RELEASED)
	{
		u32 key_code = data.data.u32[0];
		if (key_code == KEY_A)
		{
			LINFO("A was down");
		}
	}
	return true;
}

int main()
{
	//TODO:clean up 
	event_register(EVENT_CODE_KEY_PRESSED, 0, app_on_event);
	event_register(EVENT_CODE_KEY_RELEASED, 0, app_on_event);
	const u32 tex_dimension = 256;
	const u32 channels = 4;
	const u32 pixel_counts = tex_dimension * tex_dimension;
	u8* pixel = (u8*)malloc(channels * pixel_counts);

	for (u64 row = 0; row < tex_dimension; row++) {
		for (u64 col = 0; col < tex_dimension; col++) {
			u64 index = row * tex_dimension + col;
			u64 index_bpp = index * channels;
			if (row % 2) {
				if (col% 2) {
					pixel[index_bpp + 0] = 0;
					pixel[index_bpp + 1] = 0;
				}
			}
			else {
				if (!(col % 2)) {
					pixel[index_bpp + 0] = 0;
					pixel[index_bpp + 1] = 0;
				}
			}
		}
	}
	geometry_render_data data;
	data.model = glm::mat4(1.f);
	data.object_id = 0;
	global_variable_init();
	bk_win32_init(&g_context.win32_context);
	bk_win32_get_size();
	bk_vk_init(&g_context.vk_context, &g_context.win32_context);
	vk_texture t;
	bk_vk_create_texture(
		&g_context.vk_context,
		"default texture",
		false,
		tex_dimension,
		tex_dimension,
		4,
		pixel,
		false,
		&t);
	LTRACE("tex_create");
	data.texture[0] = &t;//TODO:
	g_running = true;
	MSG msg = {};
	glm::mat4 view(1.0f);
	glm::mat4 projection(1.0f);
	while (g_running == true)
	{
		while (PeekMessageA(&msg, g_context.win32_context.win_handle, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
		//TODO:input

		////TODO:render
		bool i = bk_vk_begin_frame(&g_context.vk_context);
		bk_vk_update_object_state(&g_context.vk_context, &g_context.vk_context.shader, data);
		bk_vk_update_global_state(
			&g_context.vk_context,
			projection,
			view,
			{ 0,0,0 },
			{ 1,1,1,1 },
			0);
		bool o = bk_vk_end_frame(&g_context.vk_context);
		input_update();
	}
	bk_vk_destory_texture(&g_context.vk_context, &t);
	bk_vk_shutdown(&g_context.vk_context);
	//TODO:win32 shutdown

	return 0;
}