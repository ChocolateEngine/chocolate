#include "material_editor.h"
#include "core/system_loader.h"
#include "core/asserts.h"

#include "iinput.h"
#include "igui.h"
#include "render/irender.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"

#include "core/util.h"
#include "igraphics.h"


IRender*                        render                = nullptr;
IInputSystem*                   input                 = nullptr;
IGraphics*                      graphics              = nullptr;
IRenderSystemOld*               renderOld             = nullptr;

bool                            gShowQuitConfirmation = false;

// TODO: make gRealTime and gGameTime
// real time is unmodified time since engine launched, and game time is time affected by host_timescale and pausing
double                          gCurTime              = 0.0;  // i could make this a size_t, and then just have it be every 1000 is 1 second
float                           gFrameTime            = 0.f;

u32                             gMainViewport         = UINT32_MAX;

ToolLaunchData                  gToolData;
MaterialEditor                  gMatEditorTool;


static ModuleInterface_t        gInterfaces[] = {
    { &gMatEditorTool, CH_TOOL_MAT_EDITOR_NAME, CH_TOOL_MAT_EDITOR_VER }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& srCount )
	{
		srCount = 1;
		return gInterfaces;
	}
}


static void DrawQuitConfirmation()
{
	int width = 0, height = 0;
	render->GetSurfaceSize( gToolData.graphicsWindow, width, height );

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
			Log_ErrorF( "TODO: Call IToolkit::CloseTool() when it's implemented\n" );
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


void Main_DrawMenuBar()
{
}


bool MaterialEditor::Init()
{
	// Get Modules
	CH_GET_SYSTEM( input, IInputSystem, IINPUTSYSTEM_NAME, IINPUTSYSTEM_VER );
	CH_GET_SYSTEM( render, IRender, IRENDER_NAME, IRENDER_VER );
	CH_GET_SYSTEM( graphics, IGraphics, IGRAPHICS_NAME, IGRAPHICS_VER );
	CH_GET_SYSTEM( renderOld, IRenderSystemOld, IRENDERSYSTEMOLD_NAME, IRENDERSYSTEMOLD_VER );

	return true;
}


void MaterialEditor::Shutdown()
{
}


bool MaterialEditor::Launch( const ToolLaunchData& launchData )
{
	gToolData     = launchData;
	gMainViewport = graphics->CreateViewport();

	MaterialEditor_Init();
	return true;
}


void MaterialEditor::Close()
{
	// Save and Close all open Materials
	MaterialEditor_Close();

	graphics->FreeViewport( gMainViewport );

	gToolData.graphicsWindow = 0;
	gToolData.toolkit        = nullptr;
	gToolData.window         = nullptr;
}


void MaterialEditor::Update( float frameTime )
{
}


void MaterialEditor::Render( float frameTime, bool resize, glm::uvec2 sOffset )
{
	if ( resize )
	{
		int width = 0, height = 0;
		render->GetSurfaceSize( gToolData.graphicsWindow, width, height );

		ViewportShader_t* viewport = graphics->GetViewportData( gMainViewport );
		
		if ( !viewport )
			return;
		
		viewport->aNearZ      = 0.01;
		viewport->aFarZ       = 1000;
		viewport->aSize       = { width, height };
		viewport->aOffset     = { 0, 0 };
		viewport->aProjection = glm::mat4( 1.f );
		viewport->aView       = glm::mat4( 1.f );
		viewport->aProjView   = glm::mat4( 1.f );
		
		graphics->SetViewportUpdate( true );
	}

	if ( gShowQuitConfirmation )
	{
		DrawQuitConfirmation();
	}

	MaterialEditor_Draw( sOffset );
}


void MaterialEditor::Present()
{
	renderOld->Present( gToolData.graphicsWindow, &gMainViewport, 1 );
}


bool MaterialEditor::OpenAsset( const std::string& path )
{
	return MaterialEditor_LoadMaterial( path );
}

