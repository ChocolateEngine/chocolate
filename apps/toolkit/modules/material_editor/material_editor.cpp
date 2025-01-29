#include "material_editor.h"

#include "../main_window/main.h"
#include "core/asserts.h"
#include "core/system_loader.h"
#include "core/util.h"
#include "igraphics.h"
#include "igui.h"
#include "iinput.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "render/irender.h"


bool           gShowQuitConfirmation = false;

// TODO: make gRealTime and gGameTime
// real time is unmodified time since engine launched, and game time is time affected by host_timescale and pausing

u32            gMainViewport         = UINT32_MAX;

MaterialEditor gMatEditorTool;


static void    DrawQuitConfirmation()
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


bool MaterialEditor::LoadSystems()
{
	// Get Modules
	CH_GET_SYSTEM( input, IInputSystem, IINPUTSYSTEM_NAME, IINPUTSYSTEM_VER );
	CH_GET_SYSTEM( render, IRender, IRENDER_NAME, IRENDER_VER );
	CH_GET_SYSTEM( graphics, IGraphics, IGRAPHICS_NAME, IGRAPHICS_VER );
	CH_GET_SYSTEM( renderOld, IRenderSystemOld, IRENDERSYSTEMOLD_NAME, IRENDERSYSTEMOLD_VER );

	return true;
}


bool MaterialEditor::Init()
{
	return true;
}


void MaterialEditor::Shutdown()
{
}


bool MaterialEditor::Launch( const ToolLaunchData& launchData )
{
	/*gToolData     = launchData;
	gMainViewport = graphics->CreateViewport();

	MaterialEditor_Init();*/
	return true;
}


void MaterialEditor::Close()
{
	// Save and Close all open Materials
	/*MaterialEditor_Close();

	graphics->FreeViewport( gMainViewport );

	gToolData.graphicsWindow = 0;
	gToolData.toolkit        = nullptr;
	gToolData.window         = nullptr;*/
}


void MaterialEditor::Update( float frameTime )
{
}


void MaterialEditor::Render( float frameTime, bool resize, glm::uvec2 sOffset )
{
	if ( resize )
	{
		int width = 0, height = 0;
		render->GetSurfaceSize( gGraphicsWindow, width, height );

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
	//	renderOld->Present( gToolData.graphicsWindow, &gMainViewport, 1 );
}


bool MaterialEditor::OpenAsset( const std::string& path )
{
	return MaterialEditor_LoadMaterial( path );
}
