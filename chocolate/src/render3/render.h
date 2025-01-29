#pragma once

#include <vulkan/vulkan.h>

#include "irender3.h"

// VMA
#include "vk_mem_alloc.h"


struct ImGuiContext;


CH_HANDLE_GEN_32( vk_image_h );


// TEST - array that keeps track of used slots
template< typename T >
//struct handle_list2_t
struct slot_array_t
{
	T*   data;
	u32* free_slots;  // list of free slots
	u32  count;       // count of used slots

	slot_array_t() :
		data( nullptr ), count( 0 )
	{
	}

	slot_array_t( u32 count ) :
		data( nullptr ), count( count )
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

			u32 slot     = free_slots[ count ];
			data[ slot ] = value;
			count++;

			return data[ slot ];
		}

		data          = ch_realloc< T >( data, count + 1 );
		data[ count ] = value;
		count++;

		return data[ count - 1 ];
	}
};


// --------------------------------------------------------------------------------------------
// Basic Deletion Queue


constexpr VkFormat g_draw_format              = VK_FORMAT_R16G16B16A16_SFLOAT;
constexpr VkFormat g_depth_format             = VK_FORMAT_D32_SFLOAT;


// amount to allocate at a time
constexpr u32      VK_DELETE_QUEUE_BATCH_SIZE = 16;

// deletion function pointer
typedef void       ( *fn_vk_destroy )();


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
	VkBuffer          buffer;
	VmaAllocation     alloc;
	VmaAllocationInfo info;  // TODO: see how often this is used, might be a good idea to store this elsewhere if it's rarely used
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


// --------------------------------------------------------------------------------------------
// shaders


struct vk_shader_push_data_t
{
};


typedef void ( *fn_vk_push_constants_t )( VkCommandBuffer cmd, VkPipelineLayout layout, const vk_shader_push_data_t& push_data );


struct vk_shader_module_create_t
{
	VkShaderStageFlagBits stage;
	const char*           path;
	const char*           entry;
};


struct vk_shader_create_graphics_t
{
	const char*               name;
	vk_shader_module_create_t modules[ 2 ];  // vertex and fragment

	VkPrimitiveTopology       topology;
	VkCullModeFlags           cull_mode;
	VkPolygonMode             polygon_mode;

	// dynamic state, why are these not flags in vulkan
	VkDynamicState*           dynamic_state;
	u8                        dynamic_state_count;

	// attachment formats
	u8                        color_attach_count;
	VkFormat*                 color_attach;
	VkFormat                  depth_attach;

	// push constants
	fn_vk_push_constants_t    fn_push_constants;
	VkPushConstantRange       push_constant_range;


	// descriptor set bindings

	// material data (maybe use encoding pattern for this?)
	// vk_shader_create_material_t material;
};


struct vk_shader_data_graphics_t
{
	VkShaderModule modules[ 2 ];
};


void vk_shaders_register_graphics( vk_shader_create_graphics_t* shader_create );


struct shader_static_register_graphics_t
{
	shader_static_register_graphics_t( vk_shader_create_graphics_t& srCreate )
	{
		vk_shaders_register_graphics( &srCreate );
	}
};


#define CH_SHADER_REGISTER_GRAPHICS( shader_create_struct ) \
	static shader_static_register_graphics_t g_shader_register_##shader_create_struct( shader_create_struct );


// --------------------------------------------------------------------------------------------


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
// TODO: remove the draw image stuff in this, that should probably be dependent on the game code
// TODO: maybe use a free index queue like that entity system idea
struct r_window_data_t
{
	SDL_Window*          window    = nullptr;
	VkSurfaceKHR         surface   = VK_NULL_HANDLE;
	VkSwapchainKHR       swapchain = VK_NULL_HANDLE;

	// only allocates to swap_image_count for the main window buffers
	// probably could be moved elsewhere as well
	VkCommandBuffer      command_buffers[ 2 ];

	// amount allocated is swap_image_count
	VkSemaphore          semaphore_swapchain[ 2 ];  // signals that we have finished presenting the last image
	VkSemaphore          semaphore_render[ 2 ];     // signals that we have finished executing the last command buffer

	// amount allocated is swap_image_count
	VkFence              fence_render[ 2 ];

	// swapchain info - this could be moved elsewhere, as these are only used during window creation and destruction
	// also getting the surface size with swap_extent, to avoid having to call SDL_GetWindowSize, but is it worth it?
	// also backbuffer has a size parameter currently so this is kinda useless
	VkExtent2D           swap_extent;
	VkImage              swap_images[ 2 ];
	VkImageView          swap_image_views[ 2 ];

	// Contains the framebuffers which are to be drawn to during command buffer recording.
	// backbuffer_t       backbuffer;
	vk_image_t           draw_image;
	vk_image_t           draw_image_resolve;
	vk_image_t           draw_image_depth;
	VkDescriptorSet      desc_draw_image         = VK_NULL_HANDLE;  // only used for the compute shader test

	fn_render_on_reset_t fn_on_reset             = nullptr;

	// here because of mem alignment, 2 unused bytes at the end
	u8                   frame_index             = 0;
	u8                   swap_image_count;
	bool                 need_resize = false;
};


// temporary testing
struct test_compute_push_t
{
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
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
// Test Render Stuff


// must match shader
struct gpu_vertex_t
{
	glm::vec3 pos;
	float     uv_x;
	glm::vec3 normal;
	float     uv_y;
	glm::vec4 color;
};


struct gpu_mesh_buffers_t
{
	vk_buffer_t*    index;
	vk_buffer_t*    vertex;
	VkDeviceAddress vertex_address;
};


struct vk_mesh_t
{
	const char*        name;
	gpu_mesh_buffers_t buffers;

	u32                vertex_count;
	u32                index_count;

	// TODO: store materials here
};


// struct vk_mesh_h
// {
// 	u32 id;
// 	u32 generation;
// };
//
//
// struct vk_mesh_list
// {
// 	u32        count;
// 	vk_mesh_h* handles;
// 	vk_mesh_t* mesh;
// 	u16*       ref_count;
// };

extern ch_handle_ref_list_32< r_mesh_h, vk_mesh_t > g_mesh_list;


struct mesh_render_surface_t
{
	u32 start_index;
	u32 count;
};


// Basic Mesh Rendering, does not point to any model once created
struct r_mesh_render_t
{
	const char*            name;
	r_mesh_h               mesh;

	glm::mat4              matrix;

	mesh_render_surface_t* surface;
	u32                    surface_count;

	// gpu_mesh_buffers_t     buffers;
};


struct gpu_push_t
{
	//glm::mat4       world_matrix;
	glm::mat4       proj_view_matrix;
	//glm::mat4       view_matrix;
	//glm::mat4       proj_matrix;
	VkDeviceAddress vertex_address;
};


struct test_render_t
{
	// Contains the framebuffers which are to be drawn to during command buffer recording.
	// vk_image_t         draw_image;
	// VkDescriptorSet    desc_draw_image = VK_NULL_HANDLE;
	glm::mat4          view_mat;
	glm::mat4          proj_mat;
	glm::mat4          proj_view_mat;

	gpu_mesh_buffers_t rectangle;
};


extern ch_handle_list_32< r_mesh_render_h, r_mesh_render_t > g_mesh_render_list;
extern test_render_t                                         g_test_render;


// --------------------------------------------------------------------------------------------
// Experimenting with how the renderer will actually store all data to render


// Maybe have a basic scene struct you can render to individual render targets?

struct r_scene_t
{
	u32              mesh_render_count;
	r_mesh_render_h* mesh_render;

	// u32        light_count;
	// r_light_h* light;

	// particles?
	// post processing somehow?
	// debug drawing?

	VkViewport       viewport;
	VkRect2D         scissor;
};


// --------------------------------------------------------------------------------------------


LOG_CHANNEL( Render );
LOG_CHANNEL( Vulkan );
LOG_CHANNEL( Validation );


CONVAR_BOOL_EXT( r_msaa_enabled );
CONVAR_INT_EXT( r_msaa_samples );
CONVAR_INT_EXT( r_msaa_textures );
CONVAR_FLOAT_EXT( r_msaa_textures_min );
CONVAR_BOOL_EXT( vk_render_scale_nearest );


// --------------------------------------------------------------------------------------------

extern VkInstance                                          g_vk_instance;
extern VkDevice                                            g_vk_device;
extern VkPhysicalDevice                                    g_vk_physical_device;

extern VmaAllocator                                        g_vma;

extern VkQueue                                             g_vk_queue_graphics;
extern VkQueue                                             g_vk_queue_transfer;

extern u32                                                 g_vk_queue_family_graphics;
extern u32                                                 g_vk_queue_family_transfer;

extern VkPhysicalDeviceProperties                          g_vk_device_properties;
extern VkPhysicalDeviceMemoryProperties                    g_vk_device_memory_properties;
extern VkSurfaceCapabilitiesKHR                            g_vk_surface_capabilities;
extern VkSurfaceFormatKHR                                  g_vk_surface_format;

extern ch_handle_list_32< r_window_h, r_window_data_t, 1 > g_windows;
extern r_window_h                                          g_main_window;

extern ch_handle_list_32< vk_image_h, vk_image_t >         g_vk_images;

// global delete queue
extern delete_queue_t                                      g_vk_delete_queue;

// TODO: when you multi-thread this, you need a graphics pool for each thread
extern VkCommandPool                                       g_vk_command_pool_graphics;
extern VkCommandPool                                       g_vk_command_pool_transfer;

extern VkCommandBuffer                                     g_vk_command_buffer_transfer;

extern VkDescriptorPool                                    g_vk_imgui_desc_pool;
extern VkDescriptorSetLayout                               g_vk_imgui_desc_layout;

// temp
extern VkDescriptorPool                                    g_vk_desc_pool_graphics;
extern VkDescriptorSetLayout                               g_vk_desc_layout_graphics;

extern VkDescriptorPool                                    g_vk_desc_pool;
extern VkDescriptorSetLayout                               g_vk_desc_draw_image_layout;

// temp
extern VkPipeline                                          g_pipeline_gradient;
extern VkPipelineLayout                                    g_pipeline_gradient_layout;

// graphics shader data
extern vk_shader_data_graphics_t*                          g_shader_data_graphics;
extern VkPipelineLayout*                                   g_shader_data_graphics_pipeline_layout;
extern VkPipeline*                                         g_shader_data_graphics_pipelines;
extern ch_string*                                          g_shader_data_graphics_names;
extern u32                                                 g_shader_data_graphics_count;

// extern slot_array_t< r_window_data_t >     g_windows2;
// extern SDL_Window**                        g_windows_sdl;
// extern ImGuiContext**                      g_windows_imgui_contexts;


// function pointers for debug utils

extern PFN_vkSetDebugUtilsObjectNameEXT                    pfnSetDebugUtilsObjectName;
extern PFN_vkSetDebugUtilsObjectTagEXT                     pfnSetDebugUtilsObjectTag;

extern PFN_vkQueueBeginDebugUtilsLabelEXT                  pfnQueueBeginDebugUtilsLabel;
extern PFN_vkQueueEndDebugUtilsLabelEXT                    pfnQueueEndDebugUtilsLabel;
extern PFN_vkQueueInsertDebugUtilsLabelEXT                 pfnQueueInsertDebugUtilsLabel;

extern PFN_vkCmdBeginDebugUtilsLabelEXT                    pfnCmdBeginDebugUtilsLabel;
extern PFN_vkCmdEndDebugUtilsLabelEXT                      pfnCmdEndDebugUtilsLabel;
extern PFN_vkCmdInsertDebugUtilsLabelEXT                   pfnCmdInsertDebugUtilsLabel;


// --------------------------------------------------------------------------------------------
// Vulkan Helper Functions

const char*                                                vk_str( VkResult result );
const char*                                                vk_object_str( VkObjectType type );
const char*                                                vk_samples_str( VkSampleCountFlags counts );

void                                                       vk_check_f( VkResult sResult, char const* spArgs, ... );
void                                                       vk_check( VkResult sResult, char const* spMsg );
void                                                       vk_check( VkResult sResult );

// Non Fatal Version of it, is just an error, returns true if failed
bool                                                       vk_check_ef( VkResult sResult, char const* spArgs, ... );
bool                                                       vk_check_e( VkResult sResult, char const* spMsg );
bool                                                       vk_check_e( VkResult sResult );

void                                                       vk_set_name_ex( VkObjectType type, u64 handle, const char* name, const char* type_name );
void                                                       vk_set_name( VkObjectType type, u64 handle, const char* name );

void                                                       vk_queue_wait_graphics();
void                                                       vk_queue_wait_transfer();

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

bool                                                       vk_instance_create();
void                                                       vk_instance_destroy();

VkSurfaceKHR                                               vk_surface_create( void* native_window, SDL_Window* sdl_window );
void                                                       vk_surface_destroy( VkSurfaceKHR surface );

bool                                                       vk_device_create( VkSurfaceKHR surface );  // we dont destroy this

bool                                                       vk_swapchain_create( r_window_data_t* window, VkSwapchainKHR old_swapchain );
void                                                       vk_swapchain_destroy( r_window_data_t* window );
void                                                       vk_swapchain_recreate( r_window_data_t* window );

// VkSampleCountFlagBits                                      vk_msaa_find_max_samples();
VkSampleCountFlags                                         vk_msaa_get_max_samples();
VkSampleCountFlagBits                                      vk_msaa_get_samples();
VkSampleCountFlagBits                                      vk_msaa_get_samples( bool use_msaa );

bool                                                       vk_backbuffer_create( r_window_data_t* window );
void                                                       vk_backbuffer_destroy( r_window_data_t* window );

void                                                       vk_command_pool_create();
void                                                       vk_command_pool_destroy();
void                                                       vk_command_pool_reset_graphics();

bool                                                       vk_command_buffers_create( r_window_data_t* window );
void                                                       vk_command_buffers_destroy( r_window_data_t* window );

VkCommandBuffer                                            vk_cmd_transfer_begin();
void                                                       vk_cmd_transfer_end( VkCommandBuffer command_buffer );

bool                                                       vk_render_sync_create( r_window_data_t* window );
void                                                       vk_render_sync_destroy( r_window_data_t* window );
void                                                       vk_render_sync_reset( r_window_data_t* window );

void                                                       vk_draw( r_window_h window_handle, r_window_data_t* window );
void                                                       vk_reset( r_window_h window_handle, r_window_data_t* window, e_render_reset_flags flags );
void                                                       vk_reset_all( e_render_reset_flags flags );

void                                                       vk_blit_image_to_image( VkCommandBuffer c, VkImage src, VkImage dst, VkExtent2D src_size, VkExtent2D dst_size );

bool                                                       vk_shaders_init();
void                                                       vk_shaders_shutdown();
bool                                                       vk_shaders_rebuild();
void                                                       vk_shaders_update_push_constants();

// Returns the index + 1 of the shader, if it's 0, the shader isn't found
u32                                                        vk_shaders_find( const char* shader_name );

VkDescriptorPool                                           vk_descriptor_pool_create( const char* name, u32 max_sets, vk_desc_pool_size_ratio_t* pool_sizes, u32 pool_size_count );
bool                                                       vk_descriptor_init();
void                                                       vk_descriptor_destroy();
bool                                                       vk_descriptor_allocate_window( r_window_data_t* window );
void                                                       vk_descriptor_update_window( r_window_data_t* window );

gpu_mesh_buffers_t                                         vk_mesh_upload( u32* indices, u32 index_count, gpu_vertex_t* vertices, u32 vertex_count );
gpu_mesh_buffers_t                                         vk_mesh_upload( gpu_vertex_t* vertices, u32 vertex_count );
void                                                       vk_mesh_free( gpu_mesh_buffers_t& mesh_buffers );

// --------------------------------------------------------------------------------------------
// Allocator Functions

u32                                                        vk_get_memory_type( u32 type_filter, VkMemoryPropertyFlags properties );

vk_buffer_t*                                               vk_buffer_create( VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage mem_usage );
vk_buffer_t*                                               vk_buffer_create( const char* name, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage mem_usage );

void                                                       vk_buffer_destroy( vk_buffer_t* buffer );

#if 0

struct vk_buffer_t
{
	VkBuffer       buffer;
	VkDeviceMemory memory;
	VkDeviceSize   size;
};

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

#endif

// --------------------------------------------------------------------------------------------
// Render Functions
