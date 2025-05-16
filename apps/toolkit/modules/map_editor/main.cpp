#include "main.h"

#include "../../main_window/main.h"
#include "core/asserts.h"
#include "core/system_loader.h"
#include "core/util.h"
#include "editor_view.h"
#include "entity_editor.h"
#include "game_physics.h"
#include "igraphics.h"
#include "igui.h"
#include "iinput.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "inputsystem.h"
#include "mapmanager.h"
#include "render/irender.h"
#include "skybox.h"
// #include "importer.h"

#include <SDL_hints.h>
#include <SDL_system.h>

#include <algorithm>


// TODO: make gRealTime and gGameTime
// real time is unmodified time since engine launched, and game time is time affected by host_timescale and pausing

extern bool gRunning;

CONVAR_FLOAT( phys_friction, 10, "Friction" );
CONVAR_BOOL( ui_show_imgui_demo, 0, "Show the ImGui Demo" );
CONVAR_BOOL( ui_show_render_stats, true, "Show Renderer Stats" );


static bool                     gShowQuitConfirmation = false;


EditorData_t                    gEditorData{};

ch_handle_t                     gEditorContextIdx = CH_INVALID_EDITOR_CONTEXT;
ResourceList< EditorContext_t > gEditorContexts;


MapEditor                       gMapEditorTool;


static ModuleInterface_t        gInterfaces[] = {
    { &gMapEditorTool, CH_TOOL_MAP_EDITOR_NAME, CH_TOOL_MAP_EDITOR_VER }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& srCount )
	{
		srCount = 1;
		return gInterfaces;
	}
}


// CON_COMMAND( pause )
// {
// 	gui->ShowConsole();
// }


IPhysicsObject* reallyCoolObject = nullptr;


void            DrawQuitConfirmation();


void            Main_DrawInputSettings()
{
	if ( !ImGui::BeginChild( "Input", {}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Border ) )
	{
		ImGui::EndChild();
		return;
	}

	if ( ImGui::BeginTable( "Bindings", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex( 0 );

		ImGui::Text( "Keys" );

		ImGui::TableSetColumnIndex( 1 );
		ImGui::Text( "Binding" );

		for ( u16 binding = 0; binding < EBinding_Count; binding++ )
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex( 0 );

			const char*         bindingStr = Input_BindingToStr( (EBinding)binding );
			const ButtonList_t* buttonList = Input_GetKeyBinding( (EBinding)binding );

			if ( buttonList )
			{
				ImGui::Text( Input_ButtonListToStr( buttonList ).c_str() );
			}

			ImGui::TableSetColumnIndex( 1 );
			ImGui::Text( bindingStr );
		}

		ImGui::EndTable();
	}

	ImGui::EndChild();
}


// disabled cause for some reason, it could jump to the WindowProc function mid frame and call this
#define CH_LIVE_WINDOW_RESIZE 1


RenderStats_t gRenderStats{};


void          UpdateLoop( float frameTime, bool sResize, glm::uvec2 sOffset )
{
#if 0
	PROF_SCOPE();

	if ( ui_show_imgui_demo )
		ImGui::ShowDemoWindow();

	if ( !sResize )
		MapEditor_UpdateEditor( frameTime );

	//	Phys_Simulate( GetPhysEnv(), frameTime );

	if ( ui_show_render_stats )
	{
		SDL_bool relativeMouse = SDL_GetRelativeMouseMode();
		gui->DebugMessage( "Relative Mouse Mode: %s", relativeMouse ? "ON" : "OFF" );

		static const bool& r_vis          = Con_GetConVarData_Bool( "r_vis", true );
		static const bool& r_msaa         = Con_GetConVarData_Bool( "r_msaa", false );
		static const int&  r_msaa_samples = Con_GetConVarData_Int( "r_msaa_samples", 1 );

		gui->DebugMessage( "%d Draw Calls", gRenderStats.aDrawCalls );
		gui->DebugMessage( "%d Vertices", gRenderStats.aVerticesDrawn );
		gui->DebugMessage( "%d Triangles", gRenderStats.aVerticesDrawn / 3 );
		gui->DebugMessage( "VIS %s", r_vis ? "ON" : "OFF" );

		if ( r_msaa )
			gui->DebugMessage( "MSAA %dX", r_msaa_samples );
	}

	gui->Update( frameTime );

	if ( !sResize )
		Entity_Update();

	EntEditor_Update( frameTime );

	if ( reallyCoolObject )
	{
		glm::vec3 pos = reallyCoolObject->GetPos();
		glm::vec3 ang = reallyCoolObject->GetAng();

		graphics->DrawAxis( pos, ang, { 1, 1, 1 } );
	}

	if ( !( SDL_GetWindowFlags( gToolData.window ) & SDL_WINDOW_MINIMIZED ) )
	{
		//		Main_DrawMenuBar();
		EntEditor_DrawUI();

		if ( sResize )
			Game_UpdateProjection();
	}

	if ( sResize )
		return;

	// Resource_Update();
#endif
}


void WindowResizeCallback()
{
#if CH_LIVE_WINDOW_RESIZE
	UpdateLoop( 0.f, true, {} );
#endif
}


bool MapEditor_Init()
{
	//gpEditorContext = &gEditorContexts.emplace_back();

	// Startup the Game Input System
	Input_Init();

	// if ( !Importer_Init() )
	// {
	// 	Log_ErrorF( "Failed to init Importer\n" );
	// 	return false;
	// }

	Game_UpdateProjection();

#if AUDIO_OPENAL
	// hAudioMusic = audio->RegisterChannel( "Music" );
#endif

	Phys_Init();

	Skybox_Init();
	EntEditor_Init();

	// TODO, mess with ImGui WantSaveIniSettings

	Log_Msg( "Editor Loaded!\n" );
	return true;
}


static bool        gInSkyboxSelect   = false;
static std::string gSkyboxSelectPath = "";


// return true if model selected
std::string        DrawSkyboxSelectionWindow()
{
	if ( !ImGui::Begin( "Model Selection Window" ) )
	{
		ImGui::End();
		return "";
	}

	if ( ImGui::Button( "Close" ) )
	{
		gInSkyboxSelect = false;
		gSkyboxSelectPath.clear();
	}

	std::vector< ch_string > fileList = FileSys_ScanDir( gSkyboxSelectPath.data(), gSkyboxSelectPath.size(), ReadDir_AbsPaths );

	for ( ch_string file : fileList )
	{
		bool isDir = FileSys_IsDir( file.data, file.size, true );

		if ( !isDir )
		{
			bool model = ch_str_ends_with( file, ".cmt", 4 );

			if ( !model )
				continue;
		}

		if ( ImGui::Selectable( file.data ) )
		{
			if ( isDir )
			{
				ch_string_auto cleanedPath = FileSys_CleanPath( file.data, file.size );
				gSkyboxSelectPath          = std::string( cleanedPath.data, cleanedPath.size );
			}
			else
			{
				ImGui::End();
				return std::string( file.data, file.size );
			}
		}
	}

	ImGui::End();

	return "";
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


void MapEditor_UpdateEditor( float frameTime )
{
	PROF_SCOPE();

	gFrameTime = frameTime;

	MapEditor_HandleSystemEvents();

	Input_Update();

	gCurTime += gFrameTime;

	EditorView_Update();
}


void MapEditor_HandleSystemEvents()
{
	PROF_SCOPE();

#if 0
	static std::vector< SDL_Event >* events = input->GetEvents();

	for ( auto& e: *events )
	{
		switch (e.type)
		{
			// TODO: remove this and use pause concmd, can't at the moment as binds are only parsed when game is active, hmm
			case SDL_KEYDOWN:
			{
				if ( e.key.keysym.sym == SDLK_BACKQUOTE || e.key.keysym.sym == SDLK_ESCAPE )
					gui->ShowConsole();
			
				break;
			}

			case SDL_WINDOWEVENT:
			{
				switch (e.window.event)
				{
					case SDL_WINDOWEVENT_DISPLAY_CHANGED:
					{
						// static float prevDPI = 96.f;
						// 
						// float dpi, hdpi, vdpi;
						// int test = SDL_GetDisplayDPI( e.window.data1, &dpi, &hdpi, &vdpi );
						// 
						// ImGuiStyle& style = ImGui::GetStyle();
						// style.ScaleAllSizes( dpi / prevDPI );
						// 
						// prevDPI = dpi;
					}

					case SDL_WINDOWEVENT_SIZE_CHANGED:
					{
						// Log_Msg( "SDL_WINDOWEVENT_SIZE_CHANGED\n" );
						Game_UpdateProjection();
						renderOld->Reset();
						break;
					}
					case SDL_WINDOWEVENT_EXPOSED:
					{
						// Log_Msg( "SDL_WINDOWEVENT_EXPOSED\n" );
						// TODO: RESET RENDERER AND DRAW ONTO WINDOW WINDOW !!!
						break;
					}
				}
				break;
			}

			default:
			{
				break;
			}
		}
	}
#endif
}


// TODO: this will not work for multiple camera viewports
void Game_SetView( const glm::mat4& srViewMat )
{
	if ( EditorContext_t* context = Editor_GetContext() )
		context->aView.aViewMat = srViewMat;
}


// TODO: this will not work for multiple camera viewports
void Game_UpdateProjection()
{
	PROF_SCOPE();

	int width = 0, height = 0;
	render->GetSurfaceSize( gGraphicsWindow, width, height );

	auto& io         = ImGui::GetIO();
	io.DisplaySize.x = width;
	io.DisplaySize.y = height;

	// HACK
	extern glm::vec2  gEntityListSize;
	extern glm::ivec2 gAssetBrowserSize;

	int               offsetX   = gEditorData.windowOffset.x + gEntityListSize.x;
	int               offsetY   = gEditorData.windowOffset.y + gMainMenuBarHeight;

	int               newWidth  = width - offsetX;
	int               newHeight = height - offsetY - gAssetBrowserSize.y;

	glm::mat4         projMat   = Util_ComputeProjection( newWidth, newHeight, r_nearz, r_farz, r_fov );

	// update each viewport for each context
#if 0
	for ( u32 i = 0; i < gEditorContexts.aHandles.size(); i++ )
	{
		EditorContext_t* context = nullptr;
		if ( !gEditorContexts.Get( gEditorContexts.aHandles[ i ], &context ) )
			continue;

		ViewportShader_t* viewport   = graphics->GetViewportData( context->aView.aViewportIndex );
		
		context->aView.aResolution.x = width;
		context->aView.aResolution.x = height;
		context->aView.aProjMat      = projMat;
		context->aView.aProjViewMat  = projMat * context->aView.aViewMat;

		if ( !viewport )
			continue;

		viewport->aProjView   = context->aView.aProjViewMat;
		viewport->aProjection = context->aView.aProjMat;
		viewport->aView       = context->aView.aViewMat;

		viewport->aNearZ      = r_nearz;
		viewport->aFarZ       = r_farz;
		viewport->aSize       = { width, height };
	}
#else
	ViewportShader_t* viewport = graphics->GetViewportData( gMainViewportHandle );

	if ( !viewport )
		return;

	// only 1 viewport can be seen right now
	EditorContext_t* context = Editor_GetContext();

	viewport->aNearZ         = r_nearz;
	viewport->aFarZ          = r_farz;
	viewport->aSize          = { newWidth, newHeight };
	viewport->aOffset        = { offsetX, offsetY };
	viewport->aProjection    = projMat;

	if ( context )
	{
		viewport->aView    = context->aView.aViewMat;
		viewport->aViewPos = context->aView.aPos;
	}
	else
	{
		viewport->aView = glm::mat4( 1.f );
	}

	viewport->aProjView = projMat * viewport->aView;

	// Update Context View Data
	if ( context )
	{
		context->aView.aResolution.x = newWidth;
		context->aView.aResolution.y = newHeight;
		context->aView.aOffset.x     = offsetX;
		context->aView.aOffset.y     = offsetY;
		context->aView.aProjMat      = projMat;
		context->aView.aProjViewMat  = viewport->aProjView;
	}
#endif

	graphics->SetViewportUpdate( true );
}


// ----------------------------------------------------------------------------


ch_handle_t Editor_CreateContext( EditorContext_t** spContext )
{
	if ( gEditorContexts.size() >= CH_MAX_EDITOR_CONTEXTS )
	{
		Log_ErrorF( "Max Editor Contexts Hit: Max of %zd\n", CH_MAX_EDITOR_CONTEXTS );
		return CH_INVALID_EDITOR_CONTEXT;
	}

	EditorContext_t* context = nullptr;
	ch_handle_t      handle  = gEditorContexts.Create( &context );

	if ( handle == CH_INVALID_HANDLE )
		return CH_INVALID_HANDLE;

	Editor_SetContext( handle );

	if ( spContext )
		*spContext = context;

	// lazy
	Game_UpdateProjection();

	return gEditorContextIdx;
}


void Editor_FreeContext( ch_handle_t sContext )
{
	// Delete Entities
	EditorContext_t* context = nullptr;
	if ( !gEditorContexts.Get( sContext, &context ) )
	{
		Log_ErrorF( "Tried to free invalid editor context handle: %zd\n", sContext );
		return;
	}

	for ( u32 i = 0; i < context->aMap.aMapEntities.aSize; i++ )
	{
		ch_handle_t entity = context->aMap.aMapEntities.apData[ i ];
		Entity_Delete( entity );
	}

	gEditorContexts.Remove( sContext );
}


EditorContext_t* Editor_GetContext()
{
	if ( gEditorContextIdx == CH_INVALID_EDITOR_CONTEXT )
		return nullptr;

	EditorContext_t* context = nullptr;
	if ( !gEditorContexts.Get( gEditorContextIdx, &context ) )
		return nullptr;

	return context;
}


void Editor_SetContext( ch_handle_t sContext )
{
	EditorContext_t* context = nullptr;
	if ( !gEditorContexts.Get( sContext, &context ) )
	{
		Log_ErrorF( "Tried to set invalid editor context handle: %zd\n", sContext );
		return;
	}

	ch_handle_t      lastContextIdx = gEditorContextIdx;
	EditorContext_t* lastContext    = nullptr;
	gEditorContextIdx               = sContext;

	// Hide Entities of Last Context (TODO: change this if you implement some quick hide or vis group system like hammer has)
	if ( gEditorContexts.Get( lastContextIdx, &lastContext ) )
	{
		Entity_SetEntitiesVisibleNoChild( lastContext->aMap.aMapEntities.apData, lastContext->aMap.aMapEntities.aSize, false );

		// Remove Last Search Path
		FileSys_RemoveSearchPath( lastContext->aMap.aMapPath.data(), lastContext->aMap.aMapPath.size() );
	}

	// Set New Search Path
	FileSys_InsertSearchPath( 0, context->aMap.aMapPath.data(), context->aMap.aMapPath.size() );

	// Show Entities in New Context
	Entity_SetEntitiesVisibleNoChild( context->aMap.aMapEntities.apData, context->aMap.aMapEntities.aSize, true );

	// Set Skybox Material
	Skybox_SetMaterial( context->aMap.aSkybox );

	// TODO: once multiple windows for the 3d view are supported, we will change focus of them here
}


// ------------------------------------------------------------------------
// Map Editor Interface


bool MapEditor::LoadSystems()
{
	return true;
}


bool MapEditor::Init()
{
	return true;
}


void MapEditor::Shutdown()
{
}


bool MapEditor::Launch( const ToolLaunchData& launchData )
{
	running             = true;

	gMainViewportHandle = graphics->CreateViewport();

	Game_UpdateProjection();

	// renderOld->EnableSelection( true, gMainViewportHandle );

	return MapEditor_Init();
}


void MapEditor::Close()
{
	// Save and Close all open Materials
	Log_Msg( "TODO: Save all open maps\n" );

	while ( gEditorContexts.aHandles.size() )
	{
		Editor_FreeContext( gEditorContexts.aHandles[ 0 ] );
	}

	gEditorContexts.clear();

	EntEditor_Shutdown();
	Skybox_Destroy();
	Phys_Shutdown();

	// renderOld->EnableSelection( false, gMainViewportHandle );
	graphics->FreeViewport( gMainViewportHandle );

	running = false;
}


void MapEditor::Update( float frameTime )
{
}


void MapEditor::Render( float frameTime, bool resize, glm::uvec2 offset )
{
	gEditorData.windowOffset = offset;

	if ( resize )
	{
		Game_UpdateProjection();
	}

	if ( gShowQuitConfirmation )
	{
		DrawQuitConfirmation();
	}

	// MapEditor_Update( offset );
	UpdateLoop( frameTime, resize, offset );
}


void MapEditor::Present()
{
	renderOld->Present( gGraphicsWindow, &gMainViewportHandle, 1 );

	gRenderStats = graphics->GetStats();
}


bool MapEditor::OpenAsset( const std::string& path )
{
	return false;
	// return MapEditor_LoadMap( path );
}


// ------------------------------------------------------------------------


Ray Util_GetRayFromScreenSpace( glm::ivec2 mousePos, glm::vec3 origin, u32 viewportIndex )
{
	ViewportShader_t* viewport = graphics->GetViewportData( viewportIndex );

	if ( viewport == nullptr )
	{
		Log_ErrorF( "Invalid Viewport Index for \"%s\" func - %d", CH_FUNC_NAME, viewportIndex );
		return {};
	}

	// Apply viewport offsets to mouse pos
	mousePos.x -= viewport->aOffset.x;
	mousePos.y -= viewport->aOffset.y;

	Ray ray{
		.origin = origin,
		.dir    = Util_GetRayFromScreenSpace( mousePos, viewport->aProjView, viewport->aSize ),
	};

	return ray;
}


// ------------------------------------------------------------------------
// TESTING


void CreatePhysObjectTest( const std::string& path )
{
	EditorContext_t* ctx = Editor_GetContext();

	if ( !ctx )
		return;

	IPhysicsShape* shape = GetPhysEnv()->LoadShape( path.data(), path.size(), PhysShapeType::StaticCompound );

	if ( !shape )
		return;

	PhysicsObjectInfo settings{};
	settings.aMotionType   = PhysMotionType::Kinematic;
	settings.aPos          = ctx->aView.aPos;
	settings.aAng          = ctx->aView.aAng;
	settings.aStartActive  = true;
	settings.aCustomMass   = true;
	settings.aMass         = 10.f;

	IPhysicsObject* object = GetPhysEnv()->CreateObject( shape, settings );

	reallyCoolObject       = object;
	return;
}


CONCMD( create_phys_object )
{
	if ( args.empty() )
		return;

	std::string path = args[ 0 ];
	CreatePhysObjectTest( path );
}


CONCMD( create_phys_object_chair )
{
	CreatePhysObjectTest( "riverhouse/controlroom_chair001a_physics.obj" );
}
