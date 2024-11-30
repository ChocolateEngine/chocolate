#include "sv_main.h"
#include "sv_interface.h"
#include "map_manager.h"
#include "main.h"


#define CH_GET_INTERFACE( var, type, name, ver )     \
	var = Mod_GetInterfaceCast< type >( name, ver ); \
	if ( var == nullptr )                            \
	{                                                \
		Log_Error( "Failed to load " name "\n" );    \
		return false;                                \
	}


class ServerSystem : public IServerSystem
{
   public:
	bool Init() override
	{
		// Get Modules
		CH_GET_INTERFACE( input, IInputSystem, IINPUTSYSTEM_NAME, IINPUTSYSTEM_HASH );
		//CH_GET_INTERFACE( render, IRender, IRENDER_NAME, IRENDER_VER );
		//CH_GET_INTERFACE( audio, IAudioSystem, IADUIO_NAME, IADUIO_VER );
		CH_GET_INTERFACE( ch_physics, Ch_IPhysics, IPHYSICS_NAME, IPHYSICS_HASH );
		CH_GET_INTERFACE( graphics, IGraphics, IGRAPHICS_NAME, IGRAPHICS_VER );
		//CH_GET_INTERFACE( gui, IGuiSystem, IGUI_NAME, IGUI_HASH );

		Phys_Init();

		if ( !Net_Init() )
		{
			Log_Error( "Failed to init networking\n" );
			return false;
		}

		if ( !SV_Init() )
		{
			Log_Error( "Failed to init server\n" );
			return false;
		}

		return true;
	}

	void Shutdown() override
	{
		SV_Shutdown();
		Game_Shutdown();
	}

	void Update( float sDT ) override
	{
		gFrameTime = sDT;

		if ( Game_IsPaused() )
		{
			gFrameTime = 0.f;
		}

		gCurTime += gFrameTime;

		if ( !SV_IsHosting() )
			return;

		SV_Update( sDT );
	}

	// ----------------------------------------------------------------------------

	bool IsHosting() override
	{
		return gServerData.aActive;
	}
	
	void CloseServer() override
	{
		SV_StopServer();
	}

	void PrintStatus() override
	{
		SV_PrintStatus();
	}

	void StartServer( const std::string& srMap ) override
	{
		if ( !MapManager_FindMap( srMap ) )
		{
			Log_Error( "Failed to Find map - Failed to Start Server\n" );
			return;
		}

		if ( !SV_StartServer() )
			return;

		if ( MapManager_LoadMap( srMap ) )
			return;

		Log_Error( "Failed to Load map - Failed to Start Server\n" );
		SV_StopServer();
	}
};


static ServerSystem      server;


static ModuleInterface_t gInterfaces[] = {
	{ &server, "Server", 1 }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* cframework_GetInterfaces( size_t& srCount )
	{
		srCount = 1;
		return gInterfaces;
	}
}

