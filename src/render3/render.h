#pragma once

#include "irender3.h"

#include <vulkan/vulkan.h>


struct ImGuiContext;


// TEST - array that keeps track of used slots
template< typename T >
struct slot_array_t
{
	T*   data;
	u32* free_slots;  // list of free slots
	u32  count;       // count of used slots

	slot_array_t()
		: data( nullptr )
		, count( 0 )
	{
	}

	slot_array_t( u32 count )
		: data( nullptr )
		, count( count )
	{
		data = ch_malloc< T >( count );
	}

	~slot_array_t()
	{
		ch_free( data );
	}

	T& operator[]( u32 index )
	{
		return data[ index ];
	}

	const T& operator[]( u32 index ) const
	{
		return data[ index ];
	}

	T* begin()
	{
		return data;
	}

	T* end()
	{
		return data + count;
	}

	T& append( const T& value )
	{
		// look for a free slot
		if ( free_slots )
		{
			for ( u32 i = 0; i < count; i++ )
			{
				if ( free_slots[ i ] > 0 )
				{
					data[ i ]       = value;
					free_slots[ i ] = 1;

					return data[ i ];
				}
			}

			u32 slot = free_slots[ count ];
			data[ slot ] = value;
			count++;

			return data[ slot ];
		}

		data = ch_realloc< T >( data, count + 1 );
		data[ count ] = value;
		count++;

		return data[ count - 1 ];
	}
};


enum e_backbuffer_image
{
	e_backbuffer_image_color,
	e_backbuffer_image_depth,
	e_backbuffer_image_resolve,
	e_backbuffer_image_count
};


// used when selecting a physical device
struct device_info_t
{
	// yay bools in a struct
	bool                       complete;  // no errors when getting all the data here
	bool                       has_needed_extensions;

	// uh this is 824 bytes...
	VkPhysicalDeviceProperties props;

	VkExtensionProperties*     exts;
	u32                        ext_count;

	VkSurfaceCapabilitiesKHR   surface_capabilities;

	VkSurfaceFormatKHR*        surf_formats;
	u32                        surf_format_count;

	VkPresentModeKHR*          present_modes;
	u32                        present_mode_count;

	u32                        queue_graphics;
	u32                        queue_transfer;
};


struct vk_texture_t
{

};


struct backbuffer_image_t
{
	VkImage     image;  // not used for resolve, or color is msaa is off
	VkImageView view;
	VkFormat    format;
};


struct backbuffer_t
{
	glm::uvec2          size;

	// 0 is color, 1 is depth, 2 is resolve
	backbuffer_image_t* images;
	u32                 image_count;
};


// data for each window
// TODO: this SUCKS, make it data oriented !!!
struct r_window_data_t
{
	SDL_Window*        window    = nullptr;
	ImGuiContext*      context   = nullptr;
	VkSurfaceKHR       surface   = VK_NULL_HANDLE;
	VkSwapchainKHR     swapchain = VK_NULL_HANDLE;

	// amount allocated is swap_image_count
	VkSemaphore*       semaphore_image_available;
	VkSemaphore*       semaphore_render_finished;

	// only allocates to swap_image_count for the main window buffers
	VkCommandBuffer*   command_buffers;

	// amount allocated is swap_image_count
	VkFence*           fences;
	VkFence*           fences_in_flight;

	// swapchain info
	VkSurfaceFormatKHR swap_surface_format;
	VkExtent2D         swap_extent;
	VkImage*           swap_images;
	VkImageView*       swap_image_views;
	VkPresentModeKHR   swap_present_mode;

	// Contains the framebuffers which are to be drawn to during command buffer recording.
	backbuffer_t       backbuffer;

	// Render_OnReset_t   onResetFunc = nullptr;

	// here because of mem alignment, 2 unused bytes at the end
	u8                 frame_index = 0;
	u8                 swap_image_count;
};


// --------------------------------------------------------------------------------------------


LOG_CHANNEL_EXTERN( Render );
LOG_CHANNEL_EXTERN( Vulkan );
LOG_CHANNEL_EXTERN( Validation );


extern VkInstance                          g_vk_instance;
extern VkDevice                            g_vk_device;
extern VkPhysicalDevice                    g_vk_physical_device;

extern VkQueue                             g_queue_graphics;
extern VkQueue                             g_queue_transfer;

extern u32                                 g_queue_family_graphics;
extern u32                                 g_queue_family_transfer;

extern VkPhysicalDeviceProperties          g_device_properties;
extern VkSurfaceCapabilitiesKHR            g_surface_capabilities;

extern ResourceList< r_window_data_t >     g_windows;
extern ChHandle_t                          g_main_window;

// extern slot_array_t< r_window_data_t >     g_windows2;
// extern SDL_Window**                        g_windows_sdl;
// extern ImGuiContext**                      g_windows_imgui_contexts;


// function pointers for debug utils

extern PFN_vkSetDebugUtilsObjectNameEXT    pfnSetDebugUtilsObjectName;
extern PFN_vkSetDebugUtilsObjectTagEXT     pfnSetDebugUtilsObjectTag;

extern PFN_vkQueueBeginDebugUtilsLabelEXT  pfnQueueBeginDebugUtilsLabel;
extern PFN_vkQueueEndDebugUtilsLabelEXT    pfnQueueEndDebugUtilsLabel;
extern PFN_vkQueueInsertDebugUtilsLabelEXT pfnQueueInsertDebugUtilsLabel;

extern PFN_vkCmdBeginDebugUtilsLabelEXT    pfnCmdBeginDebugUtilsLabel;
extern PFN_vkCmdEndDebugUtilsLabelEXT      pfnCmdEndDebugUtilsLabel;
extern PFN_vkCmdInsertDebugUtilsLabelEXT   pfnCmdInsertDebugUtilsLabel;


// --------------------------------------------------------------------------------------------


CONVAR_BOOL_EXT( r_msaa_enabled );
CONVAR_INT_EXT( r_msaa_samples );


// --------------------------------------------------------------------------------------------
// Vulkan Helper Functions

const char*                                vk_str( VkResult result );
const char*                                vk_object_str( VkObjectType type );
const char*                                vk_samples_str( VkSampleCountFlags counts );

void                                       vk_check_f( VkResult sResult, char const* spArgs, ... );
void                                       vk_check( VkResult sResult, char const* spMsg );
void                                       vk_check( VkResult sResult );

// Non Fatal Version of it, is just an error, returns true if failed
bool                                       vk_check_ef( VkResult sResult, char const* spArgs, ... );
bool                                       vk_check_e( VkResult sResult, char const* spMsg );
bool                                       vk_check_e( VkResult sResult );

void                                       vk_set_name_ex( VkObjectType type, u64 handle, const char* name, const char* type_name );
void                                       vk_set_name( VkObjectType type, u64 handle, const char* name );

// not sure if i should use this
// template< typename T >
// inline void                                vk_set_name2( VkObjectType type, T handle, const char* name )
// {
// 	return vk_set_name_ex( type, static_cast< u64 >( handle ), name, vk_object_str( type ) );
// }

// --------------------------------------------------------------------------------------------
// Vulkan Functions

bool                                       vk_instance_create();
void                                       vk_instance_destroy();

VkSurfaceKHR                               vk_surface_create( void* native_window, SDL_Window* sdl_window );
void                                       vk_surface_destroy( VkSurfaceKHR surface );

bool                                       vk_device_create( VkSurfaceKHR surface );

bool                                       vk_swapchain_create( r_window_data_t* window, VkSwapchainKHR old_swapchain );
void                                       vk_swapchain_destroy( r_window_data_t* window );
void                                       vk_swapchain_recreate( r_window_data_t* window );

bool                                       vk_create_backbuffer( r_window_data_t* window );

// --------------------------------------------------------------------------------------------
// Render Functions


