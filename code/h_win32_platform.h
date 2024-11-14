#pragma once 

#include "h_windows.h"
#include "h_clogger.h"
#include "h_type.h"
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
	f32 w;
	f32 h;
};


void create_window(win32_platform_context* context, win32_config* config);

static LRESULT CALLBACK WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam);
