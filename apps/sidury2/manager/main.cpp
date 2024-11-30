#include "iinput.h"
#include "igui.h"
#include "iaudio.h"
#include "physics/iphysics.h"
#include "render/irender.h"
#include "graphics/igraphics.h"
#include "steam/ch_isteam.h"

#include "../client/cl_interface.h"
#include "../server/sv_interface.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"


CONVAR( host_max_frametime, 0.1 );
CONVAR( host_fps_max, 300 );
CONVAR( host_timescale, 1 );


static bool    gArgNoSteam      = Args_Register( "Don't try to load the steam abstraction", "-no-steam" );
static bool    gWaitForDebugger = Args_Register( "Upon Program Startup, Wait for the Debuger to attach", "-debugger" );
static bool    gDedicatedServer = Args_Register( "Host a Dedicated Server", "-server" );
// static bool    gDedicatedServer = false;
static bool    gRunning         = true;


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


std::vector< std::string > gMapList;
static bool                gRebuildMapList  = true;
static float               gRebuildMapTimer = 0.f;


inline bool                RunningClient()
{
	return client && !gDedicatedServer;
}

CONVAR( map_list_rebuild_timer, 30.f, CVARF_ARCHIVE, "Timer for rebuilding the map list" );


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
	{ (ISystem**)&input,     "ch_input",     IINPUTSYSTEM_NAME, IINPUTSYSTEM_HASH },
	{ (ISystem**)&render,    "ch_render_vk", IRENDER_NAME, IRENDER_VER },  // TODO: rename to ch_render_vk
	{ (ISystem**)&audio,     "ch_aduio",     IADUIO_NAME, IADUIO_VER },
	{ (ISystem**)&physics,   "ch_physics",   IPHYSICS_NAME, IPHYSICS_HASH },
    { (ISystem**)&graphics , "ch_graphics",  IGRAPHICS_NAME, IGRAPHICS_VER },
    { (ISystem**)&renderOld, "ch_graphics",  IRENDERSYSTEMOLD_NAME,IRENDERSYSTEMOLD_VER },
	{ (ISystem**)&gui,       "ch_gui",       IGUI_NAME, IGUI_HASH },
};


static AppModule_t gAppModulesServer[] = {
	{ (ISystem**)&input,     "ch_input",     IINPUTSYSTEM_NAME, IINPUTSYSTEM_HASH },
	{ (ISystem**)&render,    "ch_render_vk", IRENDER_NAME, IRENDER_VER },  // TODO: rename to ch_render_vk
	{ (ISystem**)&physics,   "ch_physics",   IPHYSICS_NAME, IPHYSICS_HASH },
	{ (ISystem**)&graphics,  "ch_graphics",  IGRAPHICS_NAME, IGRAPHICS_VER },
	{ (ISystem**)&renderOld, "ch_graphics",  IRENDERSYSTEMOLD_NAME, IRENDERSYSTEMOLD_VER },
	{ (ISystem**)&gui,       "ch_gui",       IGUI_NAME, IGUI_HASH },
};


enum ECurrentModule
{
	ECurrentModule_None,
	ECurrentModule_Client,
	ECurrentModule_Server,
};

static ECurrentModule gCurrentModule;


bool CvarF_ClientExecuteCallback( ConVarBase* spBase, const std::vector< std::string >& args )
{
	// Must be running a dedicated server
	if ( !client )
		return true;

	if ( gCurrentModule == ECurrentModule_Server )
		return true;

	// Forward to server if we are the client
	client->SendConVar( spBase->aName, args );
	return false;
}


void Map_RebuildMapList()
{
	gMapList.clear();

	for ( const auto& mapFolder : FileSys_ScanDir( "maps", ReadDir_AllPaths | ReadDir_NoFiles ) )
	{
		if ( mapFolder.ends_with( ".." ) )
			continue;

		// Check for legacy map file and new map file
		if ( !FileSys_IsFile( mapFolder + "/mapInfo.smf", true ) && !FileSys_IsFile( mapFolder + "/mapData.smf", true ) )
			continue;

		std::string mapName = FileSys_GetFileName( mapFolder );
		gMapList.push_back( mapName );
	}
}


const std::vector< std::string >& Map_GetMapList()
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
  const std::vector< std::string >& args,  // arguments currently typed in by the user
  std::vector< std::string >&       results )    // results to populate the dropdown list with
{
	const std::vector< std::string >& mapList = Map_GetMapList();

	for ( const auto& map : mapList )
	{
		if ( args.size() && !map.starts_with( args[ 0 ] ) )
			continue;

		results.push_back( map );
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
	render->GetSurfaceSize( width, height );

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

		// Mark all convars from server dll with CVARF_CLIENT
		Con_SetConVarRegisterFlags( CVARF( CLIENT ) );
		EModLoadError modLoadRet = Mod_LoadSystem( clientModule );

		if ( modLoadRet != EModLoadError_Success )
		{
			Mod_Shutdown();
			return false;
		}
	}

	AppModule_t serverModule{ (ISystem**)&server, "ch_server", ISERVER_NAME, ISERVER_VER };

	// Mark all convars from server dll with CVARF_SERVER
	Con_SetConVarRegisterFlags( CVARF( SERVER ) );
	EModLoadError modLoadRet = Mod_LoadSystem( serverModule );

	if ( modLoadRet != EModLoadError_Success )
	{
		Mod_Shutdown();
		return false;
	}

	return true;
}


extern "C"
{
	void DLL_EXPORT game_init()
	{
		if ( gWaitForDebugger )
			sys_wait_for_debugger();

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
				return;
			}
		}
		else
		{
			if ( !Mod_AddSystems( gAppModulesClient, ARR_SIZE( gAppModulesClient ) ) )
			{
				Log_Error( "Failed to Load Systems\n" );
				return;
			}
		}

		if ( !gArgNoSteam )
		{
			AppModule_t steamModule{ (ISystem**)&steam, "ch_steam", ISTEAM_NAME, ISTEAM_VER, false };
			Mod_LoadSystem( steamModule );
		}

		if ( !LoadGameSystems() )
		{
			return;
		}

		Mod_InitSystems();

		// Reset ConVar Flags
		Con_SetConVarRegisterFlags( 0 );

		Con_SetCvarFlagCallback( CVARF( CL_EXEC ), CvarF_ClientExecuteCallback );

		// ftl::TaskSchedulerInitOptions schedOptions;
		// schedOptions.Behavior = ftl::EmptyQueueBehavior::Sleep;
		// 
		// gTaskScheduler.Init( schedOptions );
		
		auto startTime = std::chrono::high_resolution_clock::now();

		// Hack for Dedicated Server For now
		if ( gDedicatedServer )
		{
			gDedicatedViewport = graphics->CreateViewport();
		}

		// ---------------------------------------------------------------------------------------------
		// Main Loop

		while ( gRunning )
		{
			PROF_SCOPE_NAMED( "Main Loop" );

			auto  currentTime = std::chrono::high_resolution_clock::now();
			float time        = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count();

			// don't let the time go too crazy, usually happens when in a breakpoint
			time              = glm::min( time, host_max_frametime.GetFloat() );

			if ( host_fps_max.GetFloat() > 0.f )
			{
				float maxFps       = glm::clamp( host_fps_max.GetFloat(), 10.f, 5000.f );

				// check if we still have more than 2ms till next frame and if so, wait for "1ms"
				float minFrameTime = 1.0f / maxFps;
				if ( ( minFrameTime - time ) > ( 2.0f / 1000.f ) )
					sys_sleep( 1 );

				// framerate is above max
				if ( time < minFrameTime )
					continue;
			}

			// ftl::TaskCounter taskCounter( &gTaskScheduler );

			Map_UpdateTimer( time );

			input->Update( time );

			// may change from input update running the quit command
			if ( !gRunning )
				break;

			gCurrentModule = ECurrentModule_Client;

			if ( RunningClient() )
				client->PreUpdate( time );

			float frameTimeScaled = time * host_timescale;
			gCurrentModule        = ECurrentModule_Server;

			// Update Game Logic
			if ( server )
				server->Update( frameTimeScaled );

			gCurrentModule = ECurrentModule_Client;

			if ( RunningClient() )
				client->Update( frameTimeScaled );
			
			gCurrentModule = ECurrentModule_None;

			// Do this here for dedicated server
			if ( gDedicatedServer )
			{
				UpdateViewport();

				{
					PROF_SCOPE_NAMED( "Imgui New Frame" );
					ImGui::NewFrame();
					ImGui_ImplSDL2_NewFrame();
				}

				renderOld->NewFrame();
				gui->Update( time );

				if ( !( SDL_GetWindowFlags( render->GetWindow() ) & SDL_WINDOW_MINIMIZED ) )
				{
					renderOld->Present();
				}
				else
				{
					PROF_SCOPE_NAMED( "Imgui End Frame" );
					ImGui::EndFrame();
				}
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

		// ---------------------------------------------------------------------------------------------
		// Shutdown

		if ( server )
			server->Shutdown();

		if ( client )
			client->Shutdown();

		Resource_Shutdown();
	}
}

