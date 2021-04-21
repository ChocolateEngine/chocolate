#include "../../inc/core/renderer.h"

#include <set>

void renderer_c::init_vulkan
	(  )
{
	init_window(  );
	init_instance(  );
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
		SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
                );
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

	if ( enableValidationLayers )
	{
		createInfo.enabledLayerCount 	= ( unsigned int )validationLayers.size(  );
		createInfo.ppEnabledLayerNames 	= validationLayers.data(  );
	}
	else
	{
		createInfo.enabledLayerCount 	= 0;
	}

	unsigned int SDLExtensionCount = 0;
	
	if ( !SDL_Vulkan_GetInstanceExtensions( win, &SDLExtensionCount, NULL ) )
	{
		throw std::runtime_error( "Unable to get number of Vulkan instance extensions!" );
	}
	
	std::vector< const char* > SDLExtensions( SDLExtensionCount );
	
	if ( !SDL_Vulkan_GetInstanceExtensions( win, &SDLExtensionCount, SDLExtensions.data(  ) ) )
	{
		throw std::runtime_error( "Unable to get Vulkan instance extension names!" );
	}
	
	printf( "%d SDL extensions available:\n", SDLExtensionCount );
	
	for ( int i = 0; i < SDLExtensionCount; ++i )
	{
		printf( "\t%s\n", SDLExtensions[ i ] );
	}

	createInfo.enabledExtensionCount 	= SDLExtensionCount;
	createInfo.ppEnabledExtensionNames 	= SDLExtensions.data(  );
	createInfo.enabledLayerCount 		= 0;

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

void renderer_c::cleanup
	(  )
{
	vkDestroyInstance( inst, NULL );
	
	SDL_DestroyWindow( win );
	SDL_Quit(  );
}

renderer_c::renderer_c
	(  )
{
	init_vulkan(  );
	
}

renderer_c::~renderer_c
	(  )
{
	cleanup(  );
}
