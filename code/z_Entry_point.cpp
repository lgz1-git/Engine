#include "h_win32_platform.h"

#include "h_clogger.h"
#include "h_clock.h"

#include "h_backend.h"

#include "h_global_list.h"
#include "glm.hpp"


int main()
{
	global_variable_init();

	bk_win32_init(&g_context.win32_context);
	bk_win32_get_size();
	bk_vk_init(&g_context.vk_context, &g_context.win32_context);
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
		bk_vk_update_global_state(
			&g_context.vk_context,
			projection,
			view,
			{ 0,0,0 },
			{ 1,1,1,1 },
			0);
		bool o = bk_vk_end_frame(&g_context.vk_context);
	}

	bk_vk_shutdown(&g_context.vk_context);
	//TODO:win32 shutdown

	return 0;
}