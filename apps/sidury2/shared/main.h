#pragma once

#include "core/core.h"
#include "system.h"
#include "game_physics.h"

#include "iinput.h"
#include "igui.h"
#include "iaudio.h"
#include "physics/iphysics.h"
#include "render/irender.h"
#include "graphics/igraphics.h"
#include "steam/ch_isteam.h"

struct ViewportCamera_t;

// world xyz
constexpr int W_FORWARD = 0;
constexpr int W_RIGHT = 1;
constexpr int W_UP = 2;

class IGuiSystem;
class IRender;
class IInputSystem;
class IAudioSystem;
class IGraphics;
class IRenderSystemOld;
class ISteamSystem;

extern IInputSystem*     input;
extern IGraphics*        graphics;
extern IRenderSystemOld* renderOld;
extern ISteamSystem*     steam;
extern bool              gSteamLoaded;

#if CH_CLIENT
extern IRender*          render;
extern IGuiSystem*       gui;
extern IAudioSystem*     audio;
#endif

extern float             gFrameTime;
extern double            gCurTime;

constexpr int            CH_MAX_USERNAME_LEN = 256;


enum class GameState
{
	Menu,
	Loading,
	Running,
	Paused,
};


bool         Game_InMap();

void         Game_SetPaused( bool paused );
bool         Game_IsPaused();

bool         Game_Init();
void         Game_Shutdown();

void         Game_Update( float frameTime );
void         Game_UpdateGame( float frameTime );  // epic name

void         Game_HandleSystemEvents();

void         Game_UpdateProjection();

inline float Game_GetCurTime()
{
	return gCurTime;
}


// Undo Stack

