#include "render.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"

// HACK HACK HACK HACK
// VULKAN NEEDS THE SURFACE BEFORE WE CREATE A DEVICE
VkSurfaceKHR          g_surface_hack         = VK_NULL_HANDLE;
SDL_Window*           g_window_hack          = nullptr;
void*                 g_window_hack_native   = nullptr;


constexpr const char* CH_DEFAULT_WINDOW_NAME = "Vulkan Window";


// interface for the renderer
struct Render3 final : public IRender3
{
	// --------------------------------------------------------------------------------------------
	// ISystem Functions
	// --------------------------------------------------------------------------------------------

	bool Init() override
	{
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

		// create command pools

		// create descriptor sets and one time commands

		// create the missing texture

		// TODO: later add in other parts of the renderer, like the shader system

		return true;
	}

	void Shutdown()
	{
	}

	void Update( float sDT ) override
	{
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

	// --------------------------------------------------------------------------------------------
	// Windows
	// EACH ONE SHOULD HAVE THEIR OWN IMGUI CONTEXT !!!!
	// --------------------------------------------------------------------------------------------

	ChHandle_t window_create( SDL_Window* sdl_window, void* native_window = nullptr ) override
	{
		r_window_data_t* window        = nullptr;
		ChHandle_t       window_handle = g_windows.Create( &window );

		const char*      title         = SDL_GetWindowTitle( sdl_window ) == nullptr ? SDL_GetWindowTitle( sdl_window ) : CH_DEFAULT_WINDOW_NAME;

		if ( !window_handle )
		{
			Log_ErrorF( gLC_Render, "Failed to allocate data for window: \"%s\"\n", title );
			return CH_INVALID_HANDLE;
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
				g_windows.Remove( window_handle );
				return CH_INVALID_HANDLE;
			}
		}

		window->context = ImGui::GetCurrentContext();

		if ( !vk_swapchain_create( window, VK_NULL_HANDLE ) )
		{
			Log_ErrorF( gLC_Render, "Failed to create swapchain for window: \"%s\"\n", title );
			window_free( window_handle );
			return CH_INVALID_HANDLE;
		}

// 		VK_CreateBackBuffer( window );
// 		VK_CreateFences( window );
// 		VK_CreateSemaphores( window );
// 
// 		VK_AllocateCommands( window );
// 
// 		InitImGui( 0 );
// 
// 		if ( !ImGui_ImplSDL2_InitForVulkan( sdl_window ) )
// 		{
// 			Log_ErrorF( gLC_Render, "Failed to init ImGui SDL2 for Vulkan Window: \"%s\"\n", title );
// 			window_free( window_handle );
// 			return false;
// 		}
// 
// 		if ( !VK_CreateImGuiFonts() )
// 		{
// 			Log_ErrorF( gLC_Render, "Failed to create ImGui fonts for Window: \"%s\"\n", title );
// 			window_free( window_handle );
// 			return false;
// 		}

		return window_handle;
	}

	void window_free( ChHandle_t window ) override
	{
		r_window_data_t* window_data = nullptr;
		if ( !g_windows.Get( window, &window_data ) )
		{
			Log_ErrorF( gLC_Render, "Failed to find window to free!\n" );
			return;
		}

		// free vulkan resources
		vk_swapchain_destroy( window_data );
		vk_surface_destroy( window_data->surface );

		// free imgui resources
		//ImGui_ImplSDL2_Shutdown();

		// free window data
		g_windows.Remove( window );
	}
	
	glm::uvec2 window_surface_size( ChHandle_t window ) override
	{
		r_window_data_t* window_data = nullptr;
		if ( !g_windows.Get( window, &window_data ) )
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

	void new_frame() override
	{
	}

	void reset( ChHandle_t window_handle ) override
	{
		r_window_data_t* window = nullptr;
		if ( !g_windows.Get( window_handle, &window ) )
		{
			Log_ErrorF( gLC_Render, "Failed to find window to reset!\n" );
			return;
		}
	}

	void present( ChHandle_t window_handle ) override
	{
		r_window_data_t* window = nullptr;
		if ( !g_windows.Get( window_handle, &window ) )
		{
			Log_ErrorF( gLC_Render, "Failed to find window to present to!\n" );
			return;
		}

		// do rendering

		// present the window
	}
};


Render3 g_render3;

static ModuleInterface_t g_interfaces[] = {
	{ &g_render3, CH_RENDER3, CH_RENDER3_VER }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* cframework_GetInterfaces( size_t& srCount )
	{
		srCount = 1;
		return g_interfaces;
	}
}

