#include "main.h"

#include "core/app_info.h"
#include "core/profiler.h"

#include "iinput.h"
#include "render/irender.h"
#include "igraphics.h"
#include "iaudio.h"
#include "igui.h"
#include "physics/iphysics.h"

#include "imgui/imgui.h"

#include <chrono>
#include <vector>
#include <functional>

#if CH_USE_MIMALLOC
  #include "mimalloc-new-delete.h"
#endif

static bool        gWaitForDebugger = args_register( "Upon Program Startup, Wait for the Debugger to attach", "--debugger" );
static const char* gArgGamePath     = args_register_names( nullptr, "Path to the game to create assets for", 2, "--game", "-g" );
static bool        gRunning         = true;


CONVAR_RANGE_FLOAT( host_fps_max, 300, 0, 5000, "Maximum FPS the App can run at" );
CONVAR_RANGE_FLOAT( host_timescale, 1, 0, FLT_MAX, "Scaled Frametime of the App" );
CONVAR_RANGE_FLOAT( host_max_frametime, 0.1, 0, FLT_MAX, "Max time in seconds a frame can be" );


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


extern IGuiSystem*       gui;
extern IRender*          render;
extern IInputSystem*     input;
extern IAudioSystem*     audio;
extern Ch_IPhysics*      ch_physics;
extern IGraphics*        graphics;
extern IRenderSystemOld* renderOld;


static AppModule_t gAppModules[] = 
{
	{ (ISystem**)&input,      "ch_input",           IINPUTSYSTEM_NAME, IINPUTSYSTEM_VER },
	{ (ISystem**)&render,     "ch_graphics_api_vk", IRENDER_NAME, IRENDER_VER },
	{ (ISystem**)&audio,      "ch_aduio",           IADUIO_NAME, IADUIO_VER },
	{ (ISystem**)&ch_physics, "ch_physics",         IPHYSICS_NAME, IPHYSICS_VER },
    { (ISystem**)&graphics,   "ch_render",          IGRAPHICS_NAME, IGRAPHICS_VER },
    { (ISystem**)&renderOld,  "ch_render",          IRENDERSYSTEMOLD_NAME, IRENDERSYSTEMOLD_VER },
	{ (ISystem**)&gui,        "ch_gui",             IGUI_NAME, IGUI_HASH },
};


static const char* g_modules[] = {
	"ch_input",
	"ch_graphics_api_vk",
	"ch_aduio",
	"ch_physics",
	"ch_render",
	"ch_gui"
};


struct ch_system_entry_t
{
	ISystem**   system;  // TODO: try ISystem*&
	const char* system_name;
	u16         system_ver;
	bool        required = true;
};

#define CH_SYS_PTR( var ) (ISystem**)&var

static ch_system_entry_t g_systems[] = 
{
	{ (ISystem**)&input,      IINPUTSYSTEM_NAME,     IINPUTSYSTEM_VER },
	{ (ISystem**)&render,     IRENDER_NAME,          IRENDER_VER },
	{ (ISystem**)&audio,      IADUIO_NAME,           IADUIO_VER },
	{ (ISystem**)&ch_physics, IPHYSICS_NAME,         IPHYSICS_VER },
	{ (ISystem**)&graphics,   IGRAPHICS_NAME,        IGRAPHICS_VER },
	{ (ISystem**)&renderOld,  IRENDERSYSTEMOLD_NAME, IRENDERSYSTEMOLD_VER },
	{ (ISystem**)&gui,        IGUI_NAME,             IGUI_HASH },

//	{ CH_SYS_PTR( input ),      IINPUTSYSTEM_NAME,     IINPUTSYSTEM_VER },
//	{ CH_SYS_PTR( render ),     IRENDER_NAME,          IRENDER_VER },
//	{ CH_SYS_PTR( audio ),      IADUIO_NAME,           IADUIO_VER },
//	{ CH_SYS_PTR( ch_physics ), IPHYSICS_NAME,         IPHYSICS_VER },
//	{ CH_SYS_PTR( graphics ),   IGRAPHICS_NAME,        IGRAPHICS_VER },
//	{ CH_SYS_PTR( renderOld ),  IRENDERSYSTEMOLD_NAME, IRENDERSYSTEMOLD_VER },
//	{ CH_SYS_PTR( gui ),        IGUI_NAME,             IGUI_HASH },
};


struct ToolLoadDesc
{
	const char* name;
	const char* interface;
	int         version;
};


static ToolLoadDesc gToolModules[] = {
	{ "modules" CH_PATH_SEP_STR "ch_map_editor", CH_TOOL_MAP_EDITOR_NAME, CH_TOOL_MAP_EDITOR_VER },
	{ "modules" CH_PATH_SEP_STR "ch_material_editor", CH_TOOL_MAT_EDITOR_NAME, CH_TOOL_MAT_EDITOR_VER },
};


static void ShowInvalidGameOptionWindow( const char* spMessage )
{
	Log_ErrorF( 1, "%s\n", spMessage );
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
			ShowInvalidGameOptionWindow( "Failed to Load App Info" );
			return 1;
		}

		if ( gArgGamePath == nullptr || gArgGamePath[ 0 ] == '\0' )
		{
			ShowInvalidGameOptionWindow( "No Game Specified" );
			return 1;
		}

		// Load the game's app info
		ch_string appInfoPath;

		{
			const char* strings[] = { FileSys_GetExePath().data, CH_PATH_SEP_STR, gArgGamePath };
			u64         lengths[] = { FileSys_GetExePath().size, 1, strlen( strings[ 2 ] ) };
			appInfoPath           = ch_str_join( ARR_SIZE( strings ), strings, lengths );
		}

		if ( !Core_AddAppInfo( CH_STR_UR( appInfoPath ) ) )
		{
			ch_str_free( appInfoPath.data );
			ShowInvalidGameOptionWindow( "Failed to Load Game App Info" );
			return 1;
		}

		ch_str_free( appInfoPath.data );

		IMGUI_CHECKVERSION();

#if CH_USE_MIMALLOC
		Log_DevF( 1, "Using mimalloc version %d\n", mi_version() );
#endif

		// Needs to be done before Renderer is loaded
		ImGui::CreateContext();

		// if ( gArgUseGL )
		// {
		// 	gAppModules[ 1 ].apModuleName = "ch_render_gl";
		// }

		// Load Modules and Initialize them in this order
		if ( !Mod_AddSystems( gAppModules, ARR_SIZE( gAppModules ) ) )
		{
			Log_Error( "Failed to Load Systems\n" );
			return 1;
		}

		// Add Tools
		gTools.reserve( ARR_SIZE( gToolModules ) );
		for ( u32 i = 0; i < ARR_SIZE( gToolModules ); i++ )
		{
			ISystem*    toolSystem = nullptr;

			AppModule_t toolAppModule;
			toolAppModule.apInterfaceName = gToolModules[ i ].interface;
			toolAppModule.apInterfaceVer  = gToolModules[ i ].version;
			toolAppModule.apModuleName    = gToolModules[ i ].name;
			toolAppModule.apSystem        = (ISystem**)&toolSystem;
			toolAppModule.aRequired       = false;

			if ( !Mod_AddSystems( &toolAppModule, 1 ) )
			{
				Log_ErrorF( "Failed to Load Tool: %s\n", gToolModules[ i ].name );
				continue;
			}

			LoadedTool& tool = gTools.emplace_back();
			tool.interface   = gToolModules[ i ].interface;
			tool.tool        = (ITool*)toolSystem;
		}

		if ( !App_CreateMainWindow() )
		{
			Log_Error( "Failed to Create Main Window\n" );
			return 1;
		}

		if ( !Mod_InitSystems() )
		{
			Log_Error( "Failed to Init Systems\n" );
			return 1;
		}

		if ( !App_Init() )
		{
			Log_Error( "Failed to Start Editor!\n" );
			return 1;
		}

		Con_QueueCommandSilent( "exec autoexec", false );

		// ftl::TaskSchedulerInitOptions schedOptions;
		// schedOptions.Behavior = ftl::EmptyQueueBehavior::Sleep;
		// 
		// gTaskScheduler.Init( schedOptions );
		
		auto startTime = std::chrono::high_resolution_clock::now();

		// -------------------------------------------------------------------
		// Main Loop

		while ( gRunning )
		{
			PROF_SCOPE_NAMED( "Main Loop" );

			// TODO: REPLACE THIS, it's actually kinda expensive
			auto currentTime = std::chrono::high_resolution_clock::now();
			float time = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count();
	
			// don't let the time go too crazy, usually happens when in a breakpoint
			time = glm::min( time, host_max_frametime );

			if ( host_fps_max > 0.f )
			{
				float maxFps = glm::clamp( host_fps_max, 10.f, 5000.f );

				// check if we still have more than 2ms till next frame and if so, wait for "1ms"
				float minFrameTime = 1.0f / maxFps;
				if ( (minFrameTime - time) > (2.0f/1000.f))
					sys_sleep( 1 );

				// framerate is above max
				if ( time < minFrameTime )
					continue;
			}

			// ftl::TaskCounter taskCounter( &gTaskScheduler );

			input->Update( time );

			// may change from input update running the quit command
			if ( !gRunning )
				break;

			UpdateLoop( time );
			
			// Wait and help to execute unfinished tasks
			// gTaskScheduler.WaitForCounter( &taskCounter );

			startTime = currentTime;

#ifdef TRACY_ENABLE
			FrameMark;
#endif
		}

		return 0;
	}
}

