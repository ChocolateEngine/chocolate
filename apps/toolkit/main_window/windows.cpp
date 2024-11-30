#include "main.h"

#include "imgui/imgui_impl_sdl2.h"


void Window_Focus( AppWindow* window )
{
	input->SetWindowFocused( window->window );
}


AppWindow* Window_Create( const char* windowName )
{
	AppWindow* appWindow = new AppWindow;

#ifdef _WIN32
	appWindow->sysWindow = sys_create_window( windowName, 800, 600, false );

	if ( !appWindow->sysWindow )
	{
		Log_Error( "Failed to create tool window\n" );
		return nullptr;
	}

	appWindow->window = SDL_CreateWindowFrom( appWindow->sysWindow );
#else
	int flags         = SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

	appWindow->window = SDL_CreateWindow( windowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	                                      800, 600, flags );
#endif

	if ( !appWindow->window )
	{
		Log_Error( "Failed to create SDL2 Window\n" );
		return nullptr;
	}

	auto origContext   = ImGui::GetCurrentContext();
	appWindow->context = ImGui::CreateContext();
	ImGui::SetCurrentContext( appWindow->context );

	appWindow->graphicsWindow = render->CreateWindow( appWindow->window, appWindow->sysWindow );

	if ( appWindow->graphicsWindow == CH_INVALID_HANDLE )
	{
		Log_Fatal( "Failed to Create GraphicsAPI Window\n" );
		ImGui::SetCurrentContext( origContext );
		Window_OnClose( *appWindow );
		return nullptr;
	}

	gui->StyleImGui();
	input->AddWindow( appWindow->window, appWindow->context );

	// Create the Main Viewport - TODO: use this more across the game code
	//appWindow->viewport = graphics->CreateViewport();

	int width = 0, height = 0;
	render->GetSurfaceSize( appWindow->graphicsWindow, width, height );

	auto& io                   = ImGui::GetIO();
	io.DisplaySize.x           = width;
	io.DisplaySize.y           = height;

	//ViewportShader_t* viewport = graphics->GetViewportData( appWindow->viewport );
	//
	//if ( !viewport )
	//{
	//	ImGui::SetCurrentContext( origContext );
	//	Window_OnClose( *appWindow );
	//	return nullptr;
	//}
	//
	//viewport->aNearZ      = r_nearz;
	//viewport->aFarZ       = r_farz;
	//viewport->aSize       = { width, height };
	//viewport->aOffset     = { 0, 0 };
	//viewport->aProjection = glm::mat4( 1.f );
	//viewport->aView       = glm::mat4( 1.f );
	//viewport->aProjView   = glm::mat4( 1.f );
	//
	//graphics->SetViewportUpdate( true );

	ImGui::SetCurrentContext( origContext );

	Window_Focus( appWindow );

	return appWindow;
}


void Window_OnClose( AppWindow& window )
{
	ImGuiContext* origContext = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext( window.context );

	//graphics->FreeViewport( window.viewport );
	render->DestroyWindow( window.graphicsWindow );
	ImGui_ImplSDL2_Shutdown();

	ImGui::DestroyContext( window.context );
	ImGui::SetCurrentContext( origContext );
}


// TODO: this window will be used for Open and Save dialogs, not attached to any tools
// maybe change this to Window_RenderStart and Window_RenderEnd so we don't have to check if it's a tool every frame?
void Window_Render( LoadedTool& tool, float frameTime, bool sResize )
{
	auto origContext = ImGui::GetCurrentContext();

	AppWindow* window      = tool.window;

	input->SetCurrentWindow( window->window );
	ImGui::SetCurrentContext( window->context );

	if ( sResize )
	{
		renderOld->Reset( window->graphicsWindow );

		int width = 0, height = 0;
		render->GetSurfaceSize( window->graphicsWindow, width, height );

		auto& io                   = ImGui::GetIO();
		io.DisplaySize.x           = width;
		io.DisplaySize.y           = height;

		// ViewportShader_t* viewport = graphics->GetViewportData( window->viewport );
		// 
		// if ( !viewport )
		// 	return;
		// 
		// viewport->aNearZ      = r_nearz;
		// viewport->aFarZ       = r_farz;
		// viewport->aSize       = { width, height };
		// viewport->aOffset     = { 0, 0 };
		// viewport->aProjection = glm::mat4( 1.f );
		// viewport->aView       = glm::mat4( 1.f );
		// viewport->aProjView   = glm::mat4( 1.f );
		// 
		// graphics->SetViewportUpdate( true );
	}

	{
		PROF_SCOPE_NAMED( "Imgui New Frame" );
		ImGui::NewFrame();
		ImGui_ImplSDL2_NewFrame();
	}

	tool.tool->Render( frameTime, sResize, {} );

	ImGui::SetCurrentContext( origContext );
	input->SetCurrentWindow( gpWindow );
}


void Window_Present( LoadedTool& tool )
{
	if ( !tool.running )
		return;

	AppWindow* window      = tool.window;
	auto       origContext = ImGui::GetCurrentContext();

	if ( window )
		ImGui::SetCurrentContext( window->context );

	tool.tool->Present();
	ImGui::SetCurrentContext( origContext );
}


void Window_PresentAll()
{
	for ( LoadedTool& tool : gTools )
	{
		Window_Present( tool );
	}
}

