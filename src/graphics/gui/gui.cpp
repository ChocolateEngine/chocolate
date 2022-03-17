#include "gui.h"
#include "../renderer.h"
#include "util.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_sdl.h"

GuiSystem* gui = new GuiSystem;

ImFont* gBuiltInFont = nullptr;

extern Renderer* renderer;

void GuiSystem::Update( float sDT )
{
	DrawGui(  );
}

void GuiSystem::DrawGui(  )
{
	if ( !apWindow )
		return;

	static bool wasConsoleOpen = false;
	static Uint32 prevtick = 0;
	static Uint32 curtick = 0;

	if ( !aDrawnFrame )
		renderer->EnableImgui(  );

	ImGui::PushFont( gBuiltInFont );

	if ( aConsoleShown )
		DrawConsole( wasConsoleOpen );

	wasConsoleOpen = aConsoleShown;

	ImGuiWindowFlags devFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize;
	if ( !aConsoleShown )
		devFlags |= ImGuiWindowFlags_NoMove;

	//curtick = SDL_GetTicks();
	//Uint32 delta = curtick - prevtick;;

	// NOTE: imgui framerate is a rough estimate, might wanna have our own fps counter?
	ImGui::Begin( "Dev Info", (bool*)0, devFlags );
	float frameRate = ImGui::GetIO().Framerate;
	ImGui::Text("%.1f FPS (%.3f ms/frame)", frameRate, 1000.0f / frameRate);
	//ImGui::Text("%.1f FPS (%.3f ms/frame)", 1000.f / delta, (float)delta);  // ms/frame is inaccurate

	std::string debugMessages;
	for (int i = 0; i < aDebugMessages.size(); i++)
	{
		debugMessages += "\n";

		if (aDebugMessages[i] != "")
			debugMessages += aDebugMessages[i];
	}

	aDebugMessages.clear();
	if ( !debugMessages.empty() )
		ImGui::Text(debugMessages.c_str());

	ImGui::End(  );
	prevtick = SDL_GetTicks();

	aDrawnFrame = true;

	ImGui::PopFont();
}

void GuiSystem::StyleImGui()
{
	auto& io = ImGui::GetIO();

	// gBuiltInFont = BuildFont( "fonts/CascadiaMono.ttf" );
	gBuiltInFont = BuildFont( "fonts/MonoxRegular.ttf", 15.f );


	if ( cmdline->Find( "-no-vgui-style" ) )
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
	if ( cmdline->Find( "-no-imgui-font" ) )
		return nullptr;

	auto fontPath = filesys->FindFile( spPath );
	if ( fontPath == "" )
		return nullptr;

	ImFont* font = ImGui::GetIO().Fonts->AddFontFromFileTTF( fontPath.c_str(), sSizePixels, spFontConfig );

	// AWFUL
	VkCommandBuffer c = gpDevice->BeginSingleTimeCommands(  );
	ImGui_ImplVulkan_CreateFontsTexture( c );
	gpDevice->EndSingleTimeCommands( c );

	ImGui_ImplVulkan_DestroyFontUploadObjects(  );

	return font;
}

void GuiSystem::AssignWindow( SDL_Window* spWindow )
{
	apWindow = spWindow;
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
	if ( !apWindow )
		return;
		
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplSDL2_NewFrame( apWindow );
	ImGui::NewFrame();
}

void GuiSystem::Init()
{
	InitConsole();
}

GuiSystem::GuiSystem(  ) : BaseGuiSystem(  )
{
	systems->Add( this );
	gui = this;
}

GuiSystem::~GuiSystem(  )
{

}

