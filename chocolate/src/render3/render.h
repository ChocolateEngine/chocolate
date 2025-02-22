#pragma once

#include <vulkan/vulkan.h>

#include "irender3.h"

// VMA
#include "vk_mem_alloc.h"


struct ImGuiContext;


CH_HANDLE_GEN_32( vk_image_h );


// --------------------------------------------------------------------------------------------


constexpr VkFormat g_draw_format              = VK_FORMAT_R16G16B16A16_SFLOAT;
// constexpr VkFormat g_draw_format  = VK_FORMAT_B8G8R8A8_SRGB;
constexpr VkFormat g_depth_format = VK_FORMAT_D32_SFLOAT;


enum e_backbuffer_image
{
	e_backbuffer_image_color,
	e_backbuffer_image_depth,
	e_backbuffer_image_resolve,
	e_backbuffer_image_count
};


// idea for when to draw shaders
// maybe call it e_material_sort, kind of like in DOOM 3?
enum e_render_stage
{
	e_render_stage_mesh,         // standard mesh rendering
	e_render_stage_skybox,       // drawn after meshes, before postprocess
	e_render_stage_viewmodel,    // drawn first
	e_render_stage_postprocess,
	e_render_stage_gui,          // drawn last

	e_render_stage_count,
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


struct vk_texture_load_info_t
{
	bool                 no_missing_texture;  // set to true if you don't want the missing texture returned
	bool                 depth_compare;
	VkSamplerAddressMode sampler_address;
	VkImageUsageFlags    usage  = VK_IMAGE_USAGE_SAMPLED_BIT;
	VkFilter             filter = VK_FILTER_LINEAR;
};


// Usually loaded with KTX Upload
struct vk_texture_t
{
	VkImage              image;
	VkImageView          view;
	VkDeviceMemory       memory;
	VkExtent3D           extent;
	VkFormat             format;
	VkImageViewType      view_type;
	VkImageUsageFlags    usage;

	VkFilter             filter;
	VkSamplerAddressMode sampler_address;
	VkBool32             depth_compare;

	u32                  mip_count;
	u32                  frame_count;

	size_t               data_size;
};


// --------------------------------------------------------------------------------------------
// Shaders


// Shader Descriptor Set Bindings
#define CH_BINDING_TEXTURES 0


struct shader_mat_var_desc_t
{
	e_mat_var   type;
	const char* name;
	const char* desc;
	u32         data_offset;

	// internal use
	r_texture_h default_texture_handle;

	union
	{
		const char* default_texture;  // Path To Default Texture
		float       default_float;
		int         default_int;
		glm::vec2   default_vec2;
		glm::vec3   default_vec3;
		glm::vec4   default_vec4;
	};

   private:
	shader_mat_var_desc_t( e_mat_var sType, const char* spName, const char* spDesc, u32 sDataOffset )
	{
		type        = sType;
		name        = spName;
		desc        = spDesc;
		data_offset = sDataOffset;
	}

   public:
	shader_mat_var_desc_t( const char* spName, const char* spDesc, const char* spDefault, u32 sDataOffset ) :
		shader_mat_var_desc_t( e_mat_var_string, spName, spDesc, sDataOffset )
	{
		default_texture = spDefault;
	}

	shader_mat_var_desc_t( const char* spName, const char* spDesc, float sDefault, u32 sDataOffset ) :
		shader_mat_var_desc_t( e_mat_var_float, spName, spDesc, sDataOffset )
	{
		default_float = sDefault;
	}

	shader_mat_var_desc_t( const char* spName, const char* spDesc, int sDefault, u32 sDataOffset ) :
		shader_mat_var_desc_t( e_mat_var_int, spName, spDesc, sDataOffset )
	{
		default_int = sDefault;
	}

	shader_mat_var_desc_t( const char* spName, const char* spDesc, glm::vec2 sDefault, u32 sDataOffset ) :
		shader_mat_var_desc_t( e_mat_var_vec2, spName, spDesc, sDataOffset )
	{
		default_vec2 = sDefault;
	}

	shader_mat_var_desc_t( const char* spName, const char* spDesc, glm::vec3 sDefault, u32 sDataOffset ) :
		shader_mat_var_desc_t( e_mat_var_vec3, spName, spDesc, sDataOffset )
	{
		default_vec3 = sDefault;
	}

	shader_mat_var_desc_t( const char* spName, const char* spDesc, glm::vec4 sDefault, u32 sDataOffset ) :
		shader_mat_var_desc_t( e_mat_var_vec4, spName, spDesc, sDataOffset )
	{
		default_vec4 = sDefault;
	}
};


#define CH_SHADER_MATERIAL_VAR( type, name, desc, default ) \
	{ #name, desc, default, offsetof( type, name ) },

#define CH_SHADER_MAT_VAR_OFFSET_BEGIN( struct_name, array_name ) \
	using __shader_material_struct_t          = struct_name;      \
	static shader_mat_var_desc_t array_name[] = {

#define CH_SHADER_MAT_VAR_OFFSET_END() \
	}                                  \
	;

#define CH_SHADER_MAT_VAR_OFFSET_VAR( name, desc, default ) \
	{ #name, desc, default, offsetof( __shader_material_struct_t, name ) },

// maybe?
#define CH_SHADER_MAT_VAR_OFFSET_TEXTURE( name, desc, default )                           \
	{ #name, desc, default, offsetof( __shader_material_struct_t, name ) },               \
	  { #name##_fps, desc, default, offsetof( __shader_material_struct_t, name##_fps ) }, \
	  { #name##_frame, desc, default, offsetof( __shader_material_struct_t, name##_frame ) }, \


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

	// attachment formats (why is this here?)
	u8                        color_attach_count;
	VkFormat*                 color_attach;
	VkFormat                  depth_attach;

	// push constants
	fn_vk_push_constants_t    fn_push_constants;
	VkPushConstantRange       push_constant_range;

	size_t                    material_var_count;
	shader_mat_var_desc_t*    material_var;

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
// Materials


// idea for texture rendering options,
// so you don't have to manually have so many copied parameters in the shader
// like "diffuse_frame", "diffuse_fps", "normal_frame", "normal_repeat" "clamp"
// unless you made a macro to do it
struct vk_texture_draw_t
{
	r_texture_h texture;

	// animated texture controls
	float       fps;
	int         frame_current;
	int         frame_count;

	// repeat value
	// filter method override from compiled ktx, cvar overrides this still
};


struct vk_material_var_t
{
	union
	{
		r_texture_h val_texture;
		int         val_int;
		float       val_float;
		glm::vec2   val_vec2;
		glm::vec3   val_vec3;
		glm::vec4   val_vec4;
	};
};


struct vk_material_t
{
	u32                shader;
	u32                gpu_index;
	u32                var_count;
	vk_material_var_t* var;
	e_mat_var*         type;
};


CH_HANDLE_GEN_32( vk_material_h );


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
	VkDescriptorSet      desc_draw_image = VK_NULL_HANDLE;  // only used for the compute shader test

	fn_render_on_reset_t fn_on_reset     = nullptr;

	// here because of mem alignment, 2 unused bytes at the end
	u8                   frame_index     = 0;
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


struct vk_mesh_material_t
{
	u32           vertex_offset;
	u32           vertex_count;

	u32           index_offset;
	u32           index_count;

	// TODO: we should have some internal mapping to a shader material eventually, kind of like in render2, but stored here
	vk_material_h material;  // base material to copy from
};


struct vk_mesh_t
{
	const char*         name;
	gpu_mesh_buffers_t  buffers;

	u32                 vertex_count;
	u32                 index_count;

	vk_mesh_material_t* material;
	size_t              material_count;
};


extern ch_handle_ref_list_32< r_mesh_h, vk_mesh_t > g_mesh_list;


// --------------------------------------------------------------------------------------------
// Mesh Render


struct r_mesh_render_surface_t
{
	u32 vertex_offset;
	u32 vertex_count;

	u32 index_offset;
	u32 index_count;
};


// Basic Mesh Rendering
struct r_mesh_render_t
{
	const char*              name;
	r_mesh_h                 mesh;

	glm::mat4                matrix;

	vk_material_h*           material;
	r_mesh_render_surface_t* surface;
	u32                      surface_count;
};


struct gpu_push_t
{
	//glm::mat4       world_matrix;
	glm::mat4       proj_view_matrix;
	//glm::mat4       view_matrix;
	//glm::mat4       proj_matrix;
	VkDeviceAddress vertex_address;
	int             diffuse;
	int             emissive;
	//int             ambient_occlusion;
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

extern IGraphicsData*                                        graphics_data;

extern VkInstance                                            g_vk_instance;
extern VkDevice                                              g_vk_device;
extern VkPhysicalDevice                                      g_vk_physical_device;

extern VmaAllocator                                          g_vma;

// Queues
extern VkQueue                                               g_vk_queue_graphics;
extern VkQueue                                               g_vk_queue_transfer;

extern u32                                                   g_vk_queue_family_graphics;
extern u32                                                   g_vk_queue_family_transfer;

// Device Info
extern VkPhysicalDeviceProperties                            g_vk_device_properties;
extern VkPhysicalDeviceMemoryProperties                      g_vk_device_memory_properties;
extern VkSurfaceCapabilitiesKHR                              g_vk_surface_capabilities;
extern VkSurfaceFormatKHR                                    g_vk_surface_format;

extern ch_handle_list_32< r_window_h, r_window_data_t, 1 >   g_windows;
extern r_window_h                                            g_main_window;

// Loaded Assets
extern ch_handle_list_32< vk_image_h, vk_image_t >           g_vk_images;
extern ch_handle_ref_list_32< r_texture_h, vk_texture_t >    g_textures;
extern ch_handle_ref_list_32< vk_material_h, vk_material_t > g_materials;

extern u32                                                   g_texture_count;
extern u32*                                                  g_texture_gpu_index;
extern u32                                                   g_texture_gpu_index_count;
extern bool                                                  g_texture_gpu_index_dirty;

// TODO: when you multi-thread this, you need a graphics pool for each thread
extern VkCommandPool                                         g_vk_command_pool_graphics;
extern VkCommandPool                                         g_vk_command_pool_transfer;

extern VkCommandBuffer                                       g_vk_command_buffer_transfer;

// Descriptors
extern VkDescriptorPool                                      g_vk_imgui_desc_pool;
extern VkDescriptorSetLayout                                 g_vk_imgui_desc_layout;

// temp
extern VkDescriptorPool                                      g_vk_desc_pool_graphics;
extern VkDescriptorSetLayout                                 g_vk_desc_layout_graphics;

extern VkDescriptorPool                                      g_vk_desc_pool;
extern VkDescriptorSetLayout                                 g_vk_desc_draw_image_layout;

extern VkDescriptorSet                                       g_texture_desc_set;
// extern VkDescriptorSetLayout                                 g_texture_desc_set_layout;

// temp
extern VkPipeline                                            g_pipeline_gradient;
extern VkPipelineLayout                                      g_pipeline_gradient_layout;

// graphics shader data
extern vk_shader_data_graphics_t*                            g_shader_data_graphics;
extern VkPipelineLayout*                                     g_shader_data_graphics_pipeline_layout;
extern VkPipeline*                                           g_shader_data_graphics_pipelines;
extern ch_string*                                            g_shader_data_graphics_names;
extern u32                                                   g_shader_data_graphics_count;

// extern slot_array_t< r_window_data_t >     g_windows2;
// extern SDL_Window**                        g_windows_sdl;
// extern ImGuiContext**                      g_windows_imgui_contexts;


// --------------------------------------------------------------------------------------------
// function pointers for debug utils

extern PFN_vkSetDebugUtilsObjectNameEXT                      pfnSetDebugUtilsObjectName;
extern PFN_vkSetDebugUtilsObjectTagEXT                       pfnSetDebugUtilsObjectTag;

extern PFN_vkQueueBeginDebugUtilsLabelEXT                    pfnQueueBeginDebugUtilsLabel;
extern PFN_vkQueueEndDebugUtilsLabelEXT                      pfnQueueEndDebugUtilsLabel;
extern PFN_vkQueueInsertDebugUtilsLabelEXT                   pfnQueueInsertDebugUtilsLabel;

extern PFN_vkCmdBeginDebugUtilsLabelEXT                      pfnCmdBeginDebugUtilsLabel;
extern PFN_vkCmdEndDebugUtilsLabelEXT                        pfnCmdEndDebugUtilsLabel;
extern PFN_vkCmdInsertDebugUtilsLabelEXT                     pfnCmdInsertDebugUtilsLabel;


// --------------------------------------------------------------------------------------------
// Vulkan Helper Functions

const char*                                                  vk_str( VkResult result );
const char*                                                  vk_object_str( VkObjectType type );
const char*                                                  vk_samples_str( VkSampleCountFlags counts );

void                                                         vk_check_f( VkResult sResult, char const* spArgs, ... );
void                                                         vk_check( VkResult sResult, char const* spMsg );
void                                                         vk_check( VkResult sResult );

// Non Fatal Version of it, is just an error, returns true if failed
bool                                                         vk_check_ef( VkResult sResult, char const* spArgs, ... );
bool                                                         vk_check_e( VkResult sResult, char const* spMsg );
bool                                                         vk_check_e( VkResult sResult );

void                                                         vk_set_name_ex( VkObjectType type, u64 handle, const char* name, const char* type_name );
void                                                         vk_set_name( VkObjectType type, u64 handle, const char* name );

void                                                         vk_queue_wait_graphics();
void                                                         vk_queue_wait_transfer();

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

bool                                                         vk_instance_create();
void                                                         vk_instance_destroy();

VkSurfaceKHR                                                 vk_surface_create( void* native_window, SDL_Window* sdl_window );
void                                                         vk_surface_destroy( VkSurfaceKHR surface );

bool                                                         vk_device_create( VkSurfaceKHR surface );  // we dont destroy this

bool                                                         vk_swapchain_create( r_window_data_t* window, VkSwapchainKHR old_swapchain );
void                                                         vk_swapchain_destroy( r_window_data_t* window );
void                                                         vk_swapchain_recreate( r_window_data_t* window );

// VkSampleCountFlagBits                                      vk_msaa_find_max_samples();
VkSampleCountFlags                                           vk_msaa_get_max_samples();
VkSampleCountFlagBits                                        vk_msaa_get_samples();
VkSampleCountFlagBits                                        vk_msaa_get_samples( bool use_msaa );

bool                                                         vk_backbuffer_create( r_window_data_t* window );
void                                                         vk_backbuffer_destroy( r_window_data_t* window );

void                                                         vk_command_pool_create();
void                                                         vk_command_pool_destroy();
void                                                         vk_command_pool_reset_graphics();

bool                                                         vk_command_buffers_create( r_window_data_t* window );
void                                                         vk_command_buffers_destroy( r_window_data_t* window );

VkCommandBuffer                                              vk_cmd_transfer_begin();
void                                                         vk_cmd_transfer_end( VkCommandBuffer command_buffer );

bool                                                         vk_render_sync_create( r_window_data_t* window );
void                                                         vk_render_sync_destroy( r_window_data_t* window );
void                                                         vk_render_sync_reset( r_window_data_t* window );

void                                                         vk_draw( r_window_h window_handle, r_window_data_t* window );
void                                                         vk_reset( r_window_h window_handle, r_window_data_t* window, e_render_reset_flags flags );
void                                                         vk_reset_all( e_render_reset_flags flags );

void                                                         vk_blit_image_to_image( VkCommandBuffer c, VkImage src, VkImage dst, VkExtent2D src_size, VkExtent2D dst_size );

VkDescriptorPool                                             vk_descriptor_pool_create( const char* name, u32 max_sets, vk_desc_pool_size_ratio_t* pool_sizes, u32 pool_size_count );
bool                                                         vk_descriptor_init();
void                                                         vk_descriptor_destroy();
bool                                                         vk_descriptor_allocate_window( r_window_data_t* window );
void                                                         vk_descriptor_update_window( r_window_data_t* window );

void                                                         vk_descriptor_textures_create();
void                                                         vk_descriptor_textures_update();
u32                                                          vk_descriptor_texture_get_index( r_texture_h handle );

VkSampler                                                    vk_sampler_get( VkFilter filter, VkSamplerAddressMode address_mode, VkBool32 depth_compare );
void                                                         vk_sampler_destroy_all();
void                                                         vk_sampler_recreate();

gpu_mesh_buffers_t                                           vk_mesh_upload( u32* indices, u32 index_count, gpu_vertex_t* vertices, u32 vertex_count );
gpu_mesh_buffers_t                                           vk_mesh_upload( gpu_vertex_t* vertices, u32 vertex_count );
void                                                         vk_mesh_free( gpu_mesh_buffers_t& mesh_buffers );

vk_material_h                                                vk_material_create( ch_material_h base_handle );
void                                                         vk_material_free( vk_material_h handle );
void                                                         vk_material_update();

vk_material_h                                                vk_material_find( ch_material_h base_handle );
vk_material_t*                                               vk_material_get( ch_material_h base_handle );
vk_material_t*                                               vk_material_get( vk_material_h handle );

// --------------------------------------------------------------------------------------------
// Shader System


bool                                                         vk_shaders_init();
void                                                         vk_shaders_shutdown();
bool                                                         vk_shaders_rebuild();
void                                                         vk_shaders_material_update( ch_material_h base_handle, vk_material_h handle );

// Returns the index of the shader, if it's UINT32_MAX, the shader isn't found
u32                                                          vk_shaders_find( const char* shader_name );


// --------------------------------------------------------------------------------------------
// Allocator Functions

u32                                                          vk_get_memory_type( u32 type_filter, VkMemoryPropertyFlags properties );

vk_buffer_t*                                                 vk_buffer_create( VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage mem_usage );
vk_buffer_t*                                                 vk_buffer_create( const char* name, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage mem_usage );

void                                                         vk_buffer_destroy( vk_buffer_t* buffer );

// --------------------------------------------------------------------------------------------
// Texture Functions

bool                                                         ktx_init();
void                                                         ktx_shutdown();
bool                                                         ktx_load( const char* path, vk_texture_t* texture );

bool                                                         texture_load( r_texture_h& handle, const char* path, vk_texture_load_info_t& load_info );
r_texture_h                                                  texture_load( const char* path, vk_texture_load_info_t& load_info );
void                                                         texture_free( r_texture_h& handle );

bool                                                         texture_create_missing();


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
