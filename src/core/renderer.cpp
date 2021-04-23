#include "../../inc/core/renderer.h"

#include <set>
#include <fstream>
#include <iostream>

void renderer_c::init_commands
	(  )
{
	msg_s msg;
	msg.type = RENDERER_C;

	msg.msg = REND_UP;
	//msg.func = [ & ](  ){ vertices[ 0 ].pos.x += 0.05f; };
	//engineCommands.push_back( msg );
}

void renderer_c::init_vulkan
	(  )
{
	init_window(  );
	init_instance(  );
	init_validation_layers(  );
	init_surface(  );
	pick_physical_device(  );
	init_logical_device(  );
	init_swap_chain(  );
	init_image_views(  );
	init_render_pass(  );
	init_graphics_pipeline(  );
	init_frame_buffer(  );
	init_command_pool(  );
	init_vertex_buffer(  );
	init_index_buffer(  );
	init_command_buffers(  );
	init_sync(  );
}

void renderer_c::init_window

	(  )
{
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) != 0 )
	{
		throw std::runtime_error( "Unable to initialize SDL2!" );
	}
	win = SDL_CreateWindow(
		" - Chocolate Engine ( build: too early for version lol )",	//	Window Title
		SDL_WINDOWPOS_CENTERED,						//	X Pos
		SDL_WINDOWPOS_CENTERED,
		width,
		height,
		SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
                );
}

std::vector< const char* > renderer_c::init_required_extensions
	(  )
{
	unsigned int extensionCount = 0;
	if ( !SDL_Vulkan_GetInstanceExtensions( win, &extensionCount, NULL ) )
	{
		throw std::runtime_error( "Unable to query the number of Vulkan instance extensions\n" );
	}
	
	// Use the amount of extensions queried before to retrieve the names of the extensions
	std::vector< const char* > extensions( extensionCount );
	if (!SDL_Vulkan_GetInstanceExtensions( win, &extensionCount, extensions.data(  ) ) )
	{
		throw std::runtime_error( "Unable to query the number of Vulkan instance extension names\n" );
	}
	// Display names
	printf( "Found %d Vulkan extensions:\n", extensionCount );
	for ( int i = 0; i < extensionCount; ++i )
	{
	        printf( "%i : %s\n", i, extensions[ i ] );
	}

	// Add debug display extension, we need this to relay debug messages
	extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
	return extensions;
}

void renderer_c::init_instance
	(  )
{
	if ( enableValidationLayers && !check_validation_layer_support(  ) )
	{
		throw std::runtime_error( "Validation layers requested, but not available!" );
	}
	VkApplicationInfo appInfo{  };
	appInfo.sType 			= VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName 	= "Test";
	appInfo.applicationVersion 	= VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.pEngineName 		= "Chocolate";
	appInfo.engineVersion 		= VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.apiVersion 		= VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{  };
	createInfo.sType 		= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo 	= &appInfo;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if ( enableValidationLayers )
	{
		createInfo.enabledLayerCount 	= ( unsigned int )validationLayers.size(  );
		createInfo.ppEnabledLayerNames 	= validationLayers.data(  );
		init_debug_messenger_info( debugCreateInfo );
		createInfo.pNext 		= ( VkDebugUtilsMessengerCreateInfoEXT* )&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount 	= 0;
		createInfo.pNext	        = NULL;
	}

	auto SDLExtensions = init_required_extensions(  );
	
	createInfo.enabledExtensionCount 	= ( uint32_t )SDLExtensions.size(  );
	createInfo.ppEnabledExtensionNames 	= SDLExtensions.data(  );

	if ( vkCreateInstance( &createInfo, NULL, &inst ) )
	{
		throw std::runtime_error( "Failed to create Vulkan instance!" );
	}

	unsigned int extensionCount = 0;
	vkEnumerateInstanceExtensionProperties( NULL, &extensionCount, NULL );
	
	std::vector< VkExtensionProperties > extensions( extensionCount );
	vkEnumerateInstanceExtensionProperties( NULL, &extensionCount, extensions.data(  ) );

	printf( "%d Vulkan extensions available:\n", extensionCount );

	for ( const auto& extension : extensions )
	{
		printf( "\t%s\n", extension.extensionName );
	}
}

VKAPI_ATTR VkBool32 VKAPI_CALL renderer_c::debug_callback
	( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	  VkDebugUtilsMessageTypeFlagsEXT messageType,
	  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	  void* pUserData )
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
}

void renderer_c::init_debug_messenger_info
	( VkDebugUtilsMessengerCreateInfoEXT& c )
{
	c = {  };
	c.sType 	   = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        c.messageSeverity  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        c.messageType 	   = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        c.pfnUserCallback  = debug_callback;
}

VkResult renderer_c::init_debug_messenger
	( VkInstance instance,
	  const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	  const VkAllocationCallbacks* pAllocator,
	  VkDebugUtilsMessengerEXT* pDebugMessenger )
{
	auto func = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( inst, "vkCreateDebugUtilsMessengerEXT" );
	if ( func != NULL )
	{
		return func( inst, pCreateInfo, pAllocator, pDebugMessenger );
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void renderer_c::init_validation_layers
	(  )
{
	if ( !enableValidationLayers ) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{  };
	init_debug_messenger_info( createInfo );

        if ( init_debug_messenger( inst, &createInfo, NULL, &debugMessenger ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to set up debug messenger!" );
        }
}

void renderer_c::init_surface
	(  )
{
	if ( !SDL_Vulkan_CreateSurface( win, inst, &surf ) )
	{
		throw std::runtime_error( "Failed to create Vulkan Surface!" );
	}
}

bool renderer_c::check_validation_layer_support
	(  )
{
	bool layerFound;
	unsigned int layerCount;
	vkEnumerateInstanceLayerProperties( &layerCount, NULL );

	std::vector< VkLayerProperties > availableLayers( layerCount );
	vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data(  ) );
	
	for ( const char* layerName : validationLayers )
	{
		layerFound = false;

		for ( const auto& layerProperties : availableLayers )
		{
			if ( strcmp( layerName, layerProperties.layerName ) == 0 )
			{
				layerFound = true;
				break;
			}
		}

		if ( !layerFound )
		{
			return false;
		}
	}
	return true;
}

void renderer_c::pick_physical_device
	(  )
{
	physicalDevice 		= VK_NULL_HANDLE;
	uint32_t deviceCount 	= 0;
	
	vkEnumeratePhysicalDevices( inst, &deviceCount, NULL );
	if ( deviceCount == 0 ) {
		throw std::runtime_error( "Failed to find GPUs with Vulkan support!" );
	}
	std::vector< VkPhysicalDevice > devices( deviceCount );
	vkEnumeratePhysicalDevices( inst, &deviceCount, devices.data(  ) );

	for ( const auto& device : devices ) {
		if ( is_suitable_device( device ) ) {
			physicalDevice = device;
			break;
		}
	}

	if ( physicalDevice == VK_NULL_HANDLE ) {
		throw std::runtime_error( "Failed to find a suitable GPU!" );
	}	
}

bool renderer_c::is_suitable_device
	( VkPhysicalDevice d )
{
	queue_family_indices_t indices 	= find_queue_families( d );
	bool extensionsSupported 	= check_device_extension_support( d );
	bool swapChainAdequate 		= false;
	if ( extensionsSupported )
	{
		swap_chain_support_info_t swapChainSupport = check_swap_chain_support( d );
		swapChainAdequate = !swapChainSupport.f.empty(  ) && !swapChainSupport.p.empty(  );
	}
	return indices.complete(  ) && extensionsSupported && swapChainAdequate;
}

bool renderer_c::check_device_extension_support
	( VkPhysicalDevice d )
{
	unsigned int extensionCount;
	vkEnumerateDeviceExtensionProperties( d, NULL, &extensionCount, NULL );

	std::vector< VkExtensionProperties > availableExtensions( extensionCount );
	vkEnumerateDeviceExtensionProperties( d, NULL, &extensionCount, availableExtensions.data(  ) );

	std::set< std::string > requiredExtensions(  deviceExtensions.begin(  ), deviceExtensions.end(  ) );

	for ( const auto& extension : availableExtensions )
	{
		requiredExtensions.erase( extension.extensionName  );
	}
	return requiredExtensions.empty(  );
}

queue_family_indices_t renderer_c::find_queue_families
	( VkPhysicalDevice d )
{
	queue_family_indices_t indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( d, &queueFamilyCount, nullptr );

	std::vector< VkQueueFamilyProperties > queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( d, &queueFamilyCount, queueFamilies.data(  ) );// Logic to find queue family indices to populate struct with
	int i = 0;
	for ( const auto& queueFamily : queueFamilies ) {
		if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR( d, i, surf, &presentSupport );
			if ( presentSupport ) {
				indices.presentFamily = i;
			}
			indices.graphicsFamily = i;
		}
		if ( indices.complete(  ) ) {
			break;
		}
		i++;
	}
	return indices;
}

swap_chain_support_info_t renderer_c::check_swap_chain_support
	( VkPhysicalDevice d )
{
	swap_chain_support_info_t details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( d, surf, &details.c );

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR( d, surf, &formatCount, NULL );

	if ( formatCount != 0 )
	{
		details.f.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( d, surf, &formatCount, details.f.data(  ) );
	}
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR( d, surf, &presentModeCount, NULL );

	if ( presentModeCount != 0 )
	{
		details.p.resize( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR( d, surf, &presentModeCount, details.p.data(  ) );
	}
	return details;
}

void renderer_c::init_logical_device
	(  )
{
	queue_family_indices_t indices = find_queue_families( physicalDevice );

	std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
	std::set< uint32_t > uniqueQueueFamilies = { indices.graphicsFamily.value(  ), indices.presentFamily.value(  ) };

	float queuePriority = 1.0f;
	for ( uint32_t queueFamily : uniqueQueueFamilies )
	{
		VkDeviceQueueCreateInfo queueCreateInfo{  };
		
		queueCreateInfo.sType			= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex 	= queueFamily;
		queueCreateInfo.queueCount		= 1;
		queueCreateInfo.pQueuePriorities 	= &queuePriority;
		
		queueCreateInfos.push_back( queueCreateInfo );
	}

	VkPhysicalDeviceFeatures deviceFeatures{  };

	VkDeviceCreateInfo createInfo{  };
	createInfo.sType 			= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos 		= queueCreateInfos.data(  );
	createInfo.queueCreateInfoCount 	= ( uint32_t )queueCreateInfos.size(  );

	createInfo.pEnabledFeatures 		= &deviceFeatures;

	createInfo.enabledExtensionCount 	= ( uint32_t )( deviceExtensions.size(  ) );
	createInfo.ppEnabledExtensionNames 	= deviceExtensions.data(  );

	if ( enableValidationLayers )
	{
		createInfo.enabledLayerCount 	= ( uint32_t )( validationLayers.size(  ) );
		createInfo.ppEnabledLayerNames 	= validationLayers.data(  );
	}
	else
	{
		createInfo.enabledLayerCount 	= 0;
	}
	if ( vkCreateDevice( physicalDevice, &createInfo, NULL, &device ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create logical device!" );
	}
	vkGetDeviceQueue( device, indices.graphicsFamily.value(  ), 0, &graphicsQueue );
	vkGetDeviceQueue( device, indices.presentFamily.value(  ), 0, &presentQueue );
}

VkSurfaceFormatKHR renderer_c::choose_swap_surface_format
	( const std::vector< VkSurfaceFormatKHR >& availableFormats )
{
	for ( const auto& availableFormat : availableFormats )
	{
		if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
		{
			return availableFormat;
		}
	}
	return availableFormats[ 0 ];
}

VkPresentModeKHR renderer_c::choose_swap_present_mode
	( const std::vector< VkPresentModeKHR >& availablePresentModes )
{
	for ( const auto& availablePresentMode : availablePresentModes )
	{
		if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
		{
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D renderer_c::choose_swap_extent
	( const VkSurfaceCapabilitiesKHR& capabilities )
{
	if ( capabilities.currentExtent.width != UINT32_MAX )
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D size = {
			( unsigned int )width,
			( unsigned int )height
		};

	       size.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, size.width));
	       size.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, size.height));

		return size;
	}
}

void renderer_c::init_swap_chain
	(  )
{
	swap_chain_support_info_t swapChainSupport 	= check_swap_chain_support( physicalDevice );

	VkSurfaceFormatKHR surfaceFormat 		= choose_swap_surface_format( swapChainSupport.f );
	VkPresentModeKHR presentMode 			= choose_swap_present_mode( swapChainSupport.p );
	VkExtent2D extent 				= choose_swap_extent( swapChainSupport.c );

	uint32_t imageCount 				= swapChainSupport.c.minImageCount + 1;
	if ( swapChainSupport.c.maxImageCount > 0 && imageCount > swapChainSupport.c.maxImageCount )
	{
		imageCount = swapChainSupport.c.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{  };
	createInfo.sType 		= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface 		= surf;

	createInfo.minImageCount 	= imageCount;
	createInfo.imageFormat 		= surfaceFormat.format;
	createInfo.imageColorSpace 	= surfaceFormat.colorSpace;
	createInfo.imageExtent 		= extent;
	createInfo.imageArrayLayers	= 1;
	createInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        queue_family_indices_t indices 	= find_queue_families( physicalDevice );
	uint32_t queueFamilyIndices[  ] = { indices.graphicsFamily.value(  ), indices.presentFamily.value(  ) };

	if ( indices.graphicsFamily != indices.presentFamily )
	{
		createInfo.imageSharingMode 		= VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount 	= 2;
		createInfo.pQueueFamilyIndices 		= queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode 		= VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount 	= 0; // Optional
		createInfo.pQueueFamilyIndices 		= NULL; // Optional
	}

	createInfo.preTransform 	= swapChainSupport.c.currentTransform;
	createInfo.compositeAlpha 	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode 		= presentMode;
	createInfo.clipped		= VK_TRUE;
	createInfo.oldSwapchain 	= VK_NULL_HANDLE;

	if ( vkCreateSwapchainKHR(device, &createInfo, NULL, &swapChain) != VK_SUCCESS ) {
		throw std::runtime_error( "Failed to create swap chain!" );
	}
	vkGetSwapchainImagesKHR( device, swapChain, &imageCount, NULL );
	swapChainImages.resize( imageCount );
	vkGetSwapchainImagesKHR( device, swapChain, &imageCount, swapChainImages.data(  ) );

	swapChainImageFormat 	= surfaceFormat.format;
	swapChainExtent 	= extent;
}

void renderer_c::init_image_views
	(  )
{
	swapChainImageViews.resize( swapChainImages.size(  ) );
	for ( int i = 0; i < swapChainImages.size(  ); i++ )
	{
		VkImageViewCreateInfo createInfo{  };
		createInfo.sType 				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image 				= swapChainImages[ i ];
		createInfo.viewType 				= VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format 				= swapChainImageFormat;
		createInfo.components.r 			= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g 			= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b 			= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a 			= VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask 		= VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel 	= 0;
		createInfo.subresourceRange.levelCount 		= 1;
		createInfo.subresourceRange.baseArrayLayer	= 0;
		createInfo.subresourceRange.layerCount 		= 1;
		if ( vkCreateImageView( device, &createInfo, NULL, &swapChainImageViews[ i ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create image views!" );
		}
	}
}

void renderer_c::init_render_pass
	(  )
{
	VkAttachmentDescription colorAttachment{  };
	colorAttachment.format 		= swapChainImageFormat;
	colorAttachment.samples 	= VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp 		= VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp 	= VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp 	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout 	= VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout 	= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{  };
	colorAttachmentRef.attachment 	= 0;
	colorAttachmentRef.layout 	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{  };
	subpass.pipelineBindPoint 	= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount 	= 1;
	subpass.pColorAttachments 	= &colorAttachmentRef;

	VkSubpassDependency dependency{  };
	dependency.srcSubpass 		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass 		= 0;
	dependency.srcStageMask 	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask 	= 0;
	dependency.dstStageMask 	= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask 	= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{  };
	renderPassInfo.sType 		= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount 	= 1;
	renderPassInfo.pAttachments 	= &colorAttachment;
	renderPassInfo.subpassCount	= 1;
	renderPassInfo.pSubpasses 	= &subpass;
	renderPassInfo.dependencyCount 	= 1;
	renderPassInfo.pDependencies 	= &dependency;

	if ( vkCreateRenderPass( device, &renderPassInfo, nullptr, &renderPass ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create render pass!" );
	}
}

std::vector< char > renderer_c::read_file
	( const std::string& filePath )
{
	std::ifstream file( filePath, std::ios::ate | std::ios::binary );
	if ( !file.is_open(  ) ) {
		throw std::runtime_error( "Failed to open file!" );
	}
	int fileSize = ( int ) file.tellg(  );
	std::vector< char > buffer( fileSize );
	file.seekg( 0 );
	file.read( buffer.data(  ), fileSize );
	file.close(  );

	return buffer;
}

VkShaderModule renderer_c::create_shader_module
	( const std::vector< char >& code )
{
	VkShaderModuleCreateInfo createInfo{  };
	VkShaderModule shaderModule;
	createInfo.sType 	= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize 	= code.size(  );
	createInfo.pCode 	= ( const uint32_t* )code.data(  );
	if ( vkCreateShaderModule( device, &createInfo, NULL, &shaderModule ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create shader module!" );
	}
	return shaderModule;
}

void renderer_c::init_graphics_pipeline
	(  )
{
	auto vertShaderCode = read_file( "shaders/vert.spv" );
	auto fragShaderCode = read_file( "shaders/frag.spv" );
		
	VkShaderModule vertShaderModule = create_shader_module( vertShaderCode );
	VkShaderModule fragShaderModule = create_shader_module( fragShaderCode );

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{  };
	vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName  = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{  };
	fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName  = "main";

	VkPipelineShaderStageCreateInfo shaderStages[  ] = { vertShaderStageInfo, fragShaderStageInfo };
	auto bindingDescription 	= vertex_t::get_binding_desc(  );
	auto attributeDescriptions 	= vertex_t::get_attribute_desc(  );

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{  };
	vertexInputInfo.sType 				= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount 	= 1;
	vertexInputInfo.pVertexBindingDescriptions 	= &bindingDescription; // Optional
	vertexInputInfo.vertexAttributeDescriptionCount = ( uint32_t )( attributeDescriptions.size(  ) );
	vertexInputInfo.pVertexAttributeDescriptions 	= attributeDescriptions.data(  ); // Optional

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{  };
	inputAssembly.sType 			= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology 			= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable 	= VK_FALSE;

	VkViewport viewport{  };
	viewport.x		= 0.0f;
	viewport.y 		= 0.0f;
	viewport.width 		= ( float )swapChainExtent.width;
	viewport.height 	= ( float )swapChainExtent.height;
	viewport.minDepth 	= 0.0f;
	viewport.maxDepth 	= 1.0f;

	VkRect2D scissor{  };
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{  };
	viewportState.sType 		= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount 	= 1;
	viewportState.pViewports 	= &viewport;
	viewportState.scissorCount 	= 1;
	viewportState.pScissors 	= &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer{  };
	rasterizer.sType 			= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable 		= VK_FALSE;
	rasterizer.rasterizerDiscardEnable 	= VK_FALSE;
	rasterizer.polygonMode 			= VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth 			= 1.0f;
	rasterizer.cullMode 			= VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace 			= VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable 		= VK_FALSE;
	rasterizer.depthBiasConstantFactor 	= 0.0f; // Optional
	rasterizer.depthBiasClamp 		= 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor 	= 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling{  };
	multisampling.sType 		    = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable   = VK_FALSE;
	multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading 	    = 1.0f; // Optional
	multisampling.pSampleMask 	    = NULL; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable 	    = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment{  };
	colorBlendAttachment.colorWriteMask 	 = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable 	 = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp	 = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp 	 = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{  };
	colorBlending.sType 		  = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable 	  = VK_FALSE;
	colorBlending.logicOp 	          = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount 	  = 1;
	colorBlending.pAttachments 	  = &colorBlendAttachment;
	colorBlending.blendConstants[ 0 ] = 0.0f; // Optional
	colorBlending.blendConstants[ 1 ] = 0.0f; // Optional
	colorBlending.blendConstants[ 2 ] = 0.0f; // Optional
	colorBlending.blendConstants[ 3 ] = 0.0f; // Optional

	VkDynamicState dynamicStates[  ] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{  };
	dynamicState.sType 		= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount  = 2;
	dynamicState.pDynamicStates 	= dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{  };
	pipelineLayoutInfo.sType 			= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount 		= 0; // Optional
	pipelineLayoutInfo.pSetLayouts 			= NULL; // Optional
	pipelineLayoutInfo.pushConstantRangeCount 	= 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges 		= NULL; // Optional

	if ( vkCreatePipelineLayout( device, &pipelineLayoutInfo, NULL, &pipelineLayout ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create pipeline layout!" );
	}
	VkGraphicsPipelineCreateInfo pipelineInfo{  };
	pipelineInfo.sType 			= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount 		= 2;
	pipelineInfo.pStages 			= shaderStages;
	pipelineInfo.pVertexInputState 		= &vertexInputInfo;
	pipelineInfo.pInputAssemblyState 	= &inputAssembly;
	pipelineInfo.pViewportState 		= &viewportState;
	pipelineInfo.pRasterizationState 	= &rasterizer;
	pipelineInfo.pMultisampleState 		= &multisampling;
	pipelineInfo.pDepthStencilState 	= NULL; // Optional
	pipelineInfo.pColorBlendState 		= &colorBlending;
	pipelineInfo.pDynamicState 		= NULL; // Optional
	pipelineInfo.layout 			= pipelineLayout;
	pipelineInfo.renderPass 		= renderPass;
	pipelineInfo.subpass 			= 0;
	pipelineInfo.basePipelineHandle 	= VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex 		= -1; // Optional

	if ( vkCreateGraphicsPipelines( device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create graphics pipeline!" );
	}

	vkDestroyShaderModule( device, fragShaderModule, NULL );
	vkDestroyShaderModule( device, vertShaderModule, NULL );
}

void renderer_c::init_frame_buffer
	(  )
{
	swapChainFramebuffers.resize( swapChainImageViews.size(  ) );
	for ( int i = 0; i < swapChainImageViews.size(  ); i++ )
	{
		VkImageView attachments[  ] = {
			swapChainImageViews[ i ]
		};

		VkFramebufferCreateInfo framebufferInfo{  };
		framebufferInfo.sType 		= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass 	= renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments 	= attachments;
		framebufferInfo.width 		= swapChainExtent.width;
		framebufferInfo.height 		= swapChainExtent.height;
		framebufferInfo.layers 		= 1;

		if ( vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapChainFramebuffers[ i ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create framebuffer!" );
		}
	}
}

void renderer_c::init_command_pool
	(  )
{
        queue_family_indices_t indices = find_queue_families( physicalDevice );

	VkCommandPoolCreateInfo poolInfo{  };
	poolInfo.sType 			= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex 	= indices.graphicsFamily.value();
	poolInfo.flags 			= 0; // Optional

	if ( vkCreateCommandPool( device, &poolInfo, NULL, &commandPool ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create command pool!" );
	}
}

void renderer_c::init_command_buffers
	(  )
{
	commandBuffers.resize( swapChainFramebuffers.size(  ) );
	VkCommandBufferAllocateInfo allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool 		= commandPool;
	allocInfo.level 		= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount	= ( uint32_t )commandBuffers.size(  );

	if ( vkAllocateCommandBuffers( device, &allocInfo, commandBuffers.data(  ) ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to allocate command buffers!" );
	}
	for ( int i = 0; i < commandBuffers.size(  ); i++ )
	{
		VkCommandBufferBeginInfo beginInfo{  };
		beginInfo.sType 		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags 		= 0; // Optional
		beginInfo.pInheritanceInfo 	= NULL; // Optional

		if ( vkBeginCommandBuffer( commandBuffers[ i ], &beginInfo ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to begin recording command buffer!" );
		}
		VkRenderPassBeginInfo renderPassInfo{  };
		renderPassInfo.sType 		 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass 	 = renderPass;
		renderPassInfo.framebuffer 	 = swapChainFramebuffers[ i ];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor 	 = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount 	 = 1;
		renderPassInfo.pClearValues 	 = &clearColor;

		vkCmdBeginRenderPass( commandBuffers[ i ], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
		vkCmdBindPipeline( commandBuffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline );
		VkBuffer vertexBuffers[  ] = { vertexBuffer };
		VkDeviceSize offsets[  ] = { 0 };
		vkCmdBindVertexBuffers( commandBuffers[ i ], 0, 1, vertexBuffers, offsets );
		vkCmdBindIndexBuffer( commandBuffers[ i ], indexBuffer, 0, VK_INDEX_TYPE_UINT16 );

	        vkCmdDrawIndexed( commandBuffers[ i ], ( uint32_t )( indicesVector.size(  ) ), 1, 0, 0, 0 );
		vkCmdEndRenderPass( commandBuffers[ i ] );

		if ( vkEndCommandBuffer( commandBuffers[ i ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to record command buffer!" );
		}
	}
}

void renderer_c::init_sync
	(  )
{
	imageAvailableSemaphores.resize( MAX_FRAMES_PROCESSING );
	renderFinishedSemaphores.resize( MAX_FRAMES_PROCESSING );
	inFlightFences.resize( MAX_FRAMES_PROCESSING );
	imagesInFlight.resize( swapChainImages.size(  ), VK_NULL_HANDLE );
	
	VkSemaphoreCreateInfo semaphoreInfo{  };
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{  };
	fenceInfo.sType	    = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags	    = VK_FENCE_CREATE_SIGNALED_BIT;

	for ( int i = 0 ; i < MAX_FRAMES_PROCESSING; ++i )
	{
		if ( vkCreateSemaphore( device, &semaphoreInfo, NULL, &imageAvailableSemaphores[ i ] ) != VK_SUCCESS ||
		     vkCreateSemaphore( device, &semaphoreInfo, NULL, &renderFinishedSemaphores[ i ] ) != VK_SUCCESS ||
		     vkCreateFence( device, &fenceInfo, NULL, &inFlightFences[ i ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to create sync objects!" );
		}
	}
}

uint32_t renderer_c::find_memory_type
	( uint32_t typeFilter, VkMemoryPropertyFlags properties )
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties( physicalDevice, &memProperties );

	for ( uint32_t i = 0; i < memProperties.memoryTypeCount; ++i )
	{
		if ( ( typeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[ i ].propertyFlags & properties ) == properties )
		{
			return i;
		}
	}

	throw std::runtime_error( "Failed to find suitable memory type!" );
}

void renderer_c::init_vertex_buffer
	(  )
{
	VkDeviceSize bufferSize = sizeof( vertices[ 0 ] ) * vertices.size(  );
	
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	init_buffer( bufferSize,
		     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		     stagingBuffer,
		     stagingBufferMemory );
	
      	void* data;
	vkMapMemory( device, stagingBufferMemory, 0, bufferSize, 0, &data );
	memcpy( data, vertices.data(  ), ( size_t )bufferSize );
	vkUnmapMemory( device, stagingBufferMemory );

	init_buffer( bufferSize,
		     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     vertexBuffer,
		     vertexBufferMemory );

	buf_copy( stagingBuffer, vertexBuffer, bufferSize );

	vkDestroyBuffer( device, stagingBuffer, NULL );
        vkFreeMemory( device, stagingBufferMemory, NULL );
}

void renderer_c::init_index_buffer
	(  )
{
	VkDeviceSize bufferSize = sizeof( indicesVector[ 0 ] ) * indicesVector.size(  );

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	init_buffer( bufferSize,
		     VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		     stagingBuffer, stagingBufferMemory );

	void* data;
	vkMapMemory( device, stagingBufferMemory, 0, bufferSize, 0, &data );
	memcpy( data, indicesVector.data(  ), ( size_t )bufferSize );
	vkUnmapMemory( device, stagingBufferMemory );

	init_buffer( bufferSize,
		     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		     indexBuffer,
		     indexBufferMemory );

	buf_copy( stagingBuffer, indexBuffer, bufferSize );

	vkDestroyBuffer( device, stagingBuffer, NULL );
	vkFreeMemory( device, stagingBufferMemory, NULL );
}

void renderer_c::reinit_swap_chain
	(  )
{
	vkDeviceWaitIdle( device );

	destroy_swap_chain(  );
	
	init_swap_chain(  );
	init_image_views(  );
	init_render_pass(  );
	init_graphics_pipeline(  );
	init_frame_buffer(  );
	init_command_buffers(  );
}

void renderer_c::destroy_swap_chain
	(  )
{
        for ( auto framebuffer : swapChainFramebuffers )
	{
		vkDestroyFramebuffer( device, framebuffer, NULL );
        }
	vkFreeCommandBuffers( device, commandPool, ( uint32_t )commandBuffers.size(  ), commandBuffers.data(  ) );

	vkDestroyPipeline( device, graphicsPipeline, NULL );
        vkDestroyPipelineLayout( device, pipelineLayout, NULL );
        vkDestroyRenderPass( device, renderPass, NULL );

        for ( auto imageView : swapChainImageViews )
	{
		vkDestroyImageView( device, imageView, NULL );
        }

        vkDestroySwapchainKHR( device, swapChain, NULL );
}

void renderer_c::init_buffer
		( VkDeviceSize size,
		  VkBufferUsageFlags usage,
		  VkMemoryPropertyFlags properties,
		  VkBuffer& buffer,
		  VkDeviceMemory& bufferMemory )
{
	VkBufferCreateInfo bufferInfo{  };
	bufferInfo.sType 	= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size 	= size;
	bufferInfo.usage 	= usage;
	bufferInfo.sharingMode  = VK_SHARING_MODE_EXCLUSIVE;

	if ( vkCreateBuffer( device, &bufferInfo, NULL, &buffer ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create buffer!" );
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements( device, buffer, &memRequirements );

	VkMemoryAllocateInfo allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize 	= memRequirements.size;
	allocInfo.memoryTypeIndex 	= find_memory_type( memRequirements.memoryTypeBits, properties );

	if ( vkAllocateMemory( device, &allocInfo, nullptr, &bufferMemory ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to allocate buffer memory!" );
	}

	vkBindBufferMemory( device, buffer, bufferMemory, 0 );
}

void renderer_c::buf_copy
	( VkBuffer src, VkBuffer dst, VkDeviceSize size )
{
	VkCommandBufferAllocateInfo allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level 		= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool 		= commandPool;
	allocInfo.commandBufferCount 	= 1;
	
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers( device, &allocInfo, &commandBuffer );

	VkCommandBufferBeginInfo beginInfo{  };
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer( commandBuffer, &beginInfo );

	VkBufferCopy copyRegion{  };
	copyRegion.srcOffset 	= 0; // Optional
	copyRegion.dstOffset 	= 0; // Optional
	copyRegion.size 	= size;
	vkCmdCopyBuffer( commandBuffer, src, dst, 1, &copyRegion );
	vkEndCommandBuffer( commandBuffer );

	VkSubmitInfo submitInfo{  };
	submitInfo.sType 		= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount 	= 1;
	submitInfo.pCommandBuffers 	= &commandBuffer;

	vkQueueSubmit( graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
	vkQueueWaitIdle( graphicsQueue );

	vkFreeCommandBuffers( device, commandPool, 1, &commandBuffer );
}

void renderer_c::draw_frame
	(  )
{
	vkWaitForFences( device, 1, &inFlightFences[ currentFrame ], VK_TRUE, UINT64_MAX );
	
	uint32_t imageIndex;
	VkResult res =  vkAcquireNextImageKHR( device, swapChain, UINT64_MAX, imageAvailableSemaphores[ currentFrame ], VK_NULL_HANDLE, &imageIndex );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
	{
		reinit_swap_chain(  );
		return;
	}
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		throw std::runtime_error( "Failed ot acquire swap chain image!" );
	}
	if ( imagesInFlight[ imageIndex ] != VK_NULL_HANDLE )
	{
		vkWaitForFences( device, 1, &imagesInFlight[ imageIndex ], VK_TRUE, UINT64_MAX );
	}
	imagesInFlight[ imageIndex ] = inFlightFences[ currentFrame ];

	VkSubmitInfo submitInfo{  };
	submitInfo.sType 			= VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[  ] 		= { imageAvailableSemaphores[ currentFrame ] };
	VkPipelineStageFlags waitStages[  ] 	= { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount 		= 1;
	submitInfo.pWaitSemaphores 		= waitSemaphores;
	submitInfo.pWaitDstStageMask 		= waitStages;
	submitInfo.commandBufferCount 		= 1;
	submitInfo.pCommandBuffers 		= &commandBuffers[imageIndex];
	VkSemaphore signalSemaphores[  ] 	= { renderFinishedSemaphores[ currentFrame ] };
	submitInfo.signalSemaphoreCount 	= 1;
	submitInfo.pSignalSemaphores 		= signalSemaphores;

	vkResetFences( device, 1, &inFlightFences[ currentFrame ] );
	if ( vkQueueSubmit( graphicsQueue, 1, &submitInfo, inFlightFences[ currentFrame ] ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to submit draw command buffer!" );
	}
	VkPresentInfoKHR presentInfo{  };
	presentInfo.sType 			= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount 		= 1;
	presentInfo.pWaitSemaphores 		= signalSemaphores;

	VkSwapchainKHR swapChains[  ] 		= { swapChain };
	presentInfo.swapchainCount 		= 1;
	presentInfo.pSwapchains 		= swapChains;
	presentInfo.pImageIndices 		= &imageIndex;
	presentInfo.pResults 			= NULL; // Optional

	res = vkQueuePresentKHR( presentQueue, &presentInfo );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
	{
		reinit_swap_chain(  );
	}
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		throw std::runtime_error( "Failed ot present swap chain image!" );
	}
	
	vkQueueWaitIdle( presentQueue );

	currentFrame = ( currentFrame + 1 ) % MAX_FRAMES_PROCESSING;
	
	SDL_Delay( 1000 / 144 );
}

void renderer_c::destroy_debug_messenger
	( VkInstance instance,
	  VkDebugUtilsMessengerEXT debugMessenger,
	  const VkAllocationCallbacks* pAllocator )
{
	auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( inst, "vkDestroyDebugUtilsMessengerEXT" );
	if ( func != NULL )
	{
		func( instance, debugMessenger, pAllocator );
	}
}

void renderer_c::cleanup
	(  )
{
	destroy_swap_chain(  );
	vkDestroyBuffer( device, indexBuffer, NULL );
        vkFreeMemory( device, indexBufferMemory, NULL );
	vkDestroyBuffer( device, vertexBuffer, NULL );
	vkFreeMemory( device, vertexBufferMemory, NULL );
	for ( int i = 0; i < MAX_FRAMES_PROCESSING; i++ )
	{
		vkDestroySemaphore( device, renderFinishedSemaphores[ i ], NULL );
		vkDestroySemaphore( device, imageAvailableSemaphores[ i ], NULL );
		vkDestroyFence( device, inFlightFences[ i ], NULL);
        }

        vkDestroyCommandPool( device, commandPool, NULL );

        vkDestroyDevice( device, NULL );

        if ( enableValidationLayers ) {
		destroy_debug_messenger( inst, debugMessenger, NULL );
        }

        vkDestroySurfaceKHR( inst, surf, NULL );
        vkDestroyInstance( inst, NULL );
	
	SDL_DestroyWindow( win );
	SDL_Quit(  );
}

renderer_c::renderer_c
	(  )
{
	init_vulkan(  );

	systemType = RENDERER_C;
	init_commands(  );
}

renderer_c::~renderer_c
	(  )
{
	cleanup(  );
}
