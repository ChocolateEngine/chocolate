#include "../../../inc/core/renderer/device.h"

#include <set>

VKAPI_ATTR VkBool32 VKAPI_CALL device_c::debug_callback
	( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	  VkDebugUtilsMessageTypeFlagsEXT messageType,
	  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	  void* pUserData )
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
}

void device_c::init_device
	(  )
{
	init_window(  );
	init_instance(  );
	init_validation_layers(  );
	init_surface(  );
	init_physical_device(  );
	init_logical_device(  );
}

void device_c::init_window
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

std::vector< const char* > device_c::init_required_extensions
	(  )
{
	unsigned int extensionCount = 0;
	if ( !SDL_Vulkan_GetInstanceExtensions( win, &extensionCount, NULL ) )
	{
		throw std::runtime_error( "Unable to query the number of Vulkan instance extensions\n" );
	}
	
	// Use the amount of extensions queried before to retrieve the names of the extensions
	std::vector< const char* > extensions( extensionCount );
	if ( !SDL_Vulkan_GetInstanceExtensions( win, &extensionCount, extensions.data(  ) ) )
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

void device_c::init_instance
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

void device_c::init_debug_messenger_info
	( VkDebugUtilsMessengerCreateInfoEXT& c )
{
	c = {  };
	c.sType 	   = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        c.messageSeverity  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        c.messageType 	   = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        c.pfnUserCallback  = debug_callback;
}

VkResult device_c::init_debug_messenger
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


void device_c::init_validation_layers
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

void device_c::init_surface
	(  )
{
	if ( !SDL_Vulkan_CreateSurface( win, inst, &surf ) )
	{
		throw std::runtime_error( "Failed to create Vulkan Surface!" );
	}
}

void device_c::init_physical_device
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

void device_c::init_logical_device
	(  )
{
	queue_family_indices_t2 indices = find_queue_families( physicalDevice );

	std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
	std::set< uint32_t > uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

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
	deviceFeatures.samplerAnisotropy = VK_TRUE;

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
	vkGetDeviceQueue( device, indices.graphicsFamily, 0, &graphicsQueue );
	vkGetDeviceQueue( device, indices.presentFamily, 0, &presentQueue );
}

void device_c::init_command_pool
	(  )
{
        queue_family_indices_t2 indices = find_queue_families( physicalDevice );

	VkCommandPoolCreateInfo poolInfo{  };
	poolInfo.sType 			= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex 	= indices.graphicsFamily;
	poolInfo.flags 			= 0; // Optional

	if ( vkCreateCommandPool( device, &poolInfo, NULL, &commandPool ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create command pool!" );
	}
}

bool device_c::check_validation_layer_support
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

queue_family_indices_t2 device_c::find_queue_families
	( VkPhysicalDevice d )
{
	queue_family_indices_t2 indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( d, &queueFamilyCount, nullptr );

	std::vector< VkQueueFamilyProperties > queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( d, &queueFamilyCount, queueFamilies.data(  ) );// Logic to find queue family indices to populate struct with
	int i = 0;
	for ( const auto& queueFamily : queueFamilies )
	{
		if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR( d, i, surf, &presentSupport );
			if ( presentSupport )
			{
				indices.presentFamily = i;
			}
			indices.graphicsFamily = i;
		}
		if ( indices.complete(  ) )
		{
			break;
		}
		i++;
	}
	return indices;
}

bool device_c::check_device_extension_support
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

swap_chain_support_info_t device_c::check_swap_chain_support
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

bool device_c::is_suitable_device
	( VkPhysicalDevice d )
{
	queue_family_indices_t2 indices 	= find_queue_families( d );
	bool extensionsSupported 		= check_device_extension_support( d );
	bool swapChainAdequate 			= false;
	if ( extensionsSupported )
	{
		swap_chain_support_info_t swapChainSupport = check_swap_chain_support( d );
		swapChainAdequate = !swapChainSupport.f.empty(  ) && !swapChainSupport.p.empty(  );
	}
	
	return indices.complete(  ) && extensionsSupported && swapChainAdequate;
}

VkSurfaceFormatKHR device_c::choose_swap_surface_format
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

VkPresentModeKHR device_c::choose_swap_present_mode
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

VkExtent2D device_c::choose_swap_extent
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

void device_c::init_swap_chain
	( VkSwapchainKHR& swapChain, std::vector<VkImage>& swapChainImages, VkFormat& swapChainImageFormat, VkExtent2D& swapChainExtent )
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

        queue_family_indices_t2 indices 	= find_queue_families( physicalDevice );
	uint32_t queueFamilyIndices[  ] = { indices.graphicsFamily, indices.presentFamily };

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

	if ( vkCreateSwapchainKHR(device, &createInfo, NULL, &swapChain ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create swap chain!" );
	}
	vkGetSwapchainImagesKHR( device, swapChain, &imageCount, NULL );
	swapChainImages.resize( imageCount );
	vkGetSwapchainImagesKHR( device, swapChain, &imageCount, swapChainImages.data(  ) );

	swapChainImageFormat 	= surfaceFormat.format;
	swapChainExtent 	= extent;
}

void device_c::init_texture_sampler
	( VkSampler& textureSampler )
{
	VkSamplerCreateInfo samplerInfo{  };
	samplerInfo.sType  	 = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter    = VK_FILTER_LINEAR;
	samplerInfo.minFilter    = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	VkPhysicalDeviceProperties properties{  };
	vkGetPhysicalDeviceProperties( physicalDevice, &properties );

	samplerInfo.anisotropyEnable 		= VK_TRUE;
	samplerInfo.maxAnisotropy 		= properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor 		= VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates 	= VK_FALSE;
	samplerInfo.compareEnable 		= VK_FALSE;
	samplerInfo.compareOp 			= VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode 			= VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias 			= 0.0f;
	samplerInfo.minLod 			= 0.0f;
	samplerInfo.maxLod 			= 0.0f;

	if ( vkCreateSampler( device, &samplerInfo, NULL, &textureSampler ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create texture sampler!" );
	}
}

VkCommandBuffer device_c::begin_single_time_commands
	(  )
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

	return commandBuffer;
}

void device_c::end_single_time_commands
	( VkCommandBuffer c )
{
	vkEndCommandBuffer( c );

	VkSubmitInfo submitInfo{  };
	submitInfo.sType 		= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount 	= 1;
	submitInfo.pCommandBuffers 	= &c;

	vkQueueSubmit( graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
	vkQueueWaitIdle( graphicsQueue );

	vkFreeCommandBuffers( device, commandPool, 1, &c );
}

VkFormat device_c::find_supported_fmt
	( const std::vector< VkFormat >& candidates,
	  VkImageTiling tiling,
	  VkFormatFeatureFlags features )
{
	for ( VkFormat format : candidates )
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties( physicalDevice, format, &props );
		if ( tiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & features ) == features )
		{
			return format;
		}
		else if ( tiling == VK_IMAGE_TILING_OPTIMAL && ( props.optimalTilingFeatures & features ) == features )
		{
			return format;
		}
	}
	throw std::runtime_error( "Failed to find supported format!" );
}

VkFormat device_c::find_depth_format
	(  )
{
	return find_supported_fmt(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
        	VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

uint32_t device_c::find_memory_type
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

device_c::device_c
	(  )
{
	init_device(  );
}

device_c::~device_c
	(  )
{
	
}
