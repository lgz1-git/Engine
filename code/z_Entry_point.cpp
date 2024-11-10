#include "h_win32_platform.h"

#include "h_renderer.h"
#include "h_clogger.h"
#include "h_clock.h"

#include "h_backend.h"

bool g_running;


int main()
{
	backend_context context = {};
	context.vk_context.vk_allocator = nullptr;
	bk_win32_init(&context.win32_context);
	bk_vk_init(&context);

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