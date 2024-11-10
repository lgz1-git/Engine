#include "h_vulkan_API.h"
#include "h_clogger.h"

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

		LINFOP("images_counts = " , swapchain_info->image_counts);
		if (swapchain_info->images != nullptr)
		{
			free(swapchain_info->images);
		}
		swapchain_info->images = (VkImage*)malloc(swapchain_info->image_counts * sizeof(VkImage));
		vkGetSwapchainImagesKHR(context->device.logical_device, swapchain_info->swapchain_handle, &swapchain_info->image_counts, swapchain_info->images);
	}

	//views
	if (swapchain_info->views != nullptr)
	{
		free(swapchain_info->views);
	}
	swapchain_info->views = (VkImageView*)malloc(swapchain_info->image_counts * sizeof(VkImageView));
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
	context->device_extensions.emplace_back(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	context->device_layers_extensions.reserve(3);

	LINFO("extension is loaded!");
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
	LINFO("vulkan instance is created!");
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
			
			LINFOP("physical_device : ", i);
			LINFOP("Graphics Family index : ",context->device.queue_family_info.graphics_family_index);
			LINFOP("present  Family index : ",context->device.queue_family_info.present_family_index );
			LINFOP("transfer Family index : ",context->device.queue_family_info.transfer_family_index);
			LINFOP("compute  Family index : ",context->device.queue_family_info.compute_family_index );

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
		LINFOP("queue family counts is :  ", queuefamily_counts);
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

		LINFO("device queuefamily is OK , data is preserve");

	}

	if (queue_requirements->sampler_anisotropy && features->samplerAnisotropy)
	{
		
		LINFO("device support anisotropy.");
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
		device_swapchain_potency->surface_formats = (VkSurfaceFormatKHR*)malloc(device_swapchain_potency->format_counts * sizeof(VkSurfaceFormatKHR));
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
	
	LINFO("phisical device has the swapchian potency!");
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
	VkImageUsageFlagBits usage,
	VkMemoryPropertyFlags memory_flags,
	bool create_view,
	VkImageAspectFlags view_aspect_flags,
	vk_depth_image* depth_image)
{
	depth_image->w = w;
	depth_image->h = h;

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
	VkPhysicalDeviceMemoryProperties memory_properties;
	i32 memory_typeindex = 0;
	vkGetPhysicalDeviceMemoryProperties(context->device.physical_device, &memory_properties);
	//info:why is this?
	for (int i = 0; i < memory_properties.memoryTypeCount; i++) {
		if (memory_requirements.memoryTypeBits & (1 << i) && (memory_properties.memoryTypes[i].propertyFlags & memory_flags) == memory_flags) {
			memory_typeindex = i;
			break;
		}
		memory_typeindex = -1;
	}
	// Allocate memory
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



void vk_create_surface(vk_context* context, win32_platform_context* win_context)
{
	VkWin32SurfaceCreateInfoKHR surface_createinfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surface_createinfo.hinstance = win_context->win_instance;
	surface_createinfo.hwnd = win_context->win_handle;
	vkCreateWin32SurfaceKHR(context->vk_instance, &surface_createinfo, context->vk_allocator, &context->vk_surface);

	LINFO("vulkan surface is creates!");
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
	LINFO("logical device is created!");

	vkGetDeviceQueue(context->device.logical_device, context->device.queue_family_info.graphics_family_index, 0, &context->device.graphics_queue);
	vkGetDeviceQueue(context->device.logical_device, context->device.queue_family_info.graphics_family_index, 0, &context->device.present_queue);
	vkGetDeviceQueue(context->device.logical_device, context->device.queue_family_info.present_family_index, 0, &context->device.transfer_queue);
	LINFO("logical device queue handle is binded!");

	VkCommandPoolCreateInfo pool_create = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	pool_create.queueFamilyIndex = context->device.queue_family_info.graphics_family_index;
	pool_create.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkCreateCommandPool(context->device.logical_device, &pool_create,context->vk_allocator, &context->device.g_cmdpool);
	LINFO("logical device command pool is created!");
}


void vk_create_swapchain(vk_context* context, vk_swapchain_info* swapchain_info, u32 w, u32 h)
{
	CreateSwapchain(context, swapchain_info, w, h);
	LINFO("swapchain is created!");
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
		
		LERR("fail to present iamges");
	}

	context->current_frame = (context->current_frame + 1) % swapchain_info->max_frames_in_flight;
}



void vk_create_renderpass(
	vk_context* context,
	vk_renderpass* renderpass,
	f32 x, f32 y, f32 w, f32 h,
	f32 r, f32 g, f32 b, f32 a,
	f32 depth, u32 stencil)

{
	renderpass->x = x;
	renderpass->y = y;
	renderpass->w = w;
	renderpass->h = h;

	renderpass->r = r;
	renderpass->g = g;
	renderpass->b = b;
	renderpass->a = a;


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
	VkSubpassDependency dependency;
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
	LINFO("renderpass is created!");
}



void vk_renderpass_begin(vk_cmdbuffer* cmdbuffer, vk_renderpass* renderpass, VkFramebuffer frame_buffer)
{
	VkRenderPassBeginInfo begin_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	begin_info.renderPass = renderpass->renderpass_handle;
	begin_info.framebuffer = frame_buffer;
	begin_info.renderArea.extent.width = renderpass->w;
	begin_info.renderArea.extent.height = renderpass->h;
	begin_info.renderArea.offset.x = renderpass->x;
	begin_info.renderArea.offset.y = renderpass->y;

	VkClearValue clear_value[2];
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

void vk_create_frambuffer(
	vk_context* context,
	vk_renderpass* renderpass,
	u32 w,
	u32 h,
	u32 view_counts,
	VkImageView* views,
	vk_framebuffer* framebuffer)
{
	framebuffer->views = (VkImageView*)malloc(view_counts * sizeof(VkImageView));
	for (i32 i = 0; i < view_counts; i++)
	{
		framebuffer->views[i] = views[i];
	}
	framebuffer->renderpass = renderpass;
	framebuffer->view_counts = view_counts;

	VkFramebufferCreateInfo create_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
	create_info.attachmentCount = view_counts;
	create_info.pAttachments = framebuffer->views;
	create_info.renderPass = renderpass->renderpass_handle;
	create_info.width = w;
	create_info.height = h;
	create_info.layers = 1;

	vk_assert(vkCreateFramebuffer(
		context->device.logical_device,
		&create_info, context->vk_allocator, 
		&framebuffer->framebuffer_handle));
}


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
//	todo::
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



