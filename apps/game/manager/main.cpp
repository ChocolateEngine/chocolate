#include "core/app_info.h"
#include "core/build_number.h"

#include "iinput.h"
#include "igui.h"
#include "iaudio.h"
#include "physics/iphysics.h"
#include "render/irender.h"
#include "igraphics.h"
#include "steam/ch_isteam.h"

#include "../client/cl_interface.h"
#include "../server/sv_interface.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"


#if CH_USE_MIMALLOC
	#include "mimalloc-new-delete.h"
#endif


CONVAR_RANGE_FLOAT( host_fps_max, 300, 0, 5000, "Maximum FPS the App can run at" );
CONVAR_RANGE_FLOAT( host_timescale, 1, 0, FLT_MAX, "Scaled Frametime of the App" );
CONVAR_RANGE_FLOAT( host_max_frametime, 0.1, 0, FLT_MAX, "Max time in seconds a frame can be" );

CONVAR_RANGE_FLOAT( map_list_rebuild_timer, 30.f, 0, FLT_MAX, CVARF_ARCHIVE, "Timer for rebuilding the map list" );


int                        gWidth             = args_register_names( 1280, "Width of the main window", 2, "--width", "-w" );
int                        gHeight            = args_register_names( 720, "Height of the main window", 2, "--height", "-h" );
static bool                gMaxWindow         = args_register( "Maximize the main window", "--max" );
static bool                gArgNoSteam        = args_register( "Don't try to load the steam abstraction", "--no-steam" );
static bool                gWaitForDebugger   = args_register( "Upon Program Startup, Wait for the Debugger to attach", "--debugger" );
static bool                gDedicatedServer   = args_register( "Host a Dedicated Server", "--server" );
// static bool    gDedicatedServer = false;
static bool                gRunning           = true;

SDL_Window*                gpWindow           = nullptr;
void*                      gpSysWindow        = nullptr;
ch_handle_t                 gGraphicsWindow    = CH_INVALID_HANDLE;

u32                        gDedicatedViewport = UINT32_MAX;

IGuiSystem*                gui              = nullptr;
IRender*                   render           = nullptr;
IInputSystem*              input            = nullptr;
IAudioSystem*              audio            = nullptr;
IGraphics*                 graphics         = nullptr;
IRenderSystemOld*          renderOld        = nullptr;
Ch_IPhysics*               physics          = nullptr;
ISteamSystem*              steam            = nullptr;

IClientSystem*             client           = nullptr;
IServerSystem*             server           = nullptr;


std::vector< ch_string >   gMapList;
static bool                gRebuildMapList  = true;
static float               gRebuildMapTimer = 0.f;


inline bool                RunningClient()
{
	return client && !gDedicatedServer;
}



CONCMD_VA( map_list_rebuild, "Rebuild the map list now" )
{
	gRebuildMapList = true;
}


CONCMD( disconnect )
{
	if ( RunningClient() )
		client->Disconnect();

	if ( server )
		server->CloseServer();
}


CONCMD( status )
{
	if ( server && server->IsHosting() )
	{
		server->PrintStatus();
	}
	else if ( client )
	{
		client->PrintStatus();
	}
}


CONCMD( exit )
{
	gRunning = false;
}


CONCMD( quit )
{
	gRunning = false;
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


static AppModule_t gAppModulesClient[] = 
{
	{ (ISystem**)&input,     "ch_input",     IINPUTSYSTEM_NAME, IINPUTSYSTEM_VER },
	{ (ISystem**)&render,    "ch_graphics_api_vk", IRENDER_NAME, IRENDER_VER },
	{ (ISystem**)&audio,     "ch_aduio",     IADUIO_NAME, IADUIO_VER },
	{ (ISystem**)&physics,   "ch_physics",   IPHYSICS_NAME, IPHYSICS_VER },
    { (ISystem**)&graphics,  "ch_render",  IGRAPHICS_NAME, IGRAPHICS_VER },
    { (ISystem**)&renderOld, "ch_render",  IRENDERSYSTEMOLD_NAME, IRENDERSYSTEMOLD_VER },
	{ (ISystem**)&gui,       "ch_gui",       IGUI_NAME, IGUI_HASH },
};


static AppModule_t gAppModulesServer[] = {
	{ (ISystem**)&input,     "ch_input", IINPUTSYSTEM_NAME, IINPUTSYSTEM_VER },
	{ (ISystem**)&render,    "ch_graphics_api_vk", IRENDER_NAME, IRENDER_VER },
	{ (ISystem**)&physics,   "ch_physics", IPHYSICS_NAME, IPHYSICS_VER },
	{ (ISystem**)&graphics,  "ch_render", IGRAPHICS_NAME, IGRAPHICS_VER },
	{ (ISystem**)&renderOld, "ch_render", IRENDERSYSTEMOLD_NAME, IRENDERSYSTEMOLD_VER },
	{ (ISystem**)&gui,       "ch_gui", IGUI_NAME, IGUI_HASH },
};


enum ECurrentModule
{
	ECurrentModule_None,
	ECurrentModule_Client,
	ECurrentModule_Server,
};

static ECurrentModule gCurrentModule;


bool CvarF_ClientExecuteCallback( const std::string& srName, const std::vector< std::string >& args, const std::string& fullCommand )
{
	// Must be running a dedicated server
	if ( !client )
		return true;

	if ( gCurrentModule == ECurrentModule_Server )
		return true;

	// Forward to server if we are the client
	client->SendConVar( srName, args );
	return false;
}


void Map_RebuildMapList()
{
	ch_str_free( gMapList.data(), gMapList.size() );
	gMapList.clear();

	std::vector< ch_string > mapList = FileSys_ScanDir( "maps", ReadDir_AllPaths | ReadDir_NoFiles );

	for ( const auto& mapFolder : mapList )
	{
		if ( ch_str_ends_with( mapFolder, "..", 2 ) )
			continue;

		// Check for legacy map file and new map file
		const char*    strings[]     = { mapFolder.data, CH_PATH_SEP_STR "mapInfo.smf" };
		const u64      stringLens[]  = { mapFolder.size, 14 };
		ch_string_auto mapInfoPath   = ch_str_join( 2, strings, stringLens );

		const char*    strings2[]    = { mapFolder.data, CH_PATH_SEP_STR "mapData.smf" };
		const u64      stringLens2[] = { mapFolder.size, 14 };
		ch_string_auto mapDataPath   = ch_str_join( 2, strings2, stringLens2 );

		if ( !FileSys_IsFile( mapInfoPath.data, mapInfoPath.size, true ) && !FileSys_IsFile( mapDataPath.data, mapDataPath.size, true ) )
		{
			continue;
		}

		ch_string_auto mapName = FileSys_GetFileName( mapFolder.data, mapFolder.size );
		gMapList.emplace_back( mapName.data, mapName.size );
	}

	ch_str_free( mapList.data(), mapList.size() );
}


const std::vector< ch_string >& Map_GetMapList()
{
	if ( gRebuildMapList )
	{
		Map_RebuildMapList();
		gRebuildMapList  = false;
		gRebuildMapTimer = map_list_rebuild_timer;
	}

	return gMapList;
}


void Map_UpdateTimer( float frameTime )
{
	if ( gRebuildMapTimer > 0.f )
		gRebuildMapTimer -= frameTime;
	else
		gRebuildMapList = true;
}


void map_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )     // results to populate the dropdown list with
{
	const std::vector< ch_string >& mapList = Map_GetMapList();

	for ( const auto& map : mapList )
	{
		if ( args.size() && !ch_str_starts_with( map, args[ 0 ].data(), args[ 0 ].size() ) )
			continue;

		results.emplace_back( map.data, map.size );
	}
}


CONCMD_DROP( map, map_dropdown )
{
	if ( !server )
	{
		Log_Warn( "Server Module not loaded!\n" );
		return;
	}

	if ( args.size() == 0 )
	{
		Log_Warn( "No Map Path/Name specified!\n" );
		return;
	}

	// Start a server
	server->StartServer( args[ 0 ] );

	// Connect the client

	if ( RunningClient() )
		Con_RunCommandArgs( "connect", { "localhost" } );
}


void UpdateViewport()
{
	int width = 0, height = 0;
	render->GetSurfaceSize( gGraphicsWindow, width, height );

	ViewportShader_t* viewport = graphics->GetViewportData( gDedicatedViewport );

	if ( !viewport )
		return;

	viewport->aProjection = glm::mat4( 1.f );
	viewport->aView       = glm::mat4( 1.f );
	viewport->aProjView   = glm::mat4( 1.f );
	viewport->aNearZ      = 5.f;
	viewport->aFarZ       = 1000.f;
	viewport->aSize       = { width, height };

	graphics->SetViewportUpdate( true );

	auto& io         = ImGui::GetIO();
	io.DisplaySize.x = width;
	io.DisplaySize.y = height;
}


bool LoadGameSystems()
{
	if ( !gDedicatedServer )
	{
		AppModule_t clientModule{ (ISystem**)&client, "ch_client", ICLIENT_NAME, ICLIENT_VER };

		// Mark all convars from client dll with CVARF_CLIENT
		Con_SetConVarRegisterFlags( CVARF( CLIENT ) );
		EModLoadError modLoadRet = Mod_LoadSystem( clientModule );

		if ( modLoadRet != EModLoadError_Success )
		{
			Mod_Shutdown();
			return false;
		}

		// This needs to be done before we can start up the client
		client->SetWindowInfo( gpWindow, gGraphicsWindow );

		if ( !Mod_InitSystem( clientModule ) )
		{
			Mod_Shutdown();
			return false;
		}
	}

	AppModule_t serverModule{ (ISystem**)&server, "ch_server", ISERVER_NAME, ISERVER_VER };

	// Mark all convars from server dll with CVARF_SERVER
	Con_SetConVarRegisterFlags( CVARF( SERVER ) );
	EModLoadError modLoadRet = Mod_LoadAndInitSystem( serverModule );

	if ( modLoadRet != EModLoadError_Success )
	{
		Mod_Shutdown();
		return false;
	}

	return true;
}


bool CreateMainWindow()
{
	// Create Main Window
	std::string windowName;

	windowName = ( Core_GetAppInfo().apWindowTitle ) ? Core_GetAppInfo().apWindowTitle : "Chocolate Engine";
	windowName += vstring( " - Build %zd - Compiled On - %s %s", Core_GetBuildNumber(), Core_GetBuildDate(), Core_GetBuildTime() );

#ifdef _WIN32
	gpSysWindow = sys_create_window( windowName.c_str(), gWidth, gHeight, gMaxWindow );

	if ( !gpSysWindow )
	{
		Log_Error( "Failed to create game window\n" );
		return false;
	}

	gpWindow = SDL_CreateWindowFrom( gpSysWindow );
#else
	int flags = SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

	if ( gMaxWindow )
		flags |= SDL_WINDOW_MAXIMIZED;

	gpWindow = SDL_CreateWindow( windowName.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                             gWidth, gHeight, flags );
#endif

	if ( !gpWindow )
	{
		Log_Error( "Failed to create SDL2 Window\n" );
		return false;
	}

	render->SetMainSurface( gpWindow, gpSysWindow );

	input->AddWindow( gpWindow, ImGui::GetCurrentContext() );

	return true;
}


void CloseMainWindow()
{
	if ( gDedicatedViewport != UINT32_MAX )
		graphics->FreeViewport( gDedicatedViewport );

	render->DestroyWindow( gGraphicsWindow );
	ImGui_ImplSDL2_Shutdown();

	ImGui::DestroyContext( ImGui::GetCurrentContext() );

	Con_RunCommand( "quit" );
}


bool HandleEvents()
{
	std::vector< SDL_Event >* events = input->GetEvents();

	for ( SDL_Event& event : *events )
	{
		if ( event.type == SDL_QUIT )
		{
			CloseMainWindow();
			return true;
		}

		else if ( event.type == SDL_WINDOWEVENT )
		{
			if ( event.window.event == SDL_WINDOWEVENT_CLOSE )
			{
				SDL_Window* sdlWindow = SDL_GetWindowFromID( event.window.windowID );

				// Is this the main window?
				if ( sdlWindow == gpWindow )
				{
					CloseMainWindow();
					return true;
				}
			}
		}
	}

	return false;
}


// Return true if we skip this frame
bool UpdateFrameTime( std::chrono::steady_clock::time_point& startTime, std::chrono::steady_clock::time_point& currentTime, float& time )
{
	currentTime = std::chrono::high_resolution_clock::now();
	time        = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count();

	// don't let the time go too crazy, usually happens when in a breakpoint
	time        = glm::min( time, host_max_frametime );

	if ( host_fps_max > 0.f )
	{
		float maxFps       = glm::clamp( host_fps_max, 10.f, 5000.f );

		// check if we still have more than 2ms till next frame and if so, wait for "1ms"
		float minFrameTime = 1.0f / maxFps;
		if ( ( minFrameTime - time ) > ( 2.0f / 1000.f ) )
			sys_sleep( 1 );

		// framerate is above max
		if ( time < minFrameTime )
			return true;
	}

	return false;
}


void MainLoop()
{
	Log_Msg( "Entering Main Update Loop\n" );

	auto  startTime   = std::chrono::high_resolution_clock::now();
	auto  currentTime = startTime;
	float time        = 0.f;

	while ( gRunning )
	{
		PROF_SCOPE_NAMED( "Main Loop" );

		if ( UpdateFrameTime( startTime, currentTime, time ) )
			continue;

		// ftl::TaskCounter taskCounter( &gTaskScheduler );

		input->Update( time );

		if ( HandleEvents() )
			return;

		Map_UpdateTimer( time );

		gCurrentModule = ECurrentModule_Client;

		client->PreUpdate( time );

		float frameTimeScaled = time * host_timescale;
		gCurrentModule        = ECurrentModule_Server;

		// Update Game Logic
		server->Update( frameTimeScaled );

		gCurrentModule = ECurrentModule_Client;

		client->Update( frameTimeScaled );

		gCurrentModule = ECurrentModule_None;

		Con_Update();
		Resource_Update();

		// Wait and help to execute unfinished tasks
		// gTaskScheduler.WaitForCounter( &taskCounter );

		startTime = currentTime;

#ifdef TRACY_ENABLE
		FrameMark;
#endif
	}
}


void MainLoopDedicated()
{
	Log_Msg( "Entering Main Update Loop for Dedicated Server\n" );

	auto  startTime   = std::chrono::high_resolution_clock::now();
	auto  currentTime = startTime;
	float time        = 0.f;

	while ( gRunning )
	{
		PROF_SCOPE_NAMED( "Main Loop Dedicated" );

		if ( UpdateFrameTime( startTime, currentTime, time ) )
			continue;

		// ftl::TaskCounter taskCounter( &gTaskScheduler );

		input->Update( time );

		if ( HandleEvents() )
			return;

		Map_UpdateTimer( time );

		float frameTimeScaled = time * host_timescale;
		gCurrentModule        = ECurrentModule_Server;

		// Update Game Logic
		server->Update( frameTimeScaled );

		gCurrentModule = ECurrentModule_None;

		UpdateViewport();

		{
			PROF_SCOPE_NAMED( "Imgui New Frame" );
			ImGui::NewFrame();
			ImGui_ImplSDL2_NewFrame();
		}

		renderOld->NewFrame();
		gui->Update( time );

		if ( !( SDL_GetWindowFlags( gpWindow ) & SDL_WINDOW_MINIMIZED ) )
		{
			renderOld->PrePresent();
			renderOld->Present( gGraphicsWindow, &gDedicatedViewport, 1 );
		}
		else
		{
			PROF_SCOPE_NAMED( "Imgui End Frame" );
			ImGui::EndFrame();
		}

		Con_Update();
		Resource_Update();

		// Wait and help to execute unfinished tasks
		// gTaskScheduler.WaitForCounter( &taskCounter );

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
		if ( gWaitForDebugger )
			sys_wait_for_debugger();

		// Load main app info (Note that if you don't do this, you need to call FileSys_DefaultSearchPaths() before loading any files)
		if ( !Core_LoadAppInfo() )
		{
			Log_FatalF( "Failed to Load App Info" );
			return 1;
		}

		IMGUI_CHECKVERSION();

		if ( gDedicatedServer )
			Con_SetDefaultArchive( "cfg" PATH_SEP_STR "config_dedicated.cfg", "cfg" PATH_SEP_STR "config_dedicated_default.cfg" );

#if CH_USE_MIMALLOC
		Log_DevF( 1, "Using mimalloc version %d\n", mi_version() );
#endif

		// Needs to be done before Renderer is loaded
		ImGui::CreateContext();

		srand( (unsigned int)time( 0 ) );  // setup rand(  )

		// if ( gArgUseGL )
		// {
		// 	gAppModules[ 1 ].apModuleName = "ch_render_gl";
		// }

		// ---------------------------------------------------------------------------------------------
		// Load Systems

		// Load Modules and Initialize them in this order
		if ( gDedicatedServer )
		{
			if ( !Mod_AddSystems( gAppModulesServer, ARR_SIZE( gAppModulesServer ) ) )
			{
				Log_Error( "Failed to Load Systems\n" );
				return 1;
			}
		}
		else
		{
			if ( !Mod_AddSystems( gAppModulesClient, ARR_SIZE( gAppModulesClient ) ) )
			{
				Log_Error( "Failed to Load Systems\n" );
				return 1;
			}
		}

		if ( !gArgNoSteam )
		{
			AppModule_t steamModule{ (ISystem**)&steam, "ch_steam", ISTEAM_NAME, ISTEAM_VER, false };
			Mod_LoadSystem( steamModule );
		}

		if ( !CreateMainWindow() )
		{
			return 1;
		}

		if ( !Mod_InitSystems() )
		{
			return 1;
		}

		// Create the Graphics API Window
		gGraphicsWindow = render->CreateWindow( gpWindow, gpSysWindow );

		if ( gGraphicsWindow == CH_INVALID_HANDLE )
		{
			Log_Fatal( "Failed to Create GraphicsAPI Window\n" );
			return 1;
		}

		gui->StyleImGui();

		if ( !LoadGameSystems() )
		{
			return 1;
		}

		// Reset ConVar Flags
		Con_SetConVarRegisterFlags( 0 );

		Con_SetCvarFlagCallback( CVARF( CL_EXEC ), CvarF_ClientExecuteCallback );

		// ftl::TaskSchedulerInitOptions schedOptions;
		// schedOptions.Behavior = ftl::EmptyQueueBehavior::Sleep;
		// 
		// gTaskScheduler.Init( schedOptions );

		// Hack for Dedicated Server For now
		if ( gDedicatedServer )
		{
			gDedicatedViewport = graphics->CreateViewport();
		}

		// always only one window
		// TODO: if we launch the in the toolkit process, move this
		input->SetCurrentWindow( gpWindow );

		// IDEA: put autoexec built it again, and just store all convars in a list
		// Then, when each convar is registered, we check what we read from config.cfg and autoexec.cfg
		// only issue is queued concommands maybe, but we can just queue those, and then call a function run them here instead

		// Init autoexec.cfg
		Con_QueueCommandSilent( "exec autoexec.cfg", false );

		// ---------------------------------------------------------------------------------------------
		// Main Loop

		if ( gDedicatedServer )
			MainLoopDedicated();
		else
			MainLoop();

		// ---------------------------------------------------------------------------------------------
		// Shutdown

		if ( server )
			server->Shutdown();

		if ( client )
			client->Shutdown();

		Resource_Shutdown();
		return 0;
	}
}

