#include "h_win32_platform.h"

#include "h_clogger.h"
#include "h_clock.h"

#include "h_backend.h"

#include "h_global_list.h"


int main()
{
	global_variable_init();
	LTRACE("TRACE " << 32 << " well");
	LINFO("INFO " << 32 << " well");
	LTIME("TIME" << 32 << " well");
	LERR("ERROR " << 32 << " well");

	bk_win32_init(&g_context.win32_context);
	bk_vk_init(&g_context);
	g_running = true;
	MSG msg = {};
	while (g_running == true)
	{
		while (PeekMessageA(&msg, g_context.win32_context.win_handle, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
		////todo::input

		////todo::render
	}

	bk_vk_shutdown(&g_context);
	//todo:: win32 shutdown

	return 0;
}