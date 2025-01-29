#include "main.h"
#include "core/system_loader.h"
#include "core/asserts.h"

#include "iinput.h"
#include "igui.h"
#include "render/irender.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"

#include "core/util.h"
#include "game_physics.h"
#include "player.h"
#include "igraphics.h"
#include "mapmanager.h"
#include "testing.h"
#include "steam.h"
#include "network/net_main.h"

#include <SDL_system.h>
#include <SDL_hints.h>

#include <algorithm>


IInputSystem*     input     = nullptr;
IGraphics*        graphics  = nullptr;
IRenderSystemOld* renderOld = nullptr;
ISteamSystem*     steam     = nullptr;

#if CH_CLIENT
IGuiSystem*   gui    = nullptr;
IRender*      render = nullptr;
IAudioSystem* audio  = nullptr;
#endif

static bool      gPaused      = false;
float            gFrameTime   = 0.f;

// TODO: make gRealTime and gGameTime
// real time is unmodified time since engine launched, and game time is time affected by host_timescale and pausing
double           gCurTime     = 0.0;  // i could make this a size_t, and then just have it be every 1000 is 1 second

#if CH_CLIENT
Entity           gLocalPlayer = CH_ENT_INVALID;
#endif

ViewportCamera_t gView{};

extern bool      gRunning;

CONVAR_FLOAT( phys_friction, 10, "" );


// CON_COMMAND( pause )
// {
// 	gui->ShowConsole();
// }


void Game_Shutdown()
{
	TEST_Shutdown();

	Phys_Shutdown();
	Net_Shutdown();

	Steam_Shutdown();
}


bool Game_InMap()
{
	return MapManager_HasMap();
}


void Game_SetPaused( bool paused )
{
	gPaused = paused;
}


bool Game_IsPaused()
{
	return gPaused;
}

