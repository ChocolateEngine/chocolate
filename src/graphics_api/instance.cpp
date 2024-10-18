#include "core/platform.h"
#include "core/log.h"
#include "core/commandline.h"
#include "util.h"

#include "render/irender.h"
#include "render_vk.h"

#include <SDL_vulkan.h>

#include <set>


static bool gListExts    = Args_Register( false, "List All Vulkan Extensions, marking what ones are loaded", "-vk-list-exts" );
static bool gListQueues  = Args_Register( false, "List All Device Queues", "-vk-list-queues" );
static int  gDeviceIndex = Args_Register( -1, "Manually select a GPU by the index in the device list", "-gpu" );
static bool gListDevices = Args_RegisterF( false, "List Graphics Cards detected", 2, "-gpus", "-list-gpus" );
static int  gDebugUtils  = Args_Register( true, "Vulkan Debug Tools", "-vk-no-debug" );


#if _DEBUG
CONVAR_BOOL( vk_debug_messages, 1, "" );
#else
CONVAR_BOOL( vk_debug_messages, 0, "" );
#endif


// #error HAVE A LAUNCH OPTION TO DISABLE VALIDATION LAYERS IN DEBUG BUILDS


#ifdef NDEBUG
	constexpr bool        gEnableValidationLayers = false;

	constexpr bool        vk_verbose              = false;
	constexpr bool        vk_formatted            = false;
#else
	bool                  gEnableValidationLayers = Args_Register( false, "Enable Vulkan Validation Layers Extensions", "-vk-valid" );
	constexpr char const* gpValidationLayers[]    = { "VK_LAYER_KHRONOS_validation" };

	CONVAR_BOOL( vk_verbose, 0, "" );
	CONVAR_BOOL( vk_formatted, 1, "" );
#endif


constexpr char const* gpDeviceExtensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	"VK_EXT_descriptor_indexing",
#if _DEBUG
	// VK_EXT_DEVICE_MEMORY_REPORT_EXTENSION_NAME,
#if NV_CHECKPOINTS
	VK_NV_DEVICE_DIAGNOSTIC_CHECKPOINTS_EXTENSION_NAME,
#endif
#endif
};


PFN_vkSetDebugUtilsObjectNameEXT    pfnSetDebugUtilsObjectName    = nullptr;
PFN_vkSetDebugUtilsObjectTagEXT     pfnSetDebugUtilsObjectTag     = nullptr;

PFN_vkQueueBeginDebugUtilsLabelEXT  pfnQueueBeginDebugUtilsLabel  = nullptr;
PFN_vkQueueEndDebugUtilsLabelEXT    pfnQueueEndDebugUtilsLabel    = nullptr;
PFN_vkQueueInsertDebugUtilsLabelEXT pfnQueueInsertDebugUtilsLabel = nullptr;

PFN_vkCmdBeginDebugUtilsLabelEXT    pfnCmdBeginDebugUtilsLabel    = nullptr;
PFN_vkCmdEndDebugUtilsLabelEXT      pfnCmdEndDebugUtilsLabel      = nullptr;
PFN_vkCmdInsertDebugUtilsLabelEXT   pfnCmdInsertDebugUtilsLabel   = nullptr;

#if _DEBUG
#if NV_CHECKPOINTS
PFN_vkCmdSetCheckpointNV       pfnCmdSetCheckpointNV       = nullptr;
PFN_vkGetQueueCheckpointDataNV pfnGetQueueCheckpointDataNV = nullptr;
#endif
#endif


static VkInstance                        gInstance;
static VkDebugUtilsMessengerEXT          gLayers;
static VkPhysicalDevice                  gPhysicalDevice;
static VkDevice                          gDevice;
static VkQueue                           gGraphicsQueue;
static VkQueue                           gTransferQueue;

static VkPhysicalDeviceProperties        gPhysicalDeviceProperties{};

static VkSurfaceCapabilitiesKHR          gSurfaceCapabilities{};
static std::vector< VkSurfaceFormatKHR > gSwapFormats;
static std::vector< VkPresentModeKHR >   gSwapPresentModes;


// lengths to chop off from warnings:

// "Validation Performance Warning: "
int gVkStripPerf = 32;

// "Validation Error: "
int gVkStripError = 18;
// auto a = "Validation Error: [ VUID-vkFreeDescriptorSets-pDescriptorSets-00309 ] Object 0: handle = 0x7cd292000000004f, name = Light Point Set, type = VK_OBJECT_TYPE_DESCRIPTOR_SET; | MessageID = 0xbfce1114 | vkUpdateDescriptorSets() pDescriptorWrites[0] failed write update validation for VkDescriptorSet 0x7cd292000000004f[Light Point Set] with error: Cannot call vkUpdateDescriptorSets() to perform write update on VkDescriptorSet 0x7cd292000000004f[Light Point Set] allocated with VkDescriptorSetLayout 0x612f93000000004e[Light Point Layout] that is in use by a command buffer"

VKAPI_ATTR VkBool32 VKAPI_CALL VK_DebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
	if ( !gEnableValidationLayers || !vk_debug_messages )
		return VK_FALSE;

	const Log* log = Log_GetLastLog();

	// blech
	if ( log && log->aChannel != gLC_Vulkan )
		Log_Ex( gLC_Vulkan, ELogType_Raw, "\n" );

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
			Log_Dev( gLC_Vulkan, 1, formatted.c_str() );
	}

	else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT )
		Log_Error( gLC_Vulkan, formatted.c_str() );

	else if ( messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
		Log_Warn( gLC_Vulkan, formatted.c_str() );

	else
		Log_Msg( gLC_Vulkan, formatted.c_str() );

	return VK_FALSE;
}


bool VK_CheckValidationLayerSupport()
{
#if _DEBUG
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
			if ( ch_str_equals( layerName, layerProperties.layerName ) )
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
#else
	return false;
#endif
}


bool VK_CheckDebugUtilsSupport()
{
	bool         layerFound = false;
	unsigned int layerCount;
	VK_CheckResult( vkEnumerateInstanceLayerProperties( &layerCount, NULL ), "Failed to enumerate instance layer properties" );

	std::vector< VkLayerProperties > availableLayers( layerCount );
	VK_CheckResult( vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() ), "Failed to enumerate instance layer properties" );

	for ( const auto& layerProperties : availableLayers )
	{
		if ( ch_str_equals( VK_EXT_DEBUG_UTILS_EXTENSION_NAME, layerProperties.layerName ) )
		{
			return true;
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


VkResult VK_CreateDebugUtilsMessenger()
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( gInstance, "vkCreateDebugUtilsMessengerEXT" );
	if ( func != NULL )
		return func( gInstance, &gLayerInfo, nullptr, &gLayers );
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}


void VK_DestroyDebugUtilsMessenger()
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

	std::set< std::string_view > requiredExtensions( gpDeviceExtensions, gpDeviceExtensions + ARR_SIZE( gpDeviceExtensions ) );

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

	if ( requiredExtensions.empty() )
		return true;

	std::string errorMsg = "Failed to start renderer: Your device does not have these required Vulkan Extensions:";

	for ( std::string_view extension : requiredExtensions )
	{
		errorMsg += "\n    ";
		errorMsg += extension.data();
	}

	errorMsg += "\n";
	Log_Fatal( gLC_Render, errorMsg.c_str() );

	return false;
}


#if 0
std::vector< const char* > VK_GetSDL2Extensions()
{
	uint32_t extensionCount = 0;
	if ( !SDL_Vulkan_GetInstanceExtensions( gpWindow, &extensionCount, NULL ) )
		Log_Fatal( gLC_Render, "Unable to query the number of Vulkan instance extensions\n" );

	// Use the amount of extensions queried before to retrieve the names of the extensions.
	std::vector< const char* > extensions( extensionCount );

	if ( !SDL_Vulkan_GetInstanceExtensions( gpWindow, &extensionCount, extensions.data() ) )
		Log_Fatal( gLC_Render, "Unable to query the number of Vulkan instance extension names\n" );

	LogGroup group = Log_GroupBeginEx( gLC_Render, ELogType_Dev );

	// Display names
	Log_GroupF( group, "Found %d Vulkan extensions:\n", extensionCount );
	for ( uint32_t i = 0; i < extensionCount; ++i )
		Log_GroupF( group, "\t%i : %s\n", i, extensions[ i ] );

	Log_Group( group, "\n" );
	Log_GroupEnd( group );

	return extensions;
}
#endif


bool VK_CreateInstance()
{
	bool hasDebugUtils = VK_CheckDebugUtilsSupport();

#if _DEBUG
	bool hasValidation = gEnableValidationLayers;

	if ( hasValidation )
	{
		hasValidation = VK_CheckValidationLayerSupport();

		if ( !hasValidation )
			Log_Error( "Validation layers requested, but not available!\n" );
	}
#endif

	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName   = "Chocolate Engine";
	appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.pEngineName        = "Chocolate Engine Renderer";
	appInfo.engineVersion      = VK_MAKE_VERSION( 1, IRENDER_VER, 0 );
	appInfo.apiVersion         = VK_HEADER_VERSION_COMPLETE;

	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

#if _DEBUG
	if ( hasValidation )
	{
		createInfo.enabledLayerCount   = (unsigned int)ARR_SIZE( gpValidationLayers );
		createInfo.ppEnabledLayerNames = gpValidationLayers;
		createInfo.pNext               = (VkDebugUtilsMessengerCreateInfoEXT*)&gLayerInfo;
	}
	else
#endif
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

#ifdef _WIN32
	std::vector< const char* > sdlExt = { "VK_KHR_surface", "VK_KHR_win32_surface" };
#else
	// TODO: QUERY SDL EXTENSIONS INSTEAD !!!!!
	std::vector< const char* > sdlExt = { "VK_KHR_surface", "VK_KHR_xlib_surface" };
	// std::vector< const char* > sdlExt = VK_GetSDL2Extensions();
#endif

	// Add debug extension, we need this to relay debug messages
	if ( hasDebugUtils )
		sdlExt.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

	createInfo.enabledExtensionCount   = static_cast< uint32_t >( sdlExt.size() );
	createInfo.ppEnabledExtensionNames = sdlExt.data();

	VK_CheckResult( vkCreateInstance( &createInfo, NULL, &gInstance ), "Failed to create instance!" );

	if ( hasDebugUtils )
	{
		VkResult result = VK_CreateDebugUtilsMessenger();
		if ( result == VK_SUCCESS )
		{
			Log_Dev( gLC_Render, 1, "Successfully Created Debug Utils Messenger\n" );
		}
		else
		{
			Log_ErrorF( gLC_Render, "Failed to Create Debug Utils Messenger: %s\n", VKString( result ) );
		}
	}
	
	return true;
}


void VK_DestroyInstance()
{
	VK_DestroyDebugUtilsMessenger();
	vkDestroyDevice( gDevice, NULL );
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


// TODO: rethink this and check for compute and transfer queues (there could be multiple of each)
void VK_FindQueueFamilies( VkSurfaceKHR surface, const VkPhysicalDeviceProperties& srDeviceProps, VkPhysicalDevice sDevice, u32& srGraphics, u32& srTransfer )
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( sDevice, &queueFamilyCount, nullptr );

	if ( queueFamilyCount == 0 )
	{
		Log_FatalF( gLC_Render, "Device \"%s\" has no queue families???\n" );
		return;
	}

	auto queueFamilies = CH_STACK_NEW( VkQueueFamilyProperties, queueFamilyCount );

	if ( !queueFamilies )
	{
		Log_FatalF( gLC_Render, "Failed to stack allocate queue family properties array\n" );
		return;
	}

	vkGetPhysicalDeviceQueueFamilyProperties( sDevice, &queueFamilyCount, queueFamilies );  // Logic to find queue family indices to populate struct with

	// *spPresent  = UINT32_MAX;
	// *spGraphics = UINT32_MAX;

	// hack, as this is called multiple times for some reason
	static bool listedQueues = false;

	if ( gListQueues && !listedQueues )
	{
		listedQueues          = true;
		std::string queueDump = vstring( "Device \"%s\" has %zd Queues\n", srDeviceProps.deviceName, queueFamilyCount );

		for ( u32 queueIndex = 0; queueIndex < queueFamilyCount; queueIndex++ )
		{
			const VkQueueFamilyProperties& queueFamily = queueFamilies[ queueIndex ];

			queueDump += vstring( "Queue %zd:\n    Count: %zd\n    Supports Present: ", queueIndex, queueFamily.queueCount );

			// TODO: Create the device upon the first window creation
			// Then, use that same device for all new windows and make sure it's compatible

			VkBool32 presentSupport = false;
			VK_CheckResult( vkGetPhysicalDeviceSurfaceSupportKHR( sDevice, queueIndex, surface, &presentSupport ),
			                "Failed to Get Physical Device Surface Support" );

			queueDump += vstring( "%s\n    Flags:\n", presentSupport ? "Yes" : "No" );

			if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
				queueDump += "        VK_QUEUE_GRAPHICS_BIT\n";

			if ( queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT )
				queueDump += "        VK_QUEUE_COMPUTE_BIT\n";

			if ( queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT )
				queueDump += "        VK_QUEUE_TRANSFER_BIT\n";

			if ( queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT )
				queueDump += "        VK_QUEUE_SPARSE_BINDING_BIT\n";

			if ( queueFamily.queueFlags & VK_QUEUE_PROTECTED_BIT )
				queueDump += "        VK_QUEUE_PROTECTED_BIT\n";

			queueDump += "\n";
		}

		Log_Dev( gLC_Render, 1, queueDump.c_str() );
	}

	for ( u32 queueIndex = 0; queueIndex < queueFamilyCount; queueIndex++ )
	{
		const VkQueueFamilyProperties& queueFamily = queueFamilies[ queueIndex ];

		if ( queueFamily.queueCount == 0 )
			continue;

		VkBool32 presentSupport = false;
		VK_CheckResult( vkGetPhysicalDeviceSurfaceSupportKHR( sDevice, queueIndex, surface, &presentSupport ),
		                "Failed to Get Physical Device Surface Support" );

		if ( srGraphics == UINT32_MAX && queueFamily.queueFlags & ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) )
		{
			if ( !presentSupport )
				continue;

			srGraphics = queueIndex;
		}

		if ( queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && ( srTransfer == UINT32_MAX || srGraphics == srTransfer ) )
		{
			srTransfer = queueIndex;
		}
	}

	CH_STACK_FREE( queueFamilies );
}


VkSurfaceCapabilitiesKHR VK_GetSurfaceCapabilities()
{
	return gSurfaceCapabilities;
}


u32 VK_GetSurfaceImageCount()
{
	u32 imageCount = gSurfaceCapabilities.minImageCount;

	if ( gSurfaceCapabilities.maxImageCount > 0 && imageCount > gSurfaceCapabilities.maxImageCount )
		imageCount = gSurfaceCapabilities.maxImageCount;

	return imageCount;
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


VkExtent2D VK_ChooseSwapExtent( WindowVK* window )
{
	int width = 0, height = 0;
	SDL_GetWindowSize( window->window, &width, &height );

	VkExtent2D size{
		std::max( gSurfaceCapabilities.minImageExtent.width, std::min( gSurfaceCapabilities.maxImageExtent.width, (u32)width ) ),
		std::max( gSurfaceCapabilities.minImageExtent.height, std::min( gSurfaceCapabilities.maxImageExtent.height, (u32)height ) ),
	};

	return size;
}


void VK_UpdateSwapchainInfo( VkSurfaceKHR surface )
{
	if ( !gPhysicalDevice )
	{
		Log_Error( gLC_Render, "VK_UpdateSwapchainInfo(): No Physical Device?\n" );
		return;
	}

	VK_CheckResult( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( gPhysicalDevice, surface, &gSurfaceCapabilities ),
	                "Failed to Get Physical Device Surface Capabilities" );
}


bool VK_SetupSwapchainInfo( VkSurfaceKHR surf, VkPhysicalDevice sDevice )
{
	VK_CheckResult( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( sDevice, surf, &gSurfaceCapabilities ),
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


bool VK_SuitableCard( VkSurfaceKHR surface, const VkPhysicalDeviceProperties& srDeviceProps, VkPhysicalDevice sDevice )
{
	if ( !VK_CheckDeviceExtensionSupport( sDevice ) )
		return false;

	if ( !VK_SetupSwapchainInfo( surface, sDevice ) )
		return false;

	u32 graphics = UINT32_MAX, transfer = UINT32_MAX;
	VK_FindQueueFamilies( surface, srDeviceProps, sDevice, graphics, transfer );

	if ( graphics == UINT32_MAX || transfer == UINT32_MAX )
		return false;

	return true;
}


VkSurfaceKHR VK_CreateSurface( void* sSysWindow, SDL_Window* sSDLWindow )
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;

#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR surfCreateInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfCreateInfo.hwnd      = (HWND)sSysWindow;
	surfCreateInfo.hinstance = GetModuleHandle( NULL );
	surfCreateInfo.flags     = 0;
	surfCreateInfo.pNext     = NULL;

	VK_CheckResultE( vkCreateWin32SurfaceKHR( VK_GetInstance(), &surfCreateInfo, nullptr, &surface ), "Failed to create Surface" );
#else
	if ( !SDL_Vulkan_CreateSurface( sSDLWindow, VK_GetInstance(), &surface ) )
	{
		Log_ErrorF( gLC_GraphicsAPI, "Error: Failed to create SDL Vulkan Surface: %s\n", SDL_GetError() );
		return VK_NULL_HANDLE;
	}
#endif

	return surface;
}


void VK_DestroySurface( VkSurfaceKHR surface )
{
	vkDestroySurfaceKHR( gInstance, surface, NULL );
}


void VK_SelectDevice( const VkPhysicalDeviceProperties& srDeviceProps, const VkPhysicalDevice& srDevice )
{
	gPhysicalDevice = srDevice;
	vkGetPhysicalDeviceProperties( gPhysicalDevice, &gPhysicalDeviceProperties );

	// SetOption( "MSAA Samples", aSampleCount );

	if ( gPhysicalDevice == VK_NULL_HANDLE )
		Log_Fatal( "Failed to find a suitable GPU!" );

	Log_MsgF( gLC_Render, "Using GPU \"%s\"\n", gPhysicalDeviceProperties.deviceName );
}


void VK_ShowDeviceListMessageBox( SDL_MessageBoxFlags type, const char* message, VkSurfaceKHR surface, std::vector< VkPhysicalDevice >& devices )
{
	// Build a string list of devices available to us
	std::string deviceList = vstring( "%d Devices Found:\n\n", devices.size() );

	for ( size_t i = 0; i < devices.size(); i++ )
	{
		VkPhysicalDeviceProperties deviceProps;
		vkGetPhysicalDeviceProperties( devices[ i ], &deviceProps );

		bool suitable = VK_SuitableCard( surface, deviceProps, devices[ i ] );

		deviceList += vstring( "GPU %zd - %s%s\n", i, deviceProps.deviceName, suitable ? "" : " (Unsupported)" );
	}

	Log_Msg( deviceList.c_str() );
	SDL_ShowSimpleMessageBox( type, "GPU Device List", deviceList.c_str(), NULL );
}


bool VK_SetupPhysicalDevice( VkSurfaceKHR surface )
{
	gPhysicalDevice      = VK_NULL_HANDLE;
	uint32_t deviceCount = 0;

	VK_CheckResult( vkEnumeratePhysicalDevices( gInstance, &deviceCount, NULL ), "Failed to Enumerate Physical Devices" );
	if ( deviceCount == 0 )
	{
		Log_Fatal( gLC_Render, "Failed to find a GPU with Vulkan support!" );
	}

	std::vector< VkPhysicalDevice > devices( deviceCount );
	VK_CheckResult( vkEnumeratePhysicalDevices( gInstance, &deviceCount, devices.data() ), "Failed to Enumerate Physical Devices" );

	if ( gListDevices )
	{
		VK_ShowDeviceListMessageBox( SDL_MESSAGEBOX_INFORMATION, "GPU Device List", surface, devices );
		return false;
	}
	
	if ( gDeviceIndex > -1 )
	{
		// If we tried to select an invalid device index, dump all available devices to the user
		if ( gDeviceIndex >= devices.size() )
		{
			std::string message = vstring( "Only %zd GPU's Found, but user requested GPU index %d\n", devices.size(), gDeviceIndex );
			VK_ShowDeviceListMessageBox( SDL_MESSAGEBOX_ERROR, message.c_str(), surface, devices );
			return false;
		}
		else
		{
			VkPhysicalDeviceProperties deviceProps;
			vkGetPhysicalDeviceProperties( devices[ gDeviceIndex ], &deviceProps );

			if ( VK_SuitableCard( surface, deviceProps, devices[ gDeviceIndex ] ) )
			{
				VK_SelectDevice( deviceProps, devices[ gDeviceIndex ] );
				return true;
			}
		}
		
	}
	else
	{
		for ( const VkPhysicalDevice& device : devices )
		{
			VkPhysicalDeviceProperties deviceProps;
			vkGetPhysicalDeviceProperties( device, &deviceProps );

			if ( VK_SuitableCard( surface, deviceProps, device ) )
			{
				VK_SelectDevice( deviceProps, device );
				break;
			}
		}
	}

	return true;
}


void VK_CreateDevice( VkSurfaceKHR surface )
{
	if ( !gPhysicalDevice )
	{
		Log_Fatal( gLC_Render, "Failed to select a physical device???\n" );
		return;
	}

	Log_Dev( gLC_Render, 1, "Creating VkDevice\n" );

	float queuePriority = 1.0f;
	VK_FindQueueFamilies( surface, gPhysicalDeviceProperties, gPhysicalDevice, gGraphicsAPIData.aQueueFamilyGraphics, gGraphicsAPIData.aQueueFamilyTransfer );

	std::vector< VkDeviceQueueCreateInfo > queueCreateInfos;

	VkDeviceQueueCreateInfo                queueCreateInfo = {
					   .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					   .queueFamilyIndex = gGraphicsAPIData.aQueueFamilyGraphics,
					   .queueCount       = 1,
					   .pQueuePriorities = &queuePriority,
	};

	queueCreateInfos.push_back( queueCreateInfo );

	if ( gGraphicsAPIData.aQueueFamilyGraphics != gGraphicsAPIData.aQueueFamilyTransfer )
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {
			.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = gGraphicsAPIData.aQueueFamilyTransfer,
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
	deviceFeatures.samplerAnisotropy        = VK_TRUE;
	deviceFeatures.sampleRateShading        = VK_TRUE;
	deviceFeatures.depthBiasClamp           = VK_TRUE;
	deviceFeatures.depthClamp               = VK_TRUE;
	deviceFeatures.wideLines                = VK_TRUE;
	deviceFeatures.fillModeNonSolid         = VK_TRUE;

	VkDeviceCreateInfo createInfo    = {
		   .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		   .pNext                   = &indexing,
		   .flags                   = 0,
		   .queueCreateInfoCount    = (uint32_t)queueCreateInfos.size(),
		   .pQueueCreateInfos       = queueCreateInfos.data(),

#if _DEBUG
		   .enabledLayerCount       = (uint32_t)ARR_SIZE( gpValidationLayers ),
		   .ppEnabledLayerNames     = gpValidationLayers,
#endif

		   .enabledExtensionCount   = (uint32_t)ARR_SIZE( gpDeviceExtensions ),
		   .ppEnabledExtensionNames = gpDeviceExtensions,
		   .pEnabledFeatures        = &deviceFeatures,
	};

	VK_CheckResult( vkCreateDevice( gPhysicalDevice, &createInfo, NULL, &gDevice ), "Failed to create logical device!" );

	vkGetDeviceQueue( gDevice, gGraphicsAPIData.aQueueFamilyGraphics, 0, &gGraphicsQueue );
	vkGetDeviceQueue( gDevice, gGraphicsAPIData.aQueueFamilyTransfer, 0, &gTransferQueue );

	pfnSetDebugUtilsObjectName    = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkSetDebugUtilsObjectNameEXT" );
	pfnSetDebugUtilsObjectTag     = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkSetDebugUtilsObjectTagEXT" );

	pfnQueueBeginDebugUtilsLabel  = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkQueueBeginDebugUtilsLabelEXT" );
	pfnQueueEndDebugUtilsLabel    = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkQueueEndDebugUtilsLabelEXT" );
	pfnQueueInsertDebugUtilsLabel = (PFN_vkQueueInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkQueueInsertDebugUtilsLabelEXT" );

	pfnCmdBeginDebugUtilsLabel    = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkCmdBeginDebugUtilsLabelEXT" );
	pfnCmdEndDebugUtilsLabel      = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkCmdEndDebugUtilsLabelEXT" );
	pfnCmdInsertDebugUtilsLabel   = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr( VK_GetInstance(), "vkCmdInsertDebugUtilsLabelEXT" );

	if ( pfnSetDebugUtilsObjectName )
		Log_Dev( gLC_Vulkan, 2, "Loaded PFN_vkSetDebugUtilsObjectNameEXT\n" );

	if ( pfnSetDebugUtilsObjectTag )
		Log_Dev( gLC_Vulkan, 2, "Loaded PFN_vkSetDebugUtilsObjectTagEXT\n" );
	
#if _DEBUG && NV_CHECKPOINTS
	pfnCmdSetCheckpointNV       = (PFN_vkCmdSetCheckpointNV)vkGetInstanceProcAddr( VK_GetInstance(), "vkCmdSetCheckpointNV" );
	pfnGetQueueCheckpointDataNV = (PFN_vkGetQueueCheckpointDataNV)vkGetInstanceProcAddr( VK_GetInstance(), "vkGetQueueCheckpointDataNV" );
#endif

	VK_SetObjectName( VK_OBJECT_TYPE_PHYSICAL_DEVICE, (u64)gPhysicalDevice, gPhysicalDeviceProperties.deviceName );
	VK_SetObjectName( VK_OBJECT_TYPE_DEVICE, (u64)gDevice, gPhysicalDeviceProperties.deviceName );

	VK_SetObjectName( VK_OBJECT_TYPE_QUEUE, (u64)gGraphicsQueue, "Graphics" );
	VK_SetObjectName( VK_OBJECT_TYPE_QUEUE, (u64)gTransferQueue, "Transfer" );
}


VkInstance VK_GetInstance()
{
	return gInstance;
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


VkQueue VK_GetTransferQueue()
{
	return gTransferQueue;
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


const char* VK_SamplesStr( VkSampleCountFlags counts )
{
	if ( counts & VK_SAMPLE_COUNT_64_BIT ) return "VK_SAMPLE_COUNT_64_BIT";
	if ( counts & VK_SAMPLE_COUNT_32_BIT ) return "VK_SAMPLE_COUNT_32_BIT";
	if ( counts & VK_SAMPLE_COUNT_16_BIT ) return "VK_SAMPLE_COUNT_16_BIT";
	if ( counts & VK_SAMPLE_COUNT_8_BIT ) return "VK_SAMPLE_COUNT_8_BIT";
	if ( counts & VK_SAMPLE_COUNT_4_BIT ) return "VK_SAMPLE_COUNT_4_BIT";
	if ( counts & VK_SAMPLE_COUNT_2_BIT ) return "VK_SAMPLE_COUNT_2_BIT";

	return "VK_SAMPLE_COUNT_1_BIT";
}


const VkPhysicalDeviceProperties& VK_GetPhysicalDeviceProperties()
{
	return gPhysicalDeviceProperties;
}


const VkPhysicalDeviceLimits& VK_GetPhysicalDeviceLimits()
{
	return gPhysicalDeviceProperties.limits;
}


CONCMD( vk_device_info )
{
	if ( !gDevice )
	{
		Log_Error( gLC_Vulkan, "vk_device_info: No Device Loaded yet!\n" );
		return;
	}

	LogGroup group = Log_GroupBegin( gLC_Vulkan );

	Log_GroupF( group, "Device Name:    %s\n", gPhysicalDeviceProperties.deviceName );

	// deviceType
	switch ( gPhysicalDeviceProperties.deviceType )
	{
		case VK_PHYSICAL_DEVICE_TYPE_OTHER:
			Log_Group( group, "Device Type:    Other\n" );
			break;

		case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
			Log_Group( group, "Device Type:    Integrated GPU\n" );
			break;

		case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
			Log_Group( group, "Device Type:    Discrete GPU\n" );
			break;

		case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
			Log_Group( group, "Device Type:    Virtual GPU\n" );
			break;

		case VK_PHYSICAL_DEVICE_TYPE_CPU:
			Log_Group( group, "Device Type:    CPU\n" );
			break;
	}

	Log_GroupF( group, "API Version:    %zd\n", gPhysicalDeviceProperties.apiVersion );
	Log_GroupF( group, "Driver Version: %zd\n", gPhysicalDeviceProperties.driverVersion );
	Log_GroupF( group, "Device ID:      %zd\n", gPhysicalDeviceProperties.deviceID );
	Log_GroupF( group, "Vendor ID:      %zd\n", gPhysicalDeviceProperties.vendorID );

	Log_GroupEnd( group );
}


// hefty function
CONCMD_VA( vk_dump_limits, "Dump Device Limits and current usage of those limits if applicable\n" )
{
	if ( !gDevice )
	{
		Log_Error( gLC_Vulkan, "vk_dump_limits: No Device Loaded yet!\n" );
		return;
	}

	LogGroup group = Log_GroupBegin( gLC_Vulkan );

	VkPhysicalDeviceLimits& limits = gPhysicalDeviceProperties.limits;

	// general min/max values
	Log_GroupF( group, "maxImageDimension1D:             %zd\n", limits.maxImageDimension1D );
	Log_GroupF( group, "maxImageDimension2D:             %zd\n", limits.maxImageDimension2D );
	Log_GroupF( group, "maxImageDimension3D:             %zd\n", limits.maxImageDimension3D );
	Log_GroupF( group, "maxImageDimensionCube:           %zd\n", limits.maxImageDimensionCube );
	Log_GroupF( group, "maxImageArrayLayers:             %zd\n", limits.maxImageArrayLayers );
	Log_GroupF( group, "maxTexelBufferElements:          %zd\n", limits.maxTexelBufferElements );
	Log_GroupF( group, "maxUniformBufferRange:           %zd\n", limits.maxUniformBufferRange );
	Log_GroupF( group, "maxStorageBufferRange:           %zd\n", limits.maxStorageBufferRange );
	Log_GroupF( group, "maxPushConstantsSize:            %zd\n", limits.maxPushConstantsSize );  // TODO: VULKAN_TRACKING
	Log_GroupF( group, "maxMemoryAllocationCount:        %zd\n", limits.maxMemoryAllocationCount );
	Log_GroupF( group, "maxSamplerAllocationCount:       %zd\n", limits.maxSamplerAllocationCount );  // TODO: VULKAN_TRACKING
	Log_GroupF( group, "maxColorAttachments:             %zd\n", limits.maxColorAttachments );
	Log_GroupF( group, "maxClipDistances:                %zd\n", limits.maxClipDistances );
	Log_GroupF( group, "maxCullDistances:                %zd\n", limits.maxCullDistances );
	Log_GroupF( group, "maxCombinedClipAndCullDistances: %zd\n\n", limits.maxCombinedClipAndCullDistances );

	Log_GroupF( group, "maxBoundDescriptorSets:                %zd\n", limits.maxBoundDescriptorSets );
	Log_GroupF( group, "maxPerStageDescriptorSamplers:         %zd\n", limits.maxPerStageDescriptorSamplers );
	Log_GroupF( group, "maxPerStageDescriptorUniformBuffers:   %zd\n", limits.maxPerStageDescriptorUniformBuffers );
	Log_GroupF( group, "maxPerStageDescriptorStorageBuffers:   %zd\n", limits.maxPerStageDescriptorStorageBuffers );
	Log_GroupF( group, "maxPerStageDescriptorSampledImages:    %zd\n", limits.maxPerStageDescriptorSampledImages );
	Log_GroupF( group, "maxPerStageDescriptorStorageImages:    %zd\n", limits.maxPerStageDescriptorStorageImages );
	Log_GroupF( group, "maxPerStageDescriptorInputAttachments: %zd\n", limits.maxPerStageDescriptorInputAttachments );
	Log_GroupF( group, "maxPerStageResources:                  %zd\n\n", limits.maxPerStageResources );

	Log_GroupF( group, "maxDescriptorSetSamplers:              %zd\n", limits.maxDescriptorSetSamplers );
	Log_GroupF( group, "maxDescriptorSetUniformBuffers:        %zd\n", limits.maxDescriptorSetUniformBuffers );
	Log_GroupF( group, "maxDescriptorSetUniformBuffersDynamic: %zd\n", limits.maxDescriptorSetUniformBuffersDynamic );
	Log_GroupF( group, "maxDescriptorSetStorageBuffers:        %zd\n", limits.maxDescriptorSetStorageBuffers );
	Log_GroupF( group, "maxDescriptorSetStorageBuffersDynamic: %zd\n", limits.maxDescriptorSetStorageBuffersDynamic );
	Log_GroupF( group, "maxDescriptorSetSampledImages:         %zd\n", limits.maxDescriptorSetSampledImages );
	Log_GroupF( group, "maxDescriptorSetStorageImages:         %zd\n", limits.maxDescriptorSetStorageImages );
	Log_GroupF( group, "maxDescriptorSetInputAttachments:      %zd\n\n", limits.maxDescriptorSetInputAttachments );

	Log_GroupF( group, "maxVertexInputAttributes:      %zd\n", limits.maxVertexInputAttributes );
	Log_GroupF( group, "maxVertexInputBindings:        %zd\n", limits.maxVertexInputBindings );
	Log_GroupF( group, "maxVertexInputAttributeOffset: %zd\n", limits.maxVertexInputAttributeOffset );
	Log_GroupF( group, "maxVertexInputBindingStride:   %zd\n", limits.maxVertexInputBindingStride );
	Log_GroupF( group, "maxVertexOutputComponents:     %zd\n\n", limits.maxVertexOutputComponents );

	Log_GroupF( group, "maxTessellationGenerationLevel:                  %zd\n", limits.maxTessellationGenerationLevel );
	Log_GroupF( group, "maxTessellationPatchSize:                        %zd\n", limits.maxTessellationPatchSize );
	Log_GroupF( group, "maxTessellationControlPerVertexInputComponents:  %zd\n", limits.maxTessellationControlPerVertexInputComponents );
	Log_GroupF( group, "maxTessellationControlPerVertexOutputComponents: %zd\n", limits.maxTessellationControlPerVertexOutputComponents );
	Log_GroupF( group, "maxTessellationControlPerPatchOutputComponents:  %zd\n", limits.maxTessellationControlPerPatchOutputComponents );
	Log_GroupF( group, "maxTessellationControlTotalOutputComponents:     %zd\n", limits.maxTessellationControlTotalOutputComponents );
	Log_GroupF( group, "maxTessellationEvaluationInputComponents:        %zd\n", limits.maxTessellationEvaluationInputComponents );
	Log_GroupF( group, "maxTessellationEvaluationOutputComponents:       %zd\n\n", limits.maxTessellationEvaluationOutputComponents );

	Log_GroupF( group, "maxGeometryShaderInvocations:     %zd\n", limits.maxGeometryShaderInvocations );
	Log_GroupF( group, "maxGeometryInputComponents:       %zd\n", limits.maxGeometryInputComponents );
	Log_GroupF( group, "maxGeometryOutputComponents:      %zd\n", limits.maxGeometryOutputComponents );
	Log_GroupF( group, "maxGeometryOutputVertices:        %zd\n", limits.maxGeometryOutputVertices );
	Log_GroupF( group, "maxGeometryTotalOutputComponents: %zd\n\n", limits.maxGeometryTotalOutputComponents );

	Log_GroupF( group, "maxFragmentInputComponents:         %zd\n", limits.maxFragmentInputComponents );
	Log_GroupF( group, "maxFragmentOutputAttachments:       %zd\n", limits.maxFragmentOutputAttachments );
	Log_GroupF( group, "maxFragmentDualSrcAttachments:      %zd\n", limits.maxFragmentDualSrcAttachments );
	Log_GroupF( group, "maxFragmentCombinedOutputResources: %zd\n\n", limits.maxFragmentCombinedOutputResources );

	Log_GroupF( group, "maxComputeSharedMemorySize:     %zd\n", limits.maxComputeSharedMemorySize );
	Log_GroupF( group, "maxComputeWorkGroupCount:       %zd, %zd, %zd\n", limits.maxComputeWorkGroupCount[ 0 ], limits.maxComputeWorkGroupCount[ 1 ], limits.maxComputeWorkGroupCount[ 2 ] );
	Log_GroupF( group, "maxComputeWorkGroupInvocations: %zd\n", limits.maxComputeWorkGroupInvocations );
	Log_GroupF( group, "maxComputeWorkGroupSize:        %zd, %zd, %zd\n\n", limits.maxComputeWorkGroupSize[ 0 ], limits.maxComputeWorkGroupSize[ 1 ], limits.maxComputeWorkGroupSize[ 2 ] );

	Log_GroupF( group, "subPixelPrecisionBits:    %zd\n", limits.subPixelPrecisionBits );
	Log_GroupF( group, "subTexelPrecisionBits:    %zd\n", limits.subTexelPrecisionBits );
	Log_GroupF( group, "mipmapPrecisionBits:      %zd\n", limits.mipmapPrecisionBits );
	Log_GroupF( group, "maxDrawIndexedIndexValue: %zd\n", limits.maxDrawIndexedIndexValue );  // TODO: VULKAN_TRACKING
	Log_GroupF( group, "maxDrawIndirectCount:     %zd\n", limits.maxDrawIndirectCount );      // TODO: VULKAN_TRACKING
	Log_GroupF( group, "maxSamplerLodBias:        %.6f\n", limits.maxSamplerLodBias );
	Log_GroupF( group, "maxSamplerAnisotropy:     %.6f\n\n", limits.maxSamplerAnisotropy );

	Log_GroupF( group, "maxViewports:          %zd\n", limits.maxViewports );
	Log_GroupF( group, "maxViewportDimensions: %zd, %zd\n", limits.maxViewportDimensions[ 0 ], limits.maxViewportDimensions[ 1 ] );
	Log_GroupF( group, "viewportBoundsRange:   %.6f, %.6f\n", limits.viewportBoundsRange[ 0 ], limits.viewportBoundsRange[ 1 ] );
	Log_GroupF( group, "viewportSubPixelBits:  %zd\n\n", limits.viewportSubPixelBits );

	Log_GroupF( group, "minMemoryMapAlignment:           %zd\n", limits.minMemoryMapAlignment );
	Log_GroupF( group, "minTexelBufferOffsetAlignment:   %zd\n", limits.minTexelBufferOffsetAlignment );
	Log_GroupF( group, "minUniformBufferOffsetAlignment: %zd\n", limits.minUniformBufferOffsetAlignment );
	Log_GroupF( group, "minStorageBufferOffsetAlignment: %zd\n", limits.minStorageBufferOffsetAlignment );
	Log_GroupF( group, "minTexelOffset:                  %zd\n", limits.minTexelOffset );
	Log_GroupF( group, "maxTexelOffset:                  %zd\n", limits.maxTexelOffset );
	Log_GroupF( group, "minTexelGatherOffset:            %zd\n", limits.minTexelGatherOffset );
	Log_GroupF( group, "maxTexelGatherOffset:            %zd\n", limits.maxTexelGatherOffset );
	Log_GroupF( group, "minInterpolationOffset:          %.6f\n", limits.minInterpolationOffset );
	Log_GroupF( group, "maxInterpolationOffset:          %.6f\n", limits.maxInterpolationOffset );
	Log_GroupF( group, "subPixelInterpolationOffsetBits: %zd\n", limits.subPixelInterpolationOffsetBits );
	Log_GroupF( group, "maxFramebufferWidth:             %zd\n", limits.maxFramebufferWidth );
	Log_GroupF( group, "maxFramebufferHeight:            %zd\n", limits.maxFramebufferHeight );
	Log_GroupF( group, "maxFramebufferLayers:            %zd\n\n", limits.maxFramebufferLayers );

	// FLAGS
	Log_GroupF( group, "framebufferColorSampleCounts:         %s\n", VK_SamplesStr( limits.framebufferColorSampleCounts ) );
	Log_GroupF( group, "framebufferDepthSampleCounts:         %s\n", VK_SamplesStr( limits.framebufferDepthSampleCounts ) );
	Log_GroupF( group, "framebufferStencilSampleCounts:       %s\n", VK_SamplesStr( limits.framebufferStencilSampleCounts ) );
	Log_GroupF( group, "framebufferNoAttachmentsSampleCounts: %s\n", VK_SamplesStr( limits.framebufferNoAttachmentsSampleCounts ) );
	Log_GroupF( group, "sampledImageColorSampleCounts:        %s\n", VK_SamplesStr( limits.sampledImageColorSampleCounts ) );
	Log_GroupF( group, "sampledImageIntegerSampleCounts:      %s\n", VK_SamplesStr( limits.sampledImageIntegerSampleCounts ) );
	Log_GroupF( group, "sampledImageDepthSampleCounts:        %s\n", VK_SamplesStr( limits.sampledImageDepthSampleCounts ) );
	Log_GroupF( group, "sampledImageStencilSampleCounts:      %s\n", VK_SamplesStr( limits.sampledImageStencilSampleCounts ) );
	Log_GroupF( group, "storageImageSampleCounts:             %s\n\n", VK_SamplesStr( limits.storageImageSampleCounts ) );

	Log_GroupF( group, "maxSampleMaskWords:                 %zd\n", limits.maxSampleMaskWords );
	Log_GroupF( group, "timestampComputeAndGraphics:        %zd\n", limits.timestampComputeAndGraphics );
	Log_GroupF( group, "timestampPeriod:                    %.6f\n", limits.timestampPeriod );
	Log_GroupF( group, "discreteQueuePriorities:            %zd\n", limits.discreteQueuePriorities );
	Log_GroupF( group, "pointSizeRange:                     %.6f, %.6f\n", limits.pointSizeRange[ 0 ], limits.pointSizeRange[ 1 ] );
	Log_GroupF( group, "lineWidthRange:                     %.6f, %.6f\n", limits.lineWidthRange[ 0 ], limits.lineWidthRange[ 1 ] );
	Log_GroupF( group, "pointSizeGranularity:               %.6\n", limits.pointSizeGranularity );
	Log_GroupF( group, "lineWidthGranularity:               %.6\n", limits.lineWidthGranularity );
	Log_GroupF( group, "strictLines:                        %s\n", limits.strictLines ? "TRUE" : "FALSE" );
	Log_GroupF( group, "standardSampleLocations:            %s\n", limits.standardSampleLocations ? "TRUE" : "FALSE" );
	Log_GroupF( group, "optimalBufferCopyOffsetAlignment:   %zd\n", limits.optimalBufferCopyOffsetAlignment );
	Log_GroupF( group, "optimalBufferCopyRowPitchAlignment: %zd\n", limits.optimalBufferCopyRowPitchAlignment );
	Log_GroupF( group, "nonCoherentAtomSize:                %zd\n", limits.nonCoherentAtomSize );
	Log_GroupF( group, "bufferImageGranularity:             %zd\n", limits.bufferImageGranularity );
	Log_GroupF( group, "sparseAddressSpaceSize:             %zd\n", limits.sparseAddressSpaceSize );

	Log_GroupEnd( group );
}

