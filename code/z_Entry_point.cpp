#include "h_win32_platform.h"
#include "h_vulkan_API.h"

#include <iostream>
int main()
{
	win32_platform_context win32_context = {};
	win32_config config = {};
	config.win_classname = "class name";
	config.win_name = "window name";
	create_window(&win32_context, &config);

	vk_context vk_context = {};
	vk_context.vk_allocator = nullptr;
	vk_init_extensions(&vk_context);
	vk_create_instance(&vk_context);
	vk_create_surface(&vk_context, &win32_context);
	bool result = vk_select_pdevice(&vk_context);
	if (result)
	{
		std::cout << "yes";
	}
	vk_create_device(&vk_context);
	vk_create_swapchain(&vk_context, &vk_context.swapchain_info, 200, 200);
	show_window(&win32_context);
}