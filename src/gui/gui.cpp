#include "gui.h"
#include "util.h"
#include "render/irender.h"
// #include "rmlui.h"

#include "imgui/imgui_impl_sdl2.h"

#if CH_USE_MIMALLOC
  #include "mimalloc-new-delete.h"
#endif

GuiSystem*               gui           = new GuiSystem;
IRender*                 render        = nullptr;

ImFont*                  gBuiltInFont  = nullptr;

double                   gRealTime     = 0.0;
// Rml::Context*            gRmlContext   = nullptr;

CONVAR_BOOL( ui_show_fps, 1, CVARF_ARCHIVE, "Show a Framerate Counter Window" );
CONVAR_BOOL( ui_show_messages, 1, CVARF_ARCHIVE, "Show Debug Messages in a Window" );

static ModuleInterface_t gInterfaces[] = {
	{ gui, IGUI_NAME, IGUI_HASH }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& srCount )
	{
		srCount = 1;
		return gInterfaces;
	}
}

void GuiSystem::Update( float sDT )
{
	gRealTime += sDT;

	// if ( gRmlContext )
	// 	gRmlContext->Update();

	DrawGui();
}

void GuiSystem::DrawGui()
{
	PROF_SCOPE();

	static bool wasConsoleOpen = false;
	static Uint32 prevtick = 0;
	static Uint32 curtick = 0;

	// ImGui::PushFont( gBuiltInFont );

	if ( aConsoleShown )
		DrawConsole( wasConsoleOpen );

	if ( aConsoleShown && aConVarListShown )
		DrawConVarList( false );

	wasConsoleOpen = aConsoleShown;

	if ( !ui_show_fps && !ui_show_messages )
	{
		prevtick = SDL_GetTicks();
		return;
	}

	ImGuiWindowFlags devFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize;
	if ( !aConsoleShown )
		devFlags |= ImGuiWindowFlags_NoMove;

	//curtick = SDL_GetTicks();
	//Uint32 delta = curtick - prevtick;;

	// NOTE: imgui framerate is a rough estimate, might wanna have our own fps counter?
	ImGui::Begin( "Dev Info", (bool*)0, devFlags );

	if ( ui_show_fps )
	{
		float frameRate = ImGui::GetIO().Framerate;
		ImGui::Text("%.1f FPS (%.3f ms/frame)", frameRate, 1000.0f / frameRate);
		//ImGui::Text("%.1f FPS (%.3f ms/frame)", 1000.f / delta, (float)delta);  // ms/frame is inaccurate
	}

	if ( ui_show_messages )
	{
		std::string debugMessages;
		for (int i = 0; i < aDebugMessages.size(); i++)
		{
			debugMessages += "\n";

			if (aDebugMessages[i] != "")
				debugMessages += aDebugMessages[i];
		}

		aDebugMessages.clear();
		if ( !debugMessages.empty() )
			ImGui::Text( debugMessages.c_str() );
	}

	ImGui::End(  );
	prevtick = SDL_GetTicks();

	// ImGui::PopFont();
}

void GuiSystem::StyleImGui()
{
	gBuiltInFont = BuildFont( "fonts/CascadiaMono.ttf", 15.f );
	gBuiltInFont = BuildFont( "fonts/MonoxRegular.ttf", 15.f );

	if ( gBuiltInFont )
		ImGui::GetIO().FontDefault = gBuiltInFont;

	return;

	if ( Args_Find( "-no-vgui-style" ) )
		return;

	// Classic VGUI2 Style Color Scheme
	ImVec4* colors = ImGui::GetStyle().Colors;

	colors[ImGuiCol_Text]                              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_TextDisabled]              = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_WindowBg]                          = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_ChildBg]                                = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_PopupBg]                                = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_Border]                          = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_BorderShadow]              = ImVec4(0.14f, 0.16f, 0.11f, 0.52f);
	colors[ImGuiCol_FrameBg]                                = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_FrameBgHovered]          = ImVec4(0.27f, 0.30f, 0.23f, 1.00f);
	colors[ImGuiCol_FrameBgActive]            = ImVec4(0.30f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_TitleBg]                                = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_TitleBgActive]            = ImVec4(0.29f, 0.34f, 0.26f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed]          = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	colors[ImGuiCol_MenuBarBg]                        = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_ScrollbarBg]                    = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrab]            = ImVec4(0.28f, 0.32f, 0.24f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.25f, 0.30f, 0.22f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.23f, 0.27f, 0.21f, 1.00f);
	colors[ImGuiCol_CheckMark]                        = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_SliderGrab]                      = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_SliderGrabActive]          = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Button]                          = ImVec4(0.29f, 0.34f, 0.26f, 0.40f);
	colors[ImGuiCol_ButtonHovered]            = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_ButtonActive]              = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Header]                          = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_HeaderHovered]            = ImVec4(0.35f, 0.42f, 0.31f, 0.6f);
	colors[ImGuiCol_HeaderActive]              = ImVec4(0.54f, 0.57f, 0.51f, 0.50f);
	colors[ImGuiCol_Separator]                        = ImVec4(0.14f, 0.16f, 0.11f, 1.00f);
	colors[ImGuiCol_SeparatorHovered]          = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
	colors[ImGuiCol_SeparatorActive]                = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_ResizeGrip]                      = ImVec4(0.19f, 0.23f, 0.18f, 0.00f); // grip invis
	colors[ImGuiCol_ResizeGripHovered]        = ImVec4(0.54f, 0.57f, 0.51f, 1.00f);
	colors[ImGuiCol_ResizeGripActive]          = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_Tab]                                    = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	colors[ImGuiCol_TabHovered]                      = ImVec4(0.54f, 0.57f, 0.51f, 0.78f);
	colors[ImGuiCol_TabActive]                        = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_TabUnfocused]              = ImVec4(0.24f, 0.27f, 0.20f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive]      = ImVec4(0.35f, 0.42f, 0.31f, 1.00f);
	//colors[ImGuiCol_DockingPreview]          = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	//colors[ImGuiCol_DockingEmptyBg]          = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_PlotLines]                        = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]          = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_PlotHistogram]            = ImVec4(1.00f, 0.78f, 0.28f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]          = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_DragDropTarget]          = ImVec4(0.73f, 0.67f, 0.24f, 1.00f);
	colors[ImGuiCol_NavHighlight]              = ImVec4(0.59f, 0.54f, 0.18f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]        = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]          = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;
	style.WindowRounding = 0.0f;
	style.ChildRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.PopupRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabRounding = 0.0f;
	style.TabRounding = 0.0f;
}

ImFont* GuiSystem::BuildFont( const char* spPath, float sSizePixels, const ImFontConfig* spFontConfig )
{
	if ( Args_Find( "-no-imgui-font" ) )
		return nullptr;

	ch_string_auto fontPath = FileSys_FindFile( spPath );
	if ( !fontPath.data )
		return nullptr;

	ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF( fontPath.data, sSizePixels, spFontConfig );

	render->BuildFonts();

	#if 0
	// AWFUL
	VkCommandBuffer c = gpDevice->BeginSingleTimeCommands(  );
	ImGui_ImplVulkan_CreateFontsTexture( c );
	gpDevice->EndSingleTimeCommands( c );
	#endif

	// ImGui_ImplVulkan_DestroyFontUploadObjects(  );

	return font;
}

constexpr int MAX_DEBUG_MESSAGE_SIZE = 512;

// Append a Debug Message
void GuiSystem::DebugMessage( const char* format, ... )
{
	va_list args;
	va_start(args, format);

	char buf[MAX_DEBUG_MESSAGE_SIZE];
	int len = vsnprintf( buf, MAX_DEBUG_MESSAGE_SIZE, format, args );
	aDebugMessages.push_back(buf);

	va_end(args);
}

// Place a Debug Message in a specific spot
void GuiSystem::DebugMessage( size_t index, const char* format, ... )
{
	aDebugMessages.resize( glm::max(aDebugMessages.size(), index + 1) );

	va_list args;
	va_start( args, format );

	char buf[MAX_DEBUG_MESSAGE_SIZE];
	int len = vsnprintf( buf, MAX_DEBUG_MESSAGE_SIZE, format, args );
	aDebugMessages[index] = buf;

	va_end( args );
}

// Insert a debug message in-between one
void GuiSystem::InsertDebugMessage( size_t index, const char* format, ... )
{
	va_list args;
	va_start( args, format );

	char buf[MAX_DEBUG_MESSAGE_SIZE];
	int len = vsnprintf( buf, MAX_DEBUG_MESSAGE_SIZE, format, args );

	aDebugMessages.insert( aDebugMessages.begin() + index, buf );
	va_end( args );
}


/*
*    Starts a new ImGui frame.
*/
void GuiSystem::StartFrame()
{
	ImGui::NewFrame();
}


bool GuiSystem::Init()
{
	render = Mod_GetSystemCast< IRender >( IRENDER_NAME, IRENDER_VER );

	InitConsole();
	InitConVarList();

	// Set RmlUI Interfaces
	// Rml::SetSystemInterface( &gRmlUI );

	// Rml::Initialise();

	// SDL_GetWindowSize();
#pragma message( "DETECT RMLUI CONTEXT WINDOW SIZE" )
	// gRmlContext = Rml::CreateContext( "default", Rml::Vector2i( 1280, 720 ) );

	return true;
}


void GuiSystem::Shutdown()
{
	// Rml::Shutdown();
}


GuiSystem::GuiSystem(  ) : IGuiSystem(  )
{
	gui = this;
}


GuiSystem::~GuiSystem(  )
{

}

