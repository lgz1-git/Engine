#include "h_vulkan_API.h"
#include <iostream>

//::::::::::::::::::::::::::::::::::::::::::::::::::::swapchian::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
static void
CreateSwapchain(vk_context* context, vk_swapchain_info* swapchain_info, u32 w, u32 h)
{
	VkExtent2D  extent = { w,h };
	swapchain_info->max_frames_in_flight = 2;

	// Choose a swap surface format.
	bool found = FALSE;
	for (u32 i = 0; i < context->device.pdevice_swapchain_potency.format_counts; i++) {
		VkSurfaceFormatKHR format = context->device.pdevice_swapchain_potency.surface_formats[i];
		// Preferred formats
		if ((format.format == VK_FORMAT_B8G8R8A8_SRGB)&&
			(format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)) {
			swapchain_info->surface_format = format;
			found = TRUE;
			std::cout << "\nfounded\n";
			break;
		}
	}
	if (!found) {
		std::cout << "\nno founded\n";
		swapchain_info->surface_format = context->device.pdevice_swapchain_potency.surface_formats[0];
	}

	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (u32 i = 0; i < context->device.pdevice_swapchain_potency.present_modes_counts; ++i) {
		VkPresentModeKHR mode = context->device.pdevice_swapchain_potency.present_modes[i];
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
			present_mode = mode;
			break;
		}
	}
	
	//requery device potency
	//TODO::may be not necessary
	vk_query_pdevice_swapchain_potency(
		context->device.physical_device,
		context->vk_surface,
		&context->device.pdevice_swapchain_potency);

	////TODO::is this necessary?
	if (context->device.pdevice_swapchain_potency.capabilities.currentExtent.width != UINT32_MAX)
	{
		extent = context->device.pdevice_swapchain_potency.capabilities.currentExtent;
	}

	VkExtent2D min = context->device.pdevice_swapchain_potency.capabilities.minImageExtent;
	VkExtent2D max = context->device.pdevice_swapchain_potency.capabilities.maxImageExtent;
	extent.width = (extent.width <= min.width) ? min.width : (extent.width >= max.width) ? max.width : extent.width;
	extent.height = (extent.height <= min.height) ? min.height : (extent.height >= max.height) ? max.height : extent.height;

	u32 image_counts = context->device.pdevice_swapchain_potency.capabilities.minImageCount + 1;

	if ((context->device.pdevice_swapchain_potency.capabilities.minImageCount > 0)
		&& (image_counts > context->device.pdevice_swapchain_potency.capabilities.maxImageCount)) {
		image_counts = context->device.pdevice_swapchain_potency.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchain_create_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchain_create_info.surface = context->vk_surface;
	swapchain_create_info.imageExtent = extent;
	swapchain_create_info.imageFormat = swapchain_info->surface_format.format;
	swapchain_create_info.imageColorSpace = swapchain_info->surface_format.colorSpace;
	swapchain_create_info.imageArrayLayers = 1;
	swapchain_create_info.minImageCount = image_counts;
	swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_create_info.preTransform = context->device.pdevice_swapchain_potency.capabilities.currentTransform;
	swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_create_info.presentMode = present_mode;
	swapchain_create_info.clipped = VK_TRUE;
	swapchain_create_info.oldSwapchain = 0;

	////select device queuefamily 
	if (context->device.queue_family_info.graphics_family_index != context->device.queue_family_info.present_family_index) {
		u32 queue_array[] = { context->device.queue_family_info.graphics_family_index ,context->device.queue_family_info.present_family_index };
		swapchain_create_info.pQueueFamilyIndices = queue_array;
		swapchain_create_info.queueFamilyIndexCount = 2;
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	}
	else {
		swapchain_create_info.pQueueFamilyIndices = 0;
		swapchain_create_info.queueFamilyIndexCount = 0;
		swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	vk_assert(vkCreateSwapchainKHR(context->device.logical_device, &swapchain_create_info, context->vk_allocator, &swapchain_info->swapchain_handle));

	context->image_index = 0;
	context->current_frame = 0;
	vk_assert(vkGetSwapchainImagesKHR(context->device.logical_device, swapchain_info->swapchain_handle, &swapchain_info->image_counts, 0));
	if (swapchain_info->image_counts > 0) {
		//TODO::log
		std::cout << "images_counts = " << swapchain_info->image_counts << std::endl;
		swapchain_info->images = (VkImage*)alloca(swapchain_info->image_counts * sizeof(VkImage));
		vkGetSwapchainImagesKHR(context->device.logical_device, swapchain_info->swapchain_handle, &swapchain_info->image_counts, swapchain_info->images);
	}

	//views
	for (u32 i = 0; i < swapchain_info->image_counts; ++i) {
		VkImageViewCreateInfo view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		view_info.image = swapchain_info->images[i];
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.format = swapchain_info->surface_format.format;
		view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_info.subresourceRange.baseMipLevel = 0;
		view_info.subresourceRange.levelCount = 1;
		view_info.subresourceRange.baseArrayLayer = 0;
		view_info.subresourceRange.layerCount = 1;
		vk_assert(vkCreateImageView(context->device.logical_device, &view_info, context->vk_allocator, &swapchain_info->views[i]));
	}

	if (vk_query_pdevice_depth_format(&context->device) == false) {
		context->device.depth_format = VK_FORMAT_UNDEFINED;	
		//TODO::log
		std::cout << "not support depth format" << std::endl;
	}
}

static void
DestorySwapchain(vk_context* context, vk_swapchain_info* swapchain_info)
{

}
//::::::::::::::::::::::::::::::::::::::::::::::::::::swapchian::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




void vk_init_extensions(vk_context* context)
{
	context->instance_extensions.reserve(3);
	context->instance_extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
	context->instance_extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

	context->instance_layers_extensions.reserve(3);

	context->device_extensions.reserve(3);
	context->device_extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

	context->device_layers_extensions.reserve(3);
}


void vk_create_instance(vk_context* context)
{
	VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app_info.pApplicationName = "app name";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "well Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;


	VkInstanceCreateInfo instance_create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instance_create_info.pApplicationInfo = &app_info;
	instance_create_info.enabledExtensionCount = context->instance_extensions.size();
	instance_create_info.ppEnabledExtensionNames = &context->instance_extensions[0];
	instance_create_info.enabledLayerCount = 0;
	instance_create_info.ppEnabledLayerNames = 0;

	
	
	vkCreateInstance(&instance_create_info, context->vk_allocator, &context->vk_instance);
}


bool vk_select_pdevice(vk_context* context)
{
	u32 counts = 0;
	vkEnumeratePhysicalDevices(context->vk_instance, &counts, 0);
	if (counts == 0)
	{
		///TODO:logging
		std::cout << "no physical_deivce\n";
		return false;
	}

	VkPhysicalDevice* physical_device = (VkPhysicalDevice*)alloca(counts * sizeof(VkPhysicalDevice));

	vkEnumeratePhysicalDevices(context->vk_instance, &counts, physical_device);

	for (u32 i = 0; i < counts; i++)
	{
		VkPhysicalDeviceProperties properties = {};
		vkGetPhysicalDeviceProperties(physical_device[i], &properties);

		VkPhysicalDeviceFeatures deviceFeatures = {};
		vkGetPhysicalDeviceFeatures(physical_device[i], &deviceFeatures);

		VkPhysicalDeviceMemoryProperties memory_properties = {};
		vkGetPhysicalDeviceMemoryProperties(physical_device[i], &memory_properties);
		////TODO::config
		vk_pdevice_queue_requirements requirements = {};
		requirements.graphics = true;
		requirements.present = true;
		requirements.compute = true;
		requirements.transfer = true;
		requirements.sampler_anisotropy = true;
		requirements.discrete_gpu = true;

		////TODO::need to solve
		vk_select_queuefamily queue_family_info = {};

		////check physical device suitable?
		bool result = vk_pdevice_meets_required(
						physical_device[i],
						context->vk_surface,
						&properties,
						&deviceFeatures,
						&requirements,
						&queue_family_info,
						&context->device.pdevice_swapchain_potency);
		if (result == true)
		{
			context->device.physical_device = physical_device[i];
			context->device.queue_family_info = queue_family_info;
			////TODO::logging
			std::cout << "physical_device : " << i << "\n";
			std::cout << "Graphics Family index : " << context->device.queue_family_info.graphics_family_index << "\n";
			std::cout << "present Family index  :  " << context->device.queue_family_info.present_family_index << "\n";
			std::cout << "transfer Family index : " << context->device.queue_family_info.transfer_family_index << "\n";
			std::cout << "compute Family index  :  " << context->device.queue_family_info.compute_family_index << std::endl;

			////pdevice_swapchain_potency
			vk_query_pdevice_swapchain_potency(
				context->device.physical_device,
				context->vk_surface,
				&context->device.pdevice_swapchain_potency);

			
			return true;
		}
	}
	//TODO::logging
	std::cout << "device not suitable\n";
	return false;
}


bool vk_pdevice_meets_required(
	VkPhysicalDevice pdevice,
	VkSurfaceKHR surface,
	const VkPhysicalDeviceProperties* properties,
	const VkPhysicalDeviceFeatures* features,
	const vk_pdevice_queue_requirements* queue_requirements,
	vk_select_queuefamily* queue_family_info,
	vk_pdevice_swapchain_potency* pdevice_swapchain_potency)
{
	//init index...........................................................................
	queue_family_info->graphics_family_index = -1;
	queue_family_info->present_family_index = -1;
	queue_family_info->compute_family_index = -1;
	queue_family_info->transfer_family_index = -1;

	if (queue_requirements->discrete_gpu)
	{
		if (properties->deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			///TODO:LOGGING
			std::cout << "not discrete gpu\n";
			return false;
		}
	}

	u32 queuefamily_counts = {};
	VkQueueFamilyProperties* queuefamily_properties = {};
	vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queuefamily_counts, 0);
	if (queuefamily_counts != 0)
	{
		//TODO::log
		std::cout << queuefamily_counts << std::endl;
		queuefamily_properties = (VkQueueFamilyProperties*)alloca(queuefamily_counts * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queuefamily_counts, queuefamily_properties);
	}
	else {
		////TODO::
		std::cout << "queuefamily_counts = 0\n";
	}


	u8 min_transfer_score = 255;
	for (u32 i = 0; i < queuefamily_counts; i++) 
	{
		u8 current_transfer_score = 0;

		// Graphics queue?
		if (queue_family_info->graphics_family_index == -1 && queuefamily_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			queue_family_info->graphics_family_index = i;
			++current_transfer_score;
		}

		// Compute queue?
		if (queuefamily_properties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
			queue_family_info->compute_family_index = i;
			++current_transfer_score;
		}

		// Transfer queue?
		if (queuefamily_properties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
			// Take the index if it is the current lowest. This increases the
			// liklihood that it is a dedicated transfer queue.
			if (current_transfer_score <= min_transfer_score) {
				min_transfer_score = current_transfer_score;
				queue_family_info->transfer_family_index = i;
			}
		}

		//present queue?
		VkBool32 present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(pdevice, i, surface, &present);
		if (present) {
			queue_family_info->present_family_index = i;
		}
	}

	if (
		(!queue_requirements->graphics || (queue_requirements->graphics && queue_family_info->graphics_family_index != -1)) &&
		(!queue_requirements->present || (queue_requirements->present && queue_family_info->present_family_index != -1)) &&
		(!queue_requirements->compute || (queue_requirements->compute && queue_family_info->compute_family_index != -1)) &&
		(!queue_requirements->transfer || (queue_requirements->transfer && queue_family_info->transfer_family_index != -1))) {
		//TODO:LOGGING
		std::cout << "device queuefamily is enable\n";
		std::cout << "Graphics Family index : " << queue_family_info->graphics_family_index << "\n";
		std::cout << "present Family index : " << queue_family_info->present_family_index << "\n";
		std::cout << "transfer Family index : " << queue_family_info->transfer_family_index << "\n";
		std::cout << "compute Family index : " << queue_family_info->compute_family_index << std::endl;


	}

	if (queue_requirements->sampler_anisotropy && features->samplerAnisotropy)
	{
		//TODO::LOGGING
		std::cout << "device support anisotropy\n";
		return true;
	}
	else
	{
		//TODO::LOGGing
		std::cout << " fail" << std::endl;
	}

	return false;
	
}

void vk_query_pdevice_swapchain_potency(VkPhysicalDevice pdevice,
	VkSurfaceKHR surface,
	vk_pdevice_swapchain_potency* device_swapchain_potency)
{
	vk_assert(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdevice, surface, &device_swapchain_potency->capabilities));

	vk_assert(vkGetPhysicalDeviceSurfaceFormatsKHR(pdevice, surface, &device_swapchain_potency->format_counts, nullptr));

	if (device_swapchain_potency->format_counts > 0) {
		device_swapchain_potency->surface_formats = (VkSurfaceFormatKHR*)malloc(device_swapchain_potency->format_counts * sizeof(VkSurfaceFormatKHR));
		vk_assert(vkGetPhysicalDeviceSurfaceFormatsKHR(
			pdevice,
			surface,
			&device_swapchain_potency->format_counts,
			device_swapchain_potency->surface_formats));
	}
	else {
		//TODO::log
		std::cout << "device no swapchain potency" << std::endl;
	}
	vk_assert(vkGetPhysicalDeviceSurfacePresentModesKHR(
		pdevice,
		surface,
		&device_swapchain_potency->present_modes_counts,
		0));

	if (device_swapchain_potency->present_modes != nullptr)
	{
		free(device_swapchain_potency->present_modes);
	}
	device_swapchain_potency->present_modes = (VkPresentModeKHR*)malloc(device_swapchain_potency->present_modes_counts * sizeof(VkPresentModeKHR));
	vk_assert(vkGetPhysicalDeviceSurfacePresentModesKHR(
		pdevice,
		surface,
		&device_swapchain_potency->present_modes_counts,
		device_swapchain_potency->present_modes));
	////TODO:: log
	std::cout << "device potency init!\n" ;
}

bool vk_query_pdevice_depth_format(vk_device* device)
{
	const u32 type_counts = 3;
	VkFormat type[type_counts] = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
	};
	u32 flags = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
	for (i32 i = 0; i < type_counts; i ++ ) {
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(device->physical_device, type[i], &properties);

		if ((properties.linearTilingFeatures & flags) == flags) {
			device->depth_format = type[i];
			return true;
		}
		else if ((properties.optimalTilingFeatures & flags) == flags) {
			device->depth_format = type[i];
			return true;
		}
	}
	return false;
}

void vk_create_image(
	vk_context* context,
	VkImageType image_type,
	u32 w,
	u32 h,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlagBits memory_flags,
	bool create_view,
	VkImageAspectFlags view_aspect_flags,
	vk_image* image)
{


}



void vk_create_surface(vk_context* context, win32_platform_context* win_context)
{
	VkWin32SurfaceCreateInfoKHR surface_createinfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surface_createinfo.hinstance = win_context->win_instance;
	surface_createinfo.hwnd = win_context->win_handle;
	vkCreateWin32SurfaceKHR(context->vk_instance, &surface_createinfo, context->vk_allocator, &context->vk_surface);
}


void vk_create_device(vk_context* context)
{
	VkDeviceQueueCreateInfo queue_create_info[2] = {};
	for (i32 i = 0; i < 2; i++)
	{
		queue_create_info[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info[i].queueFamilyIndex = i * 2;
		queue_create_info[i].queueCount = 1;
		f32 priorities = 1.f;
		queue_create_info[i].pQueuePriorities = &priorities;
	}
	
	VkDeviceCreateInfo device_create_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	device_create_info.pQueueCreateInfos = queue_create_info;
	device_create_info.queueCreateInfoCount = 2;
	VkPhysicalDeviceFeatures enable_featrues = {};
	enable_featrues.samplerAnisotropy = true;
	device_create_info.pEnabledFeatures = &enable_featrues;
	device_create_info.enabledExtensionCount = 1;
	device_create_info.ppEnabledExtensionNames = &context->device_extensions[0];

	vk_assert(vkCreateDevice(context->device.physical_device,&device_create_info,context->vk_allocator,&context->device.logical_device));

	vkGetDeviceQueue(context->device.logical_device, context->device.queue_family_info.graphics_family_index, 0, &context->device.graphics_queue);
	vkGetDeviceQueue(context->device.logical_device, context->device.queue_family_info.graphics_family_index, 0, &context->device.present_queue);
	vkGetDeviceQueue(context->device.logical_device, context->device.queue_family_info.present_family_index, 0, &context->device.transfer_queue);
}


void vk_create_swapchain(vk_context* context, vk_swapchain_info* swapchain_info, u32 w, u32 h)
{
	CreateSwapchain(context, swapchain_info, w, h);
}


void vk_recreate_swapchain(vk_context* context, vk_swapchain_info* swapchain_info, u32 w, u32 h)
{
	DestorySwapchain(context, swapchain_info);
	CreateSwapchain(context, swapchain_info, w, h);
}


void vk_destory_swapchain(vk_context* context, vk_swapchain_info* swapchain_info)
{
	DestorySwapchain(context, swapchain_info);
}


bool vk_swapchain_acquire_next_image_index(
	vk_context* context,
	vk_swapchain_info* swapchain_info,
	u64 timeout_ns,
	VkSemaphore semaphore,
	VkFence fence,
	u32 next_image_index)
{
	VkResult result = vkAcquireNextImageKHR(
		context->device.logical_device,
		context->swapchain_info.swapchain_handle,
		timeout_ns,
		semaphore,
		fence,
		&next_image_index);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		vk_recreate_swapchain(context, swapchain_info, context->frame_buffer_w, context->frame_buffer_h);
		return false;
	}
	else if (result!=VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		//TODO::log
		std::cout << "fail to get next image" << std::endl;
		return false;
	}

	return true;
}


void vk_swapchain_present(
	vk_context* context,
	vk_swapchain_info* swapchain_info,
	VkQueue graphics_queue,
	VkQueue present_queue,
	VkSemaphore render_complete_semaphore,
	u32 present_image_index)
{
	// Return the image to the swapchain for presentation.
	VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };

	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = &render_complete_semaphore;
	present_info.swapchainCount = 1;
	present_info.pSwapchains = &swapchain_info->swapchain_handle;
	present_info.pImageIndices = &present_image_index;
	present_info.pResults = 0;

	VkResult result = vkQueuePresentKHR(present_queue, &present_info);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		vk_recreate_swapchain(context, swapchain_info, context->frame_buffer_w, context->frame_buffer_h);

	}
	else if (result != VK_SUCCESS) {
		//TODO::log
		std::cout << "fail to present iamges\n";
	}
}
