#include "h_win32_platform.h"

#include "h_renderer.h"
#include "h_clogger.h"
#include "h_clock.h"

bool g_running;


int main()
{
	renderer_context context = {};
	context.vk_context.vk_allocator = nullptr;

	win32_config config = {};
	config.win_classname = "class name";
	config.win_name = "window name";
	create_window(&context.win32_context, &config);
	assert(context.win32_context.win_handle != 0);
	
	renderer_init(&context);
	
	ShowWindow(context.win32_context.win_handle, SW_SHOW);
	g_running = true;
	MSG msg = {};
	while (g_running == true)
	{
		while (PeekMessageA(&msg, context.win32_context.win_handle, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
			if (msg.message == WM_CLOSE)
			{
				g_running = false;
				break;
			}
		}
		////todo::input

		////todo::render
	}

	return 0;
}