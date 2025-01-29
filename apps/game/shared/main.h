#pragma once

#include "core/core.h"
#include "game_physics.h"
#include "entity/entity.h"

#include "iinput.h"
#include "igui.h"
#include "iaudio.h"
#include "physics/iphysics.h"
#include "render/irender.h"
#include "igraphics.h"
#include "steam/ch_isteam.h"

struct ViewportCamera_t;

// world xyz
constexpr int W_FORWARD = 0;
constexpr int W_RIGHT = 1;
constexpr int W_UP = 2;

//constexpr glm::vec3 vec3_forward(1, 0, 0);
//constexpr glm::vec3 vec3_right(0, 1, 0);
//constexpr glm::vec3 vec3_up(0, 0, 1);

const glm::vec3 vec3_forward(1, 0, 0);
const glm::vec3 vec3_right(0, 1, 0);
const glm::vec3 vec3_up(0, 0, 1);

//#include "game_rules.h"

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
extern IRender*      render;
extern IGuiSystem*   gui;
extern IAudioSystem* audio;

extern Entity        gLocalPlayer;
#endif

extern ViewportCamera_t gView;
extern float            gFrameTime;
extern double           gCurTime;


constexpr int           CH_MAX_USERNAME_LEN = 256;
constexpr float         CH_GRAVITY_SPEED    = 9.80665f;


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

void         Main_PreGameUpdate( float frameTime );
void         Main_PostGameUpdate( float frameTime );

void         Game_Update( float frameTime );
void         Game_UpdateGame( float frameTime );  // epic name

void         Game_HandleSystemEvents();

void         Game_SetView( const glm::mat4& srViewMat );
void         Game_UpdateProjection();

inline float Game_GetCurTime()
{
	return gCurTime;
}


constexpr const char* MAP_DATA_FILE     = "mapData.smf";
constexpr u64         MAP_DATA_FILE_LEN = 11;


// Undo Stack

