#pragma once

#include "core/core.h"
#include "igraphics.h"
#include "igui.h"
#include "iinput.h"
#include "imgui/imgui.h"
#include "itool.h"
#include "physics/iphysics.h"
#include "render/irender.h"

class IGuiSystem;
class IRender;
class IInputSystem;
class IAudioSystem;
class IGraphics;
class IRenderSystemOld;
class ImGuiContext;


struct AppWindow
{
	SDL_Window*   window         = nullptr;
	void*         sysWindow      = nullptr;
	ch_handle_t   graphicsWindow = CH_INVALID_HANDLE;
	ImGuiContext* context        = nullptr;
};


struct LoadedTool
{
	const char* interface          = nullptr;
	ITool*      tool               = nullptr;
	AppWindow*  window             = nullptr;
	bool        running            = false;
	bool        renderInMainWindow = false;  // if false, this tool has it's own window, if true, it's stored in a tab in the main window
};


class Toolkit : public IToolkit
{
   public:
	// Open an Asset in a Tool
	// - spToolInterface is the name of the module interface
	// - spPath is the path to the asset to open
	void OpenAsset( const char* spToolInterface, const char* spPath ) override;
};


extern IGuiSystem*               gui;
extern IRender*                  render;
extern IInputSystem*             input;
extern IAudioSystem*             audio;
extern IGraphics*                graphics;
extern IRenderSystemOld*         renderOld;

extern Toolkit                   toolkit;

extern u32                       gMainViewportHandle;
extern SDL_Window*               gpWindow;
extern ch_handle_t               gGraphicsWindow;
extern std::vector< LoadedTool > gTools;

extern int                       gWidth;
extern int                       gHeight;
extern int                       gMainMenuBarHeight;

CONVAR_FLOAT_EXT( r_nearz );
CONVAR_FLOAT_EXT( r_farz );
CONVAR_FLOAT_EXT( r_fov );

void        Util_DrawTextureInfo( TextureInfo_t& info );

LoadedTool* App_GetTool( const char* tool );
bool        App_CreateMainWindow();
bool        App_Init();

void        UpdateLoop( float frameTime, bool sResize = false );
void        UpdateProjection();

AppWindow*  Window_Create( const char* windowName );
void        Window_OnClose( AppWindow& window );
void        Window_Focus( AppWindow* window );
void        Window_Render( LoadedTool& tool, float frameTime, bool sResize );
void        Window_Present( LoadedTool& window );
void        Window_PresentAll();

bool        AssetBrowser_Init();
void        AssetBrowser_Close();
void        AssetBrowser_Draw();

void        ResourceUsage_Draw();
