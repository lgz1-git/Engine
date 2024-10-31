#pragma once 

#include "h_windows.h"
#include"h_type.h"
#include <assert.h>

struct win32_platform_context
{
	HWND win_handle;
	HINSTANCE win_instance;

};

struct win32_config 
{
	const char* win_classname ;
	const char* win_name ;
};

void create_window_info(win32_platform_context* context, win32_config* config);

void create_window(win32_platform_context* context, win32_config* config);

void show_window(win32_platform_context* context);

static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
