#include "core/app_info.h"
#include "core/build_number.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_sdl2.h"

#include "render_test.h"


CONVAR_FLOAT_NAME( host_fps_max, "app.fps.max", 300, "Maximum FPS the App can run at - 0 for no limit, 10 minimum" );
CONVAR_RANGE_FLOAT_NAME( host_timescale, "app.time.scale", 1, 0, FLT_MAX, "Scaled Frametime of the App" );
CONVAR_RANGE_FLOAT_NAME( host_max_frametime, "app.max.frametime", 0.1, 0, FLT_MAX, "Max time in seconds a frame can be" );

CONVAR_FLOAT_NAME( g_move_speed, "input.move.speed", 5, "Move Speed" );

CONVAR_FLOAT_NAME( g_mouse_pitch, "input.mouse.sensitivity.pitch", 0.022, CVARF_ARCHIVE, "Mouse Pitch" );
CONVAR_FLOAT_NAME( g_mouse_yaw, "input.mouse.sensitivity.yaw", 0.022, CVARF_ARCHIVE, "Mouse Yaw" );
CONVAR_FLOAT_NAME( g_mouse_sensitivity, "input.mouse.sensitivity", 1.0, CVARF_ARCHIVE, "Mouse Sensitivity" );

CONVAR_FLOAT_NAME( g_view_nearz, "view.nearz", 0.05f, CVARF_ARCHIVE, "Near Z" );
CONVAR_FLOAT_NAME( g_view_farz, "view.farz", 1000.f, CVARF_ARCHIVE, "Far Z" );
CONVAR_FLOAT_NAME( g_view_fov, "view.fov", 106.f, CVARF_ARCHIVE, "Field of View" );


IGuiSystem*    gui                 = nullptr;
IRender3*      render              = nullptr;
IGraphicsData* graphics_data       = nullptr;
IInputSystem*  input               = nullptr;

int            g_width             = args_register_names( 1280, "Width of the main window", 2, "--width", "-w" );
int            g_height            = args_register_names( 720, "Height of the main window", 2, "--height", "-h" );
static bool    g_maximize          = args_register( "Maximize the main window", "--max" );
static bool    g_wait_for_debugger = args_register( "Upon Program Startup, Wait for the Debugger to attach", "--debugger" );
static bool    g_running           = true;

SDL_Window*    g_window            = nullptr;
void*          g_window_native     = nullptr;  // Only Used on WIN32
ch_handle_t    g_graphics_window   = CH_INVALID_HANDLE;

glm::mat4      g_view{};
glm::mat4      g_projection{};

glm::vec3      g_pos{};
glm::vec3      g_ang{};


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


// why is this not in transform.h?
inline glm::mat4 Util_ComputeProjection( float sWidth, float sHeight, float sNearZ, float sFarZ, float sFov )
{
	float hAspect = (float)sWidth / (float)sHeight;
	float vAspect = (float)sHeight / (float)sWidth;

	float V       = 2.0f * atanf( tanf( glm::radians( sFov ) / 2.0f ) * vAspect );

	return glm::perspective( V, hAspect, sNearZ, sFarZ );
}


void update_viewport()
{
	glm::vec2 size  = render->window_surface_size( g_graphics_window );

	auto&      io    = ImGui::GetIO();
	io.DisplaySize.x = size.x;
	io.DisplaySize.y = size.y;

	g_projection = Util_ComputeProjection( size.x, size.y, g_view_nearz, g_view_farz, g_view_fov );
}


void render_on_reset( ch_handle_t window, e_render_reset_flags flags )
{
	update_viewport();
}


bool create_main_window()
{
	// Create Main Window
	std::string window_name;

	window_name = ( Core_GetAppInfo().apWindowTitle ) ? Core_GetAppInfo().apWindowTitle : "Chocolate Engine Render 3 Test";
	window_name += vstring( " - Build %zd - Compiled On - %s %s", Core_GetBuildNumber(), Core_GetBuildDate(), Core_GetBuildTime() );

	if ( !sys_create_window( g_window_native, g_window, window_name.c_str(), g_width, g_height, g_maximize ) )
	{
		Log_Error( "Failed to create render test window\n" );
		return false;
	}

	render->set_main_surface( g_window, g_window_native );

	input->AddWindow( g_window, ImGui::GetCurrentContext() );

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


// Check the function FindHoveredWindow() in imgui.cpp to see if you need to update this when updating imgui
bool mouse_hovering_imgui_window( glm::ivec2 mousePos )
{
	ImGuiContext& g = *ImGui::GetCurrentContext();

	ImVec2        imMousePos{ (float)mousePos.x, (float)mousePos.y };

	ImGuiWindow*  hovered_window                        = NULL;
	ImGuiWindow*  hovered_window_ignoring_moving_window = NULL;
	if ( g.MovingWindow && !( g.MovingWindow->Flags & ImGuiWindowFlags_NoMouseInputs ) )
		hovered_window = g.MovingWindow;

	ImVec2 padding_regular    = g.Style.TouchExtraPadding;
	ImVec2 padding_for_resize = g.IO.ConfigWindowsResizeFromEdges ? g.WindowsHoverPadding : padding_regular;
	for ( int i = g.Windows.Size - 1; i >= 0; i-- )
	{
		ImGuiWindow* window = g.Windows[ i ];
		IM_MSVC_WARNING_SUPPRESS( 28182 );  // [Static Analyzer] Dereferencing NULL pointer.
		if ( !window->WasActive || window->Hidden )
			continue;
		if ( window->Flags & ImGuiWindowFlags_NoMouseInputs )
			continue;
		IM_ASSERT( window->Viewport );
		if ( window->Viewport != g.MouseViewport )
			continue;

		// Using the clipped AABB, a child window will typically be clipped by its parent (not always)
		ImVec2 hit_padding = ( window->Flags & ( ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize ) ) ? padding_regular : padding_for_resize;
		if ( !window->OuterRectClipped.ContainsWithPad( imMousePos, hit_padding ) )
			continue;

		// Support for one rectangular hole in any given window
		// FIXME: Consider generalizing hit-testing override (with more generic data, callback, etc.) (#1512)
		if ( window->HitTestHoleSize.x != 0 )
		{
			ImVec2 hole_pos( window->Pos.x + (float)window->HitTestHoleOffset.x, window->Pos.y + (float)window->HitTestHoleOffset.y );
			ImVec2 hole_size( (float)window->HitTestHoleSize.x, (float)window->HitTestHoleSize.y );
			ImVec2 hole_pos_size{};

			hole_pos_size.x = hole_pos.x + hole_size.x;
			hole_pos_size.y = hole_pos.y + hole_size.y;

			if ( ImRect( hole_pos, hole_pos_size ).Contains( imMousePos ) )
				continue;
		}

		if ( hovered_window == NULL )
			hovered_window = window;
		IM_MSVC_WARNING_SUPPRESS( 28182 );  // [Static Analyzer] Dereferencing NULL pointer.
		if ( hovered_window_ignoring_moving_window == NULL && ( !g.MovingWindow || window->RootWindowDockTree != g.MovingWindow->RootWindowDockTree ) )
			hovered_window_ignoring_moving_window = window;
		if ( hovered_window && hovered_window_ignoring_moving_window )
			break;
	}

	if ( hovered_window )
	{
		//if ( editor_spew_imgui_window_hover )
		//	Log_DevF( 1, "HOVERING WINDOW \"%s\"\n", hovered_window->Name );

		return true;
	}

	return false;
}


bool is_mouse_in_view()
{
	if ( !input->WindowHasFocus( g_window ) )
		return false;

	glm::vec2         size     = render->window_surface_size( g_graphics_window );\
	glm::ivec2        mouse_pos = input->GetMousePos();

//	if ( viewport->aOffset.x > mouse_pos.x )
//		return false;
//
//	if ( viewport->aOffset.y > mouse_pos.y )
//		return false;
//
//	if ( viewport->aOffset.x + viewport->aSize.x < mouse_pos.x )
//		return false;
//
//	if ( viewport->aOffset.y + viewport->aSize.y < mouse_pos.y )
//		return false;

	if ( size.x < mouse_pos.x )
		return false;

	if ( size.y < mouse_pos.y )
		return false;

	if ( mouse_hovering_imgui_window( mouse_pos ) )
		return false;

	return true;
}


static void center_mouse()
{
	glm::ivec2 size = render->window_surface_size( g_graphics_window );

	int x = ( size.x / 2.f );
	int y = ( size.y / 2.f );

	SDL_WarpMouseInWindow( g_window, x, y );
}


bool mouse_capture_pressed()
{
	return input->KeyPressed( (EButton)SDL_SCANCODE_SPACE ) || input->KeyPressed( EButton_MouseRight );
}


inline float DegreeConstrain( float num )
{
	num = std::fmod( num, 360.0f );
	return ( num < 0.0f ) ? num += 360.0f : num;
}


void handle_mouse()
{
	static bool captured_mouse = false;
	bool        mouse_in_view  = false;

	if ( !captured_mouse )
		mouse_in_view = is_mouse_in_view();

	if ( mouse_in_view && mouse_capture_pressed() )
	{
		SDL_SetRelativeMouseMode( SDL_TRUE );
		captured_mouse = true;
	}

	bool should_center_mouse = false;

	if ( captured_mouse )
	{
		if ( mouse_capture_pressed() )
		{
			// not sure why i need to do this after updating imgui
			SDL_ShowCursor( SDL_FALSE );

			// ch_handle_t Mouse Input
			const glm::vec2 mouse = input->GetMouseDelta();

			// transform.aAng[PITCH] = -mouse.y;
			g_ang[ PITCH ] -= mouse.y * g_mouse_pitch * g_mouse_sensitivity;
			g_ang[ YAW ] += mouse.x * g_mouse_yaw * g_mouse_sensitivity;

			g_ang[ YAW ]   = DegreeConstrain( g_ang[ YAW ] );
			g_ang[ PITCH ] = std::clamp( g_ang[ PITCH ], -90.0f, 90.0f );

			should_center_mouse = true;
		}
		else
		{
			SDL_SetRelativeMouseMode( SDL_FALSE );
			SDL_ShowCursor( SDL_TRUE );

			if ( captured_mouse )
				should_center_mouse = true;

			captured_mouse = false;
		}
	}

	if ( should_center_mouse )
		center_mouse();
}


void handle_inputs( float frame_time )
{
	// Handle Mouse Updates
	handle_mouse();

	glm::vec3 move{};
	float     move_speed = g_move_speed * frame_time;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_W ) )
		move.x = move_speed;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_S ) )
		move.x += -move_speed;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_A ) )
		move.y = -move_speed;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_D ) )
		move.y += move_speed;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_Q ) )
		move.z = -move_speed;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_E ) )
		move.z += move_speed;

	// get forward and right vectors
	glm::vec3 forward, right, up;
	//Util_GetDirectionVectors( g_ang, &forward, &right, &up );
	//
	//for ( int i = 0; i < 3; i++ )
	//	g_pos[ i ] += right[ i ] * move.x + forward[ i ] * move.y + up[ i ] * move.z;

	Util_ToViewMatrixZ( g_view, g_pos, g_ang );
	Util_GetViewMatrixZDirection( g_view, forward, right, up );

	for ( int i = 0; i < 3; i++ )
		g_pos[ i ] += forward[ i ] * move.x + right[ i ] * move.y + up[ i ] * move.z;

	// stupid, because Util_GetDirectionVectors is broken or something
	Util_ToViewMatrixZ( g_view, g_pos, g_ang );

	// Update Projection
	update_viewport();

	gui->DebugMessage( "Pos: %s", ch_vec3_to_str( g_pos ).c_str() );
	gui->DebugMessage( "Ang: %s", ch_vec3_to_str( g_ang ).c_str() );
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

		ImGui::NewFrame();
		ImGui_ImplSDL2_NewFrame();

		render->new_frame();

		gui->Update( time );
		ImGui::Render();

		handle_inputs( time * host_timescale );

		// RENDER TEST
		render->test_update( time * host_timescale, g_graphics_window, g_view, g_projection );

		render->present( g_graphics_window );

		Con_Update();
		Resource_Update();

		startTime = currentTime;

#ifdef TRACY_ENABLE
		FrameMark;
#endif
	}
}


constexpr const char* TEST_SCENE_PATH = "../sidury/maps/riverhouse_v1";
constexpr const char* TEST_MODEL_PATH = "../sidury/models/riverhouse/riverhouse_v1.obj";
// constexpr const char* TEST_MODEL_PATH = "models/test/test_cube.obj";


static ch_model_h      g_test_model{};
static r_mesh_h        g_test_model_gpu{};
static r_mesh_render_h g_test_model_render{};


bool load_scene()
{
	// hmm, maybe this could be condensed into one call, mesh_render_create(), pass in a model path instead of a handle, and do the model load and upload internally if needed?
	g_test_model = graphics_data->model_load( TEST_MODEL_PATH );
	
	if ( !g_test_model )
		return false;
	
	g_test_model_gpu = render->mesh_upload( g_test_model );
	
	if ( !g_test_model_gpu.generation )
		return false;
	
	g_test_model_render = render->mesh_render_create( g_test_model_gpu );
	
	if ( !g_test_model_render.generation )
		return false;

	// now we can free the test model since it's use is over
	graphics_data->model_free( g_test_model );

	return true;
}


extern "C"
{
	int DLL_EXPORT app_init()
	{
		if ( g_wait_for_debugger )
			sys_wait_for_debugger();

		// Load main app info (Note that if you don't do this, you need to call FileSys_DefaultSearchPaths() before loading any files)
		if ( !Core_LoadAppInfo() )
		{
			Log_FatalF( "Failed to Load App Info" );
			return 1;
		}

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

		if ( g_graphics_window == CH_INVALID_HANDLE )
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
		Con_QueueCommandSilent( "exec autoexec.cfg", false );

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

