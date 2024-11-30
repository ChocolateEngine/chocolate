#include "main.h"
#include "cl_main.h"
#include "cl_interface.h"
#include "game_shared.h"
#include "inputsystem.h"
#include "network/net_main.h"
#include "skybox.h"
#include "player.h"
#include "testing.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"


CONVAR_BOOL( r_render, 1, "" );

// blech
u32                 gMainViewportIndex = UINT32_MAX;

SDL_Window*         gpWindow           = nullptr;
ch_handle_t          gGraphicsWindow    = CH_INVALID_HANDLE;

extern EClientState gClientState;


void Game_HandleSystemEvents()
{
	PROF_SCOPE();

	static std::vector< SDL_Event >* events = input->GetEvents();

	for ( auto& e : *events )
	{
		switch ( e.type )
		{
			// TODO: remove this and use pause concmd, can't at the moment as binds are only parsed when game is active, hmm
			case SDL_KEYDOWN:
			{
				SDL_Window* window = SDL_GetWindowFromID( e.key.windowID );

				if ( window != gpWindow )
					continue;

				if ( e.key.keysym.sym == SDLK_BACKQUOTE || e.key.keysym.sym == SDLK_ESCAPE )
					gui->ShowConsole();

				break;
			}

			case SDL_WINDOWEVENT:
			{
				SDL_Window* window = SDL_GetWindowFromID( e.window.windowID );

				if ( window != gpWindow )
					continue;

				switch ( e.window.event )
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
						renderOld->Reset( gGraphicsWindow );
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
}


// TODO: this will not work for multiple camera viewports
void Game_SetView( const glm::mat4& srViewMat )
{
	gView.aViewMat = srViewMat;
}


// TODO: this will not work for multiple camera viewports
void Game_UpdateProjection()
{
	PROF_SCOPE();

	int width = 0, height = 0;
	render->GetSurfaceSize( gGraphicsWindow, width, height );
	gView.ComputeProjection( width, height );

	ViewportShader_t* viewport = graphics->GetViewportData( gMainViewportIndex );

	if ( !viewport )
		return;

	// um
	viewport->aProjView   = gView.aProjViewMat;
	viewport->aProjection = gView.aProjMat;
	viewport->aView       = gView.aViewMat;
	viewport->aNearZ      = gView.aNearZ;
	viewport->aFarZ       = gView.aFarZ;
	viewport->aSize       = { width, height };

	graphics->SetViewportUpdate( true );

	auto& io         = ImGui::GetIO();
	io.DisplaySize.x = width;
	io.DisplaySize.y = height;
}


class ClientSystem : public IClientSystem
{
public:

	bool Init() override
	{
		// Get Interfaces
		CH_GET_SYSTEM( input, IInputSystem, IINPUTSYSTEM_NAME, IINPUTSYSTEM_VER );
		CH_GET_SYSTEM( render, IRender, IRENDER_NAME, IRENDER_VER );
		CH_GET_SYSTEM( audio, IAudioSystem, IADUIO_NAME, IADUIO_VER );
		CH_GET_SYSTEM( ch_physics, Ch_IPhysics, IPHYSICS_NAME, IPHYSICS_VER );
		CH_GET_SYSTEM( graphics, IGraphics, IGRAPHICS_NAME, IGRAPHICS_VER );
		CH_GET_SYSTEM( renderOld, IRenderSystemOld, IRENDERSYSTEMOLD_NAME, IRENDERSYSTEMOLD_VER );
		CH_GET_SYSTEM( gui, IGuiSystem, IGUI_NAME, IGUI_HASH );

		// Now Init
		Input_Init();
		Phys_Init();

		// Init Entity Component Registry
		Ent_RegisterBaseComponents();
		PlayerManager::RegisterComponents();
		TEST_Init();

		if ( !Net_Init() )
		{
			Log_Error( "Failed to init networking\n" );
			return false;
		}

		if ( !CL_Init() )
		{
			Log_Error( "Failed to init client\n" );
			return false;
		}

		// Create the Main Viewport - TODO: use this more across the game code
		gMainViewportIndex = graphics->CreateViewport();

		Game_UpdateProjection();

		return true;
	}

	void Shutdown() override
	{
		CL_Shutdown();

		Skybox_Destroy();
		Entity_Shutdown();

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

		CL_Update( sDT );

		gui->Update( sDT );

		if ( !( SDL_GetWindowFlags( gpWindow ) & SDL_WINDOW_MINIMIZED ) && r_render )
		{
			// update render lists
			// for now, we only have one primary render list, no cameras or anything yet
			static ChVector< ch_handle_t > renderList;
			renderList.clear();
			renderList.resize( graphics->GetRenderableCount() );

			for ( u32 i = 0; i < graphics->GetRenderableCount(); i++ )
			{
				renderList[ i ] = graphics->GetRenderableByIndex( i );
			}

			// TODO: Make this less stupid with allocating memory each frame
			u32* viewportList = ch_malloc< u32 >( graphics->GetViewportCount() );
			viewportList[ 0 ] = gMainViewportIndex;
			u32 viewportCount = 1;

			// Add Shadowmaps to this list
			for ( u32 i = 0; i < graphics->GetLightCount(); i++ )
			{
				Light_t* light = graphics->GetLightByIndex( i );
				if ( !light->apShadowMap )
					continue;

				viewportList[ viewportCount++ ] = light->apShadowMap->aViewportHandle;
			}

			graphics->SetViewportRenderList( gMainViewportIndex, renderList.data(), renderList.size() );

			renderOld->PrePresent();
			renderOld->Present( gGraphicsWindow, viewportList, viewportCount );

			free( viewportList );
		}
		else
		{
			PROF_SCOPE_NAMED( "Imgui End Frame" );
			ImGui::EndFrame();
		}
	}

	// ----------------------------------------------------------------------------

	void PreUpdate( float frameTime ) override
	{
		{
			PROF_SCOPE_NAMED( "Imgui New Frame" );
			ImGui::NewFrame();
			ImGui_ImplSDL2_NewFrame();
		}

		renderOld->NewFrame();

		if ( steam )
			steam->Update( frameTime );

		Game_HandleSystemEvents();

		Input_Update();
	}

	void PostUpdate( float frameTime ) override
	{
	}

	void SetWindowInfo( SDL_Window* window, ch_handle_t graphicsWindow ) override
	{
		gpWindow        = window;
		gGraphicsWindow = graphicsWindow;
	}

	bool Connected() override
	{
		return gClientState == EClientState_Connected;
	}

	void Disconnect() override
	{
		CL_Disconnect();
	}

	void PrintStatus() override
	{
		CL_PrintStatus();
	}

	// Sends this convar over to the server to execute
	void SendConVar( std::string_view sName, const std::vector< std::string >& srArgs ) override
	{
		CL_SendConVar( sName, srArgs );
	}
};


static ClientSystem client;


static ModuleInterface_t gInterfaces[] = {
	{ &client, ICLIENT_NAME, ICLIENT_VER }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& srCount )
	{
		srCount = 1;
		return gInterfaces;
	}
}

