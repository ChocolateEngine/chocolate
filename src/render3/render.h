#pragma once

#include "irender3.h"

#include <vulkan/vulkan.h>

// VMA
#include "vk_mem_alloc.h"


struct ImGuiContext;


// TEST - need to not use unordered_map
template< typename T >
struct handle_list_t
{
	std::unordered_map< u32, T > handles;

	handle_list_t()
	{
	}

	~handle_list_t()
	{
		handles.clear();
	}

	u32 create( T* data )
	{
		u32 handle = 1;

		while ( handles.find( handle ) != handles.end() )
		{
			handle++;
		}

		T& data = handles[ handle ];

		// replace the pointer
		*data = *data;

		return handle;
	}

	T* get( u32 handle )
	{
		auto it = handles.find( handle );
		if ( it == handles.end() )
		{
			return nullptr;
		}

		return &it->second;
	}

	void remove( u32 handle )
	{
		auto it = handles.find( handle );
		if ( it != handles.end() )
		{
			handles.erase( it );
		}
	}
};


// TEST - array that keeps track of used slots
template< typename T >
//struct handle_list2_t
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


// --------------------------------------------------------------------------------------------
// Basic Deletion Queue


// amount to allocate at a time
constexpr u32 VK_DELETE_QUEUE_BATCH_SIZE = 16;

// deletion function pointer
typedef void  ( *fn_vk_destroy )();


struct delete_queue_t
{
	fn_vk_destroy* funcs;
	u32            count;
	u32            capacity;

	delete_queue_t() :
		funcs( nullptr ), count( 0 ), capacity( 0 )
	{
	}

	~delete_queue_t()
	{
		ch_free( funcs );
	}

	inline void add( fn_vk_destroy func )
	{
		// are we at the capacity limit?
		if ( count >= capacity )
		{
			// allocate some more slots for function pointers
			capacity += VK_DELETE_QUEUE_BATCH_SIZE;
			funcs = ch_realloc< fn_vk_destroy >( funcs, capacity );
		}

		funcs[ count ] = func;
		count++;
	}

	inline void flush()
	{
		// call all the functions in the delete queue in reverse order
		for ( u32 i = count; i > 0; i-- )
		{
			funcs[ i ]();
		}

		// reset the count
		count = 0;
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


struct vk_desc_pool_size_ratio_t
{
	VkDescriptorType type;
	float            ratio;
};


struct vk_buffer_t
{
	VkBuffer       buffer;
	VkDeviceMemory memory;
	VkDeviceSize   size;
};


struct vk_image_t
{
	VkImage       image;
	VkImageView   view;
	VmaAllocation memory;
	VkExtent2D    extent;
	VkFormat      format;
};


struct vk_texture_t
{

};


struct vk_shader_module_create_t
{
	VkShaderStageFlagBits stage;
	const char*           path;
	const char* 		  entry;
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
// TODO: dont use ResourceList to store this, maybe use a free index queue like that entity system idea
struct r_window_data_t
{
	SDL_Window*           window    = nullptr;
	VkSurfaceKHR          surface   = VK_NULL_HANDLE;
	VkSwapchainKHR        swapchain = VK_NULL_HANDLE;

	// only allocates to swap_image_count for the main window buffers
	// probably could be moved elsewhere as well
	VkCommandBuffer*      command_buffers;

	// amount allocated is swap_image_count
	VkSemaphore*          semaphore_swapchain;  // signals that we have finished presenting the last image
	VkSemaphore*          semaphore_render;     // signals that we have finished executing the last command buffer

	// amount allocated is swap_image_count
	VkFence*              fence_render;

	// swapchain info - this could be moved elsewhere, as these are only used during window creation and destruction
	// also getting the surface size with swap_extent, to avoid having to call SDL_GetWindowSize, but is it worth it?
	// also backbuffer has a size parameter currently so this is kinda useless
	VkExtent2D            swap_extent;
	VkImage*              swap_images;
	VkImageView*          swap_image_views;

	// Contains the framebuffers which are to be drawn to during command buffer recording.
	// backbuffer_t       backbuffer;
	vk_image_t            draw_image;
	VkDescriptorSet       desc_draw_image = VK_NULL_HANDLE;

	delete_queue_t        delete_queue;

	fn_render_on_reset_t  fn_on_reset = nullptr;

	// here because of mem alignment, 2 unused bytes at the end
	u8                    frame_index = 0;
	u8                    swap_image_count;
};


// data for getting next image from swapchain (vkAcquireNextImageKHR)
// 
// swapchain
// fences
// frame_index
// semaphore_image_available
// 

// data for presenting a window (vkQueuePresentKHR)
// 
// swapchain
// fences
// fences_in_flight
// frame_index
// semaphore_image_available
// semaphore_render_finished
// command_buffers
// swap_image_count
// 


// --------------------------------------------------------------------------------------------


LOG_CHANNEL( Render );
LOG_CHANNEL( Vulkan );
LOG_CHANNEL( Validation );


CONVAR_BOOL_EXT( r_msaa_enabled );
CONVAR_INT_EXT( r_msaa_samples );


// --------------------------------------------------------------------------------------------


extern VkInstance                          g_vk_instance;
extern VkDevice                            g_vk_device;
extern VkPhysicalDevice                    g_vk_physical_device;

extern VmaAllocator                        g_vma;

extern VkQueue                             g_vk_queue_graphics;
extern VkQueue                             g_vk_queue_transfer;

extern u32                                 g_vk_queue_family_graphics;
extern u32                                 g_vk_queue_family_transfer;

extern VkPhysicalDeviceProperties          g_vk_device_properties;
extern VkPhysicalDeviceMemoryProperties    g_vk_device_memory_properties;
extern VkSurfaceCapabilitiesKHR            g_vk_surface_capabilities;
extern VkSurfaceFormatKHR                  g_vk_surface_format;

extern ResourceList< r_window_data_t >     g_windows;
extern ch_handle_t                          g_main_window;

extern handle_list_t< vk_image_t >         g_vk_images;

// global delete queue
extern delete_queue_t                      g_vk_delete_queue;

// TODO: when you multi-thread this, you need a graphics pool for each thread
extern VkCommandPool                       g_vk_command_pool_graphics;
extern VkCommandPool                       g_vk_command_pool_transfer;

extern VkCommandBuffer                     g_vk_command_buffer_transfer;

extern VkDescriptorPool                    g_vk_imgui_desc_pool;
extern VkDescriptorSetLayout               g_vk_imgui_desc_layout;

extern VkDescriptorPool                    g_vk_desc_pool;
extern VkDescriptorSetLayout               g_vk_desc_draw_image_layout;

// temp
extern VkPipeline                          g_pipeline_gradient;
extern VkPipelineLayout                    g_pipeline_gradient_layout;

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

void                                       vk_queue_wait_graphics();
void                                       vk_queue_wait_transfer();

// void                                       vk_add_delete( fn_vk_destroy* destroy_func );
// void                                       vk_flush_delete_queue();

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

bool                                       vk_device_create( VkSurfaceKHR surface );  // we dont destroy this

bool                                       vk_swapchain_create( r_window_data_t* window, VkSwapchainKHR old_swapchain );
void                                       vk_swapchain_destroy( r_window_data_t* window );
void                                       vk_swapchain_recreate( r_window_data_t* window );

bool                                       vk_backbuffer_create( r_window_data_t* window );
void                                       vk_backbuffer_destroy( r_window_data_t* window );

void                                       vk_command_pool_create();
void                                       vk_command_pool_destroy();
void                                       vk_command_pool_reset_graphics();

bool                                       vk_command_buffers_create( r_window_data_t* window );
void                                       vk_command_buffers_destroy( r_window_data_t* window );

VkCommandBuffer                            vk_cmd_transfer_begin();
void                                       vk_cmd_transfer_end( VkCommandBuffer command_buffer );

bool                                       vk_render_sync_create( r_window_data_t* window );
void                                       vk_render_sync_destroy( r_window_data_t* window );
void                                       vk_render_sync_reset( r_window_data_t* window );

void                                       vk_draw( ch_handle_t window_handle, r_window_data_t* window );
void                                       vk_reset( ch_handle_t window_handle, r_window_data_t* window, e_render_reset_flags flags );

void                                       vk_blit_image_to_image( VkCommandBuffer c, VkImage src, VkImage dst, VkExtent2D src_size, VkExtent2D dst_size );

bool                                       vk_shaders_init();
void                                       vk_shaders_shutdown();

VkDescriptorPool                           vk_descriptor_pool_create( const char* name, u32 max_sets, vk_desc_pool_size_ratio_t* pool_sizes, u32 pool_size_count );
bool                                       vk_descriptor_init();
void                                       vk_descriptor_destroy();
bool                                       vk_descriptor_allocate_window( r_window_data_t* window );
void                                       vk_descriptor_update_window( r_window_data_t* window );

// --------------------------------------------------------------------------------------------
// Allocator Functions

u32                                        vk_get_memory_type( u32 type_filter, VkMemoryPropertyFlags properties );

vk_buffer_t*                               vk_buffer_create( VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_bits );
vk_buffer_t*                               vk_buffer_create( const char* name, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags mem_bits );

void                                       vk_buffer_destroy( vk_buffer_t* buffer );

// returns how many bytes written to the buffer
VkDeviceSize                               vk_buffer_write( vk_buffer_t* buffer, void* data, VkDeviceSize size, VkDeviceSize offset = 0 );

// returns how many bytes read from the buffer
VkDeviceSize                               vk_buffer_read( vk_buffer_t* buffer, void* data, VkDeviceSize size, VkDeviceSize offset = 0 );

// copies data from one buffer to another in regions
bool                                       vk_buffer_copy( vk_buffer_t* src, vk_buffer_t* dst, VkDeviceSize size = 0, VkDeviceSize offset = 0 );

// copies data from one buffer to another in regions
bool                                       vk_buffer_copy_region( vk_buffer_t* src, vk_buffer_t* dst, VkBufferCopy* regions, u32 region_count );

// --------------------------------------------------------------------------------------------
// Render Functions


