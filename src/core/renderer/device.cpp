#include "../../../inc/core/renderer/device.h"

#include <set>

VKAPI_ATTR VkBool32 VKAPI_CALL Device::DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
					      const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
}

void Device::InitDevice(  )
{
        InitWindow(  );
        InitInstance(  );
        InitValidationLayers(  );
	InitSurface(  );
	InitPhysicalDevice(  );
	InitLogicalDevice(  );
        InitCommandPool(  );
	InitSwapChain(  );
}

void Device::InitWindow(  )
{
	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS ) != 0 )
		throw std::runtime_error( "Unable to initialize SDL2!" );
	apWindow = SDL_CreateWindow( " - Chocolate Engine ( build: too early for version lol )", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				     aWidth, aHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );
}

StringList Device::InitRequiredExtensions(  )
{
        uint32_t extensionCount = 0;
	if ( !SDL_Vulkan_GetInstanceExtensions( apWindow, &extensionCount, NULL ) )
		throw std::runtime_error( "Unable to query the number of Vulkan instance extensions\n" );
	/* Use the amount of extensions queried before to retrieve the names of the extensions.  */
        StringList extensions( extensionCount );
	/* PICK UP REFACTORING */
	/*





	 */
	if ( !SDL_Vulkan_GetInstanceExtensions( apWindow, &extensionCount, extensions.data(  ) ) )
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

void Device::InitInstance(  )
{
	if ( gEnableValidationLayers && !CheckValidationLayerSupport(  ) )
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
	if ( gEnableValidationLayers )
	{
		createInfo.enabledLayerCount 	= ( unsigned int )gValidationLayers.size(  );
		createInfo.ppEnabledLayerNames 	= gValidationLayers.data(  );
	        InitDebugMessengerInfo( debugCreateInfo );
		createInfo.pNext 		= ( VkDebugUtilsMessengerCreateInfoEXT* )&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount 	= 0;
		createInfo.pNext	        = NULL;
	}

	auto SDLExtensions = InitRequiredExtensions(  );
	
	createInfo.enabledExtensionCount 	= ( uint32_t )SDLExtensions.size(  );
	createInfo.ppEnabledExtensionNames 	= SDLExtensions.data(  );

	if ( vkCreateInstance( &createInfo, NULL, &aInstance ) )
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

void Device::InitDebugMessengerInfo( VkDebugUtilsMessengerCreateInfoEXT &srCreateInfo )
{
	srCreateInfo = {  };
	srCreateInfo.sType 	   		= VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        srCreateInfo.messageSeverity  		= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        srCreateInfo.messageType 	   	= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        srCreateInfo.pfnUserCallback  		= DebugCallback;
}

VkResult Device::InitDebugMessenger( VkInstance sInstance, const VkDebugUtilsMessengerCreateInfoEXT *spCreateInfo,
				     const VkAllocationCallbacks *spAllocator, VkDebugUtilsMessengerEXT *spDebugMessenger )
{
	auto func = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( aInstance, "vkCreateDebugUtilsMessengerEXT" );
	if ( func != NULL )
	{
		return func( aInstance, spCreateInfo, spAllocator, spDebugMessenger );
	}
	else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}


void Device::InitValidationLayers(  )
{
	if ( !gEnableValidationLayers ) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{  };
        InitDebugMessengerInfo( createInfo );

        if ( InitDebugMessenger( aInstance, &createInfo, NULL, &aDebugMessenger ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to set up debug messenger!" );
        }
}

void Device::InitSurface(  )
{
	if ( !SDL_Vulkan_CreateSurface( apWindow, aInstance, &aSurface ) )
	{
		throw std::runtime_error( "Failed to create Vulkan Surface!" );
	}
}

void Device::InitPhysicalDevice(  )
{
	aPhysicalDevice 		= VK_NULL_HANDLE;
	uint32_t deviceCount 		= 0;
	
	vkEnumeratePhysicalDevices( aInstance, &deviceCount, NULL );
	if ( deviceCount == 0 ) {
		throw std::runtime_error( "Failed to find GPUs with Vulkan support!" );
	}
	std::vector< VkPhysicalDevice > devices( deviceCount );
	vkEnumeratePhysicalDevices( aInstance, &deviceCount, devices.data(  ) );

	for ( const auto& device : devices ) {
		if ( IsSuitableDevice( device ) ) {
			aPhysicalDevice = device;
			break;
		}
	}

	if ( aPhysicalDevice == VK_NULL_HANDLE ) {
		throw std::runtime_error( "Failed to find a suitable GPU!" );
	}	
}

void Device::InitLogicalDevice(  )
{
	QueueFamilyIndices indices = FindQueueFamilies( aPhysicalDevice );

	std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
	std::set< uint32_t > uniqueQueueFamilies = { ( uint32_t )indices.aGraphicsFamily, ( uint32_t )indices.aPresentFamily };

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
	deviceFeatures.samplerAnisotropy = VK_FALSE;	//	Temporarily disabled for PBP builds

	VkDeviceCreateInfo createInfo{  };
	createInfo.sType 			= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos 		= queueCreateInfos.data(  );
	createInfo.queueCreateInfoCount 	= ( uint32_t )queueCreateInfos.size(  );

	createInfo.pEnabledFeatures 		= &deviceFeatures;

	createInfo.enabledExtensionCount 	= ( uint32_t )( gDeviceExtensions.size(  ) );
	createInfo.ppEnabledExtensionNames 	= gDeviceExtensions.data(  );

	if ( gEnableValidationLayers )
	{
		createInfo.enabledLayerCount 	= ( uint32_t )( gValidationLayers.size(  ) );
		createInfo.ppEnabledLayerNames 	= gValidationLayers.data(  );
	}
	else
	{
		createInfo.enabledLayerCount 	= 0;
	}
	if ( vkCreateDevice( aPhysicalDevice, &createInfo, NULL, &aDevice ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create logical device!" );
	}
	vkGetDeviceQueue( aDevice, indices.aGraphicsFamily, 0, &aGraphicsQueue );
	vkGetDeviceQueue( aDevice, indices.aPresentFamily, 0, &aPresentQueue );
}

void Device::InitCommandPool(  )
{
        QueueFamilyIndices indices = FindQueueFamilies( aPhysicalDevice );

	VkCommandPoolCreateInfo poolInfo{  };
	poolInfo.sType 			= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex 	= indices.aGraphicsFamily;
	poolInfo.flags 			= 0; // Optional

	if ( vkCreateCommandPool( aDevice, &poolInfo, NULL, &aCommandPool ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create command pool!" );
	}
}

bool Device::CheckValidationLayerSupport(  )
{
	bool layerFound;
	unsigned int layerCount;
	vkEnumerateInstanceLayerProperties( &layerCount, NULL );

	std::vector< VkLayerProperties > availableLayers( layerCount );
	vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data(  ) );
	
	for ( const char* layerName : gValidationLayers )
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

QueueFamilyIndices Device::FindQueueFamilies( VkPhysicalDevice sDevice )
{
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( sDevice, &queueFamilyCount, nullptr );

	std::vector< VkQueueFamilyProperties > queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( sDevice, &queueFamilyCount, queueFamilies.data(  ) );// Logic to find queue family indices to populate struct with
	int i = 0;
	for ( const auto& queueFamily : queueFamilies )
	{
		if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR( sDevice, i, aSurface, &presentSupport );
			if ( presentSupport )
			{
				indices.aPresentFamily = i;
			}
			indices.aGraphicsFamily = i;
		}
		if ( indices.Complete(  ) )
		{
			break;
		}
		i++;
	}
	return indices;
}

bool Device::CheckDeviceExtensionSupport( VkPhysicalDevice sDevice )
{
	unsigned int extensionCount;
	vkEnumerateDeviceExtensionProperties( sDevice, NULL, &extensionCount, NULL );

	std::vector< VkExtensionProperties > availableExtensions( extensionCount );
	vkEnumerateDeviceExtensionProperties( sDevice, NULL, &extensionCount, availableExtensions.data(  ) );

	std::set< std::string > requiredExtensions(  gDeviceExtensions.begin(  ), gDeviceExtensions.end(  ) );

	for ( const auto& extension : availableExtensions )
	{
		requiredExtensions.erase( extension.extensionName  );
	}
	return requiredExtensions.empty(  );
}

SwapChainSupportInfo Device::CheckSwapChainSupport( VkPhysicalDevice sDevice )
{
	SwapChainSupportInfo details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( sDevice, aSurface, &details.aCapabilities );

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR( sDevice, aSurface, &formatCount, NULL );

	if ( formatCount != 0 )
	{
		details.aFormats.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( sDevice, aSurface, &formatCount, details.aFormats.data(  ) );
	}
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR( sDevice, aSurface, &presentModeCount, NULL );

	if ( presentModeCount != 0 )
	{
		details.aPresentModes.resize( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR( sDevice, aSurface, &presentModeCount, details.aPresentModes.data(  ) );
	}
	return details;
}

bool Device::IsSuitableDevice( VkPhysicalDevice sDevice )
{
        QueueFamilyIndices indices 		= FindQueueFamilies( sDevice );
	bool extensionsSupported 		= CheckDeviceExtensionSupport( sDevice );
	bool swapChainAdequate 			= false;
	if ( extensionsSupported )
	{
		SwapChainSupportInfo swapChainSupport = CheckSwapChainSupport( sDevice );
		swapChainAdequate = !swapChainSupport.aFormats.empty(  ) && !swapChainSupport.aPresentModes.empty(  );
	}
	
	return indices.Complete(  ) && extensionsSupported && swapChainAdequate;
}

VkSurfaceFormatKHR Device::ChooseSwapSurfaceFormat( const SurfaceFormats &srAvailableFormats )
{
	for ( const auto& availableFormat : srAvailableFormats )
	{
		if ( availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
		{
			return availableFormat;
		}
	}
	return srAvailableFormats[ 0 ];
}

VkPresentModeKHR Device::ChooseSwapPresentMode( const PresentModes &srAvailablePresentModes )
{
	for ( const auto& availablePresentMode : srAvailablePresentModes )
	{
		if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
		{
			return availablePresentMode;
		}
	}
	/* Unlimited fps     ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! !!!!!!!!!!!!!!!!!!!!!!!!!1  */
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Device::ChooseSwapExtent( const VkSurfaceCapabilitiesKHR &srCapabilities )
{
	if ( srCapabilities.currentExtent.width != UINT32_MAX )
	{
		return srCapabilities.currentExtent;
	}
	else
	{
		VkExtent2D size = {
			( unsigned int )aWidth,
			( unsigned int )aHeight
		};

	       size.width = std::max( srCapabilities.minImageExtent.width, std::min( srCapabilities.maxImageExtent.width, size.width ) );
	       size.height = std::max( srCapabilities.minImageExtent.height, std::min( srCapabilities.maxImageExtent.height, size.height ) );

		return size;
	}
}

void Device::DestroyDebugMessenger( const VkAllocationCallbacks *pAllocator )
{
	auto func = ( PFN_vkDestroyDebugUtilsMessengerEXT )vkGetInstanceProcAddr( aInstance, "vkDestroyDebugUtilsMessengerEXT" );
	if ( func != NULL )
	{
		func( aInstance, aDebugMessenger, pAllocator );
	}
}

void Device::Cleanup(  )
{
	vkDestroyCommandPool( aDevice, aCommandPool, NULL );
	vkDestroyDevice( aDevice, NULL );
	if ( gEnableValidationLayers )
	{
		DestroyDebugMessenger( NULL );
	}
	vkDestroySurfaceKHR( aInstance, aSurface, NULL );
        vkDestroyInstance( aInstance, NULL );
		
	SDL_DestroyWindow( apWindow );
	SDL_Quit(  );
}

void Device::InitSwapChain(  )
{
        SwapChainSupportInfo 	swapChainSupport        = CheckSwapChainSupport( aPhysicalDevice );

	VkSurfaceFormatKHR 	surfaceFormat 		= ChooseSwapSurfaceFormat( swapChainSupport.aFormats );
	VkPresentModeKHR 	presentMode 	        = ChooseSwapPresentMode( swapChainSupport.aPresentModes );
	VkExtent2D 		extent 		        = ChooseSwapExtent( swapChainSupport.aCapabilities );
	VkSwapchainKHR		swapChain;
	ImageSet		swapChainImages;

	uint32_t imageCount 				= swapChainSupport.aCapabilities.minImageCount + 1;
	if ( swapChainSupport.aCapabilities.maxImageCount > 0 && imageCount > swapChainSupport.aCapabilities.maxImageCount )
	{
		imageCount = swapChainSupport.aCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{  };
	createInfo.sType 		= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface 		= aSurface;

	createInfo.minImageCount 	= imageCount;
	createInfo.imageFormat 		= surfaceFormat.format;
	createInfo.imageColorSpace 	= surfaceFormat.colorSpace;
	createInfo.imageExtent 		= extent;
	createInfo.imageArrayLayers	= 1;
	createInfo.imageUsage		= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices 	= FindQueueFamilies( aPhysicalDevice );
	uint32_t queueFamilyIndices[  ] = { ( uint32_t )indices.aGraphicsFamily, ( uint32_t )indices.aPresentFamily };

	if ( indices.aGraphicsFamily != indices.aPresentFamily )
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

	createInfo.preTransform 	= swapChainSupport.aCapabilities.currentTransform;
	createInfo.compositeAlpha 	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode 		= presentMode;
	createInfo.clipped		= VK_TRUE;
	createInfo.oldSwapchain 	= VK_NULL_HANDLE;

	if ( vkCreateSwapchainKHR( aDevice, &createInfo, NULL, &swapChain ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create swap chain!" );
	}
	vkGetSwapchainImagesKHR( aDevice, swapChain, &imageCount, NULL );
	swapChainImages.resize( imageCount );
	vkGetSwapchainImagesKHR( aDevice, swapChain, &imageCount, swapChainImages.data(  ) );

	aSwapChain.Init( swapChain, swapChainImages, surfaceFormat.format, extent );
}

void Device::InitTextureSampler( VkSampler& textureSampler, VkSamplerAddressMode mode )
{
	VkSamplerCreateInfo samplerInfo{  };
	samplerInfo.sType  	 = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter    = VK_FILTER_LINEAR;
	samplerInfo.minFilter    = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = mode;
	samplerInfo.addressModeV = mode;
	samplerInfo.addressModeW = mode;

	VkPhysicalDeviceProperties properties{  };
	vkGetPhysicalDeviceProperties( aPhysicalDevice, &properties );

	samplerInfo.anisotropyEnable 		= VK_FALSE;
	samplerInfo.maxAnisotropy 		= properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor 		= VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates 	= VK_FALSE;
	samplerInfo.compareEnable 		= VK_FALSE;
	samplerInfo.compareOp 			= VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode 			= VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias 			= 0.0f;
	samplerInfo.minLod 			= 0.0f;
	samplerInfo.maxLod 			= 0.0f;

	if ( vkCreateSampler( aDevice, &samplerInfo, NULL, &textureSampler ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create texture sampler!" );
	}
}

VkCommandBuffer Device::BeginSingleTimeCommands(  )
{
	VkCommandBufferAllocateInfo allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level 		= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool 		= aCommandPool;
	allocInfo.commandBufferCount 	= 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers( aDevice, &allocInfo, &commandBuffer );

	VkCommandBufferBeginInfo beginInfo{  };
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer( commandBuffer, &beginInfo );

	return commandBuffer;
}

void Device::EndSingleTimeCommands( VkCommandBuffer sCommandBuffer )
{
	vkEndCommandBuffer( sCommandBuffer );

	VkSubmitInfo submitInfo{  };
	submitInfo.sType 		= VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount 	= 1;
	submitInfo.pCommandBuffers 	= &sCommandBuffer;

	vkQueueSubmit( aGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
	vkQueueWaitIdle( aGraphicsQueue );

	vkFreeCommandBuffers( aDevice, aCommandPool, 1, &sCommandBuffer );
}

VkFormat Device::FindSupportedFormat( const FormatSet &srCandidates, VkImageTiling sTiling, VkFormatFeatureFlags sFeatures )
{
	for ( VkFormat format : srCandidates )
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties( aPhysicalDevice, format, &props );
		if ( sTiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & sFeatures ) == sFeatures )
		{
			return format;
		}
		else if ( sTiling == VK_IMAGE_TILING_OPTIMAL && ( props.optimalTilingFeatures & sFeatures ) == sFeatures )
		{
			return format;
		}
	}
	throw std::runtime_error( "Failed to find supported format!" );
}

VkFormat Device::FindDepthFormat(  )
{
	return FindSupportedFormat( { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL,
				    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
}

uint32_t Device::FindMemoryType( uint32_t sTypeFilter, VkMemoryPropertyFlags sProperties )
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties( aPhysicalDevice, &memProperties );

	for ( uint32_t i = 0; i < memProperties.memoryTypeCount; ++i )
	{
		if ( ( sTypeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[ i ].propertyFlags & sProperties ) == sProperties )
		{
			return i;
		}
	}

	throw std::runtime_error( "Failed to find suitable memory type!" );
}

Device::Device(  )
{
        InitDevice(  );
}

Device::~Device(  )
{
	Cleanup(  );
}
