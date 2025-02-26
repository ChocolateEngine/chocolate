#include "render_test.h"
#include "shared/map_system.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_sdl2.h"


CONVAR_FLOAT_NAME( g_move_speed, "input.move.speed", 15, "Move Speed" );

CONVAR_FLOAT_NAME( g_mouse_pitch, "input.mouse.sensitivity.pitch", 0.022, CVARF_ARCHIVE, "Mouse Pitch" );
CONVAR_FLOAT_NAME( g_mouse_yaw, "input.mouse.sensitivity.yaw", 0.022, CVARF_ARCHIVE, "Mouse Yaw" );
CONVAR_FLOAT_NAME( g_mouse_sensitivity, "input.mouse.sensitivity", 1.0, CVARF_ARCHIVE, "Mouse Sensitivity" );

CONVAR_FLOAT_NAME( g_view_nearz, "view.nearz", 0.05f, CVARF_ARCHIVE, "Near Z" );
CONVAR_FLOAT_NAME( g_view_farz, "view.farz", 1000.f, CVARF_ARCHIVE, "Far Z" );
CONVAR_FLOAT_NAME( g_view_fov, "view.fov", 90.f, CVARF_ARCHIVE, "Field of View" );

glm::mat4                   g_view{};
glm::mat4                   g_projection{};

glm::vec3                   g_pos{};
glm::vec3                   g_ang{};

//constexpr const char*       TEST_SCENE_PATH  = "../sidury/maps/riverhouse_v1";
constexpr const char*       TEST_SCENE_PATH  = "D:\\projects\\chocolate\\dev\\output\\projects\\l4d_maps\\c2m1_highway";

constexpr const char*       TEST_MODEL_PATH  = "../sidury/models/riverhouse/riverhouse_v1.obj";
//constexpr const char*       TEST_MODEL2_PATH = "../sidury/models/protogen_wip_25d/protogen_wip_25d.obj";
//constexpr const char*       TEST_MODEL_PATH  = "D:\\projects\\chocolate\\dev\\output\\projects\\l4d_maps\\c2m1_highway\\models\\props_urban\\fence002_128.obj";
// constexpr const char* TEST_MODEL_PATH = "models/test/test_cube.obj";

static ch_model_h           g_test_model{};
static r_mesh_h             g_test_model_gpu{}; 
static r_mesh_render_h      g_test_model_render{};

static ch_model_h           g_test_model2{};
static r_mesh_h             g_test_model2_gpu{};
static r_mesh_render_h      g_test_model2_render{};


chmap::Map*                 g_test_map = nullptr;


struct mesh_render_entry_t
{
	r_mesh_h        model;
	r_mesh_render_h render;
};

std::unordered_map< ch_string, r_mesh_h > g_gpu_mesh_map;
ChVector< mesh_render_entry_t >           g_gpu_mesh_renders;


bool load_map()
{
	g_test_map = chmap::Load( TEST_SCENE_PATH, strlen( TEST_SCENE_PATH ) );

	if ( !g_test_map )
	{
		Log_ErrorF( "Failed to find map \"%s\"\n", TEST_SCENE_PATH );
		return false;
	}

	FileSys_AddSearchPath( TEST_SCENE_PATH );

	for ( chmap::Entity& entity : g_test_map->scenes[ 0 ].entites )
	{
		// check for a renderable component
		for ( chmap::Component& comp : entity.components )
		{
			if ( !ch_str_equals( comp.name, "renderable" ) )
				continue;

			r_mesh_h  gpu_mesh{};
			ch_string path = comp.values[ "path" ].aString;

			auto      it   = g_gpu_mesh_map.find( path );
			if ( it == g_gpu_mesh_map.end() )
			{
				ch_model_h model = graphics_data->model_load( path.data );

				if ( !model )
				{
					Log_ErrorF( "Failed to find map model \"%s\"\n", path.data );
					continue;
				}

				gpu_mesh = render->mesh_upload( model );
				graphics_data->model_free( model );

				if ( !gpu_mesh )
				{
					Log_ErrorF( "Failed to upload map model \"%s\"\n", path.data );
					continue;
				}

				g_gpu_mesh_map[ path ] = gpu_mesh;
			}
			else
			{
				gpu_mesh = it->second;
			}

			r_mesh_render_h mesh_render = render->mesh_render_create( gpu_mesh );

			if ( !mesh_render )
			{
				Log_ErrorF( "Failed to create map model render \"%s\"\n", path.data );
				continue;
			}

			mesh_render_entry_t& entry = g_gpu_mesh_renders.emplace_back();
			entry.model                = gpu_mesh;
			entry.render               = mesh_render;

			glm::mat4 matrix           = Util_ToMatrix( &entity.pos, &entity.ang, &entity.scale );

			render->mesh_render_set_matrix( mesh_render, matrix );

			break;
		}
	}

	return true;
}


bool load_scene()
{
	return load_map();

	// hmm, maybe this could be condensed into one call, mesh_render_create(), pass in a model path instead of a handle, and do the model load and upload internally if needed?
	g_test_model = graphics_data->model_load( TEST_MODEL_PATH );

	if ( !g_test_model )
		return false;

	g_test_model_gpu = render->mesh_upload( g_test_model );

	if ( !g_test_model_gpu )
		return false;

	g_test_model_render = render->mesh_render_create( g_test_model_gpu );

	if ( !g_test_model_render )
		return false;

	// now we can free the test model since it's use is over
	graphics_data->model_free( g_test_model );



	// hmm, maybe this could be condensed into one call, mesh_render_create(), pass in a model path instead of a handle, and do the model load and upload internally if needed?
//	g_test_model2 = graphics_data->model_load( TEST_MODEL2_PATH );
//
//	if ( !g_test_model2 )
//		return false;
//
//	g_test_model2_gpu = render->mesh_upload( g_test_model2 );
//
//	if ( !g_test_model2_gpu )
//		return false;
//
//	g_test_model2_render = render->mesh_render_create( g_test_model2_gpu );
//
//	if ( !g_test_model2_render )
//		return false;
//
//	// now we can free the test model since it's use is over
//	graphics_data->model_free( g_test_model2 );


	return true;
}


// =================================================================================================


// Proper Horizontal Projection
inline glm::mat4 Util_ComputeProjection( float sWidth, float sHeight, float sNearZ, float sFarZ, float sFov )
{
	float aspect = (float)sWidth / (float)sHeight;
	return glm::perspective( glm::radians( sFov ), aspect, sNearZ, sFarZ );
}


void update_viewport()
{
	glm::vec2 size   = render->window_surface_size( g_graphics_window );

	auto&     io     = ImGui::GetIO();
	io.DisplaySize.x = size.x;
	io.DisplaySize.y = size.y;

	#if 0
	glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)_drawExtent.width / (float)_drawExtent.height, 10000.f, 0.1f);

	// invert the Y direction on projection matrix so that we are more similar
	// to opengl and gltf axis
	projection[1][1] *= -1;
	#endif

	g_projection     = Util_ComputeProjection( size.x, size.y, g_view_nearz, g_view_farz, g_view_fov );
}


void render_on_reset( ch_handle_t window, e_render_reset_flags flags )
{
	update_viewport();
}


// Check the function FindHoveredWindow() in imgui.cpp to see if you need to update this when updating imgui
bool mouse_hovering_imgui_window( glm::ivec2 mousePos )
{
	ImGuiContext& g = *ImGui::GetCurrentContext();

	ImVec2        imMousePos{ (float)mousePos.x, (float)mousePos.y };

	ImGuiWindow*  hovered_window                        = NULL;
	ImGuiWindow*  hovered_window_ignoring_moving_window = NULL;
	if ( g.MovingWindow && !( g.MovingWindow->Flags & ImGuiWindowFlags_NoMouseInputs ) )
		hovered_window = g.MovingWindow;

	ImVec2 padding_regular    = g.Style.TouchExtraPadding;
	ImVec2 padding_for_resize = g.IO.ConfigWindowsResizeFromEdges ? g.WindowsHoverPadding : padding_regular;
	for ( int i = g.Windows.Size - 1; i >= 0; i-- )
	{
		ImGuiWindow* window = g.Windows[ i ];
		IM_MSVC_WARNING_SUPPRESS( 28182 );  // [Static Analyzer] Dereferencing NULL pointer.
		if ( !window->WasActive || window->Hidden )
			continue;
		if ( window->Flags & ImGuiWindowFlags_NoMouseInputs )
			continue;
		IM_ASSERT( window->Viewport );
		if ( window->Viewport != g.MouseViewport )
			continue;

		// Using the clipped AABB, a child window will typically be clipped by its parent (not always)
		ImVec2 hit_padding = ( window->Flags & ( ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize ) ) ? padding_regular : padding_for_resize;
		if ( !window->OuterRectClipped.ContainsWithPad( imMousePos, hit_padding ) )
			continue;

		// Support for one rectangular hole in any given window
		// FIXME: Consider generalizing hit-testing override (with more generic data, callback, etc.) (#1512)
		if ( window->HitTestHoleSize.x != 0 )
		{
			ImVec2 hole_pos( window->Pos.x + (float)window->HitTestHoleOffset.x, window->Pos.y + (float)window->HitTestHoleOffset.y );
			ImVec2 hole_size( (float)window->HitTestHoleSize.x, (float)window->HitTestHoleSize.y );
			ImVec2 hole_pos_size{};

			hole_pos_size.x = hole_pos.x + hole_size.x;
			hole_pos_size.y = hole_pos.y + hole_size.y;

			if ( ImRect( hole_pos, hole_pos_size ).Contains( imMousePos ) )
				continue;
		}

		if ( hovered_window == NULL )
			hovered_window = window;
		IM_MSVC_WARNING_SUPPRESS( 28182 );  // [Static Analyzer] Dereferencing NULL pointer.
		if ( hovered_window_ignoring_moving_window == NULL && ( !g.MovingWindow || window->RootWindowDockTree != g.MovingWindow->RootWindowDockTree ) )
			hovered_window_ignoring_moving_window = window;
		if ( hovered_window && hovered_window_ignoring_moving_window )
			break;
	}

	if ( hovered_window )
	{
		//if ( editor_spew_imgui_window_hover )
		//	Log_DevF( 1, "HOVERING WINDOW \"%s\"\n", hovered_window->Name );

		return true;
	}

	return false;
}


bool is_mouse_in_view()
{
	if ( !input->WindowHasFocus( g_window ) )
		return false;

	glm::vec2  size      = render->window_surface_size( g_graphics_window );
	glm::ivec2 mouse_pos = input->GetMousePos();

	//	if ( viewport->aOffset.x > mouse_pos.x )
	//		return false;
	//
	//	if ( viewport->aOffset.y > mouse_pos.y )
	//		return false;
	//
	//	if ( viewport->aOffset.x + viewport->aSize.x < mouse_pos.x )
	//		return false;
	//
	//	if ( viewport->aOffset.y + viewport->aSize.y < mouse_pos.y )
	//		return false;

	if ( size.x < mouse_pos.x )
		return false;

	if ( size.y < mouse_pos.y )
		return false;

	if ( mouse_hovering_imgui_window( mouse_pos ) )
		return false;

	return true;
}


static void center_mouse()
{
	glm::ivec2 size = render->window_surface_size( g_graphics_window );

	int        x    = ( size.x / 2.f );
	int        y    = ( size.y / 2.f );

	SDL_WarpMouseInWindow( g_window, x, y );
}


bool mouse_capture_pressed()
{
	return input->KeyPressed( (EButton)SDL_SCANCODE_SPACE ) || input->KeyPressed( EButton_MouseRight );
}


inline float DegreeConstrain( float num )
{
	num = std::fmod( num, 360.0f );
	return ( num < 0.0f ) ? num += 360.0f : num;
}


void handle_mouse()
{
	static bool captured_mouse = false;
	bool        mouse_in_view  = false;

	if ( !captured_mouse )
		mouse_in_view = is_mouse_in_view();

	if ( mouse_in_view && mouse_capture_pressed() )
	{
		SDL_SetRelativeMouseMode( SDL_TRUE );
		captured_mouse = true;
	}

	bool should_center_mouse = false;

	if ( captured_mouse )
	{
		if ( mouse_capture_pressed() )
		{
			// not sure why i need to do this after updating imgui
			SDL_ShowCursor( SDL_FALSE );

			// ch_handle_t Mouse Input
			const glm::vec2 mouse = input->GetMouseDelta();

			// transform.aAng[PITCH] = -mouse.y;
			g_ang[ PITCH ] += mouse.y * g_mouse_pitch * g_mouse_sensitivity;
			g_ang[ YAW ] += mouse.x * g_mouse_yaw * g_mouse_sensitivity;

			g_ang[ YAW ]        = DegreeConstrain( g_ang[ YAW ] );
			g_ang[ PITCH ]      = std::clamp( g_ang[ PITCH ], -90.0f, 90.0f );

			should_center_mouse = true;
		}
		else
		{
			SDL_SetRelativeMouseMode( SDL_FALSE );
			SDL_ShowCursor( SDL_TRUE );

			if ( captured_mouse )
				should_center_mouse = true;

			captured_mouse = false;
		}
	}

	if ( should_center_mouse )
		center_mouse();
}


void handle_inputs( float frame_time )
{
	// Handle Mouse Updates
	handle_mouse();

	glm::vec3 move{};
	float     move_speed = g_move_speed * frame_time;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_W ) )
		move.x = move_speed;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_S ) )
		move.x += -move_speed;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_A ) )
		move.y = -move_speed;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_D ) )
		move.y += move_speed;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_Q ) )
		move.z = -move_speed;

	if ( input->KeyPressed( (EButton)SDL_SCANCODE_E ) )
		move.z += move_speed;

	// get forward and right vectors
	glm::vec3 forward, right, up;
	//Util_GetDirectionVectors( g_ang, &forward, &right, &up );
	//
	//for ( int i = 0; i < 3; i++ )
	//	g_pos[ i ] += right[ i ] * move.x + forward[ i ] * move.y + up[ i ] * move.z;

	Util_ToViewMatrixZ( g_view, g_pos, g_ang );
	Util_GetViewMatrixZDirection( g_view, forward, right, up );

	for ( int i = 0; i < 3; i++ )
		g_pos[ i ] += forward[ i ] * move.x + right[ i ] * move.y + up[ i ] * move.z;

	// stupid, because Util_GetDirectionVectors is broken or something
	Util_ToViewMatrixZ( g_view, g_pos, g_ang );

	// Update Projection
	update_viewport();

	gui->DebugMessage( "Pos: %s", ch_vec3_to_str( g_pos ).c_str() );
	gui->DebugMessage( "Ang: %s", ch_vec3_to_str( g_ang ).c_str() );
}


// =================================================================================================
// protogen render test



