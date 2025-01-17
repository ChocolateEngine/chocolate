#include "render.h"


// ------------------------------------------------------------------
// General Helper Functions
// ------------------------------------------------------------------


void vk_set_name_ex( VkObjectType type, u64 handle, const char* name, const char* type_name )
{
	// add a debug label onto it
	if ( !pfnSetDebugUtilsObjectName || !name )
		return;
	
	if ( !type_name )
		type_name = vk_object_str( type );

	const char*                         strings[]  = { type_name, " - ", name };
	const u64                           lengths[]  = { strlen( type_name ), 3, strlen( name ) };

	ch_string                           final_name = ch_str_join( 3, strings, lengths );

	const VkDebugUtilsObjectNameInfoEXT nameInfo  = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,  // sType
        NULL,                                                // pNext
        type,                                                // objectType
        handle,                                              // objectHandle
        final_name.data,                                     // pObjectName
	};

	VkResult result = pfnSetDebugUtilsObjectName( g_vk_device, &nameInfo );

	ch_str_free( final_name.data );

	if ( result == VK_SUCCESS )
	{
		Log_DevF( gLC_Render, 2, "Set Name for Object \"%s\" - \"%s\"\n", vk_object_str( type ), name );
		return;
	}

	Log_ErrorF( gLC_Render, "Failed to Set Name for Object \"%s\" - \"%s\"\n", type_name, name );
}


void vk_set_name( VkObjectType type, u64 handle, const char* name )
{
	vk_set_name_ex( type, handle, name, vk_object_str( type ) );
}


void vk_queue_wait_graphics()
{
	vk_check( vkQueueWaitIdle( g_vk_queue_graphics ), "Failed to wait for graphics queue" );
}


void vk_queue_wait_transfer()
{
	vk_check( vkQueueWaitIdle( g_vk_queue_transfer ), "Failed to wait for transfer queue" );
}


#if 0
// ------------------------------------------------------------------
// Basic Deletion Queue
// ------------------------------------------------------------------


static fn_vk_destroy* g_vk_delete_queue          = nullptr;
static u32            g_vk_delete_queue_count    = 0;
static u32            g_vk_delete_queue_capacity = 0;


void vk_add_delete( fn_vk_destroy destroy_func )
{
	if ( !destroy_func )
		return;

	// are we at the capacity limit?
	if ( g_vk_delete_queue_count == g_vk_delete_queue_capacity )
	{
		// allocate some more slots for function pointers
		g_vk_delete_queue_capacity += VK_DELETE_QUEUE_BATCH_SIZE;
		g_vk_delete_queue          = ch_realloc( g_vk_delete_queue, g_vk_delete_queue_capacity );
	}

	g_vk_delete_queue[ g_vk_delete_queue_count ] = destroy_func;
	g_vk_delete_queue_count++;
}


void vk_flush_delete_queue()
{
	// call all the functions in the delete queue in reverse order
	for ( u32 i = g_vk_delete_queue_count - 1; i > 0; i-- )
	{
		g_vk_delete_queue[ i ]();
	}

	// reset the count
	g_vk_delete_queue_count = 0;	
}
#endif


// ------------------------------------------------------------------
// Vulkan Result Error Checking
// ------------------------------------------------------------------


void vk_check_f( VkResult sResult, char const* spArgs, ... )
{
	if ( sResult == VK_SUCCESS )
		return;

	va_list args;
	va_start( args, spArgs );

	va_list copy;
	va_copy( copy, args );
	int len = std::vsnprintf( nullptr, 0, spArgs, copy );
	va_end( copy );

	if ( len < 0 )
	{
		Log_Error( gLC_Render, "\n *** Sys_ExecuteV: vsnprintf failed?\n\n" );
		Log_FatalF( gLC_Render, "Vulkan Error: %s", vk_str( sResult ) );
		return;
	}

	char* str_data = ch_malloc< char >( len + 1 );

	if ( !str_data )
	{
		print( "Failed to allocate memory for error string\n" );
		return;
	}

	std::vsnprintf( str_data, len + 1, spArgs, args );
	str_data[ len ] = '\0';

	va_end( args );

	vk_check( sResult, str_data );

	ch_free( str_data );
}


bool vk_check_ef( VkResult sResult, char const* spArgs, ... )
{
	if ( sResult == VK_SUCCESS )
		return false;

	va_list args;
	va_start( args, spArgs );

	va_list copy;
	va_copy( copy, args );
	int len = std::vsnprintf( nullptr, 0, spArgs, copy );
	va_end( copy );

	if ( len < 0 )
	{
		Log_Error( gLC_Render, "\n *** Sys_ExecuteV: vsnprintf failed?\n\n" );
		Log_ErrorF( gLC_Render, "Vulkan Error: %s", vk_str( sResult ) );
		return true;
	}

	char* str_data = ch_malloc< char >( len + 1 );

	if ( !str_data )
	{
		print( "Failed to allocate memory for error string\n" );
		return true;
	}

	std::vsnprintf( str_data, len + 1, spArgs, args );
	str_data[ len ] = '\0';

	va_end( args );

	bool ret = vk_check_e( sResult, str_data );

	ch_free( str_data );
	return ret;
}


void vk_check( VkResult sResult, char const* spMsg )
{
	if ( sResult == VK_SUCCESS )
		return;

	Log_FatalF( gLC_Render, "Vulkan Error: %s: %s", spMsg, vk_str( sResult ) );
}


void vk_check( VkResult sResult )
{
	if ( sResult == VK_SUCCESS )
		return;

	Log_FatalF( gLC_Render, "Vulkan Error: %s", vk_str( sResult ) );
}


// Non-Fatal Version
bool vk_check_e( VkResult sResult, char const* spMsg )
{
	if ( sResult == VK_SUCCESS )
		return false;

	Log_ErrorF( gLC_Render, "Vulkan Error: %s: %s", spMsg, vk_str( sResult ) );
	return true;
}


bool vk_check_e( VkResult sResult )
{
	if ( sResult == VK_SUCCESS )
		return false;

	Log_ErrorF( gLC_Render, "Vulkan Error: %s", vk_str( sResult ) );
	return true;
}


// ------------------------------------------------------------------
// Conversion Functions
// ------------------------------------------------------------------


const char* vk_str( VkResult result )
{
	switch ( result )
	{
		default:
			return "Unknown";
		case VK_SUCCESS:
			return "VK_SUCCESS";
		case VK_NOT_READY:
			return "VK_NOT_READY";
		case VK_TIMEOUT:
			return "VK_TIMEOUT";
		case VK_EVENT_SET:
			return "VK_EVENT_SET";
		case VK_EVENT_RESET:
			return "VK_EVENT_RESET";
		case VK_INCOMPLETE:
			return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED:
			return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST:
			return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED:
			return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT:
			return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT:
			return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS:
			return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL:
			return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN:
			return "VK_ERROR_UNKNOWN";
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:
			return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_FRAGMENTATION:
			return "VK_ERROR_FRAGMENTATION";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
			return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case VK_ERROR_SURFACE_LOST_KHR:
			return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR:
			return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR:
			return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT:
			return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV:
			return "VK_ERROR_INVALID_SHADER_NV";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
			return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_NOT_PERMITTED_EXT:
			return "VK_ERROR_NOT_PERMITTED_EXT";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
			return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case VK_THREAD_IDLE_KHR:
			return "VK_THREAD_IDLE_KHR";
		case VK_THREAD_DONE_KHR:
			return "VK_THREAD_DONE_KHR";
		case VK_OPERATION_DEFERRED_KHR:
			return "VK_OPERATION_DEFERRED_KHR";
		case VK_OPERATION_NOT_DEFERRED_KHR:
			return "VK_OPERATION_NOT_DEFERRED_KHR";
		case VK_PIPELINE_COMPILE_REQUIRED_EXT:
			return "VK_PIPELINE_COMPILE_REQUIRED_EXT";
	}
}


const char* vk_object_str( VkObjectType type )
{
	switch ( type )
	{
		default:
		case VK_OBJECT_TYPE_UNKNOWN:
			return "Unknown";

		case VK_OBJECT_TYPE_INSTANCE:
			return "Instance";

		case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
			return "Physical Device";

		case VK_OBJECT_TYPE_DEVICE:
			return "Device";

		case VK_OBJECT_TYPE_QUEUE:
			return "Queue";

		case VK_OBJECT_TYPE_SEMAPHORE:
			return "Semaphore";

		case VK_OBJECT_TYPE_COMMAND_BUFFER:
			return "Command Buffer";

		case VK_OBJECT_TYPE_FENCE:
			return "Fence";

		case VK_OBJECT_TYPE_DEVICE_MEMORY:
			return "Device Memory";

		case VK_OBJECT_TYPE_BUFFER:
			return "Buffer";

		case VK_OBJECT_TYPE_IMAGE:
			return "Image";

		case VK_OBJECT_TYPE_EVENT:
			return "Event";

		case VK_OBJECT_TYPE_QUERY_POOL:
			return "Query Pool";

		case VK_OBJECT_TYPE_BUFFER_VIEW:
			return "Buffer View";

		case VK_OBJECT_TYPE_IMAGE_VIEW:
			return "Image View";

		case VK_OBJECT_TYPE_SHADER_MODULE:
			return "Shader Module";

		case VK_OBJECT_TYPE_PIPELINE_CACHE:
			return "Pipeline Cache";

		case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
			return "Pipeline Layout";

		case VK_OBJECT_TYPE_RENDER_PASS:
			return "Render Pass";

		case VK_OBJECT_TYPE_PIPELINE:
			return "Pipeline";

		case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
			return "Descriptor Set Layout";

		case VK_OBJECT_TYPE_SAMPLER:
			return "Sampler";

		case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
			return "Descriptor Pool";

		case VK_OBJECT_TYPE_DESCRIPTOR_SET:
			return "Descriptor Set";

		case VK_OBJECT_TYPE_FRAMEBUFFER:
			return "Framebuffer";

		case VK_OBJECT_TYPE_COMMAND_POOL:
			return "Command Pool";

		case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:
			return "Sampler YCbCr Conversion";

		case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:
			return "Descriptor Update Template";

		case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT:
			return "Private Data Slot";

		case VK_OBJECT_TYPE_SURFACE_KHR:
			return "KHR - Surface";

		case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
			return "KHR - Swapchain";

		case VK_OBJECT_TYPE_DISPLAY_KHR:
			return "KHR - Display";

		case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
			return "KHR - Display Mode";

		case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
			return "EXT - Debug Report Callback";

		case VK_OBJECT_TYPE_VIDEO_SESSION_KHR:
			return "KHR - Video Session";

		case VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR:
			return "KHR - Video Session Paremeters";

		case VK_OBJECT_TYPE_CU_MODULE_NVX:
			return "NVX - CU Module";

		case VK_OBJECT_TYPE_CU_FUNCTION_NVX:
			return "NVX - CU Function";

		case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
			return "EXT - Debug Utils Messenger";

		case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:
			return "KHR - Acceleration Structure";

		case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:
			return "EXT - Validation Cache";

		case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV:
			return "NVIDIA EXT - Acceleration Structure";

		case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL:
			return "INTEL - Performance Configuration";

		case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR:
			return "KHR - Deferred Operation";

		case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV:
			return "NVIDIA EXT - Indirect Commands Layout";

		case VK_OBJECT_TYPE_CUDA_MODULE_NV:
			return "NVIDIA EXT - CUDA Module";

		case VK_OBJECT_TYPE_CUDA_FUNCTION_NV:
			return "NVIDIA EXT - CUDA Function";

		case VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA:
			return "Buffer Collection Fuchsia";

		case VK_OBJECT_TYPE_MICROMAP_EXT:
			return "EXT - Micromap";

		case VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV:
			return "NVIDIA EXT - Optical Flow Session";

		case VK_OBJECT_TYPE_SHADER_EXT:
			return "EXT - Shader";
	}
}


const char* vk_samples_str( VkSampleCountFlags counts )
{
	if ( counts & VK_SAMPLE_COUNT_64_BIT ) return "VK_SAMPLE_COUNT_64_BIT";
	if ( counts & VK_SAMPLE_COUNT_32_BIT ) return "VK_SAMPLE_COUNT_32_BIT";
	if ( counts & VK_SAMPLE_COUNT_16_BIT ) return "VK_SAMPLE_COUNT_16_BIT";
	if ( counts & VK_SAMPLE_COUNT_8_BIT ) return "VK_SAMPLE_COUNT_8_BIT";
	if ( counts & VK_SAMPLE_COUNT_4_BIT ) return "VK_SAMPLE_COUNT_4_BIT";
	if ( counts & VK_SAMPLE_COUNT_2_BIT ) return "VK_SAMPLE_COUNT_2_BIT";

	return "VK_SAMPLE_COUNT_1_BIT";
}

