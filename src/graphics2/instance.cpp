#include "core/platform.h"
#include "core/log.h"
#include "core/commandline.h"
#include "util.h"

#include "render/irender.h"
#include "render_vk.h"

#include <SDL_vulkan.h>

#include <set>


#ifdef NDEBUG
constexpr bool        gEnableValidationLayers = false;
bool                  gListExts               = false;

constexpr char const* gpValidationLayers[]    = { 0 };
bool                  vk_verbose              = false;
bool                  vk_formatted            = false;

#else

bool                  gEnableValidationLayers = Args_Register( false, "Enable Vulkan Validation Layers Extensions", "-vk-valid" );
bool                  gListExts               = Args_Register( false, "List All Vulkan Extensions, marking what ones are loaded", "-list-exts" );

constexpr char const* gpValidationLayers[]    = { "VK_LAYER_KHRONOS_validation" };
CONVAR( vk_verbose, 0 );
CONVAR( vk_formatted, 1 );
#endif


constexpr char const* gpExtensions[] = {
	VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
	"VK_KHR_win32_surface",
#endif
#if _DEBUG
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
};


constexpr char const* gpDeviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	"VK_EXT_descriptor_indexing",
#if _DEBUG
	// VK_EXT_DEVICE_MEMORY_REPORT_EXTENSION_NAME,
#endif
};


constexpr char const* gpOptionalExtensions[] = {
	VK_EXT_FILTER_CUBIC_EXTENSION_NAME,
};


constexpr char const* gpOptionalDeviceExtensions[] = {
	VK_EXT_FILTER_CUBIC_EXTENSION_NAME,
};


#if _DEBUG
PFN_vkSetDebugUtilsObjectNameEXT    pfnSetDebugUtilsObjectName    = nullptr;
PFN_vkSetDebugUtilsObjectTagEXT     pfnSetDebugUtilsObjectTag     = nullptr;

PFN_vkQueueBeginDebugUtilsLabelEXT  pfnQueueBeginDebugUtilsLabel  = nullptr;
PFN_vkQueueEndDebugUtilsLabelEXT    pfnQueueEndDebugUtilsLabel    = nullptr;
PFN_vkQueueInsertDebugUtilsLabelEXT pfnQueueInsertDebugUtilsLabel = nullptr;

PFN_vkCmdBeginDebugUtilsLabelEXT    pfnCmdBeginDebugUtilsLabel    = nullptr;
PFN_vkCmdEndDebugUtilsLabelEXT      pfnCmdEndDebugUtilsLabel      = nullptr;
PFN_vkCmdInsertDebugUtilsLabelEXT   pfnCmdInsertDebugUtilsLabel   = nullptr;
#endif


static VkInstance                        gInstance;
static VkDebugUtilsMessengerEXT          gLayers;
static VkSurfaceKHR                      gSurface;
static VkPhysicalDevice                  gPhysicalDevice;
static VkDevice                          gDevice;
static VkQueue                           gGraphicsQueue;
static VkQueue                           gPresentQueue;

static VkPhysicalDeviceProperties        gPhysicalDeviceProperties;

static VkSurfaceCapabilitiesKHR          gSwapCapabilities;
static std::vector< VkSurfaceFormatKHR > gSwapFormats;
static std::vector< VkPresentModeKHR >   gSwapPresentModes;

extern SDL_Window*                       gpWindow;


// lengths to chop off from warnings:

// "Validation Performance Warning: "
int gVkStripPerf = 32;

// "Validation Error: "
int gVkStripError = 18;
// auto a = "Validation Error: [ VUID-vkFreeDescriptorSets-pDescriptorSets-00309 ] Object 0: handle = 0x7cd292000000004f, name = Light Point Set, type = VK_OBJECT_TYPE_DESCRIPTOR_SET; | MessageID = 0xbfce1114 | vkUpdateDescriptorSets() pDescriptorWrites[0] failed write update validation for VkDescriptorSet 0x7cd292000000004f[Light Point Set] with error: Cannot call vkUpdateDescriptorSets() to perform write update on VkDescriptorSet 0x7cd292000000004f[Light Point Set] allocated with VkDescriptorSetLayout 0x612f93000000004e[Light Point Layout] that is in use by a command buffer"

VKAPI_ATTR VkBool32 VKAPI_CALL VK_DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
	if ( !gEnableValidationLayers )
		return VK_FALSE;

	const Log* log = Log_GetLastLog();

	// blech
	if ( log && log->aChannel != gVulkanChannel )
		Log_Ex( gVulkanChannel, LogType::Raw, "\n" );

	std::string formatted;

	if ( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT )
		vstring( formatted, "Validation: %s\n\n", pCallbackData->pMessage + gVkStripError );

	else if ( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT )
		vstring( formatted, "Performance: %s\n\n", pCallbackData->pMessage + gVkStripPerf );

	else
		vstring( formatted, "%s\n\n", pCallbackData->pMessage );

	if ( vk_formatted )
	{
		std::string copy = formatted;
		formatted.clear();

		bool foundBar = false;

		for ( int i = 0; i < copy.size(); i++ )
		{
			if ( copy[ i ] == '|' )
			{
				foundBar = true;

				formatted += "\n  ";
				i++;
			}
			else if ( copy[ i ] == ',' && !foundBar )
			{
				formatted += "\n  ";
				i++;
			}
			else if ( copy[ i ] == ']' && !foundBar )
			{
				formatted += "]\n  ";
				i++;
			}
			else
			{
				// if ( "vk" == copy.substr( i, i + 2 ) )
				// {
				// 	formatted += "\nvk";
				// 	i++;
				// 	continue;
				// }

				// if ( i + 25 < copy.size() && strncmp( "The Vulkan spec states: ", copy.substr( i, i + 25 ), 25 ) == 0 )
				// if ( i + 25 < copy.size() && "The Vulkan spec states: " == copy.substr( i, i + 25 ) )
				auto test = copy.substr( i, 24 );
				if ( "The Vulkan spec states: " == test )
				{
					formatted += "\n\n  ";
					formatted += copy.substr( i, copy.size() );
					break;
				}

				formatted += copy[ i ];
			}
		}
	}

	if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT )
	{
		if ( vk_verbose )
			Log_Dev( gVulkanChannel, 1, formatted.c_str() );
	}

	else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
		Log_Error( gVulkanChannel, formatted.c_str() );

	else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
		Log_Warn( gVulkanChannel,  formatted.c_str() );

	else
		Log_Msg( gVulkanChannel, formatted.c_str() );

	return VK_FALSE;
}


bool VK_CheckValidationLayerSupport()
{
	bool         layerFound = false;
	unsigned int layerCount;
	VK_CheckResult( vkEnumerateInstanceLayerProperties( &layerCount, NULL ), "Failed to enumerate instance layer properties" );

	std::vector< VkLayerProperties > availableLayers( layerCount );
	VK_CheckResult( vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() ), "Failed to enumerate instance layer properties" );

	for ( auto layerName : gpValidationLayers )
	{
		if ( layerName == nullptr )
			return false;

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

constexpr VkDebugUtilsMessengerCreateInfoEXT gLayerInfo = {
	.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	.pfnUserCallback = VK_DebugCallback,
};

VkResult VK_CreateValidationLayers()
{
	// TODO: look into VkDebugReportCallbackCreateInfoEXT
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( gInstance, "vkCreateDebugUtilsMessengerEXT" );
	if ( func != NULL )
		return func( gInstance, &gLayerInfo, nullptr, &gLayers );
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}


void VK_DestroyValidationLayers()
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr( gInstance, "vkDestroyDebugUtilsMessengerEXT" );
	if ( func != NULL )
		func( gInstance, gLayers, nullptr );
}


bool VK_CheckDeviceExtensionSupport( VkPhysicalDevice sDevice )
{
	unsigned int extensionCount;
	vkEnumerateDeviceExtensionProperties( sDevice, NULL, &extensionCount, NULL );

	std::vector< VkExtensionProperties > availableExtensions( extensionCount );
	vkEnumerateDeviceExtensionProperties( sDevice, NULL, &extensionCount, availableExtensions.data() );

	std::set< std::string > requiredExtensions( gpDeviceExtensions, gpDeviceExtensions + ARR_SIZE( gpDeviceExtensions ) );

	if ( gListExts )
	{
		Log_MsgF( gLC_Render, "Device Extensions: %zd\n", availableExtensions.size() );

		for ( const auto& extension : availableExtensions )
		{
			auto ret = requiredExtensions.find( extension.extensionName );

			if ( ret != requiredExtensions.end() )
			{
				Log_MsgF( gLC_Render, " [LOADED] %s \n", extension.extensionName );
				requiredExtensions.erase( extension.extensionName );
			}
			else
			{
				Log_MsgF( gLC_Render, "          %s\n", extension.extensionName );
			}
		}

		Log_Msg( gLC_Render, "\n" );
	}
	else
	{
		for ( const auto& extension : availableExtensions )
		{
			requiredExtensions.erase( extension.extensionName );
		}
	}

	return requiredExtensions.empty();
}


std::vector< const char* > VK_GetSDL2Extensions()
{
	uint32_t extensionCount = 0;
	if ( !SDL_Vulkan_GetInstanceExtensions( gpWindow, &extensionCount, NULL ) )
		Log_Fatal( gLC_Render, "Unable to query the number of Vulkan instance extensions\n" );

	// Use the amount of extensions queried before to retrieve the names of the extensions.
	std::vector< const char* > extensions( extensionCount );

	if ( !SDL_Vulkan_GetInstanceExtensions( gpWindow, &extensionCount, extensions.data() ) )
		Log_Fatal( gLC_Render, "Unable to query the number of Vulkan instance extension names\n" );

	LogGroup group = Log_GroupBeginEx( gLC_Render, LogType::Dev );

	// Display names
	Log_GroupF( group, "Found %d Vulkan extensions:\n", extensionCount );
	for ( uint32_t i = 0; i < extensionCount; ++i )
		Log_GroupF( group, "\t%i : %s\n", i, extensions[ i ] );

	Log_Group( group, "\n" );
	Log_GroupEnd( group );

	return extensions;
}


bool VK_CreateInstance()
{
	bool hasValidation = gEnableValidationLayers;

	if ( hasValidation )
	{
		hasValidation = VK_CheckValidationLayerSupport();

		if ( !hasValidation )
			Log_Error( "Validation layers requested, but not available!\n" );
	}

	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName   = "ProtoViewer";
	appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.pEngineName        = "ProtoViewer";
	appInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.apiVersion         = VK_HEADER_VERSION_COMPLETE;

	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

	if ( hasValidation )
	{
		createInfo.enabledLayerCount   = (unsigned int)ARR_SIZE( gpValidationLayers );
		createInfo.ppEnabledLayerNames = gpValidationLayers;
		createInfo.pNext               = (VkDebugUtilsMessengerCreateInfoEXT*)&gLayerInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext             = NULL;
	}

	unsigned int extensionCount = 0;
	vkEnumerateInstanceExtensionProperties( NULL, &extensionCount, NULL );

	std::vector< VkExtensionProperties > extProps( extensionCount );
	vkEnumerateInstanceExtensionProperties( NULL, &extensionCount, extProps.data() );

	if ( gListExts )
	{
		Log_MsgF( gLC_Render, "%d Instance extensions :\n", extensionCount );
	
		for ( const auto& ext : extProps )
			Log_MsgF( gLC_Render, "    %s\n", ext.extensionName );
	
		Log_Msg( gLC_Render, "\n" );
	}

	std::vector< const char* > sdlExt = VK_GetSDL2Extensions();

	// Add debug extension, we need this to relay debug messages
	if ( hasValidation )
		sdlExt.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

	createInfo.enabledExtensionCount   = static_cast< uint32_t >( sdlExt.size() );
	createInfo.ppEnabledExtensionNames = sdlExt.data();

	VK_CheckResult( vkCreateInstance( &createInfo, NULL, &gInstance ), "Failed to create instance!" );

	if ( hasValidation )
	{
		VkResult result = VK_CreateValidationLayers();
		if ( result == VK_SUCCESS )
		{
			Log_Dev( gLC_Render, 1, "Successfully Created Validation Layers\n" );
		}
		else
		{
			Log_ErrorF( gLC_Render, "Failed to Create Validation Layers: %s\n", VKString( result ) );
		}
	}
	
	return true;
}


void VK_DestroyInstance()
{
	VK_DestroyValidationLayers();
	vkDestroyDevice( gDevice, NULL );
	vkDestroySurfaceKHR( gInstance, gSurface, NULL );
	vkDestroyInstance( gInstance, NULL );
}


uint32_t VK_GetMemoryType( uint32_t sTypeFilter, VkMemoryPropertyFlags sProperties )
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties( gPhysicalDevice, &memProperties );

	for ( uint32_t i = 0; i < memProperties.memoryTypeCount; ++i )
	{
		if ( ( sTypeFilter & ( 1 << i ) ) && ( memProperties.memoryTypes[ i ].propertyFlags & sProperties ) == sProperties )
		{
			return i;
		}
	}

	return UINT32_MAX;
}


// TODO: rethink this
void VK_FindQueueFamilies( VkPhysicalDevice sDevice, u32* spGraphics, u32* spPresent )
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( sDevice, &queueFamilyCount, nullptr );

	std::vector< VkQueueFamilyProperties > queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( sDevice, &queueFamilyCount, queueFamilies.data() );  // Logic to find queue family indices to populate struct with

	// *spPresent  = UINT32_MAX;
	// *spGraphics = UINT32_MAX;

	u32 i = 0;
	for ( const auto& queueFamily : queueFamilies )
	{
		if ( queueFamily.queueFlags & ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) )
		{
			VkBool32 presentSupport = false;
			VK_CheckResult( vkGetPhysicalDeviceSurfaceSupportKHR( sDevice, i, VK_GetSurface(), &presentSupport ),
			                "Failed to Get Physical Device Surface Support" );

			if ( presentSupport && spPresent )
				*spPresent = i;

			*spGraphics = i;
		}

		// if ( *spGraphics != UINT32_MAX && *spPresent != UINT32_MAX )
		// 	break;
		return;

		i++;
	}
}

bool VK_ValidQueueFamilies( u32& srPresent, u32& srGraphics )
{
	return true;
}


VkSurfaceCapabilitiesKHR VK_GetSwapCapabilities()
{
	return gSwapCapabilities;
}


VkSurfaceFormatKHR VK_ChooseSwapSurfaceFormat()
{
	for ( const auto& availableFormat : gSwapFormats )
	{
		if ( availableFormat.format == gColorFormat && availableFormat.colorSpace == gColorSpace )
		{
			return availableFormat;
		}
	}
	return gSwapFormats[ 0 ];
}


VkPresentModeKHR VK_ChooseSwapPresentMode()
{
	// if ( !GetOption( "VSync" ) )
	// return VK_PRESENT_MODE_IMMEDIATE_KHR;

	for ( const auto& availablePresentMode : gSwapPresentModes )
	{
		if ( availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR )
			return availablePresentMode;

		if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
			return availablePresentMode;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D VK_ChooseSwapExtent()
{
	int width = 0, height = 0;
	SDL_GetWindowSize( gpWindow, &width, &height );

	VkExtent2D size{
		std::max( gSwapCapabilities.minImageExtent.width, std::min( gSwapCapabilities.maxImageExtent.width, (u32)width ) ),
		std::max( gSwapCapabilities.minImageExtent.height, std::min( gSwapCapabilities.maxImageExtent.height, (u32)height ) ),
	};

	return size;
}


void VK_UpdateSwapchainInfo()
{
	if ( !gPhysicalDevice )
	{
		Log_Error( gLC_Render, "VK_UpdateSwapchainInfo(): No Physical Device?\n" );
		return;
	}

	VK_CheckResult( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( gPhysicalDevice, VK_GetSurface(), &gSwapCapabilities ),
	                "Failed to Get Physical Device Surface Capabilities" );
}


bool VK_SetupSwapchainInfo( VkPhysicalDevice sDevice )
{
	VkSurfaceKHR surf = VK_GetSurface();
	VK_CheckResult( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( sDevice, surf, &gSwapCapabilities ),
	                "Failed to Get Physical Device Surface Capabilities" );

	uint32_t formatCount;
	VK_CheckResult( vkGetPhysicalDeviceSurfaceFormatsKHR( sDevice, surf, &formatCount, NULL ),
	                "Failed to Get Physical Device Surface Formats" );

	if ( formatCount != 0 )
	{
		gSwapFormats.resize( formatCount );
		VK_CheckResult( vkGetPhysicalDeviceSurfaceFormatsKHR( sDevice, surf, &formatCount, gSwapFormats.data() ),
		                "Failed to Get Physical Device Surface Formats" );
	}

	uint32_t presentModeCount;
	VK_CheckResult( vkGetPhysicalDeviceSurfacePresentModesKHR( sDevice, surf, &presentModeCount, NULL ),
	                "Failed to Get Physical Device Surface Present Modes" );

	if ( presentModeCount != 0 )
	{
		gSwapPresentModes.resize( presentModeCount );
		VK_CheckResult( vkGetPhysicalDeviceSurfacePresentModesKHR( sDevice, surf, &presentModeCount, gSwapPresentModes.data() ),
		                "Failed to Get Physical Device Surface Present Modes" );
	}

	return !gSwapFormats.empty() && !gSwapPresentModes.empty();
}


bool VK_SuitableCard( VkPhysicalDevice sDevice )
{
	bool extensionsSupported = VK_CheckDeviceExtensionSupport( sDevice );
	bool swapChainAdequate   = false;

	if ( extensionsSupported )
		swapChainAdequate = VK_SetupSwapchainInfo( sDevice );

	u32 graphics, present;
	VK_FindQueueFamilies( sDevice, &graphics, &present );

	return VK_ValidQueueFamilies( present, graphics ) && extensionsSupported && swapChainAdequate;
}


void VK_CreateSurface( void* spWindow )
{
	if ( !SDL_Vulkan_CreateSurface( (SDL_Window*)spWindow, VK_GetInstance(), &gSurface ) )
	{
		Log_FatalF( gLC_Render, "Error: Failed to create SDL Vulkan Surface: %s\n", SDL_GetError() );
	}
}


void VK_DestroySurface()
{
}


void VK_SetupPhysicalDevice()
{
	gPhysicalDevice      = VK_NULL_HANDLE;
	uint32_t deviceCount = 0;

	VK_CheckResult( vkEnumeratePhysicalDevices( gInstance, &deviceCount, NULL ), "Failed to Enumerate Physical Devices" );
	if ( deviceCount == 0 )
	{
		Log_Fatal( gLC_Render, "Failed to find GPUs with Vulkan support!" );
	}

	std::vector< VkPhysicalDevice > devices( deviceCount );
	VK_CheckResult( vkEnumeratePhysicalDevices( gInstance, &deviceCount, devices.data() ), "Failed to Enumerate Physical Devices" );

	for ( const auto& device : devices )
	{
		if ( VK_SuitableCard( device ) )
		{
			gPhysicalDevice = device;
			vkGetPhysicalDeviceProperties( gPhysicalDevice, &gPhysicalDeviceProperties );

			// SetOption( "MSAA Samples", aSampleCount );
			break;
		}
	}

	if ( gPhysicalDevice == VK_NULL_HANDLE )
		Log_Fatal( "Failed to find a suitable GPU!" );
}


void VK_CreateDevice()
{
	float queuePriority = 1.0f;
	u32   graphics, present;
	VK_FindQueueFamilies( gPhysicalDevice, &graphics, &present );

	std::set< u32 >                        uniqueQueueFamilies = { graphics, present };
	std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;

	for ( uint32_t queueFamily : uniqueQueueFamilies )
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {
			.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = queueFamily,
			.queueCount       = 1,
			.pQueuePriorities = &queuePriority,
		};

		queueCreateInfos.push_back( queueCreateInfo );
	}

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT indexing{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT };
	indexing.pNext                                    = nullptr;
	indexing.descriptorBindingPartiallyBound          = VK_TRUE;
	indexing.runtimeDescriptorArray                   = VK_TRUE;
	indexing.descriptorBindingVariableDescriptorCount = VK_TRUE;

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.sampleRateShading = VK_TRUE;
	deviceFeatures.depthBiasClamp    = VK_TRUE;
	deviceFeatures.depthClamp        = VK_TRUE;
	deviceFeatures.wideLines         = VK_TRUE;
	deviceFeatures.fillModeNonSolid  = VK_TRUE;

	VkDeviceCreateInfo createInfo    = {
		   .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		   .pNext                   = &indexing,
		   .flags                   = 0,
		   .queueCreateInfoCount    = (uint32_t)queueCreateInfos.size(),
		   .pQueueCreateInfos       = queueCreateInfos.data(),
		   .enabledLayerCount       = (uint32_t)ARR_SIZE( gpValidationLayers ),
		   .ppEnabledLayerNames     = gpValidationLayers,
		   .enabledExtensionCount   = (uint32_t)ARR_SIZE( gpDeviceExtensions ),
		   .ppEnabledExtensionNames = gpDeviceExtensions,
		   .pEnabledFeatures        = &deviceFeatures,
	};

	VK_CheckResult( vkCreateDevice( gPhysicalDevice, &createInfo, NULL, &gDevice ), "Failed to create logical device!" );

	vkGetDeviceQueue( gDevice, graphics, 0, &gGraphicsQueue );
	vkGetDeviceQueue( gDevice, present, 0, &gPresentQueue );

#if _DEBUG
	pfnSetDebugUtilsObjectName    = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkSetDebugUtilsObjectNameEXT" );
	pfnSetDebugUtilsObjectTag     = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkSetDebugUtilsObjectTagEXT" );

	pfnQueueBeginDebugUtilsLabel  = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkQueueBeginDebugUtilsLabelEXT" );
	pfnQueueEndDebugUtilsLabel    = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkQueueEndDebugUtilsLabelEXT" );
	pfnQueueInsertDebugUtilsLabel = (PFN_vkQueueInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkQueueInsertDebugUtilsLabelEXT" );

	pfnCmdBeginDebugUtilsLabel    = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkCmdBeginDebugUtilsLabelEXT" );
	pfnCmdEndDebugUtilsLabel      = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkCmdEndDebugUtilsLabelEXT" );
	pfnCmdInsertDebugUtilsLabel   = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkCmdInsertDebugUtilsLabelEXT" );

	if ( pfnSetDebugUtilsObjectName )
		Log_Dev( gVulkanChannel, 1, "Loaded PFN_vkSetDebugUtilsObjectNameEXT\n" );

	if ( pfnSetDebugUtilsObjectTag )
		Log_Dev( gVulkanChannel, 1, "Loaded PFN_vkSetDebugUtilsObjectTagEXT\n" );
#endif
}


VkInstance VK_GetInstance()
{
	return gInstance;
}


VkSurfaceKHR VK_GetSurface()
{
	return gSurface;
}


VkDevice VK_GetDevice()
{
	return gDevice;
}


VkPhysicalDevice VK_GetPhysicalDevice()
{
	return gPhysicalDevice;
}


VkQueue VK_GetGraphicsQueue()
{
	return gGraphicsQueue;
}


VkQueue VK_GetPresentQueue()
{
	return gPresentQueue;
}


VkSampleCountFlags VK_GetMaxMSAASamples()
{
	return gPhysicalDeviceProperties.limits.framebufferColorSampleCounts & gPhysicalDeviceProperties.limits.framebufferDepthSampleCounts;
}


VkSampleCountFlagBits VK_FindMaxMSAASamples()
{
	VkSampleCountFlags counts = gPhysicalDeviceProperties.limits.framebufferColorSampleCounts & gPhysicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if ( counts & VK_SAMPLE_COUNT_64_BIT ) return VK_SAMPLE_COUNT_64_BIT;
	if ( counts & VK_SAMPLE_COUNT_32_BIT ) return VK_SAMPLE_COUNT_32_BIT;
	if ( counts & VK_SAMPLE_COUNT_16_BIT ) return VK_SAMPLE_COUNT_16_BIT;
	if ( counts & VK_SAMPLE_COUNT_8_BIT ) return VK_SAMPLE_COUNT_8_BIT;
	if ( counts & VK_SAMPLE_COUNT_4_BIT ) return VK_SAMPLE_COUNT_4_BIT;
	if ( counts & VK_SAMPLE_COUNT_2_BIT ) return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}


const VkPhysicalDeviceProperties& VK_GetPhysicalDeviceProperties()
{
	return gPhysicalDeviceProperties;
}

