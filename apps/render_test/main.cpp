#include "core/app_info.h"
#include "core/build_number.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_sdl2.h"

#include "render_test.h"

#include <chrono>


CONVAR_FLOAT_NAME( host_fps_max, "app.fps.max", 300, "Maximum FPS the App can run at - 0 for no limit, 10 minimum" );
CONVAR_RANGE_FLOAT_NAME( host_timescale, "app.time.scale", 1, 0, FLT_MAX, "Scaled Frametime of the App" );
CONVAR_RANGE_FLOAT_NAME( host_max_frametime, "app.max.frametime", 0.1, 0, FLT_MAX, "Max time in seconds a frame can be" );


IGuiSystem*    gui                 = nullptr;
IRender3*      render              = nullptr;
IGraphicsData* graphics_data       = nullptr;
IInputSystem*  input               = nullptr;

int            g_width             = args_register_names( 1280, "Width of the main window", 2, "--width", "-w" );
int            g_height            = args_register_names( 720, "Height of the main window", 2, "--height", "-h" );
static bool    g_maximize          = args_register( "Maximize the main window", "--max" );
static bool    g_running           = true;

SDL_Window*    g_window            = nullptr;
void*          g_window_native     = nullptr;  // Only Used on WIN32
r_window_h     g_graphics_window{};


CONCMD( exit )
{
	g_running = false;
}


CONCMD( quit )
{
	g_running = false;
}


#if CH_USE_MIMALLOC
CONCMD( mimalloc_print )
{
	// TODO: have this output to the logging system
	mi_collect( true );
	mi_stats_merge();
	mi_stats_print( nullptr );
}
#endif


static AppModule_t g_app_modules[] = 
{
	{ (ISystem**)&input,         "ch_input",         IINPUTSYSTEM_NAME, IINPUTSYSTEM_VER },
	{ (ISystem**)&graphics_data, "ch_graphics_data", CH_GRAPHICS_DATA,  CH_GRAPHICS_DATA_VER },
	{ (ISystem**)&render,        "ch_render_3",      CH_RENDER3,        CH_RENDER3_VER },
	{ (ISystem**)&gui,           "ch_gui",           IGUI_NAME,         IGUI_HASH },
};


void window_resize_callback( void* hwnd )
{
	if ( hwnd == g_window_native )
	{
		render->reset( g_graphics_window );

		update_viewport();

		render->test_update( 0.f, g_graphics_window, g_view, g_projection );
		render->present( g_graphics_window );
	}
}


bool create_main_window()
{
	// Create Main Window
	std::string window_name;

	window_name = ( core_app_info_get().apWindowTitle ) ? core_app_info_get().apWindowTitle : "Chocolate Engine Render 3 Test";
	window_name += vstring( " - Compiled On - %s %s", Core_GetBuildDate(), Core_GetBuildTime() );

	if ( !sys_create_window( g_window_native, g_window, window_name.c_str(), g_width, g_height, g_maximize ) )
	{
		Log_Error( "Failed to create render test window\n" );
		return false;
	}

	render->set_main_surface( g_window, g_window_native );

	input->AddWindow( g_window, ImGui::GetCurrentContext() );

#ifdef _WIN32
	sys_set_resize_callback( window_resize_callback );
#endif /* _WIN32  */

	return true;
}


void close_main_window()
{
	// if ( gDedicatedViewport != UINT32_MAX )
	// 	render->free( gDedicatedViewport );

	render->window_free( g_graphics_window );
	ImGui_ImplSDL2_Shutdown();

	ImGui::DestroyContext( ImGui::GetCurrentContext() );

	Con_RunCommand( "quit" );
}


bool handle_events()
{
	std::vector< SDL_Event >* events = input->GetEvents();

	for ( SDL_Event& event : *events )
	{
		if ( event.type == SDL_QUIT )
		{
			close_main_window();
			return true;
		}

		else if ( event.type == SDL_WINDOWEVENT )
		{
			if ( event.window.event == SDL_WINDOWEVENT_CLOSE )
			{
				SDL_Window* sdlWindow = SDL_GetWindowFromID( event.window.windowID );

				// Is this the main window?
				if ( sdlWindow == g_window )
				{
					close_main_window();
					return true;
				}
			}
		}
	}

	return false;
}


void MainLoop()
{
	Log_Msg( "Entering Main Update Loop\n" );

	auto  startTime   = std::chrono::high_resolution_clock::now();
	auto  currentTime = startTime;
	float time        = 0.f;

	while ( g_running )
	{
		PROF_SCOPE_NAMED( "Main Loop" );

		// Update Frame Time
		{
			PROF_SCOPE_NAMED( "Framerate Limit" );

			currentTime = std::chrono::high_resolution_clock::now();
			time        = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count();

			// don't let the time go too crazy, usually happens when in a breakpoint
			time        = glm::min( time, host_max_frametime );

			if ( host_fps_max > 0.f )
			{
				float max_fps        = glm::clamp( host_fps_max, 10.f, 5000.f );

				// check if we still have more than 2ms till next frame and if so, wait for "1ms"
				float min_frame_time = 1.0f / max_fps;
				if ( ( min_frame_time - time ) > ( 2.0f / 1000.f ) )
					sys_sleep( 1 );

				// framerate is above max
				if ( time < min_frame_time )
					continue;
			}
		}

		input->Update( time );

		if ( handle_events() )
			return;

		render->new_frame( g_graphics_window );

		ImGui::NewFrame();
		ImGui_ImplSDL2_NewFrame();

		gui->Update( time );
		ImGui::Render();

		handle_inputs( time * host_timescale );

		// RENDER TEST
		render->test_update( time * host_timescale, g_graphics_window, g_view, g_projection );

		render->present( g_graphics_window );

		Con_Update();

		startTime = currentTime;

#ifdef TRACY_ENABLE
		FrameMark;
#endif
	}
}


extern "C"
{
	int DLL_EXPORT app_init()
	{
		IMGUI_CHECKVERSION();

		// Needs to be done before Renderer is loaded
		ImGui::CreateContext();

		// ---------------------------------------------------------------------------------------------
		// Load Systems

		if ( !Mod_AddSystems( g_app_modules, ARR_SIZE( g_app_modules ) ) )
		{
			Log_Error( "Failed to Load Systems\n" );
			return 1;
		}

		if ( !create_main_window() )
			return 1;

		if ( !Mod_InitSystems() )
			return 1;

		// Create the Graphics API Window
		g_graphics_window = render->window_create( g_window, g_window_native );

		if ( !g_graphics_window.generation )
		{
			Log_Fatal( "Failed to Create GraphicsAPI Window\n" );
			return 1;
		}

		// gui->StyleImGui();

		// always only one window
		// TODO: if we launch the in the toolkit process, move this
		input->SetCurrentWindow( g_window );

		// render->window_set_reset_callback();

		update_viewport();

		// Init autoexec.cfg
		Con_RunCommand( "exec autoexec" );

		// ---------------------------------------------------------------------------------------------
		// Load Test Scene

		if ( !load_scene() )
		 	return 1;

		// ---------------------------------------------------------------------------------------------
		// Main Loop

		MainLoop();

		// ---------------------------------------------------------------------------------------------
		// Shutdown

		return 0;
	}
}

