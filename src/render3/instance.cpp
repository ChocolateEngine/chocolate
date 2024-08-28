// --------------------------------------------------------------------------------------------
// Vulkan Instance and Device creation
// --------------------------------------------------------------------------------------------

#ifdef _WIN32
  #define VK_USE_PLATFORM_WIN32_KHR 1
#endif

#include "render.h"


static bool                         g_no_debug_utils        = Args_Register( false, "List All Vulkan Extensions, marking what ones are loaded", "--vk-no-debug-utils" );
static bool                         g_list_exts             = Args_Register( false, "List All Vulkan Extensions, marking what ones are loaded", "--vk-list-exts" );
static bool                         g_list_queues           = Args_Register( false, "List All Vulkan Device Queues", "--vk-list-queues" );
static bool                         g_list_devices          = Args_RegisterF( false, "List Graphics Cards detected", 2, "--gpus", "--list-gpus" );
static int                          g_device_index          = Args_Register( -1, "Manually select a GPU by the index in the device list", "--gpu" );

static bool                         g_has_debug_utils       = false;

VkInstance                          g_vk_instance           = VK_NULL_HANDLE;
VkDevice                            g_vk_device             = VK_NULL_HANDLE;
VkPhysicalDevice                    g_vk_physical_device    = VK_NULL_HANDLE;

VkQueue                             g_queue_graphics        = VK_NULL_HANDLE;
VkQueue                             g_queue_transfer        = VK_NULL_HANDLE;

u32                                 g_queue_family_graphics = UINT32_MAX;
u32                                 g_queue_family_transfer = UINT32_MAX;

VkPhysicalDeviceProperties          g_device_properties{};


// function pointers for debug utils
static VkDebugUtilsMessengerEXT     g_debug_messenger_ext;

PFN_vkSetDebugUtilsObjectNameEXT    pfnSetDebugUtilsObjectName    = nullptr;
PFN_vkSetDebugUtilsObjectTagEXT     pfnSetDebugUtilsObjectTag     = nullptr;

PFN_vkQueueBeginDebugUtilsLabelEXT  pfnQueueBeginDebugUtilsLabel  = nullptr;
PFN_vkQueueEndDebugUtilsLabelEXT    pfnQueueEndDebugUtilsLabel    = nullptr;
PFN_vkQueueInsertDebugUtilsLabelEXT pfnQueueInsertDebugUtilsLabel = nullptr;

PFN_vkCmdBeginDebugUtilsLabelEXT    pfnCmdBeginDebugUtilsLabel    = nullptr;
PFN_vkCmdEndDebugUtilsLabelEXT      pfnCmdEndDebugUtilsLabel      = nullptr;
PFN_vkCmdInsertDebugUtilsLabelEXT   pfnCmdInsertDebugUtilsLabel   = nullptr;


constexpr char const* g_device_extensions[] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	"VK_EXT_descriptor_indexing",
#if _DEBUG
// VK_EXT_DEVICE_MEMORY_REPORT_EXTENSION_NAME,
#endif
};


extern void vk_swapchain_setup_info( device_info_t& info );


#ifdef NDEBUG
	constexpr bool        g_use_validation_layers = false;

	constexpr bool        r_vk_verbose            = false;
	constexpr bool        r_vk_formatted          = false;
#else
	bool                  g_use_validation_layers = Args_Register( false, "Enable Vulkan Validation Layers Extensions", "-vk-valid" );
	constexpr char const* g_validation_layers[]   = { "VK_LAYER_KHRONOS_validation" };
	static bool           g_has_validation        = false;

	CONVAR_EX_BOOL( r_vk_debug_messages, "r.vk.debug.enabled", true );
	CONVAR_EX_BOOL( r_vk_verbose, "r.vk.debug.verbose", true );
	CONVAR_EX_BOOL( r_vk_formatted, "r.vk.debug.formatted", true );
#endif



// --------------------------------------------------------------------------------------------


// lengths to chop off from warnings:

// "Validation Performance Warning: "
int                            gVkStripPerf  = 32;

// "Validation Error: "
int                            gVkStripError = 18;
// auto a = "Validation Error: [ VUID-vkFreeDescriptorSets-pDescriptorSets-00309 ] Object 0: handle = 0x7cd292000000004f, name = Light Point Set, type = VK_OBJECT_TYPE_DESCRIPTOR_SET; | MessageID = 0xbfce1114 | vkUpdateDescriptorSets() pDescriptorWrites[0] failed write update validation for VkDescriptorSet 0x7cd292000000004f[Light Point Set] with error: Cannot call vkUpdateDescriptorSets() to perform write update on VkDescriptorSet 0x7cd292000000004f[Light Point Set] allocated with VkDescriptorSetLayout 0x612f93000000004e[Light Point Layout] that is in use by a command buffer"

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData )
{
	if ( !g_use_validation_layers || !r_vk_debug_messages )
		return VK_FALSE;

	const Log* log = Log_GetLastLog();

	// blech
	if ( log && log->aChannel != gLC_Vulkan )
		Log_Ex( gLC_Vulkan, LogType::Raw, "\n" );

	std::string formatted;

	if ( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT )
		vstring( formatted, "Validation: %s\n\n", pCallbackData->pMessage + gVkStripError );

	else if ( messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT )
		vstring( formatted, "Performance: %s\n\n", pCallbackData->pMessage + gVkStripPerf );

	else
		vstring( formatted, "%s\n\n", pCallbackData->pMessage );

	if ( r_vk_formatted )
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
		if ( r_vk_verbose )
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


// --------------------------------------------------------------------------------------------


constexpr VkDebugUtilsMessengerCreateInfoEXT g_debug_messenger_info = {
	.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
	.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
	.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
	.pfnUserCallback = vk_debug_callback,
};


static VkResult debug_utils_messenger_create()
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( g_vk_instance, "vkCreateDebugUtilsMessengerEXT" );
	if ( func != NULL )
		return func( g_vk_instance, &g_debug_messenger_info, nullptr, &g_debug_messenger_ext );
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}


void debug_utils_messenger_destroy()
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr( g_vk_instance, "vkDestroyDebugUtilsMessengerEXT" );
	if ( func != NULL )
		func( g_vk_instance, g_debug_messenger_ext, nullptr );
}


// --------------------------------------------------------------------------------------------
// Instance Setup
// --------------------------------------------------------------------------------------------


bool vk_instance_create()
{
#if _DEBUG
	// check instance layer support for validation layers
	u32 instance_layer_count;
	vk_check( vkEnumerateInstanceLayerProperties( &instance_layer_count, NULL ), "Failed to enumerate instance layer properties" );

	VkLayerProperties* layer_props = ch_malloc< VkLayerProperties >( instance_layer_count );
	vk_check( vkEnumerateInstanceLayerProperties( &instance_layer_count, layer_props ), "Failed to enumerate instance layer properties" );

	for ( u32 i = 0; i < instance_layer_count || g_has_validation; i++ )
	{
		// check for validation layers
		if ( ch_str_equals( "VK_LAYER_KHRONOS_validation", 27, layer_props[ i ].layerName ) )
		{
			g_has_validation = true;
			break;
		}
	}

	ch_free( layer_props );
#endif

	// create the instance
	VkApplicationInfo app_info{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	app_info.pApplicationName   = "Chocolate Engine";
	app_info.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
	app_info.pEngineName        = "Chocolate Engine Render 3";
	app_info.engineVersion      = VK_MAKE_VERSION( 3, CH_RENDER3_VER, 0 );
	// app_info.apiVersion         = VK_HEADER_VERSION_COMPLETE;
	app_info.apiVersion         = VK_MAKE_API_VERSION( 0, 1, 2, 0 );

	VkInstanceCreateInfo create_info{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	create_info.pApplicationInfo = &app_info;

#if _DEBUG
	if ( g_has_validation )
	{
		create_info.enabledLayerCount   = (unsigned int)ARR_SIZE( g_validation_layers );
		create_info.ppEnabledLayerNames = g_validation_layers;
		create_info.pNext               = (VkDebugUtilsMessengerCreateInfoEXT*)&g_debug_messenger_info;
	}
	else
#endif
	{
		create_info.enabledLayerCount = 0;
		create_info.pNext             = NULL;
	}

	u32 extension_count = 0;
	vkEnumerateInstanceExtensionProperties( NULL, &extension_count, NULL );

	VkExtensionProperties* ext_props = ch_malloc< VkExtensionProperties >( extension_count );
	vkEnumerateInstanceExtensionProperties( NULL, &extension_count, ext_props );

	if ( g_list_exts )
	{
		Log_MsgF( gLC_Render, "%d Instance extensions:\n", extension_count );

		for ( u32 i = 0; i < extension_count; i++ )
			Log_MsgF( gLC_Render, "    %s\n", ext_props[ i ].extensionName );

		Log_Msg( gLC_Render, "\n" );
	}

	ChVector< const char* > instance_exts( 2 );
	instance_exts[ 0 ] = "VK_KHR_surface";

#ifdef _WIN32
	instance_exts[ 1 ] = "VK_KHR_win32_surface";
#else
	// TODO: QUERY SDL EXTENSIONS INSTEAD !!!!! important if on wayland, i think at least
	instance_exts[ 1 ] = "VK_KHR_xlib_surface";
	// std::vector< const char* > instance_exts = VK_GetSDL2Extensions();
#endif

	bool instance_exts_found[ 2 ] = { false, false };

	// check for all extensions
	u32 found_extensions = 0;
	for ( u32 i = 0; i < extension_count; i++ )
	{
		for ( u32 j = 0; j < ARR_SIZE( instance_exts ); j++ )
		{
			if ( !instance_exts_found[ j ] && ch_str_equals( instance_exts[ j ], ext_props[ i ].extensionName ) )
			{
				found_extensions++;
				instance_exts_found[ j ] = true;
				break;
			}
		}

		// if we don't want debug utils, don't bother loading it
		if ( !g_no_debug_utils )
		{
			if ( !g_has_debug_utils && ch_str_equals( VK_EXT_DEBUG_UTILS_EXTENSION_NAME, ext_props[ i ].extensionName ) )
			{
				g_has_debug_utils = true;
			}
		}
	}

	if ( found_extensions != instance_exts.size() )
	{
		// check which extensions are missing
		for ( u32 i = 0; i < ARR_SIZE( instance_exts ); i++ )
		{
			if ( !instance_exts_found[ i ] )
			{
				Log_ErrorF( gLC_Render, "Failed to find required extension: %s\n", instance_exts[ i ] );
			}
		}

		ch_free( ext_props );
		return false;
	}

	if ( g_has_debug_utils )
		instance_exts.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
	
	create_info.enabledExtensionCount   = instance_exts.size();
	create_info.ppEnabledExtensionNames = instance_exts.data();

	ch_free( ext_props );

	vk_check( vkCreateInstance( &create_info, NULL, &g_vk_instance ), "Failed to create vulkan instance!" );

	if ( g_has_debug_utils )
	{
		VkResult result = debug_utils_messenger_create();

		if ( result == VK_SUCCESS )
		{
			Log_Dev( gLC_Render, 1, "Successfully Created Debug Utils Messenger\n" );
		}
		else
		{
			Log_ErrorF( gLC_Render, "Failed to Create Debug Utils Messenger: %s\n", vk_str( result ) );
		}
	}

	return true;
}


void vk_instance_destroy()
{
	debug_utils_messenger_destroy();
	vkDestroyDevice( g_vk_device, NULL );
	vkDestroyInstance( g_vk_instance, NULL );
}


VkSurfaceKHR vk_surface_create( void* native_window, SDL_Window* sdl_window )
{
	VkSurfaceKHR surface = VK_NULL_HANDLE;

#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR create_info{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	create_info.hwnd      = (HWND)native_window;
	create_info.hinstance = GetModuleHandle( NULL );
	create_info.flags     = 0;
	create_info.pNext     = NULL;

	if ( vk_check_e( vkCreateWin32SurfaceKHR( g_vk_instance, &create_info, nullptr, &surface ), "Failed to create Surface" ) )
		return VK_NULL_HANDLE;
#else
	if ( !SDL_Vulkan_CreateSurface( sdl_window, g_vk_instance, &surface ) )
	{
		Log_ErrorF( gLC_Render, "Error: Failed to create SDL Vulkan Surface: %s\n", SDL_GetError() );
		return VK_NULL_HANDLE;
	}
#endif

	return surface;
}


void vk_surface_destroy( VkSurfaceKHR surface )
{
	vkDestroySurfaceKHR( g_vk_instance, surface, NULL );
}


// --------------------------------------------------------------------------------------------
// Device Setup
// --------------------------------------------------------------------------------------------


bool vk_device_check_extensions( VkPhysicalDevice device, device_info_t& info )
{
	bool found_exts[ ARR_SIZE( g_device_extensions ) ] = { false };
	u32  found_ext_count                               = 0;

	if ( g_list_exts )
	{
		Log_MsgF( gLC_Render, "\"%s\" Device Extensions: %d\n", info.props.deviceName, info.ext_count );

		for ( u32 i = 0; i < info.ext_count; i++ )
		{
			bool found = false;
			for ( u32 dev_i = 0; dev_i < ARR_SIZE( g_device_extensions ); dev_i++ )
			{
				if ( ch_str_equals( g_device_extensions[ dev_i ], info.exts[ i ].extensionName ) )
				{
					Log_MsgF( gLC_Render, " [LOADED] %s \n", info.exts[ i ].extensionName );
					found               = true;
					found_exts[ dev_i ] = true;
					found_ext_count++;
					break;
				}
			}

			if ( !found )
				Log_MsgF( gLC_Render, "          %s\n", info.exts[ i ].extensionName );
		}

		Log_Msg( gLC_Render, "\n" );
	}
	else
	{
		for ( u32 dev_i = 0; dev_i < ARR_SIZE( g_device_extensions ); dev_i++ )
		{
			size_t ext_len = strlen( g_device_extensions[ dev_i ] );

			for ( u32 j = 0; j < info.ext_count; j++ )
			{
				if ( ch_str_equals( g_device_extensions[ dev_i ], ext_len, info.exts[ j ].extensionName ) )
				{
					found_exts[ dev_i ] = true;
					found_ext_count++;
					break;
				}
			}
		}
	}

	// all good, found all the extensions
	if ( found_ext_count == ARR_SIZE( g_device_extensions ) )
	{
		info.has_needed_extensions = true;
		return true;
	}
	
	// NOTE: if we have 2 devices, and index 0 is not suitable, but 1 is, this will always print, and the engine will still start, hmmm
	ch_string error_msg = ch_str_copy_f( "Device \"%s\" does not have these required Vulkan Extensions:", info.props.deviceName );
	
	for ( u32 i = 0; i < ARR_SIZE( g_device_extensions ); i++ )
	{
		if ( !found_exts[ i ] )
		{
			error_msg = ch_str_concat( CH_STR_UR( error_msg ), "\n    ", 5 );
			error_msg = ch_str_concat( CH_STR_UR( error_msg ), g_device_extensions[ i ], -1 );
		}
	}
	
	error_msg = ch_str_concat( CH_STR_UR( error_msg ), "\n", 1 );
	
	Log_Warn( gLC_Render, error_msg.data );
	
	ch_str_free( error_msg.data );
	return false;
}


// TODO: rethink this and check for compute and transfer queues (there could be multiple of each)
// TODO: maybe we should try to have a separate graphics queue and present queue? not sure what that would actually give us
static void vk_find_queue_families( VkSurfaceKHR surface, const VkPhysicalDeviceProperties& device_props, VkPhysicalDevice device, u32& graphics, u32& transfer )
{
	u32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queue_family_count, nullptr );

	if ( queue_family_count == 0 )
	{
		Log_ErrorF( gLC_Render, "Device \"%s\" has no queue families???\n", device_props.deviceName );
		return;
	}

	auto queue_families = CH_STACK_NEW( VkQueueFamilyProperties, queue_family_count );

	if ( !queue_families )
	{
		Log_FatalF( gLC_Render, "Failed to stack allocate queue family properties array\n" );
		return;
	}

	vkGetPhysicalDeviceQueueFamilyProperties( device, &queue_family_count, queue_families );  // Logic to find queue family indices to populate struct with

	// hack, as this is called multiple times for some reason
	static bool listed_queues = false;

	if ( g_list_queues && !listed_queues )
	{
		listed_queues          = true;
		std::string queueDump = vstring( "Device \"%s\" has %zd Queues\n", device_props.deviceName, queue_family_count );

		for ( u32 queue_index = 0; queue_index < queue_family_count; queue_index++ )
		{
			const VkQueueFamilyProperties& queueFamily = queue_families[ queue_index ];

			queueDump += vstring( "Queue %zd:\n    Count: %zd\n    Supports Present: ", queue_index, queueFamily.queueCount );

			VkBool32 present_support = false;
			if ( vk_check_e( vkGetPhysicalDeviceSurfaceSupportKHR( device, queue_index, surface, &present_support ), "Failed to Get Physical Device Surface Support" ) )
			{
				CH_STACK_FREE( queue_families );
				return;
			}

			queueDump += vstring( "%s\n    Flags:\n", present_support ? "Yes" : "No" );

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

	for ( u32 queue_index = 0; queue_index < queue_family_count; queue_index++ )
	{
		const VkQueueFamilyProperties& queueFamily = queue_families[ queue_index ];

		if ( queueFamily.queueCount == 0 )
			continue;

		VkBool32 present_support = false;
		if ( vk_check_e( vkGetPhysicalDeviceSurfaceSupportKHR( device, queue_index, surface, &present_support ), "Failed to Get Physical Device Surface Support" ) )
		{
			CH_STACK_FREE( queue_families );
			return;
		}

		if ( graphics == UINT32_MAX && queueFamily.queueFlags & ( VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT ) )
		{
			if ( !present_support )
				continue;

			graphics = queue_index;
		}

		if ( queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && ( transfer == UINT32_MAX || graphics == transfer ) )
		{
			transfer = queue_index;
		}
	}

	CH_STACK_FREE( queue_families );
}


static void vk_device_select( VkPhysicalDevice device, device_info_t& info )
{
	if ( device == VK_NULL_HANDLE )
	{
		Log_Fatal( "Failed to find a suitable GPU!" );
		return;
	}

	g_vk_physical_device    = device;
	g_device_properties     = info.props;

	g_queue_family_graphics = info.queue_graphics;
	g_queue_family_transfer = info.queue_transfer;

	vk_swapchain_setup_info( info );

	Log_MsgF( gLC_Render, "Using GPU \"%s\"\n", g_device_properties.deviceName );
}


static void show_device_list_msg_box( SDL_MessageBoxFlags type, const char* message, VkSurfaceKHR surface, VkPhysicalDevice* devices, device_info_t* infos, bool* suitable_devices, u32 device_count )
{
	// Build a string list of devices available to us
	std::string deviceList = vstring( "%d Devices Found:\n\n", device_count );

	for ( u32 i = 0; i < device_count; i++ )
	{
		bool suitable = suitable_devices[ i ];

		deviceList += vstring( "GPU %zd - %s%s\n", i, infos[ i ].props.deviceName, suitable ? "" : " (Unsupported)" );
	}

	deviceList += "\n Use the --gpu <index> argument to select a GPU, or --list-gpus to show this again\n";
	Log_Msg( deviceList.c_str() );
	SDL_ShowSimpleMessageBox( type, "GPU Device List", deviceList.c_str(), NULL );
}


static void vk_get_physical_device_info( VkSurfaceKHR surface, u32 device_count, VkPhysicalDevice* devices, device_info_t* device_infos )
{
	for ( u32 i = 0; i < device_count; i++ )
	{
		VkPhysicalDevice& device = devices[ i ];
		device_info_t&    info   = device_infos[ i ];

		info.queue_graphics = UINT32_MAX;
		info.queue_transfer = UINT32_MAX;

		vkGetPhysicalDeviceProperties( device, &info.props );

		// ---------------------------------------------------------------
		// this device must support the required extensions

		if ( vk_check_e( vkEnumerateDeviceExtensionProperties( device, NULL, &info.ext_count, NULL ), "Failed to Get Device Extension Count" ) )
			continue;

		device_infos[ i ].exts = ch_malloc< VkExtensionProperties >( info.ext_count );
		if ( vk_check_e( vkEnumerateDeviceExtensionProperties( device, NULL, &info.ext_count, info.exts ), "Failed to Get Device Extension List" ) )
			continue;

		// ---------------------------------------------------------------
		// check swap chain info

		if ( vk_check_e( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, surface, &info.surface_capabilities ), "Failed to Get Physical Device Surface Capabilities" ) )
			continue;

		if ( vk_check_e( vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &info.surf_format_count, NULL ), "Failed to Get Physical Device Surface Formats" ) )
			continue;

		if ( info.surf_format_count != 0 )
		{
			info.surf_formats = ch_malloc< VkSurfaceFormatKHR >( info.surf_format_count );

			if ( vk_check_e( vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &info.surf_format_count, info.surf_formats ), "Failed to Get Physical Device Surface Formats" ) )
				continue;
		}

		if ( vk_check_e( vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &info.present_mode_count, NULL ), "Failed to Get Physical Device Surface Present Modes" ) )
			continue;

		if ( info.present_mode_count != 0 )
		{
			info.present_modes = ch_malloc< VkPresentModeKHR >( info.present_mode_count );
			if ( vk_check_e( vkGetPhysicalDeviceSurfacePresentModesKHR( device, surface, &info.present_mode_count, info.present_modes ), "Failed to Get Physical Device Surface Present Modes" ) )
				continue;
		}

		vk_find_queue_families( surface, info.props, device, info.queue_graphics, info.queue_transfer );

		info.complete = true;
	}
}


static void vk_clear_device_infos( u32 device_count, device_info_t* device_infos )
{
	if ( !device_infos )
		return;

	for ( u32 i = 0; i < device_count; i++ )
	{
		if ( device_infos[ i ].exts )
			ch_free( device_infos[ i ].exts );

		if ( device_infos[ i ].surf_formats )
			ch_free( device_infos[ i ].surf_formats );

		if ( device_infos[ i ].present_modes )
			ch_free( device_infos[ i ].present_modes );
	}

	ch_free( device_infos );
}


static bool select_physical_device( VkSurfaceKHR surface )
{
	u32 device_count = 0;

	vk_check( vkEnumeratePhysicalDevices( g_vk_instance, &device_count, NULL ), "Failed to Enumerate Physical Devices" );
	if ( device_count == 0 )
	{
		Log_Fatal( gLC_Render, "Failed to find a GPU with Vulkan support!" );
		return false;
	}

	VkPhysicalDevice* devices = ch_malloc< VkPhysicalDevice >( device_count );

	vk_check( vkEnumeratePhysicalDevices( g_vk_instance, &device_count, devices ), "Failed to Enumerate Physical Devices" );

	// get info of all physical devices
	device_info_t* device_infos = ch_calloc< device_info_t >( device_count );
	vk_get_physical_device_info( surface, device_count, devices, device_infos );

	// determine which ones are suitable
	bool* suitable_devices = ch_calloc< bool >( device_count );
	
	for ( u32 i = 0; i < device_count; i++ )
	{
		// don't bother if there was an error getting info for this device
		if ( !device_infos[ i ].complete )
			continue;

		// must have these queue families
		if ( device_infos[ i ].queue_graphics == UINT32_MAX || device_infos[ i ].queue_transfer == UINT32_MAX )
			continue;

		// check extensions
		if ( !vk_device_check_extensions( devices[ i ], device_infos[ i ] ) )
			continue;

		suitable_devices[ i ] = true;
	}

	if ( g_list_devices )
	{
		show_device_list_msg_box( SDL_MESSAGEBOX_INFORMATION, "GPU Device List", surface, devices, device_infos, suitable_devices, device_count );
		ch_free( devices );
		return false;
	}

	bool return_value = false;

	// check if no suitable devices were found (after message box launch option so users can see what's available)
	for ( u32 i = 0; i < device_count; i++ )
	{
		if ( suitable_devices[ i ] )
			continue;

		if ( i == device_count - 1 )
		{
			Log_FatalF( gLC_Render, "Failed to find a suitable GPU out of the %d available!", device_count );
			goto exit;
		}
	}

	// if the user has specified a device index, use that
	if ( g_device_index > -1 )
	{
		// If we tried to select an invalid device index, dump all available devices to the user
		if ( g_device_index >= device_count )
		{
			ch_string message = ch_str_copy_f( "Only %zd GPU's Found, but user requested GPU index %d\n", device_count, g_device_index );
			show_device_list_msg_box( SDL_MESSAGEBOX_ERROR, message.data, surface, devices, device_infos, suitable_devices, device_count );
			ch_str_free( message.data );
			goto exit;
		}
		// this card isn't suitable
		else if ( !suitable_devices[ g_device_index ] )
		{
			ch_string message = ch_str_copy_f( "GPU %d (%s) is not suitable for use\n", g_device_index, device_infos[ g_device_index ].props.deviceName );
			show_device_list_msg_box( SDL_MESSAGEBOX_ERROR, message.data, surface, devices, device_infos, suitable_devices, device_count );
			ch_str_free( message.data );
			goto exit;
		}
		else
		{
			vk_device_select( devices[ g_device_index ], device_infos[ g_device_index ] );
			return_value = true;
			goto exit;
		}
	}
	else
	{
		// pick the first suitable device
		for ( u32 i = 0; i < device_count; i++ )
		{
			if ( suitable_devices[ i ] )
			{
				vk_device_select( devices[ i ], device_infos[ i ] );
				return_value = true;
				goto exit;
			}
		}
	}

exit:
	ch_free( devices );
	ch_free( suitable_devices );
	vk_clear_device_infos( device_count, device_infos );
	return return_value;
}


bool vk_device_create( VkSurfaceKHR surface )
{
	if ( !select_physical_device( surface ) )
		return false;

	// create device

	// create device queues
	float queue_priority = 1.0f;
	vk_find_queue_families( surface, g_device_properties, g_vk_physical_device, g_queue_family_graphics, g_queue_family_transfer );

	ChVector< VkDeviceQueueCreateInfo > queue_create_infos;

	VkDeviceQueueCreateInfo             queueCreateInfo = {
					.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = g_queue_family_graphics,
					.queueCount       = 1,
					.pQueuePriorities = &queue_priority,
	};

	queue_create_infos.push_back( queueCreateInfo );

	if ( g_queue_family_graphics != g_queue_family_transfer )
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {
			.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = g_queue_family_transfer,
			.queueCount       = 1,
			.pQueuePriorities = &queue_priority,
		};

		queue_create_infos.push_back( queueCreateInfo );
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
		   .sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		   .pNext                = &indexing,
		   .flags                = 0,
		   .queueCreateInfoCount = queue_create_infos.size(),
		   .pQueueCreateInfos    = queue_create_infos.data(),

#if _DEBUG
		.enabledLayerCount   = (u32)ARR_SIZE( g_validation_layers ),
		.ppEnabledLayerNames = g_validation_layers,
#endif

		.enabledExtensionCount   = (u32)ARR_SIZE( g_device_extensions ),
		.ppEnabledExtensionNames = g_device_extensions,
		.pEnabledFeatures        = &deviceFeatures,
	};

	vk_check( vkCreateDevice( g_vk_physical_device, &createInfo, NULL, &g_vk_device ), "Failed to create logical device!" );

	vkGetDeviceQueue( g_vk_device, g_queue_family_graphics, 0, &g_queue_graphics );
	vkGetDeviceQueue( g_vk_device, g_queue_family_transfer, 0, &g_queue_transfer );

	// do we have debug utils?
	if ( g_has_debug_utils )
	{
		// get debug util functions
		pfnSetDebugUtilsObjectName    = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr( g_vk_instance, "vkSetDebugUtilsObjectNameEXT" );
		pfnSetDebugUtilsObjectTag     = (PFN_vkSetDebugUtilsObjectTagEXT)vkGetInstanceProcAddr( g_vk_instance, "vkSetDebugUtilsObjectTagEXT" );

		pfnQueueBeginDebugUtilsLabel  = (PFN_vkQueueBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr( g_vk_instance, "vkQueueBeginDebugUtilsLabelEXT" );
		pfnQueueEndDebugUtilsLabel    = (PFN_vkQueueEndDebugUtilsLabelEXT)vkGetInstanceProcAddr( g_vk_instance, "vkQueueEndDebugUtilsLabelEXT" );
		pfnQueueInsertDebugUtilsLabel = (PFN_vkQueueInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr( g_vk_instance, "vkQueueInsertDebugUtilsLabelEXT" );

		pfnCmdBeginDebugUtilsLabel    = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr( g_vk_instance, "vkCmdBeginDebugUtilsLabelEXT" );
		pfnCmdEndDebugUtilsLabel      = (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr( g_vk_instance, "vkCmdEndDebugUtilsLabelEXT" );
		pfnCmdInsertDebugUtilsLabel   = (PFN_vkCmdInsertDebugUtilsLabelEXT)vkGetInstanceProcAddr( g_vk_instance, "vkCmdInsertDebugUtilsLabelEXT" );

		if ( pfnSetDebugUtilsObjectName )
			Log_Dev( gLC_Vulkan, 2, "Loaded PFN_vkSetDebugUtilsObjectNameEXT\n" );

		if ( pfnSetDebugUtilsObjectTag )
			Log_Dev( gLC_Vulkan, 2, "Loaded PFN_vkSetDebugUtilsObjectTagEXT\n" );
	}

	vk_set_name( VK_OBJECT_TYPE_PHYSICAL_DEVICE, (u64)g_vk_physical_device, g_device_properties.deviceName );
	vk_set_name( VK_OBJECT_TYPE_DEVICE, (u64)g_vk_device, g_device_properties.deviceName );

	vk_set_name( VK_OBJECT_TYPE_QUEUE, (u64)g_queue_graphics, "Graphics" );
	vk_set_name( VK_OBJECT_TYPE_QUEUE, (u64)g_queue_transfer, "Transfer" );

	return true;
}

