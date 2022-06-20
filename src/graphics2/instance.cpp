#include "instance.h"

#include <set>

#include "config.hh"
#include "gutil.hh"
#include "swapchain.h"

#include "core/core.h"

constexpr char const *gpDeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_EXT_descriptor_indexing" };

VkSampleCountFlagBits gMaxSamples = VK_SAMPLE_COUNT_1_BIT;

LOG_REGISTER_CHANNEL( Validation, LogColor::Blue );
LOG_REGISTER_CHANNEL( Vulkan, LogColor::Blue );

#ifdef NDEBUG
// const bool 	gEnableValidationLayers = false;
bool g_vk_verbose = false;
#else
// const bool 	gEnableValidationLayers = cmdline->Find( "-vlayers" );
CONVAR( g_vk_verbose, 1 );
#endif

// lengths to chop off from warnings:

// "Validation Performance Warning: "
int gVkStripPerf = 32;

// "Validation Error: "
int gVkStripError = 18;


VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
	if ( !gEnableValidationLayers )
		return VK_FALSE;

	const Log* log = LogGetLastLog();

	// blech
	if ( log && log->aChannel != gVulkanChannel )
		LogEx( gVulkanChannel, LogType::Raw, "\n" );

	std::string formatted;

	if ( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT )
		vstring( formatted, "Validation: %s\n\n", pCallbackData->pMessage + gVkStripError );

	else if ( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT )
		vstring( formatted, "Performance: %s\n\n", pCallbackData->pMessage + gVkStripPerf );

	else
		vstring( formatted, "%s\n\n", pCallbackData->pMessage );


	if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT )
	{
		if ( g_vk_verbose )
			LogPutsDev( gVulkanChannel, 1, formatted.c_str() );
	}

	else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
		LogEx( gVulkanChannel, LogType::Error, formatted.c_str() );

	else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
		LogEx( gVulkanChannel, LogType::Warning, formatted.c_str() );

	else
		LogPuts( gVulkanChannel, formatted.c_str() );

	return VK_FALSE;
}


constexpr VkDebugUtilsMessengerCreateInfoEXT gLayerInfo = {
	.sType 	   		  = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	.messageSeverity  = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	.messageType 	  = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	.pfnUserCallback  = DebugCallback,
};

bool CheckValidationLayerSupport()
{
	bool layerFound;
	unsigned int layerCount;
	vkEnumerateInstanceLayerProperties( &layerCount, NULL );

	std::vector< VkLayerProperties > availableLayers( layerCount );
	vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data(  ) );
	
	for ( auto layerName : gpValidationLayers )
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

void GInstance::CreateWindow() 
{
	aWindow.aWidth  = cmdline->GetValue( "-w", GetOption( "Width"  ) ); 
	aWindow.aHeight = cmdline->GetValue( "-h", GetOption( "Height" ) );

	if ( SDL_Init( SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO ) != 0 )
		LogFatal( "SDL_Init Error: %s", SDL_GetError(  ) );

	aWindow.apWindow = SDL_CreateWindow( " - Chocolate Engine - Compiled on " __DATE__, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				                   aWindow.aWidth, aWindow.aHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE );

	if ( !aWindow.apWindow )
		LogFatal( "SDL_CreateWindow Error: %s", SDL_GetError(  ) );
}

void GInstance::CreateInstance( void ) 
{
    if ( gEnableValidationLayers && !CheckValidationLayerSupport(  ) )
		LogFatal( "Validation layers requested, but not available!" );

	VkApplicationInfo appInfo{  };
	appInfo.sType 			    = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName 	= "Test";
	appInfo.applicationVersion 	= VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.pEngineName 		= "Chocolate";
	appInfo.engineVersion 		= VK_MAKE_VERSION( 1, 1, 0 );
	appInfo.apiVersion 		    = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{  };
	createInfo.sType 		= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo 	= &appInfo;

	if ( gEnableValidationLayers )
	{
		createInfo.enabledLayerCount 	= ( unsigned int )ARR_SIZE( gpValidationLayers );
		createInfo.ppEnabledLayerNames 	= gpValidationLayers;
		createInfo.pNext 		        = ( VkDebugUtilsMessengerCreateInfoEXT* )&gLayerInfo;
	}
	else
	{
		createInfo.enabledLayerCount 	= 0;
		createInfo.pNext	            = NULL;
	}

	auto SDLExtensions = InitRequiredExtensions(  );
	
	createInfo.enabledExtensionCount 	= ( uint32_t )SDLExtensions.size(  );
	createInfo.ppEnabledExtensionNames 	= SDLExtensions.data(  );

	CheckVKResult( vkCreateInstance( &createInfo, NULL, &aInstance ), "Failed to create instance!" );

	unsigned int extensionCount = 0;
	vkEnumerateInstanceExtensionProperties( NULL, &extensionCount, NULL );
	
	std::vector< VkExtensionProperties > extensions( extensionCount );
	vkEnumerateInstanceExtensionProperties( NULL, &extensionCount, extensions.data(  ) );

	LogDev( gVulkanChannel, "%d Vulkan extensions available:\n", extensionCount );

	for ( const auto& extension : extensions )
		Print( "\t%s\n", extension.extensionName );

	Print( "\n" );
}

std::vector< const char* > GInstance::InitRequiredExtensions()
{
    u32 extensionCount = 0;
	if ( !SDL_Vulkan_GetInstanceExtensions( aWindow.apWindow, &extensionCount, NULL ) )
		LogFatal( "Unable to query the number of Vulkan instance extensions\n" );

	/* Use the amount of extensions queried before to retrieve the names of the extensions.  */
    std::vector< const char * > extensions( extensionCount );

	if ( !SDL_Vulkan_GetInstanceExtensions( aWindow.apWindow, &extensionCount, extensions.data(  ) ) )
		LogFatal( "Unable to query the number of Vulkan instance extension names\n" );

	// Display names
	LogDev( gVulkanChannel, "Found % d Vulkan extensions : \n", extensionCount );
	for ( u32 i = 0; i < extensionCount; ++i )
		LogDev( gVulkanChannel, "\t%u : %s\n", i, extensions[ i ] );

	LogPutsDev( gVulkanChannel, "\n" );

	// Add debug display extension, we need this to relay debug messages
	extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
	return extensions;
}

VkResult GInstance::CreateValidationLayers()
{
	auto func = ( PFN_vkCreateDebugUtilsMessengerEXT )vkGetInstanceProcAddr( aInstance, "vkCreateDebugUtilsMessengerEXT" );
	if ( func != NULL )
		return func( aInstance, &gLayerInfo, nullptr, &aLayers );
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

bool CheckDeviceExtensionSupport( VkPhysicalDevice sDevice )
{
	unsigned int extensionCount;
	vkEnumerateDeviceExtensionProperties( sDevice, NULL, &extensionCount, NULL );

	std::vector< VkExtensionProperties > availableExtensions( extensionCount );
	vkEnumerateDeviceExtensionProperties( sDevice, NULL, &extensionCount, availableExtensions.data(  ) );

	std::set< std::string > requiredExtensions( gpDeviceExtensions, gpDeviceExtensions + sizeof( gpDeviceExtensions ) / sizeof( gpDeviceExtensions[ 0 ] ) );

	for ( const auto& extension : availableExtensions )
	{
		requiredExtensions.erase( extension.extensionName  );
	}
	return requiredExtensions.empty(  );
}

QueueFamilyIndices GInstance::FindQueueFamilies( VkPhysicalDevice sDevice )
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
			vkGetPhysicalDeviceSurfaceSupportKHR( sDevice, i, GetSurface(), &presentSupport );
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

SwapChainSupportInfo GInstance::CheckSwapChainSupport( VkPhysicalDevice sDevice )
{
    auto surf = GetSurface();
	SwapChainSupportInfo details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( sDevice, surf, &details.aCapabilities );

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR( sDevice, surf, &formatCount, NULL );

	if ( formatCount != 0 )
	{
		details.aFormats.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( sDevice, surf, &formatCount, details.aFormats.data(  ) );
	}
	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR( sDevice, surf, &presentModeCount, NULL );

	if ( presentModeCount != 0 )
	{
		details.aPresentModes.resize( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR( sDevice, surf, &presentModeCount, details.aPresentModes.data(  ) );
	}
	return details;
}

bool GInstance::SuitableCard( VkPhysicalDevice sDevice )
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

VkSampleCountFlagBits GInstance::FindMaxMSAASamples()
{
	VkPhysicalDeviceProperties 	physicalDeviceProperties;
	vkGetPhysicalDeviceProperties( aPhysicalDevice, &physicalDeviceProperties );

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if ( counts & VK_SAMPLE_COUNT_64_BIT ) 	return VK_SAMPLE_COUNT_64_BIT;
	if ( counts & VK_SAMPLE_COUNT_32_BIT ) 	return VK_SAMPLE_COUNT_32_BIT;
	if ( counts & VK_SAMPLE_COUNT_16_BIT ) 	return VK_SAMPLE_COUNT_16_BIT;
	if ( counts & VK_SAMPLE_COUNT_8_BIT ) 	return VK_SAMPLE_COUNT_8_BIT;
	if ( counts & VK_SAMPLE_COUNT_4_BIT ) 	return VK_SAMPLE_COUNT_4_BIT;
	if ( counts & VK_SAMPLE_COUNT_2_BIT ) 	return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}

void GInstance::SetupPhysicalDevice() 
{
    aPhysicalDevice 	   = VK_NULL_HANDLE;
	uint32_t deviceCount   = 0;
	
	vkEnumeratePhysicalDevices( aInstance, &deviceCount, NULL );
	if ( deviceCount == 0 ) {
		throw std::runtime_error( "Failed to find GPUs with Vulkan support!" );
	}
	std::vector< VkPhysicalDevice > devices( deviceCount );
	vkEnumeratePhysicalDevices( aInstance, &deviceCount, devices.data(  ) );

	for ( const auto& device : devices ) {
		if ( SuitableCard( device ) ) {
			aPhysicalDevice = device;
			aSampleCount    = gMaxSamples = FindMaxMSAASamples();
			SetOption( "MSAA Samples", aSampleCount );
			break;
		}
	}

	if ( aPhysicalDevice == VK_NULL_HANDLE ) 
		LogFatal( "Failed to find a suitable GPU!" );
}

void GInstance::CreateDevice()
{
    QueueFamilyIndices indices = FindQueueFamilies( aPhysicalDevice );

	std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;
	std::set< uint32_t > uniqueQueueFamilies = { ( uint32_t )indices.aGraphicsFamily, ( uint32_t )indices.aPresentFamily };

	float queuePriority = 1.0f;
	for ( uint32_t queueFamily : uniqueQueueFamilies )
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {
		    .sType			    = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		    .queueFamilyIndex 	= queueFamily,
		    .queueCount		    = 1,
		    .pQueuePriorities 	= &queuePriority,
        };
		
		queueCreateInfos.push_back( queueCreateInfo );
	}

	VkPhysicalDeviceFeatures deviceFeatures{  };
	deviceFeatures.samplerAnisotropy = VK_FALSE;	//	Temporarily disabled for PBP builds

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexing{};
    indexing.sType                                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    indexing.pNext                                    = nullptr;
    indexing.descriptorBindingPartiallyBound          = VK_TRUE;
    indexing.runtimeDescriptorArray                   = VK_TRUE;
    indexing.descriptorBindingVariableDescriptorCount = VK_TRUE;


	VkDeviceCreateInfo createInfo = {
	    .sType 			         = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
	    .pNext                   = &indexing,
        .flags                   = 0,	    
	    .queueCreateInfoCount 	 = ( uint32_t )queueCreateInfos.size(  ),
        .pQueueCreateInfos 		 = queueCreateInfos.data(  ),
        .enabledLayerCount       = ( uint32_t )ARR_SIZE( gpValidationLayers ),
        .ppEnabledLayerNames     = gpValidationLayers,
        .enabledExtensionCount   = ( uint32_t )ARR_SIZE( gpDeviceExtensions ),
        .ppEnabledExtensionNames = gpDeviceExtensions,
        .pEnabledFeatures        = &deviceFeatures,
    };

	CheckVKResult( vkCreateDevice( aPhysicalDevice, &createInfo, NULL, &aDevice ), "Failed to create logical device!" );

	vkGetDeviceQueue( aDevice, indices.aGraphicsFamily, 0, &aGraphicsQueue );
	vkGetDeviceQueue( aDevice, indices.aPresentFamily,  0, &aPresentQueue );
}

GInstance::GInstance()
{
}

GInstance::~GInstance()
{
	vkDestroyDevice( aDevice, NULL );
	vkDestroySurfaceKHR( aInstance, aSurface, NULL );
	vkDestroyInstance( aInstance, NULL );
	SDL_DestroyWindow( aWindow.apWindow );
	SDL_Quit();
}

void GInstance::Init()
{
	CreateWindow();
	CreateInstance();
	if ( gEnableValidationLayers && CreateValidationLayers() != VK_SUCCESS )
		LogFatal( "Failed to create validation layers!" );

	if ( !SDL_Vulkan_CreateSurface( aWindow.apWindow, aInstance, &aSurface ) )
		LogFatal( "Failed to create Vulkan surface!" );

	SetupPhysicalDevice();
	CreateDevice();
}

uint32_t GInstance::GetMemoryType( uint32_t sTypeFilter, VkMemoryPropertyFlags sProperties )
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

	return INT32_MAX;
}



GInstance &GetGInstance()
{
	static GInstance instance;
	return instance;
}

VkInstance GetInst()
{
	return GetGInstance().GetInstance();
}

VkDevice GetDevice()
{
	return GetGInstance().GetDevice();
}

VkPhysicalDevice GetPhysicalDevice()
{
	return GetGInstance().GetPhysicalDevice();
}