#include "h_vulkan_API.h"
#include "h_clogger.h"
#include "h_filesystem.h"
#include <sstream>

//@Param:static meta func
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
			LTRACE("surface format is founded");
			break;
		}
	}
	if (!found) {
		LERR("surface format is not founded");
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

	swapchain_info->image_counts = 0;
	
	vk_assert(vkGetSwapchainImagesKHR(context->device.logical_device, swapchain_info->swapchain_handle, &swapchain_info->image_counts, 0));
	if (swapchain_info->image_counts > 0) {

		LTRACE("images_counts = " << swapchain_info->image_counts);
		/*if (swapchain_info->images != nullptr)
		{
			free(swapchain_info->images);
		}*/
		if (swapchain_info->images == nullptr) {
			swapchain_info->images = (VkImage*)malloc(swapchain_info->image_counts * sizeof(VkImage));
		}
		vkGetSwapchainImagesKHR(context->device.logical_device, swapchain_info->swapchain_handle, &swapchain_info->image_counts, swapchain_info->images);
	}

	//views
	/*if (swapchain_info->views != nullptr)
	{
		free(swapchain_info->views);
	}*/
	if (swapchain_info->views == nullptr) {
		swapchain_info->views = (VkImageView*)malloc(swapchain_info->image_counts * sizeof(VkImageView));
	}
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
		LERR("not support depth format");
	}

	//@Param:create depth image and image view
	vk_create_image(
		context,
		VK_IMAGE_TYPE_2D,
		extent.width,
		extent.height,
		context->device.depth_format,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		true,
		VK_IMAGE_ASPECT_DEPTH_BIT,
		&swapchain_info->depth_image);
	context->extent_w = extent.width;
	context->extent_h = extent.height;

}

static void
DestroySwapchain(vk_context* context, vk_swapchain_info* swapchain_info)
{
	vkDeviceWaitIdle(context->device.logical_device);
	vk_destroy_image(context, &swapchain_info->depth_image);
	for (i32 i = 0; i < swapchain_info->image_counts; i++)
	{
		vkDestroyImageView(
			context->device.logical_device,
			swapchain_info->views[i],
			context->vk_allocator);
	}
	vkDestroySwapchainKHR(
		context->device.logical_device,
		swapchain_info->swapchain_handle,
		context->vk_allocator);
}
//::::::::::::::::::::::::::::::::::::::::::::::::::::swapchian::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
static i32 vk_find_memtype_index(VkPhysicalDevice device,u32 typefilter,u32 flag) {
	VkPhysicalDeviceMemoryProperties memory_properties;
	i32 memory_typeindex = 0;
	vkGetPhysicalDeviceMemoryProperties(device, &memory_properties);
	//info:why is this?
	for (int i = 0; i < memory_properties.memoryTypeCount; i++) {
		if (typefilter & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & flag) == flag) {
			memory_typeindex = i;
			break;
		}
		memory_typeindex = -1;
	}
	return memory_typeindex;
}



void vk_init_extensions(vk_context* context)
{
	context->instance_extensions.reserve(3);
	context->instance_extensions.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
	context->instance_extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

	context->instance_layers_extensions.reserve(3);

	context->device_extensions.reserve(3);
	context->device_extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	context->device_extensions.emplace_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	context->device_layers_extensions.reserve(3);

	LTRACE("extension is loaded!");
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
	LTRACE("vulkan instance is created!");
}


void vk_destroy_instance(vk_context* context)
{
	vkDestroyInstance(context->vk_instance, context->vk_allocator);;
}


bool vk_select_pdevice(vk_context* context)
{
	u32 counts = 0;
	vkEnumeratePhysicalDevices(context->vk_instance, &counts, 0);
	if (counts == 0)
	{
		LERR("no physical_deivce");
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
			
			LTRACE("physical_device : "<< i);
			LTRACE("Graphics Family index : "<<context->device.queue_family_info.graphics_family_index);
			LTRACE("present  Family index : "<<context->device.queue_family_info.present_family_index );
			LTRACE("transfer Family index : "<<context->device.queue_family_info.transfer_family_index);
			LTRACE("compute  Family index : "<<context->device.queue_family_info.compute_family_index );

			////pdevice_swapchain_potency
			vk_query_pdevice_swapchain_potency(
				context->device.physical_device,
				context->vk_surface,
				&context->device.pdevice_swapchain_potency);

			
			return true;
		}
	}
	
	LERR("device not suitable");
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
			LERR("not discrete gpu");
			return false;
		}
	}

	u32 queuefamily_counts = {};
	VkQueueFamilyProperties* queuefamily_properties = {};
	vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queuefamily_counts, 0);
	if (queuefamily_counts != 0)
	{
		LTRACE("queue family counts is :  "<< queuefamily_counts);
		queuefamily_properties = (VkQueueFamilyProperties*)alloca(queuefamily_counts * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(pdevice, &queuefamily_counts, queuefamily_properties);
	}
	else {
		LERR("queuefamily_counts = 0.");
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

		LTRACE("device queuefamily is OK , data is preserve");

	}

	if (queue_requirements->sampler_anisotropy && features->samplerAnisotropy)
	{
		
		LTRACE("device support anisotropy.");
		return true;
	}
	else
	{
		LERR("phisical device is not support anisotripy.")
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
		if (device_swapchain_potency->surface_formats == nullptr) {
			device_swapchain_potency->surface_formats = (VkSurfaceFormatKHR*)malloc(device_swapchain_potency->format_counts * sizeof(VkSurfaceFormatKHR));
		}
		vk_assert(vkGetPhysicalDeviceSurfaceFormatsKHR(
			pdevice,
			surface,
			&device_swapchain_potency->format_counts,
			device_swapchain_potency->surface_formats));
	}
	else {
		
		LERR( "phisical device no swapchain potency");
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
	
	LTRACE("phisical device has the swapchian potency!");
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


//@Param:depth image
void vk_create_image(
	vk_context* context,
	VkImageType image_type,
	u32 w,
	u32 h,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlagBits usage,
	VkMemoryPropertyFlags memory_flags,
	bool create_view,
	VkImageAspectFlags view_aspect_flags,
	vk_depth_image* depth_image)
{

	VkImageCreateInfo create_info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	create_info.arrayLayers = 1;
	create_info.extent.width = w;
	create_info.extent.height = h;
	create_info.extent.depth = 1;//TODO
	create_info.imageType = VK_IMAGE_TYPE_2D;
	create_info.format = format;
	create_info.tiling = tiling;
	create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	create_info.usage = usage;
	create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	create_info.mipLevels = 4;

	vk_assert(vkCreateImage(context->device.logical_device, &create_info, context->vk_allocator, &depth_image->image_handle));

	VkMemoryRequirements memory_requirements;
	vkGetImageMemoryRequirements(context->device.logical_device, depth_image->image_handle, &memory_requirements);

	//Query memory requirements.
	
	//i32 memory_typeindex = 0;
	//VkPhysicalDeviceMemoryProperties memory_properties;
	//vkGetPhysicalDeviceMemoryProperties(context->device.physical_device, &memory_properties);
	////info:why is this?
	//for (int i = 0; i < memory_properties.memoryTypeCount; i++) {
	//	if (memory_requirements.memoryTypeBits & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & memory_flags) == memory_flags) {
	//		memory_typeindex = i;
	//		break;
	//	}
	//	memory_typeindex = -1;
	//}
	
	// Allocate memory
	i32 memory_typeindex = vk_find_memtype_index(context->device.physical_device, memory_requirements.memoryTypeBits, memory_flags);
	if (memory_typeindex == -1)
	{
		LERR("no memory type for image");
	}
	VkMemoryAllocateInfo memory_allocate_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; 
	memory_allocate_info.allocationSize  = memory_requirements.size;
	memory_allocate_info.memoryTypeIndex = memory_typeindex;

	vk_assert(vkAllocateMemory(context->device.logical_device, &memory_allocate_info, context->vk_allocator, &depth_image->memory)); 

	// Bind the memory
	vk_assert(vkBindImageMemory(context->device.logical_device, depth_image->image_handle, depth_image->memory, 0));
	// TODO: configurable memory offset.

	if (create_view) {
		depth_image->view = 0;
		vk_create_image_view(context, format, depth_image, view_aspect_flags);
	}

}

void vk_create_image_view(
	vk_context* context,
	VkFormat format,
	vk_depth_image* image,
	VkImageAspectFlags aspect_flag)
{
	VkImageViewCreateInfo view_create_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO }; 
	view_create_info.image = image->image_handle;
	view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;         // TODO: Make configurable. view_create_info.format = format;
	view_create_info.subresourceRange.aspectMask = aspect_flag;//-TODO:-Make configurable
	view_create_info.subresourceRange.baseMipLevel = 0; 
	view_create_info.subresourceRange.levelCount = 1; 
	view_create_info.subresourceRange.baseArrayLayer = 0;
	view_create_info.subresourceRange.layerCount = 1;
	view_create_info.format = format;
	vk_assert(vkCreateImageView(context->device.logical_device, &view_create_info, context->vk_allocator, &image->view));
}

void vk_destroy_image(vk_context* context, vk_depth_image* image)
{
	if (image->view) {
		vkDestroyImageView(context->device.logical_device, image->view, context->vk_allocator);
	}
	if (image->memory) {
		vkFreeMemory(context->device.logical_device, image->memory, context->vk_allocator);
	}
	if (image->image_handle) {
		vkDestroyImage(context->device.logical_device, image->image_handle, context->vk_allocator);
	}
}

//@Param:surface
void vk_create_surface(vk_context* context, win32_platform_context* win_context)
{
	VkWin32SurfaceCreateInfoKHR surface_createinfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surface_createinfo.hinstance = win_context->win_instance;
	surface_createinfo.hwnd = win_context->win_handle;
	vkCreateWin32SurfaceKHR(context->vk_instance, &surface_createinfo, context->vk_allocator, &context->vk_surface);

	LTRACE("vulkan surface is creates!");
}


void vk_destroy_surface(vk_context* context)
{
	vkDestroySurfaceKHR(context->vk_instance, context->vk_surface, context->vk_allocator);
}


//@Param:device
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
	LTRACE("logical device is created!");

	vkGetDeviceQueue(context->device.logical_device, context->device.queue_family_info.graphics_family_index, 0, &context->device.graphics_queue);
	vkGetDeviceQueue(context->device.logical_device, context->device.queue_family_info.graphics_family_index, 0, &context->device.present_queue);
	vkGetDeviceQueue(context->device.logical_device, context->device.queue_family_info.present_family_index, 0, &context->device.transfer_queue);
	LTRACE("logical device queue handle is binded!");

	VkCommandPoolCreateInfo pool_create = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	pool_create.queueFamilyIndex = context->device.queue_family_info.graphics_family_index;
	pool_create.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkCreateCommandPool(context->device.logical_device, &pool_create,context->vk_allocator, &context->device.g_cmdpool);
	LTRACE("logical device command pool is created!");
}

void vk_destroy_device(vk_context* context)
{
	vkDestroyDevice(context->device.logical_device, context->vk_allocator);
}


//@Param:cmdpool is created in vk_create_device function ,and destroy at this
void vk_destory_cmdpool(vk_context* context)
{
	vkDestroyCommandPool(
		context->device.logical_device,
		context->device.g_cmdpool,
		context->vk_allocator);
	context->device.g_cmdpool = 0;
}



//@Param: swapchain
void vk_create_swapchain(vk_context* context, vk_swapchain_info* swapchain_info, u32 w, u32 h)
{
	CreateSwapchain(context, swapchain_info, w, h);
	LTRACE("swapchain is created!");
}


void vk_recreate_swapchain(vk_context* context, vk_swapchain_info* swapchain_info, u32 w, u32 h)
{
	DestroySwapchain(context, swapchain_info);
	CreateSwapchain(context, swapchain_info, w, h);
}


void vk_destroy_swapchain(vk_context* context, vk_swapchain_info* swapchain_info)
{
	DestroySwapchain(context, swapchain_info);
}


bool vk_swapchain_acquire_next_image_index(
	vk_context* context,
	vk_swapchain_info* swapchain_info,
	u64 timeout_ns,
	VkSemaphore semaphore,
	VkFence fence,
	u32* next_image_index)
{
	VkResult result = vkAcquireNextImageKHR(
		context->device.logical_device,
		context->swapchain_info.swapchain_handle,
		timeout_ns,
		semaphore,
		fence,
		next_image_index);
	vk_assert(result);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		vk_recreate_swapchain(context, swapchain_info, context->extent_w, context->extent_h);
		return false;
	}
	else if (result!=VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		
		LERR( "fail to get next image") ;
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
	u32 present_image_index,
	f32 w,
	f32 h)
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
	vk_assert(result);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		vk_recreate_swapchain(context, swapchain_info, w,h);

	}
	else if (result != VK_SUCCESS) {
		
		LERR("fail to present images");
	}

	context->current_frame = (context->current_frame + 1) % swapchain_info->max_frames_in_flight;
}


//@Param:renderpass
void vk_create_renderpass(
	vk_context* context,
	vk_renderpass* renderpass,
	f32 r, f32 g, f32 b, f32 a,
	f32 depth, u32 stencil)

{
	renderpass->r = r;
	renderpass->g = g;
	renderpass->b = b;
	renderpass->a = a;

	renderpass->depth = depth;
	renderpass->stencil = stencil;


	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	

	const u32 count = 2;
	VkAttachmentDescription attachments_description[count] = {};

	VkAttachmentDescription color_attachment = {};
	color_attachment.format = context->swapchain_info.surface_format.format;//todo :: config
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	color_attachment.flags = 0;

	attachments_description[0] = color_attachment;

	VkAttachmentReference color_attachment_ref = {};
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


	//depth_attachment
	VkAttachmentDescription depth_attachment = {};
	depth_attachment.format = context->device.depth_format;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments_description[1] = depth_attachment;

	VkAttachmentReference depth_attachment_ref = {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//attach to subpass
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;
	subpass.pDepthStencilAttachment = &depth_attachment_ref;

	//shader
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = 0;

	//multisampling colour attachments
	subpass.pResolveAttachments = 0;

	// Attachments not used in this subpass, but must be preserved for the next.
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = 0;

	// Render pass dependencies. TODO: make this configurable.
	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = 0;

	VkRenderPassCreateInfo create_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	// xenaer pass create.
	create_info.attachmentCount = count;
	create_info.pAttachments = attachments_description;
	create_info.subpassCount = 1;
	create_info.pSubpasses = &subpass;
	create_info.dependencyCount = 1;
	create_info.pDependencies = &dependency;
	create_info.pNext = 0;
	create_info.flags = 0;
	

	vk_assert(vkCreateRenderPass(context->device.logical_device, &create_info, context->vk_allocator, &renderpass->renderpass_handle));
	LTRACE("renderpass is created!");
}


void vk_destroy_renderpass(vk_context* context, vk_renderpass* renderpass)
{
	if (renderpass && renderpass->renderpass_handle) {
		vkDestroyRenderPass(context->device.logical_device, renderpass->renderpass_handle, context->vk_allocator);
		renderpass->renderpass_handle = 0;
	}
}

void vk_renderpass_begin(
	vk_cmdbuffer* cmdbuffer,
	vk_renderpass* renderpass,
	VkFramebuffer frame_buffer)
{
	
	VkRenderPassBeginInfo begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	begin_info.renderPass = renderpass->renderpass_handle;
	begin_info.framebuffer = frame_buffer;
	begin_info.renderArea.extent.width = renderpass->w;
	begin_info.renderArea.extent.height = renderpass->h;
	begin_info.renderArea.offset.x = 0;
	begin_info.renderArea.offset.y = 0;

	VkClearValue clear_value[2] = {};
	clear_value[0].color.float32[0] = renderpass->r;
	clear_value[0].color.float32[1] = renderpass->g;
	clear_value[0].color.float32[2] = renderpass->b;
	clear_value[0].color.float32[3] = renderpass->a;
	clear_value[1].depthStencil.depth = renderpass->depth;
	clear_value[1].depthStencil.stencil= renderpass->stencil;

	begin_info.clearValueCount = 2;
	begin_info.pClearValues = clear_value;

	vkCmdBeginRenderPass(cmdbuffer->cmdbuffer_handle, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
	cmdbuffer->cmdbuffer_state = CMD_IN_RENDER_PASS;
}

void vk_renderpass_end(vk_cmdbuffer* cmdbuffer,vk_renderpass* renderpass)
{
	vkCmdEndRenderPass(cmdbuffer->cmdbuffer_handle);
	cmdbuffer->cmdbuffer_state = CMD_RECORDING;

}


//@Param:cmdbufffer
void vk_cmdbuffer_allocate(
	vk_context* context,
	VkCommandPool pool,
	bool is_primary,
	vk_cmdbuffer* cmdbuffer)
{

	VkCommandBufferAllocateInfo create_info = {};
	create_info.commandBufferCount = 1;
	create_info.commandPool = pool;
	create_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	create_info.pNext = 0;
	create_info.level = is_primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;

	cmdbuffer->cmdbuffer_state = CMD_NOT_ALLOCATED;
	vk_assert(vkAllocateCommandBuffers(context->device.logical_device, &create_info, &cmdbuffer->cmdbuffer_handle));
	cmdbuffer->cmdbuffer_state = CMD_READY;

}

void vk_cmdbuffer_free(
	vk_context* context,
	VkCommandPool pool,
	vk_cmdbuffer* cmdbuffer)
{
	vkFreeCommandBuffers(context->device.logical_device, pool, 1, &cmdbuffer->cmdbuffer_handle);

	cmdbuffer->cmdbuffer_handle = 0;
	cmdbuffer->cmdbuffer_state = CMD_NOT_ALLOCATED;
}

void vk_cmdbuffer_begin(
	vk_cmdbuffer* cmdbuffer,
	bool is_single_use,
	bool is_renderpass_continue_use,
	bool is_simultaneous_use)
{
	VkCommandBufferBeginInfo create_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	create_info.flags = 0;
	if (is_single_use)
		create_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (is_renderpass_continue_use)
		create_info.flags |= VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	if (is_simultaneous_use)
		create_info.flags |= VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

	vk_assert(vkBeginCommandBuffer(cmdbuffer->cmdbuffer_handle, &create_info));
	cmdbuffer->cmdbuffer_state = CMD_RECORDING;
}

void vk_cmdbuffer_end(vk_cmdbuffer* cmdbuffer)
{
	vk_assert(vkEndCommandBuffer(cmdbuffer->cmdbuffer_handle));
	cmdbuffer->cmdbuffer_state = CMD_RECORDING_ENDED;
}


void vk_cmdbuffer_update_submitted(vk_cmdbuffer* cmdbuffer)
{
	cmdbuffer->cmdbuffer_state = CMD_SUBMITTED;
}
void vk_cmdbuffer_reset(vk_cmdbuffer* cmdbuffer)
{
	cmdbuffer->cmdbuffer_state = CMD_READY;
}


void vk_cmdbuffer_allocate_and_begin_single_use(
	vk_context* context,
	VkCommandPool pool,
	vk_cmdbuffer* cmdbuffer)
{
	vk_cmdbuffer_allocate(context, pool, true, cmdbuffer);
	vk_cmdbuffer_begin(cmdbuffer, true, false, false);
}

void vk_cmdbuffer_free_and_end_single_use(
	vk_context* context,
	VkCommandPool pool,
	vk_cmdbuffer* cmdbuffer,
	VkQueue queue)
{
	vk_cmdbuffer_end(cmdbuffer);

	VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmdbuffer->cmdbuffer_handle;

	vk_assert(vkQueueSubmit(queue, 1, &submit_info, 0));

	vk_assert(vkQueueWaitIdle(queue));

	vk_cmdbuffer_free(context, pool, cmdbuffer);
}



//@Param:framebuffer
void vk_create_framebuffer(
	vk_context* context,
	vk_renderpass* renderpass,
	u32 w,
	u32 h,
	u32 attach_counts,
	VkImageView* views,
	vk_framebuffer* framebuffer)
{
	framebuffer->views = (VkImageView*)malloc(attach_counts * sizeof(VkImageView));
	for (i32 i = 0; i < attach_counts; i++)
	{
		framebuffer->views[i] = views[i];
	}
	framebuffer->renderpass = renderpass;
	framebuffer->attachment_counts = attach_counts;

	VkFramebufferCreateInfo create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
	create_info.attachmentCount = attach_counts;
	create_info.pAttachments = &framebuffer->views[0];
	create_info.renderPass = renderpass->renderpass_handle;
	create_info.width = w;
	create_info.height = h;
	create_info.layers = 1;

	vk_assert(vkCreateFramebuffer(
		context->device.logical_device,
		&create_info,
		context->vk_allocator, 
		&framebuffer->framebuffer_handle));
}


void vk_destroy_framebuffer(vk_context* context, vk_framebuffer* framebuffer)
{
	vkDestroyFramebuffer(
		context->device.logical_device,
		framebuffer->framebuffer_handle,
		context->vk_allocator);
}



//@Param:signal
void vk_create_fence(vk_context* context, bool create_signal, vk_fence* fence)
{
	fence->is_signaled = create_signal;
	VkFenceCreateInfo create_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	if (fence->is_signaled) {
		create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	}
	vkCreateFence(
		context->device.logical_device,
		&create_info,
		context->vk_allocator,
		&fence->fence_handle);

}

void vk_destroy_fence(vk_context* context, vk_fence* fence)
{
	if (fence->fence_handle) {
		vkDestroyFence(context->device.logical_device, fence->fence_handle, context->vk_allocator);
	}
	fence->is_signaled = false;
}

bool vk_fence_wait(vk_context* context, vk_fence* fence, u64 time_out_ns)
{
	if (!fence->is_signaled)
	{
		VkResult result = vkWaitForFences(context->device.logical_device, 1, &fence->fence_handle, true, time_out_ns);
		//todo::info::
		switch (result)
		{
		case VK_SUCCESS: {
			fence->is_signaled = true;
			return true;
		}break;

		case VK_TIMEOUT: {

		}break;

		case VK_ERROR_DEVICE_LOST: {

		}break;

		case VK_ERROR_OUT_OF_HOST_MEMORY: {

		}break;

		default: {

		} break;
		}
	}
	else {
		return true;
	}
}

void vk_fence_reset(vk_context* context, vk_fence* fence)
{
	if (fence->is_signaled) {
		vkResetFences(context->device.logical_device, 1, &fence->fence_handle);
		fence->is_signaled = false;
	}
}

//1:@Param:shader
#define BUILTIN_SHADER_NAME_OBJECT "builtin_shader" //TODO:clean up

static bool vk_create_shader_module(
		vk_context* context,
		const char* name,
		const char* type_str,
		VkShaderStageFlagBits flag,
		u32 stage_index,
		vk_shader_stage* shader_stage)
{
	std::stringstream stream;
	stream << "asset/shaders/shader_code/" << name << "." << type_str << ".spv";
	shader_stage[stage_index].create_info.sType = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };

	fs_file_handle handle = {};
	if (!fs_open(stream.str().c_str(), FILE_MODE_READ, true, &handle))
	{
		LERR("can not read shader module: " << stream.str());
		return false;
	}

	size_t size = 0;
	u8* file_buf = 0;
	if (!fs_read_all_bytes(&handle, &file_buf, &size)) {
		LERR("can not read shader module as binary: " << stream.str());
		return false;
	}
	shader_stage[stage_index].create_info.codeSize = size;
	shader_stage[stage_index].create_info.pCode = (u32*)file_buf;

	fs_close(&handle);

	vk_assert(vkCreateShaderModule(
		context->device.logical_device,
		&shader_stage[stage_index].create_info,
		context->vk_allocator,
		&shader_stage[stage_index].handle));

	shader_stage[stage_index].shader_stage_create_info.sType = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shader_stage[stage_index].shader_stage_create_info.stage = flag;
	shader_stage[stage_index].shader_stage_create_info.module = shader_stage[stage_index].handle;
	shader_stage[stage_index].shader_stage_create_info.pName = "main";

	if (file_buf) {
		free(file_buf);
	}

}

bool vk_create_shader(vk_context* context, vk_shader* shader)
{
	char stage_type_str[shader_counts][5] = { "vert" , "frag" };
	VkShaderStageFlagBits stage_type[shader_counts] = { VK_SHADER_STAGE_VERTEX_BIT,VK_SHADER_STAGE_FRAGMENT_BIT };
	for (i32 i = 0; i < shader_counts; i++) {
		if (!vk_create_shader_module(context, BUILTIN_SHADER_NAME_OBJECT, stage_type_str[i], stage_type[i], i,shader->stages)) {
			LERR("can not create shader for: " << stage_type_str[i] << BUILTIN_SHADER_NAME_OBJECT);
			return false;
		}
	}
	//TODO:descriptors

	//@Param:pipeline
	VkViewport view = {};
	view.x = 0.f;
	view.y = 0.f;
	view.width = context->extent_w;
	view.height = context->extent_h;
	view.minDepth = 0.f;
	view.maxDepth = 1.f;
	
	VkRect2D scissor = {};
	scissor.offset = { 0,0 };
	scissor.extent = { context->extent_w ,context->extent_h };

	u32 offest = 0;
	const i32 attribute_count = 1;
	VkVertexInputAttributeDescription attribute_description[attribute_count];

	VkFormat format[attribute_count] = {
		VK_FORMAT_R32G32B32_SFLOAT,
	};
	size_t size[attribute_count] = { 3 * sizeof(f32) };
	for (i32 i = 0; i < attribute_count; i++) {
		attribute_description[i].offset = offest;
		attribute_description[i].format = format[i];
		attribute_description[i].location = i;
		attribute_description[i].binding = 0;
		offest += size[i];
	}

	//TODO:descriptor set layout

	//@Param:pipeline shader stages
	VkPipelineShaderStageCreateInfo stage_create_info[shader_counts]; 
	for (i32 i = 0; i < shader_counts; i++) {
		stage_create_info[i].sType = shader->stages[i].shader_stage_create_info.sType;
		stage_create_info[i] = shader->stages[i].shader_stage_create_info;
	}

	if (!vk_create_g_pipeline(
		context,
		&context->main_renderpass,
		attribute_count,
		attribute_description,
		0,
		0,
		shader_counts,
		stage_create_info,
		view,
		scissor,
		false,
		&shader->pipeline))
	{
		LERR("fail to load g pipeline!");
		return false;
	}
	return true;
}
void vk_destroy_shader(vk_context* context, vk_shader* shader)
{
	vk_destroy_pipeline(context, &shader->pipeline);

	for (i32 i = 0; i < shader_counts; i++) {
		vkDestroyShaderModule(
			context->device.logical_device,
			shader->stages[i].handle,
			context->vk_allocator);

		shader->stages[i].handle = 0;
	}
}
void vk_use_shader(vk_context* context, vk_shader* shader)
{
	//TODO:
}

bool vk_create_g_pipeline(
	vk_context* context,
	vk_renderpass* renderpass,
	u32 attribute_counts,
	VkVertexInputAttributeDescription* attributes,
	u32 descriptors_set_layout_counts,
	VkDescriptorSetLayout* descriptors_set_layout,
	u32 stage_counts,
	VkPipelineShaderStageCreateInfo* stages,
	VkViewport viewport,
	VkRect2D scissor,
	bool is_wireframe,
	vk_pipeline* pipeline)
{
	VkPipelineViewportStateCreateInfo view_state = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	view_state.pScissors						 = &scissor;
	view_state.pViewports					     = &viewport;
	view_state.viewportCount					 = 1;
	view_state.scissorCount						 = 1;

	VkPipelineRasterizationStateCreateInfo ras   = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	ras.depthClampEnable                         = false;
	ras.rasterizerDiscardEnable                  = false;
	ras.polygonMode				                 = is_wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	ras.lineWidth				                 = 1.f;
	ras.cullMode			                     = VK_CULL_MODE_BACK_BIT;
	ras.frontFace				                 = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	ras.depthBiasClamp			                 = 0.f;
	ras.depthBiasConstantFactor                  = 0.f;
	ras.depthBiasEnable			                 = false;
	ras.depthBiasSlopeFactor	                 = 0.f; 

	// Multisampling.
	VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling.sampleShadingEnable				   = false;
	multisampling.rasterizationSamples				   = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading					   = 1.f;
	multisampling.pSampleMask						   = 0;
	multisampling.alphaToCoverageEnable				   = false;
	multisampling.alphaToOneEnable					   = false;

	// Depth and stencil testing.
	VkPipelineDepthStencilStateCreateInfo depth_stencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depth_stencil.depthTestEnable					    = true;
	depth_stencil.depthWriteEnable					    = true;
	depth_stencil.depthCompareOp					    = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable				    = false;
	depth_stencil.stencilTestEnable					    = false;

	VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
	color_blend_attachment_state.blendEnable		 = true;
	color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment_state.colorBlendOp		 = VK_BLEND_OP_ADD;
	color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	color_blend_attachment_state.alphaBlendOp		 = VK_BLEND_OP_ADD;
	color_blend_attachment_state.colorWriteMask		 = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
												       VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;



	VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	color_blend_state_create_info.attachmentCount = 1;
	color_blend_state_create_info.logicOpEnable = false;
	color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
	color_blend_state_create_info.pAttachments = &color_blend_attachment_state;

	// Dynamic state
	const u32 dynamic_state_count = 3;
	VkDynamicState dynamic_states[dynamic_state_count] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_LINE_WIDTH };

	VkPipelineDynamicStateCreateInfo dynamic_states_create_info = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamic_states_create_info.dynamicStateCount = dynamic_state_count;
	dynamic_states_create_info.pDynamicStates = dynamic_states;

	//@Param:vertex description
	VkVertexInputBindingDescription binding_description = {};
	binding_description.binding = 0;
	binding_description.stride = 3*sizeof(f32);
	binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkPipelineVertexInputStateCreateInfo vertex_input_info = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertex_input_info.vertexBindingDescriptionCount = 1;
	vertex_input_info.pVertexBindingDescriptions = &binding_description;
	vertex_input_info.vertexAttributeDescriptionCount = attribute_counts;
	vertex_input_info.pVertexAttributeDescriptions = attributes;

	VkPipelineInputAssemblyStateCreateInfo input_assembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = false;

	//@Param:pipeline layout
	VkPipelineLayoutCreateInfo layout = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	layout.setLayoutCount = descriptors_set_layout_counts;
	layout.pSetLayouts = descriptors_set_layout;

	vk_assert(vkCreatePipelineLayout(
		context->device.logical_device,
		&layout,
		context->vk_allocator,
		&pipeline->pipeline_layout));

	VkGraphicsPipelineCreateInfo pipeline_create_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipeline_create_info.stageCount = stage_counts;
	pipeline_create_info.pStages = stages;
	pipeline_create_info.pVertexInputState = &vertex_input_info;
	pipeline_create_info.pInputAssemblyState = &input_assembly;

	pipeline_create_info.pViewportState = &view_state;
	pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
	pipeline_create_info.pDepthStencilState = &depth_stencil;
	pipeline_create_info.pDynamicState = &dynamic_states_create_info;
	pipeline_create_info.pMultisampleState = &multisampling;
	pipeline_create_info.pRasterizationState = &ras;
	pipeline_create_info.pTessellationState = 0;

	pipeline_create_info.layout = pipeline->pipeline_layout;

	pipeline_create_info.renderPass = renderpass->renderpass_handle;
	pipeline_create_info.subpass = 0;
	pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_create_info.basePipelineIndex = -1;

	vk_assert(vkCreateGraphicsPipelines(
		context->device.logical_device,
		VK_NULL_HANDLE,
		1,
		&pipeline_create_info,
		context->vk_allocator,
		&pipeline->handle));
	return true;
}

void vk_destroy_pipeline(vk_context* context, vk_pipeline* pipeline)
{
	if (pipeline->handle) {
		vkDestroyPipeline(context->device.logical_device, pipeline->handle, context->vk_allocator);
		pipeline->handle = 0;
	}
	if (pipeline->pipeline_layout)
	{
		vkDestroyPipelineLayout(context->device.logical_device, pipeline->pipeline_layout, context->vk_allocator);
		pipeline->pipeline_layout = 0;
	}
}

void vk_bind_pipeline(vk_cmdbuffer* cmdbuf, VkPipelineBindPoint bindpoint, vk_pipeline* pipeline)
{
	vkCmdBindPipeline(cmdbuf->cmdbuffer_handle, bindpoint, pipeline->handle);
}



//1 @Param:buffer

bool vk_create_buffer(
	vk_context* context,
	size_t size,
	VkBufferUsageFlagBits usage,
	u32 mem_flag,
	bool bind_on_create,
	vk_buffer* buf)
{
	buf->usage = usage;
	buf->total_size = size;
	buf->mem_property_flags = mem_flag;

	VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	info.size = size;
	info.usage = usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	vk_assert(vkCreateBuffer(context->device.logical_device, &info, context->vk_allocator, &buf->handle));

	VkMemoryRequirements requirements = {};
	vkGetBufferMemoryRequirements(context->device.logical_device, buf->handle, &requirements);
	buf->mem_index = vk_find_memtype_index(context->device.physical_device, requirements.memoryTypeBits, mem_flag);
	if (buf->mem_index == -1) {
		LERR("memory type not found!");
		return false;
	}

	VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	alloc_info.allocationSize = requirements.size;
	alloc_info.memoryTypeIndex = buf->mem_index;

	vk_assert(vkAllocateMemory(context->device.logical_device, &alloc_info, context->vk_allocator, &buf->memory));

	if (bind_on_create) {
		vk_bind_buffer(context, buf, 0);
	}

	return true;
}

void vk_destroy_buffer(vk_context* context, vk_buffer* buf)
{
	if (buf->memory) {
		vkFreeMemory(context->device.logical_device, buf->memory, context->vk_allocator);
		buf->memory = 0;
	}
	if (buf->handle) {
		vkDestroyBuffer(context->device.logical_device, buf->handle, context->vk_allocator);
		buf->handle = 0;
	}
	buf->total_size = 0;
	buf->is_locked = false;
}

void* vk_lock_buffer_mem(vk_context* context, vk_buffer* buf, size_t offest, size_t size, u32 flags)
{
	void* data;
	vk_assert(vkMapMemory(context->device.logical_device, buf->memory, offest, size, flags, &data));
	return data;
}

void vk_unlock_buffer_mem(vk_context* context, vk_buffer* buf)
{
	vkUnmapMemory(context->device.logical_device, buf->memory);
}

void vk_buffer_load_data(vk_context* context, vk_buffer* buf, size_t offest, size_t size, u32 flags, const void* data)
{
	void* data_ptr;
	vk_assert(vkMapMemory(context->device.logical_device, buf->memory, offest, size, flags, &data_ptr));
	memcpy(data_ptr, data, size);
	vkUnmapMemory(context->device.logical_device, buf->memory);
}

void vk_copy_buffer(
	vk_context* context,
	VkCommandPool pool,
	VkFence fence,
	VkQueue queue,
	VkBuffer src,
	size_t offest,
	VkBuffer dest,
	size_t dest_offest,
	size_t size)
{
	vk_cmdbuffer temp_cmdbuf;
	vk_cmdbuffer_allocate_and_begin_single_use(context, pool, &temp_cmdbuf);

	VkBufferCopy copy_region;
	copy_region.dstOffset = dest_offest;
	copy_region.srcOffset = offest;
	copy_region.size = size;

	vkCmdCopyBuffer(temp_cmdbuf.cmdbuffer_handle, src, dest, 1, &copy_region);

	vk_cmdbuffer_free_and_end_single_use(context, pool, &temp_cmdbuf, queue);
}

bool vk_resize_buffer(
	vk_context* context,
	VkCommandPool pool,
	vk_buffer* buf,
	VkQueue queue,
	size_t new_size)
{
	VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	info.size = new_size;
	info.usage = buf->usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	
	VkBuffer new_buf = {};
	vk_assert(vkCreateBuffer(context->device.logical_device, &info, context->vk_allocator, &new_buf));

	VkMemoryRequirements requirements = {};
	vkGetBufferMemoryRequirements(context->device.logical_device, new_buf, &requirements);

	
	VkMemoryAllocateInfo alloc_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	alloc_info.allocationSize = requirements.size;
	alloc_info.memoryTypeIndex = buf->mem_index;


	VkDeviceMemory mem = {};
	vk_assert(vkAllocateMemory(context->device.logical_device, &alloc_info, context->vk_allocator, &mem));

	vk_assert(vkBindBufferMemory(context->device.logical_device, new_buf, mem, 0));

	vk_copy_buffer(context, pool, 0, queue, buf->handle, 0, new_buf, 0, buf->total_size);

	vkDeviceWaitIdle((context->device.logical_device);

	if (buf->memory) {
		vkFreeMemory(context->device.logical_device, buf->memory, context->vk_allocator);
		buf->memory = 0;
	}
	if (buf->handle) {
		vkDestroyBuffer(context->device.logical_device, buf->handle, context->vk_allocator);
		buf->handle = 0;
	}
	buf->total_size = new_size;
	buf->memory = mem;
	buf->handle = new_buf;

	return true;
}

void vk_bind_buffer(vk_context* context, vk_buffer* buf, size_t offest)
{
	vk_assert(vkBindBufferMemory(context->device.logical_device, buf->handle, buf->memory, offest));
}
