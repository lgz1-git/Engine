#include "h_win32_platform.h"

#include "h_global_list.h"

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
	        100, 100,
	        config->w, config->h,
	        0, 0, context->win_instance, 0);


	assert(handle != nullptr);
	context->win_handle = handle;

}

static LRESULT
CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg){

        case WM_CLOSE:{
        
            LTRACE("window close");
            g_running = false;
               
        }break;
        case WM_DESTROY: {
            LTRACE("window destory");
        }break;
        case WM_SIZE: {
            RECT client_react;
            GetClientRect(wnd, &client_react);
            int32_t width = client_react.right - client_react.left;
            int32_t height = client_react.bottom - client_react.top;
            g_rect_w = width;
            g_rect_h = height;
            LINFO("w: " << g_rect_w);
            LINFO("h: " << g_rect_h);
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
            if (wasdown) {
                OutputDebugStringA("W: wasdown");
               /* g_rect_w++;
                g_rect_h++;*/
            }
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
        case WM_CHAR: {
        //todo::input text

        }
        default:
        return DefWindowProcA(wnd, msg, wparam, lparam);
    }
}







