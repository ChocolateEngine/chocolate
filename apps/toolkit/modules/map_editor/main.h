#pragma once

#include "core/core.h"
#include "itool.h"

struct ViewportCamera_t;

// world xyz
constexpr int W_FORWARD = 0;
constexpr int W_RIGHT = 1;
constexpr int W_UP = 2;

#include "game_physics.h"
#include "entity.h"
#include "igraphics.h"
#include "mapmanager.h"

class IGuiSystem;
class IRender;
class IInputSystem;
class IAudioSystem;
class IGraphics;
class IRenderSystemOld;

extern IGuiSystem*       gui;
extern IRender*          render;
extern IInputSystem*     input;
extern IAudioSystem*     audio;
extern IGraphics*        graphics;
extern IRenderSystemOld* renderOld;

extern float             gFrameTime;
extern double            gCurTime;
extern u32               gMainViewport;

extern ToolLaunchData    gToolData;


enum EGizmoMode
{
	EGizmoMode_Translation,
	EGizmoMode_Rotation,
	EGizmoMode_Scale,
	EGizmoMode_Bounds,
	EGizmoMode_SuperGizmo,  // https://www.youtube.com/watch?v=jqtiCB1A5lI
};


enum EGizmoAxis
{
	EGizmoAxis_None,
	EGizmoAxis_X,
	EGizmoAxis_Y,
	EGizmoAxis_Z,
	EGizmoAxis_PlaneXY,
	EGizmoAxis_PlaneXZ,
	EGizmoAxis_PlaneYZ,
	EGizmoAxis_Screen,

	EGizmoAxis_Count,
};


struct GizmoData
{
	EGizmoMode mode;
	EGizmoAxis selectedAxis;
	bool       isWorldSpace;
	// bool       snapEnabled;
	// float      snapUnit;

	// Offset of gizmo from last frame to add onto entity position, rotation, or scale
	glm::vec3  offset;

	// Previous Mouse Ray from last frame
	Ray        lastFrameRay;
	glm::ivec2 lastMousePos;
	glm::vec3  lastIntersectionPoint;
	glm::vec4  translationPlane;
	glm::vec3  translationPlaneOrigin;
	glm::vec3  matrixOrigin;
	glm::vec3  relativeOrigin;
	float      screenFactor;
};


struct EditorView_t
{
	glm::mat4        aViewMat;
	glm::mat4        aProjMat;

	// projection matrix * view matrix
	glm::mat4        aProjViewMat;

	// TODO: use this in the future
	// u32              aViewportIndex;

	// TODO: remove this from here, right now this is the same between ALL contexts
	glm::uvec2       aResolution;
	glm::uvec2       aOffset;

	glm::vec3        aPos{};
	glm::vec3        aAng{};

	glm::vec3        aForward{};
	glm::vec3        aRight{};
	glm::vec3        aUp{};
};


// multiple of these exist per map open
struct EditorContext_t
{
	// Viewport Data
	EditorView_t           aView;

	// Map Data
	SiduryMap              aMap;

	// Entities Selected - NOT IMPLEMENTED YET
	ChVector< ch_handle_t > aEntitiesSelected;
};


struct EditorData_t
{
	// Inputs
	glm::vec3  aMove{};
	bool       aMouseInView;
	bool       aMouseCaptured;

	// hack for single window mode
	glm::uvec2 windowOffset;

	// Gizmo Data
	GizmoData  gizmo;
};

bool         MapEditor_Init();
void         MapEditor_Shutdown();

void         MapEditor_Update( float frameTime );
void         MapEditor_UpdateEditor( float frameTime );  // epic name

void         MapEditor_HandleSystemEvents();

void         Game_SetView( const glm::mat4& srViewMat );
void         Game_UpdateProjection();

inline float Game_GetCurTime()
{
	return gCurTime;
}


// -------------------------------------------------------------
// Map Editor Interface


class MapEditor : public ITool
{
   public:
	bool        running = false;

	const char* GetName() override
	{
		return "Map Editor";
	}

	// ISystem
	bool Init() override;
	void Shutdown() override;

	// ITool
	bool Launch( const ToolLaunchData& launchData ) override;
	void Close() override;

	void Update( float frameTime ) override;
	void Render( float frameTime, bool resize, glm::uvec2 offset ) override;
	void Present() override;

	bool OpenAsset( const std::string& path ) override;
};


extern MapEditor                       gMapEditorTool;


// -------------------------------------------------------------
// Editor Context


extern EditorData_t                    gEditorData;

extern ch_handle_t                      gEditorContextIdx;
// extern std::vector< EditorContext_t > gEditorContexts;
extern ResourceList< EditorContext_t > gEditorContexts;

constexpr u32                          CH_MAX_EDITOR_CONTEXTS    = 32;
constexpr ch_handle_t                   CH_INVALID_EDITOR_CONTEXT = CH_INVALID_HANDLE;

// return the context index, input a pointer address to the context if one was made
ch_handle_t                             Editor_CreateContext( EditorContext_t** spContext );
void                                   Editor_FreeContext( ch_handle_t sContext );

EditorContext_t*                       Editor_GetContext();
void                                   Editor_SetContext( ch_handle_t sContext );

// maybe do this?
//
// void                                  Editor_SetCurrentContext( u32 sContextId );


// Helper function that uses stuff from the graphics viewport system
Ray                                    Util_GetRayFromScreenSpace( glm::ivec2 mousePos, glm::vec3 origin, u32 viewportIndex );
