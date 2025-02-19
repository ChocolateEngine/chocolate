#include "render.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

LOG_CHANNEL_REGISTER( Render, ELogColor_Cyan );
LOG_CHANNEL_REGISTER( Vulkan );
LOG_CHANNEL_REGISTER( Validation );

ch_handle_list_32< r_window_h, r_window_data_t, 1 >   g_windows;
r_window_h                                            g_main_window;

SDL_Window**                                          g_windows_sdl;
ImGuiContext**                                        g_windows_imgui_contexts;

ch_handle_list_32< vk_image_h, vk_image_t >           g_vk_images;

// HACK HACK HACK HACK
// VULKAN NEEDS THE SURFACE BEFORE WE CREATE A DEVICE
// only so we can see if the device can actually create a surface with the right format and capabilities
VkSurfaceKHR                                          g_surface_hack         = VK_NULL_HANDLE;
SDL_Window*                                           g_window_hack          = nullptr;
void*                                                 g_window_hack_native   = nullptr;

VmaAllocator                                          g_vma                  = VK_NULL_HANDLE;

constexpr const char*                                 CH_DEFAULT_WINDOW_NAME = "Vulkan Window";

IGraphicsData*                                        graphics_data          = nullptr;

// each ref count allocated starts at one here when uploaded, then incremeneted when a mesh render is created
// which would be a ref count of 2 for one mesh render
ch_handle_ref_list_32< r_mesh_h, vk_mesh_t >          g_mesh_list;
ch_handle_list_32< r_mesh_render_h, r_mesh_render_t > g_mesh_render_list;

test_render_t                                         g_test_render;


// interface for the renderer
struct Render3 final : public IRender3
{
	// --------------------------------------------------------------------------------------------
	// ISystem Functions
	// --------------------------------------------------------------------------------------------

	bool Init() override
	{
		graphics_data = Mod_GetSystemCast< IGraphicsData >( CH_GRAPHICS_DATA, CH_GRAPHICS_DATA_VER );

		// create the vulkan instance
		if ( !vk_instance_create() )
		{
			Log_ErrorF( "Failed to create vulkan instance" );
			return false;
		}

		// stupid ugly hack
		if ( g_window_hack == nullptr )
		{
			Log_FatalF( "Forgot to call SetMainSurface before GraphicsAPI Init with SDL_Window* because vulkan funny\n" );
			return false;
		}

		g_surface_hack = vk_surface_create( g_window_hack_native, g_window_hack );

		if ( g_surface_hack == VK_NULL_HANDLE )
		{
			Log_Error( gLC_Render, "Failed to create surface hack\n" );
			return false;
		}

		if ( !vk_device_create( g_surface_hack ) )
		{
			Log_Error( gLC_Render, "Failed to create device\n" );
			return false;
		}

		vk_command_pool_create();

		// init vma
		VmaAllocatorCreateInfo vma_create{};
		vma_create.physicalDevice   = g_vk_physical_device;
		vma_create.device           = g_vk_device;
		vma_create.instance         = g_vk_instance;
		vma_create.vulkanApiVersion = VK_HEADER_VERSION_COMPLETE;

		vma_create.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		if ( vmaCreateAllocator( &vma_create, &g_vma ) != VK_SUCCESS )
		{
			Log_Error( gLC_Render, "Failed to create VMA allocator\n" );
			return false;
		}

		if ( !vk_descriptor_init() )
		{
			Log_Error( gLC_Render, "Failed to init descriptor pool and layouts\n" );
			return false;
		}

		if ( !ktx_init() )
		{
			Log_Error( gLC_Render, "Failed to init KTX texture loader\n" );
			return false;
		}

		if ( !vk_shaders_init() )
		{
			Log_Error( gLC_Render, "Failed to create shaders\n" );
			return false;
		}

		// create the missing texture

		// TODO: later add in other parts of the renderer, like the shader system

		test_init();

		return true;
	}

	void Shutdown()
	{
		// free windows
		for ( u32 i = g_windows.capacity - 1; i > 0; i-- )
		{
			if ( g_windows.use_list[ i ] )
				window_free( { i, g_windows.generation[ i ] } );
		}

		ktx_shutdown();

		vk_shaders_shutdown();
		vk_descriptor_destroy();

		vmaDestroyAllocator( g_vma );

		vk_command_pool_destroy();
		vk_instance_destroy();
	}

	void Update( float sDT ) override
	{
		// check for dirty materials and update them
	}

	// --------------------------------------------------------------------------------------------
	// General
	// EACH ONE SHOULD HAVE THEIR OWN IMGUI CONTEXT !!!!
	// --------------------------------------------------------------------------------------------

	// Required to be called before Init(), blame vulkan
	// TODO: If this is not called, we will be in headless mode
	void set_main_surface( SDL_Window* window, void* native_window ) override
	{
		g_window_hack        = window;
		g_window_hack_native = native_window;
	}

	const char* get_device_name() override
	{
		return g_vk_device_properties.deviceName;
	}

	// --------------------------------------------------------------------------------------------
	// Windows
	// EACH ONE SHOULD HAVE THEIR OWN IMGUI CONTEXT !!!!
	// --------------------------------------------------------------------------------------------

	r_window_h window_create( SDL_Window* sdl_window, void* native_window = nullptr ) override
	{
		r_window_h       window_handle{};
		r_window_data_t* window = nullptr;

		const char*      title  = SDL_GetWindowTitle( sdl_window ) == nullptr ? SDL_GetWindowTitle( sdl_window ) : CH_DEFAULT_WINDOW_NAME;

		if ( !g_windows.create( window_handle, &window ) )
		{
			Log_ErrorF( gLC_Render, "Failed to allocate render window data for: \"%s\"\n", title );
			return {};
		}

		window->window = sdl_window;

		// fun
		if ( sdl_window == g_window_hack )
		{
			window->surface = g_surface_hack;
			g_main_window   = window_handle;
		}
		else
		{
			window->surface = vk_surface_create( native_window, sdl_window );

			if ( window->surface == VK_NULL_HANDLE )
			{
				Log_ErrorF( gLC_Render, "Failed to create surface for window: \"%s\"\n", title );
				g_windows.free( window_handle );
				return {};
			}
		}

		if ( !vk_swapchain_create( window, VK_NULL_HANDLE ) )
		{
			Log_ErrorF( gLC_Render, "Failed to create swapchain for window: \"%s\"\n", title );
			window_free( window_handle );
			return {};
		}

		if ( !vk_backbuffer_create( window ) )
		{
			Log_ErrorF( gLC_Render, "Failed to create backbuffer for window: \"%s\"\n", title );
			window_free( window_handle );
			return {};
		}

		if ( !vk_command_buffers_create( window ) )
		{
			Log_ErrorF( gLC_Render, "Failed to create command buffers for window: \"%s\"\n", title );
			window_free( window_handle );
			return {};
		}

		if ( !vk_render_sync_create( window ) )
		{
			Log_ErrorF( gLC_Render, "Failed to create fences and semaphores for window: \"%s\"\n", title );
			window_free( window_handle );
			return {};
		}

	//	if ( !vk_descriptor_allocate_window( window ) )
	//	{
	//		Log_Error( gLC_Render, "Failed to allocate descriptor sets for window\n" );
	//		return {};
	//	}
		
		// init imgui
		ImGui_ImplVulkan_InitInfo imgui_init{};
		imgui_init.Instance            = g_vk_instance;
		imgui_init.PhysicalDevice      = g_vk_physical_device;
		imgui_init.Device              = g_vk_device;
		imgui_init.Queue               = g_vk_queue_graphics;
		imgui_init.DescriptorPool      = g_vk_imgui_desc_pool;
		imgui_init.RenderPass          = VK_NULL_HANDLE;
		imgui_init.UseDynamicRendering = true;
		imgui_init.MinImageCount       = window->swap_image_count;
		imgui_init.ImageCount          = window->swap_image_count;
		imgui_init.CheckVkResultFn     = vk_check;
		imgui_init.MSAASamples         = VK_SAMPLE_COUNT_1_BIT;  // no msaa yet

		//dynamic rendering parameters for imgui to use
		imgui_init.PipelineRenderingCreateInfo                         = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		imgui_init.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
		imgui_init.PipelineRenderingCreateInfo.pColorAttachmentFormats = &g_vk_surface_format.format;

		// VkPipelineRenderingCreateInfoKHR

		if ( !ImGui_ImplVulkan_Init( &imgui_init ) )
		{
			Log_ErrorF( gLC_Render, "Failed to init ImGui Vulkan for Window: \"%s\"\n", title );
			window_free( window_handle );
			return {};
		}

		// upload fonts
		if ( !ImGui_ImplVulkan_CreateFontsTexture() )
		{
			Log_ErrorF( gLC_Render, "Failed to create ImGui fonts for Window: \"%s\"\n", title );
			window_free( window_handle );
			return {};
		}

		if ( !ImGui_ImplSDL2_InitForVulkan( sdl_window ) )
 		{
 			Log_ErrorF( gLC_Render, "Failed to init ImGui SDL2 for Vulkan Window: \"%s\"\n", title );
 			window_free( window_handle );
			return {};
 		}

		return window_handle;
	}

	void window_free( r_window_h window ) override
	{
		r_window_data_t* window_data = g_windows.get( window );
		if ( !window_data )
		{
			Log_ErrorF( gLC_Render, "Failed to find window to free!\n" );
			return;
		}

		vk_queue_wait_graphics();

		// free vulkan resources
		vk_render_sync_destroy( window_data );
		vk_backbuffer_destroy( window_data );
		vk_command_buffers_destroy( window_data );
		vk_swapchain_destroy( window_data );
		vk_surface_destroy( window_data->surface );

		// free imgui resources
		ImGui_ImplVulkan_Shutdown();

		// free window data
		g_windows.free( window );
	}

	void window_set_reset_callback( r_window_h window, fn_render_on_reset_t func )
	{
		r_window_data_t* window_data = g_windows.get( window );
		if ( !window_data )
		{
			Log_ErrorF( gLC_Render, "Failed to find window to set reset callback!\n" );
			return;
		}

		window_data->fn_on_reset = func;
	}
	
	glm::uvec2 window_surface_size( r_window_h window ) override
	{
		r_window_data_t* window_data = g_windows.get( window );
		if ( !window_data )
		{
			Log_ErrorF( gLC_Render, "Failed to find window to get surface size!\n" );
			return {};
		}

		// return the swap extent
		glm::uvec2 size = { window_data->swap_extent.width, window_data->swap_extent.height };
		return size;
	}

	// --------------------------------------------------------------------------------------------
	// Rendering
	// --------------------------------------------------------------------------------------------

	void new_frame( r_window_h window_handle ) override
	{
		r_window_data_t* window = g_windows.get( window_handle );
		if ( !window )
		{
			Log_ErrorF( gLC_Render, "Failed to find window to call new_frame() on!\n" );
			return;
		}

		if ( window->need_resize )
		{
			vk_reset( window_handle, window, e_render_reset_flags_resize );
			window->need_resize = false;
		}

		ImGui_ImplVulkan_NewFrame();

		if ( g_texture_gpu_index_dirty )
			vk_descriptor_textures_update();
	}

	void reset( r_window_h window_handle ) override
	{
		r_window_data_t* window = g_windows.get( window_handle );
		if ( !window )
		{
			Log_ErrorF( gLC_Render, "Failed to find window to reset!\n" );
			return;
		}

		vk_reset( window_handle, window, e_render_reset_flags_resize );
	}

	void present( r_window_h window_handle ) override
	{
		r_window_data_t* window = g_windows.get( window_handle );
		if ( !window )
		{
			Log_ErrorF( gLC_Render, "Failed to find window to present to!\n" );
			return;
		}

		// do rendering

		// present the window

		vk_draw( window_handle, window );
	}
	// --------------------------------------------------------------------------------------------
	// Textures
	// --------------------------------------------------------------------------------------------

#if 0
	// Creates a new texture
	ch_texture_h texture_load( const char* path, s64 pathLen, const texture_load_info_t& create_info ) override
	{
		if ( !ch_str_ends_with( path, pathLen, ".ktx", 4 ) )
		{
			Log_ErrorF( "We only support KTX textures\n" );
			return CH_INVALID_HANDLE;
		}

		return texture_load_ktx( path, pathLen, create_info );
	}


	ch_handle_t* texture_load( const char** paths, s64* pathLens, const texture_load_info_t* create_infos, size_t count = 1 ) override
	{
		// for each path
		ch_handle_t* textures = ch_calloc< ch_handle_t >( count );

		if ( pathLens )
		{
			for ( size_t i = 0; i < count; i++ )
			{
				textures[ i ] = texture_load( paths[ i ], pathLens[ i ], create_infos[ i ] );
			}
		}
		else
		{
			for ( size_t i = 0; i < count; i++ )
			{
				textures[ i ] = texture_load( paths[ i ], strlen( paths[ i ] ), create_infos[ i ] );
			}
		}

		return textures;
	}


	ch_handle_t* texture_load( const char** paths, const texture_load_info_t* create_infos, size_t count = 1 ) override
	{
		return texture_load( paths, nullptr, create_infos, count );
	}


	// ch_handle_t texture_load( ch_string* paths, texture_load_info_t* create_infos, size_t count = 1 ) override
	// {
	// }

	void texture_free( ch_handle_t* texture, size_t count = 1 ) override
	{
	}

	//	texture_data_t texture_get_data( ch_handle_t texture ) override
	//	{
	//	}
#endif

	// --------------------------------------------------------------------------------------------
	// Test Rendering
	// --------------------------------------------------------------------------------------------

	bool test_init() override
	{
		// create a test rectangle
		gpu_vertex_t verts[ 4 ];
		u32          indices[ 6 ];

		verts[ 0 ].pos          = { 0.5, -0.5, 0 };
		verts[ 1 ].pos          = { 0.5, 0.5, 0 };
		verts[ 2 ].pos          = { -0.5, -0.5, 0 };
		verts[ 3 ].pos          = { -0.5, 0.5, 0 };

		verts[ 0 ].color        = { 0, 0, 0, 1 };
		verts[ 1 ].color        = { 0.5, 0.5, 0.5, 1 };
		verts[ 2 ].color        = { 1, 0, 0, 1 };
		verts[ 3 ].color        = { 0, 1, 0, 1 };

		indices[ 0 ]            = 0;
		indices[ 1 ]            = 1;
		indices[ 2 ]            = 2;

		indices[ 3 ]            = 2;
		indices[ 4 ]            = 1;
		indices[ 5 ]            = 3;

		// g_test_render.rectangle = vk_mesh_upload( indices, 6, verts, 4 );

		return true;
	}

	void test_shutdown() override
	{
		// vk_mesh_free( g_test_render.rectangle );
	}

	void test_update( float frame_time, r_window_h window, glm::mat4 view, glm::mat4 projection ) override
	{
		g_test_render.view_mat      = view;
		g_test_render.proj_mat      = projection;
		g_test_render.proj_view_mat = projection * view;
	}

	// TODO: make a separate r_mesh handle the app needs to hold on to, and make a mesh_render_create() function
	// while we could just use ch_model_h, that is supposed to point to the model in graphics_data, which can cause confusion
	// and if the mesh isn't uploaded, and the handle isn't valid in graphics_data, nothing gets uploaded

	// This combines all meshes in a model into one, and uploads it to the gpu
	// It creates a new handle for it for the app code to store
	// Then the app can use this handle to make new mesh render instances
	r_mesh_h mesh_upload( ch_model_h model_handle ) override
	{
		model_t* model = graphics_data->model_get( model_handle );

		if ( !model )
			return {};

		ch_material_h* mats        = 0;
		u32            total_mats  = 0;

		u32            total_verts = 0;
		u32            total_ind   = 0;

		// Get the total amount of vertices and indices in all meshes
		for ( u32 mesh_i = 0; mesh_i < model->mesh_count; mesh_i++ )
		{
			total_verts += model->mesh[ mesh_i ].vertex_count;
			total_ind += model->mesh[ mesh_i ].index_count;
		}

		gpu_vertex_t* verts   = ch_calloc< gpu_vertex_t >( total_verts );
		u32*          indices = 0;

		if ( total_ind )
			indices = ch_calloc< u32 >( total_ind );

		u32 vert_offset      = 0;
		u32 ind_offset       = 0;
		u32 ind_value_offset = 0;

		// For each mesh, copy all mesh vertex data to one big array of verts and indices
		for ( u32 mesh_i = 0; mesh_i < model->mesh_count; mesh_i++ )
		{
			glm::vec4 temp_color{};
			temp_color[ 3 ] = 1.f;

			if ( mesh_i < 4 )
				temp_color[ mesh_i ] = 1.f;
			else
				printf( "a\n" );

			mesh_t& mesh = model->mesh[ mesh_i ];

			// find a material

			mesh.surface->material;

			for ( u32 vert_i = 0; vert_i < mesh.vertex_count; vert_i++ )
			{
				verts[ vert_offset ].pos    = mesh.vertex_data.pos[ vert_i ];
				verts[ vert_offset ].normal = mesh.vertex_data.normal[ vert_i ];
				verts[ vert_offset ].uv_x   = mesh.vertex_data.tex_coord[ vert_i ].x;
				verts[ vert_offset ].uv_y   = mesh.vertex_data.tex_coord[ vert_i ].y;
				// verts[ vert_offset ].color = mesh.vertex_data.color[ vert_i ];
				verts[ vert_offset ].color  = temp_color;
				vert_offset++;
			}
		}

		// If we have indices, copy those as well like the above loop
		if ( indices )
		{
			for ( u32 mesh_i = 0; mesh_i < model->mesh_count; mesh_i++ )
			{
				mesh_t& mesh = model->mesh[ mesh_i ];
				for ( u32 ind_i = 0; ind_i < mesh.index_count; ind_i++ )
				{
					indices[ ind_offset++ ] = mesh.index[ ind_i ] + ind_value_offset;
				}

				ind_value_offset += mesh.vertex_count;
			}
		}

		// Now upload these new vertex and index arrays to the gpu
		gpu_mesh_buffers_t buffers{};

		if ( total_ind > 0 )
		{
			buffers = vk_mesh_upload( indices, total_ind, verts, total_verts );
		}
		else
		{
			buffers = vk_mesh_upload( verts, total_verts );
		}

		// Now we can free these temporary arrays
		free( verts );
		free( indices );

		if ( buffers.vertex == nullptr )
		{
			Log_ErrorF( "Failed to upload mesh: \"%s\"", graphics_data->model_get_path( model_handle ) );
			return {};
		}

		// store an internal handle to the uploaded mesh here
		r_mesh_h   upload_handle{};
		vk_mesh_t* vk_mesh = nullptr;

		if ( !g_mesh_list.create( upload_handle, &vk_mesh ) )
			return {};

		vk_mesh->buffers        = buffers;
		vk_mesh->vertex_count   = total_verts;
		vk_mesh->index_count    = total_ind;

		// for now, just one material
		vk_mesh->material       = ch_calloc< vk_mesh_material_t >( 1 );
		vk_mesh->material_count = 1;

		// for ( size_t surf_i = 0; surf_i < model->)

		return upload_handle;
	}

	r_mesh_render_h mesh_render_create( r_mesh_h mesh_handle ) override
	{
		if ( !g_mesh_list.handle_valid( mesh_handle ) )
		{
			Log_ErrorF( "Can't Create Mesh Render, Mesh Handle is Invalid!\n" );
			return {};
		}

		r_mesh_render_h  render_handle{};
		r_mesh_render_t* render_data = nullptr;

		if ( !g_mesh_render_list.create( render_handle, &render_data ) )
		{
			return {};
		}

		render_data->matrix = glm::identity< glm::mat4 >();
		render_data->mesh   = mesh_handle;

		g_mesh_list.ref_increment( mesh_handle );

		return render_handle;
	}
};


Render3 g_render3;

static ModuleInterface_t g_interfaces[] = {
	{ &g_render3, CH_RENDER3, CH_RENDER3_VER }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& srCount )
	{
		srCount = 1;
		return g_interfaces;
	}
}

