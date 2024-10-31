#include "h_win32_platform.h"

void 
create_window_info(win32_platform_context* context, win32_config* config)
{

}

void 
create_window(win32_platform_context* context, win32_config* config)
{
	WNDCLASSEXA wc = {};
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = context->win_instance;
	wc.lpszClassName = config->win_classname;

	assert(RegisterClassExA(&wc) != 0);

	HWND handle = CreateWindowExA(
	        0, 
            config->win_classname, 
            config->win_name,
            WS_SIZEBOX | WS_CAPTION | WS_SYSMENU| WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE,
	        0, 0,
	        240, 240,
	        0, 0, context->win_instance, 0);


	assert(handle != nullptr);
	context->win_handle = handle;

}

void show_window(win32_platform_context* context)
{
    ShowWindow(context->win_handle, SW_SHOW);

    MSG msg = {};
    while (GetMessageA(&msg, nullptr, 0, 0))
    {
        /*if (msg.message == WM_QUIT)*/

        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}

static LRESULT
CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
   
    case WM_CLOSE:
    {
       
    }break;
   
    case WM_SYSKEYUP:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_KEYDOWN:
    {
        u32 vkcode = wparam;
        bool wasdown = ((lparam >> 30 & 1) != 0);
        bool isdown = ((lparam >> 31 & 1) == 0);
        if (vkcode == 'W')
        {
            if (wasdown)
                OutputDebugStringA("W: wasdown");
            if (isdown)
                OutputDebugStringA("W: isdown");
        }
        else if (vkcode == 'S')
        {

        }
        else if (vkcode == 'A')
        {

        }
        else if (vkcode == 'D')
        {

        }
        else if (vkcode == VK_UP)
        {

        }
        else if (vkcode == VK_DOWN)
        {

        }
        else if (vkcode == VK_LEFT)
        {

        }
        else if (vkcode == VK_RIGHT)
        {

        }
        else if (vkcode == VK_ESCAPE)
        {

        }

    }break;
    /*default:
    {

    }break;*/
    }
    return DefWindowProcA(wnd, msg, wparam, lparam);
}






