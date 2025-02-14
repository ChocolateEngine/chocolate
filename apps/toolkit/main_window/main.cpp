#include "main.h"

#include <SDL_hints.h>
#include <SDL_system.h>

#include <algorithm>

#include "core/app_info.h"
#include "core/asserts.h"
#include "core/build_number.h"
#include "core/system_loader.h"
#include "core/util.h"
#include "igraphics.h"
#include "igui.h"
#include "iinput.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "material_editor/material_editor.h"
#include "physics/iphysics.h"
#include "render/irender.h"


int                       gWidth          = args_register_names( 1280, "Width of the main window", 2, "--width", "-w" );
int                       gHeight         = args_register_names( 720, "Height of the main window", 2, "--height", "-h" );
static bool               gMaxWindow      = args_register( "Maximize the main window", "--max" );
static bool               gSingleWindow   = args_register( "Single Window Mode, all tools will be rendered in tabs on the main window", "--single-window" );


SDL_Window*               gpWindow        = nullptr;
void*                     gpSysWindow     = nullptr;  // Only Used on WIN32
ch_handle_t               gGraphicsWindow = CH_INVALID_HANDLE;

Toolkit                   toolkit;

// std::vector< AppWindow >  gWindows;
std::vector< LoadedTool > gTools;


IGuiSystem*               gui        = nullptr;
IRender*                  render     = nullptr;
IInputSystem*             input      = nullptr;
IAudioSystem*             audio      = nullptr;
IGraphics*                graphics   = nullptr;
IRenderSystemOld*         renderOld  = nullptr;
Ch_IPhysics*              ch_physics = nullptr;

//ITool*                    toolMapEditor     = nullptr;
//ITool*                    toolMatEditor     = nullptr;


//bool                      toolMapEditorOpen = false;
//bool                      toolMatEditorOpen = false;

static bool               gPaused    = false;
float                     gFrameTime = 0.f;

// TODO: make gRealTime and gGameTime
// real time is unmodified time since engine launched, and game time is time affected by host_timescale and pausing
double                    gCurTime   = 0.0;  // i could make this a size_t, and then just have it be every 1000 is 1 second

extern bool               gRunning;

u32                       gMainViewportHandle   = UINT32_MAX;

int                       gMainMenuBarHeight    = 0.f;
static bool               gShowQuitConfirmation = false;

CONVAR_FLOAT_EXT( host_timescale );

CONVAR_FLOAT( r_nearz, 0.01, "Camera Near Z Plane" );
CONVAR_FLOAT( r_farz, 10000, "Camera Far Z Plane" );
CONVAR_FLOAT( r_fov, 106, "FOV" );


void Util_DrawTextureInfo( TextureInfo_t& info )
{
	ImGui::Text( "Name: %s", info.name.size ? info.name.data : "UNNAMED" );

	if ( info.aPath.size )
		ImGui::TextUnformatted( info.aPath.data );

	ImGui::Text( "%d x %d - %.6f MB", info.aSize.x, info.aSize.y, ch_bytes_to_mb( info.aMemoryUsage ) );
	ImGui::Text( "Format: TODO" );
	ImGui::Text( "Mip Levels: TODO" );
	ImGui::Text( "GPU Index: %d", info.aGpuIndex );
	ImGui::Text( "Ref Count: %d", info.aRefCount );
}


void DrawQuitConfirmation();


void Main_DrawGraphicsSettings()
{
	auto windowSize = ImGui::GetWindowSize();
	windowSize.x -= 60;
	windowSize.y -= 60;

	//ImGui::SetNextWindowContentSize( windowSize );

	if ( !ImGui::BeginChild( "Graphics Settings", {}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border ) )
	{
		ImGui::EndChild();
		return;
	}

	static ConVarData_t* r_msaa          = Con_GetConVarData( "r_msaa" );
	static ConVarData_t* r_msaa_samples  = Con_GetConVarData( "r_msaa_samples" );
	static ConVarData_t* r_msaa_textures = Con_GetConVarData( "r_msaa_textures" );
	static ConVarData_t* r_fov           = Con_GetConVarData( "r_fov" );

	bool                 msaa            = *r_msaa->aBool.apData;
	int                  msaa_samples    = *r_msaa_samples->aInt.apData;
	bool                 msaa_textures   = *r_msaa_textures->aBool.apData;
	float                fov             = *r_fov->aFloat.apData;

	if ( ImGui::SliderFloat( "FOV", &fov, 0.1f, 179.9f ) )
	{
		std::string    fovStr = ch_to_string( fov );
		ch_string_auto fovCmd = ch_str_join( "r_fov ", 6, fovStr.data(), fovStr.size() );

		Con_QueueCommandSilent( fovCmd.data, fovCmd.size );
	}

	std::string msaaPreview = msaa ? ch_to_string( msaa_samples ) + "X" : "Off";
	if ( ImGui::BeginCombo( "MSAA", msaaPreview.c_str() ) )
	{
		if ( ImGui::Selectable( "Off", !msaa ) )
		{
			Con_QueueCommandSilent( "r_msaa 0" );
		}

		int maxSamples = render->GetMaxMSAASamples();

		// TODO: check what your graphics card actually supports
		if ( maxSamples >= 2 && ImGui::Selectable( "2X", msaa && msaa_samples == 2 ) )
		{
			Con_QueueCommandSilent( "r_msaa 1; r_msaa_samples 2" );
		}

		if ( maxSamples >= 4 && ImGui::Selectable( "4X", msaa && msaa_samples == 4 ) )
		{
			Con_QueueCommandSilent( "r_msaa 1; r_msaa_samples 4" );
		}

		if ( maxSamples >= 8 && ImGui::Selectable( "8X", msaa && msaa_samples == 8 ) )
		{
			Con_QueueCommandSilent( "r_msaa 1; r_msaa_samples 8" );
		}

		if ( maxSamples >= 16 && ImGui::Selectable( "16X", msaa && msaa_samples == 16 ) )
		{
			Con_QueueCommandSilent( "r_msaa 1; r_msaa_samples 16" );
		}

		if ( maxSamples >= 32 && ImGui::Selectable( "32X", msaa && msaa_samples == 32 ) )
		{
			Con_QueueCommandSilent( "r_msaa 1; r_msaa_samples 32" );
		}

		if ( maxSamples >= 64 && ImGui::Selectable( "64X", msaa && msaa_samples == 64 ) )
		{
			Con_QueueCommandSilent( "r_msaa 1; r_msaa_samples 64" );
		}

		ImGui::EndCombo();
	}

	if ( ImGui::Checkbox( "MSAA Textures/Sample Shading - Very Expensive", &msaa_textures ) )
	{
		if ( msaa_textures )
			Con_QueueCommandSilent( "r_msaa_textures 1" );
		else
			Con_QueueCommandSilent( "r_msaa_textures 0" );
	}

	//ImGui::SetWindowSize( windowSize );

	ImGui::EndChild();
}


void Main_DrawSettingsMenu()
{
	if ( !ImGui::BeginChild( "Settings Menu" ) )
	{
		ImGui::EndChild();
		return;
	}

	if ( ImGui::BeginTabBar( "settings tabs" ) )
	{
		if ( ImGui::BeginTabItem( "Graphics" ) )
		{
			Main_DrawGraphicsSettings();
			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Other" ) )
		{
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImVec2 windowSize = ImGui::GetWindowSize();

	ImGui::SetCursorPosY( windowSize.y - 28 );

	if ( ImGui::Button( "Save" ) )
	{
		Con_Archive();
	}

	ImGui::EndChild();
}


LoadedTool* App_GetTool( const char* tool )
{
	if ( tool == nullptr )
		return nullptr;

	for ( u32 i = 0; i < gTools.size(); i++ )
	{
		if ( ch_str_equals( gTools[ i ].interface, tool ) )
			return &gTools[ i ];
	}

	Log_ErrorF( "Failed to find tool \"%s\"\n", tool );
	return nullptr;
}


AppWindow* App_GetToolWindow( const char* toolInterface )
{
	LoadedTool* tool = App_GetTool( toolInterface );

	if ( tool && tool->window )
		return tool->window;

	Log_ErrorF( "Failed to find window for tool!\n" );
	return nullptr;
}


void App_LaunchTool( const char* toolInterface )
{
	if ( !toolInterface )
	{
		Log_ErrorF( "Tool not Loaded!\n" );
		return;
	}

	LoadedTool* tool = App_GetTool( toolInterface );

	if ( tool->running )
	{
		if ( tool->window )
			Window_Focus( tool->window );
		else
			Log_ErrorF( "TODO: FOCUS TOOL TAB!\n" );

		return;
	}

	// Launch this tool instead
	const char* windowName = tool->tool->GetName();
	AppWindow*  window     = nullptr;

	if ( !gSingleWindow )
	{
		window = Window_Create( windowName );

		if ( !window )
		{
			Log_ErrorF( "Failed to open tool window: \"%s\"\n", windowName );
			return;
		}
	}

	ToolLaunchData launchData{};
	launchData.toolkit = &toolkit;

	if ( gSingleWindow )
	{
		launchData.window         = gpWindow;
		launchData.graphicsWindow = gGraphicsWindow;
	}
	else
	{
		launchData.window         = window->window;
		launchData.graphicsWindow = window->graphicsWindow;
	}

	if ( !tool->tool->Launch( launchData ) )
	{
		Log_ErrorF( "Failed to launch tool: \"%s\"\n", windowName );

		if ( window )
			Window_OnClose( *window );
	}

	tool->window  = window;
	tool->running = true;
}


void Main_DrawMenuBar()
{
	if ( gShowQuitConfirmation )
	{
		DrawQuitConfirmation();
	}

	if ( ImGui::BeginMainMenuBar() )
	{
		if ( ImGui::BeginMenu( "Load Tool" ) )
		{
			if ( ImGui::MenuItem( "Game" ) )
			{
			}

			ImGui::Separator();

			for ( auto& tool : gTools )
			{
				if ( ImGui::MenuItem( tool.tool->GetName(), nullptr, tool.running ) )
					App_LaunchTool( tool.interface );
			}

			ImGui::Separator();

			if ( ImGui::MenuItem( "Quit" ) )
			{
				gShowQuitConfirmation = true;
			}

			ImGui::EndMenu();
		}

		if ( ImGui::BeginMenu( "Edit" ) )
		{
			ImGui::EndMenu();
		}

		ImGui::Separator();

		ImGui::EndMainMenuBar();
	}

	auto yeah          = ImGui::GetItemRectSize();
	gMainMenuBarHeight = yeah.y;
}


// disabled cause for some reason, it could jump to the WindowProc function mid frame and call this
#define CH_LIVE_WINDOW_RESIZE 1


// return true if we should stop the UpdateLoop
void App_CloseMainWindow()
{
	// Close ALL Windows and Tools
	for ( u32 i = 0; i < gTools.size(); i++ )
	{
		if ( gTools[ i ].running )
		{
			gTools[ i ].tool->Close();
		}

		if ( gTools[ i ].window )
		{
			input->RemoveWindow( gTools[ i ].window->window );
			Window_OnClose( *gTools[ i ].window );
		}
	}

	gTools.clear();

	AssetBrowser_Close();

	// lazy way to tell engine to quit
	Con_RunCommand( "quit" );
}


bool App_HandleEvents()
{
	std::vector< SDL_Event >* events = input->GetEvents();

	for ( SDL_Event& event : *events )
	{
		if ( event.type == SDL_QUIT )
		{
			App_CloseMainWindow();
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
					App_CloseMainWindow();
					return true;
				}

				for ( u32 i = 0; i < gTools.size(); i++ )
				{
					if ( !gTools[ i ].window )
						continue;

					if ( gTools[ i ].window->window != sdlWindow )
						continue;

					gTools[ i ].tool->Close();
					Window_OnClose( *gTools[ i ].window );
					gTools[ i ].window  = nullptr;
					gTools[ i ].running = false;
					break;
				}
			}
		}
	}

	return false;
}


void RenderMainWindow( float frameTime, bool sResize )
{
	PROF_SCOPE();

	Main_DrawMenuBar();

	// hack
	static bool showConsole     = false;
	static bool wasConsoleShown = false;

	// set position
	int         width, height;
	render->GetSurfaceSize( gGraphicsWindow, width, height );

	// ImGui::SetNextWindowSizeConstraints( { (float)width, (float)( (height) - gMainMenuBarHeight ) }, { (float)width, (float)( (height) - gMainMenuBarHeight ) } );
	//if ( showConsole )
	//	ImGui::SetNextWindowSizeConstraints( { (float)width, 64 }, { (float)width, 64 } );
	//else

	static bool inToolTab      = false;

	// value... fresh from my ass
	// float       titleBarHeight = 16.f;
	// float       titleBarHeight = 10.f;
	// float       titleBarHeight = 28.f;
	float       titleBarHeight = 31.f;

	ImGui::SetNextWindowPos( { 0, static_cast< float >( gMainMenuBarHeight ) } );

	if ( inToolTab )
	{
		// ImGui::SetNextWindowSizeConstraints( { (float)width, (float)( gMainMenuBarHeight + titleBarHeight ) }, { (float)width, (float)( gMainMenuBarHeight + titleBarHeight ) } );
		ImGui::SetNextWindowSizeConstraints( { (float)width, (float)( titleBarHeight ) }, { (float)width, (float)( titleBarHeight ) } );
	}
	else
	{
		ImGui::SetNextWindowSizeConstraints( { (float)width, (float)( (height)-gMainMenuBarHeight ) }, { (float)width, (float)( (height)-gMainMenuBarHeight ) } );
	}

	if ( ImGui::Begin( "##Asset Browser", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollWithMouse ) )
	{
		if ( ImGui::BeginTabBar( "Toolkit Tabs" ) )
		{
			inToolTab = false;

			if ( ImGui::BeginTabItem( "Asset List" ) )
			{
				AssetBrowser_Draw();
				ImGui::EndTabItem();
			}

			if ( ImGui::BeginTabItem( "Material Editor" ) )
			{
				MaterialEditor_Draw( { 0, 32 } );
				ImGui::EndTabItem();
			}

			if ( ImGui::BeginTabItem( "Console" ) )
			{
				showConsole = true;
				gui->DrawConsole( wasConsoleShown, true );
				ImGui::EndTabItem();
			}
			else
			{
				showConsole = false;
			}

			if ( ImGui::BeginTabItem( "Resource Usage" ) )
			{
				ResourceUsage_Draw();
				ImGui::EndTabItem();
			}

			if ( ImGui::BeginTabItem( "Settings" ) )
			{
				Main_DrawSettingsMenu();
				ImGui::EndTabItem();
			}

			// Draw the tabs for the main tools
			for ( auto& tool : gTools )
			{
				if ( !tool.running )
					continue;

				if ( tool.window )
					continue;

				if ( ImGui::BeginTabItem( tool.tool->GetName() ) )
				{
					inToolTab = true;
					tool.tool->Render( frameTime, sResize, { 0, titleBarHeight } );
					ImGui::EndTabItem();
				}
			}

			ImGui::EndTabBar();
		}

		if ( wasConsoleShown != showConsole )
			wasConsoleShown = showConsole;

		ImGui::End();
	}

	if ( sResize )
		UpdateProjection();
}


void UpdateLoop( float frameTime, bool sResize )
{
	PROF_SCOPE();

	{
		PROF_SCOPE_NAMED( "Imgui New Frame" );
		ImGui::NewFrame();
		ImGui_ImplSDL2_NewFrame();
	}

	if ( App_HandleEvents() )
		return;

	renderOld->NewFrame();

	if ( sResize )
		renderOld->Reset( gGraphicsWindow );

	// Run Tools
	if ( !sResize )
	{
		for ( u32 i = 0; i < gTools.size(); i++ )
		{
			if ( gTools[ i ].running )
				gTools[ i ].tool->Update( frameTime );
		}
	}

	input->SetCurrentWindow( gpWindow );

	if ( !( SDL_GetWindowFlags( gpWindow ) & SDL_WINDOW_MINIMIZED ) )
	{
		RenderMainWindow( frameTime, sResize );
	}
	else
	{
		PROF_SCOPE_NAMED( "Imgui End Frame" );
		ImGui::EndFrame();
	}

	if ( !sResize )
	{
		// Draw other windows
		for ( auto& tool : gTools )
		{
			if ( !tool.running )
				continue;

			if ( tool.window )
				Window_Render( tool, frameTime, sResize );
		}
	}

	if ( sResize )
		return;

	renderOld->PrePresent();
	renderOld->Present( gGraphicsWindow, &gMainViewportHandle, 1 );
	Window_PresentAll();

	Con_Update();
}


void WindowResizeCallback( void* hwnd )
{
#if CH_LIVE_WINDOW_RESIZE
	if ( hwnd == gpSysWindow )
	{
		UpdateLoop( 0.f, true );

		renderOld->PrePresent();
		renderOld->Present( gGraphicsWindow, &gMainViewportHandle, 1 );
		return;
	}

	for ( LoadedTool& tool : gTools )
	{
		if ( !tool.window )
			continue;

		if ( tool.window->sysWindow != hwnd )
			continue;

		Window_Render( tool, 0.f, true );
		renderOld->PrePresent();
		Window_Present( tool );
		return;
	}
#endif
}


bool App_CreateMainWindow()
{
	// Create Main Window
	std::string window_name;

	window_name = ( core_app_info_get().apWindowTitle ) ? core_app_info_get().apWindowTitle : "Chocolate Engine";
	window_name += vstring( " - Build %zd - Compiled On - %s %s", Core_GetBuildNumber(), Core_GetBuildDate(), Core_GetBuildTime() );

	if ( !sys_create_window( gpSysWindow, gpWindow, window_name.c_str(), gWidth, gHeight, gMaxWindow ) )
	{
		Log_Error( "Failed to create toolkit window\n" );
		return false;
	}

	render->SetMainSurface( gpWindow, gpSysWindow );

	input->AddWindow( gpWindow, ImGui::GetCurrentContext() );

	return true;
}


bool App_Init()
{
	gGraphicsWindow = render->CreateWindow( gpWindow, gpSysWindow );

	if ( gGraphicsWindow == CH_INVALID_HANDLE )
	{
		Log_Fatal( "Failed to Create GraphicsAPI Window\n" );
		return false;
	}

	gui->StyleImGui();

	// Create the Main Viewport - TODO: use this more across the game code
	gMainViewportHandle = graphics->CreateViewport();

	UpdateProjection();
#ifdef _WIN32
	sys_set_resize_callback( WindowResizeCallback );
#endif /* _WIN32  */

	AssetBrowser_Init();

	Log_Msg( "Toolkit Loaded!\n" );
	return true;
}


static void DrawQuitConfirmation()
{
	int width = 0, height = 0;
	render->GetSurfaceSize( gGraphicsWindow, width, height );

	ImGui::SetNextWindowSize( { (float)width, (float)height } );
	ImGui::SetNextWindowPos( { 0.f, 0.f } );

	if ( ImGui::Begin( "FullScreen Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav ) )
	{
		ImGui::SetNextWindowFocus();

		ImGui::SetNextWindowSize( { 250, 60 } );

		ImGui::SetNextWindowPos( { ( width / 2.f ) - 125.f, ( height / 2.f ) - 30.f } );

		if ( !ImGui::Begin( "Quit Editor", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove ) )
		{
			ImGui::End();
			return;
		}

		ImGui::Text( "Are you sure you want to quit?" );

		ImGui::Separator();

		if ( ImGui::Button( "Yes", { 50, 0 } ) )
		{
			Con_QueueCommand( "quit" );
		}

		ImGui::SameLine( 190 );

		if ( ImGui::Button( "No", { 50, 0 } ) )
		{
			gShowQuitConfirmation = false;
		}

		ImGui::End();
	}

	ImGui::End();
}


void UpdateProjection()
{
	PROF_SCOPE();

	int width = 0, height = 0;
	render->GetSurfaceSize( gGraphicsWindow, width, height );

	auto& io                   = ImGui::GetIO();
	io.DisplaySize.x           = width;
	io.DisplaySize.y           = height;

	ViewportShader_t* viewport = graphics->GetViewportData( gMainViewportHandle );

	if ( !viewport )
		return;

	// HACK HACK HACK
	// if ( !gSingleWindow )
	{
		viewport->aNearZ      = r_nearz;
		viewport->aFarZ       = r_farz;
		viewport->aSize       = { width, height };
		viewport->aOffset     = { 0, 0 };
		viewport->aProjection = glm::mat4( 1.f );
		viewport->aView       = glm::mat4( 1.f );
		viewport->aProjView   = glm::mat4( 1.f );
	}

	graphics->SetViewportUpdate( true );
}


void Toolkit::OpenAsset( const char* spToolInterface, const char* spPath )
{
	LoadedTool* tool = App_GetTool( spToolInterface );

	if ( tool == nullptr )
	{
		Log_ErrorF( "Failed to find tool \"%s\" to open asset with: \"%s\"\n", spToolInterface, spPath );
		return;
	}

	if ( !tool->running )
		App_LaunchTool( spToolInterface );

	if ( !tool->running )
	{
		Log_ErrorF( "Failed to launch tool \"%s\" to open asset with: \"%s\"\n", spToolInterface, spPath );
		return;
	}

	if ( !tool->tool->OpenAsset( spPath ) )
	{
		Log_ErrorF( "Failed to open asset in tool \"%s\": \"%s\"\n", spToolInterface, spPath );
		return;
	}

	if ( tool->window )
		Window_Focus( tool->window );
}
