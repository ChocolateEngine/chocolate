#include "editor_view.h"
#include "main.h"
#include "game_shared.h"
#include "skybox.h"
#include "inputsystem.h"
#include "entity_editor.h"
#include "gizmos.h"

#include "igui.h"
#include "iinput.h"
#include "render/irender.h"
#include "igraphics.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"


CONVAR_FLOAT( r_fov, 106.f, CVARF_ARCHIVE, "FOV" );
CONVAR_FLOAT( r_nearz, 0.01f, "Near Z Plane" );
CONVAR_FLOAT( r_farz, 1000.f, "Far Z Plane" );

CONVAR_FLOAT_EXT( m_yaw );
CONVAR_FLOAT_EXT( m_pitch );

CONVAR_FLOAT( view_move_slow, 0.2f );
CONVAR_FLOAT( view_move_fast, 3.0f );

CONVAR_FLOAT( view_move_speed, 30.0f );

CONVAR_FLOAT( view_move_min, 0.0625f );
CONVAR_FLOAT( view_move_max, 15.f );
CONVAR_FLOAT( view_move_scroll_sens, 0.0625f );

CONVAR_BOOL( editor_show_pos, 1.f, "Show Position on-screen" );
CONVAR_BOOL( editor_spew_imgui_window_hover, 0 );


static bool gClearSelection = false;


// Check the function FindHoveredWindow() in imgui.cpp to see if you need to update this when updating imgui
bool MouseHoveringImGuiWindow( glm::ivec2 mousePos )
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
		if ( editor_spew_imgui_window_hover )
			Log_DevF( 1, "HOVERING WINDOW \"%s\"\n", hovered_window->Name );

		return true;
	}

	return false;
}


void EditorView_Init()
{
}


void EditorView_Shutdown()
{
}


static float gMoveScale = 1.f;
static bool gCheckSelectionResult = false;


bool EditorView_IsMouseInView()
{
	if ( !input->WindowHasFocus( gToolData.window ) )
		return false;

	ViewportShader_t* viewport = graphics->GetViewportData( gMainViewport );

	glm::ivec2 mousePos = input->GetMousePos();

	if ( viewport->aOffset.x > mousePos.x )
		return false;

	if ( viewport->aOffset.y > mousePos.y )
		return false;

	if ( viewport->aOffset.x + viewport->aSize.x < mousePos.x )
		return false;

	if ( viewport->aOffset.y + viewport->aSize.y < mousePos.y )
		return false;

	if ( MouseHoveringImGuiWindow( mousePos ) )
		return false;

	return true;
}


static void UpdateSelectionRenderables()
{
	glm::ivec2                      mousePos = input->GetMousePos();
	EditorContext_t*                context  = Editor_GetContext();

	ChVector< SelectionRenderable > selectList;
	for ( ch_handle_t entHandle : context->aMap.aMapEntities )
	{
		Entity_t* ent = Entity_GetData( entHandle );

		if ( !ent )
		{
			continue;
		}

		if ( !ent->aRenderable )
		{
			continue;
		}

		SelectionRenderable selectRenderable;
		selectRenderable.renderable = ent->aRenderable;

		Renderable_t* renderable    = graphics->GetRenderableData( ent->aRenderable );

		selectRenderable.colors.resize( renderable->aMaterialCount );

		if ( ent->aMaterialColors.size() && ent->aMaterialColors.size() == renderable->aMaterialCount )
		{
			for ( u32 i = 0; i < selectRenderable.colors.size(); i++ )
			{
				Color3& color3 = ent->aMaterialColors[ i ];
				u32&    color  = selectRenderable.colors[ i ];
				color          = ( color3.r << 16 | color3.g << 8 | color3.b );
			}
		}
		else
		{
			for ( u32 i = 0; i < selectRenderable.colors.size(); i++ )
			{
				u32& color = selectRenderable.colors[ i ];
				color      = ( ent->aSelectColor[ 0 ] << 16 | ent->aSelectColor[ 1 ] << 8 | ent->aSelectColor[ 2 ] );
			}
		}

		selectList.push_back( selectRenderable );
	}

	renderOld->SetSelectionRenderablesAndCursorPos( selectList, mousePos );

	gCheckSelectionResult = true;
}


void EditorView_UpdateInputs( bool mouseInView )
{
	PROF_SCOPE();

	EditorContext_t* context = Editor_GetContext();

	gEditorData.aMove        = { 0.f, 0.f, 0.f };

	Gizmo_UpdateInputs( context, mouseInView );

	if ( !mouseInView )
		return;

	glm::ivec2 mouseScroll = input->GetMouseScroll();

	if ( mouseScroll.y != 0 )
	{
		gMoveScale += mouseScroll.y * view_move_scroll_sens;
		gMoveScale = std::clamp( gMoveScale, view_move_min, view_move_max );
		Log_DevF( 2, "Movement Speed Scale: %.4f\n", gMoveScale );
	}

	gui->DebugMessage( "Movement Speed Scale: %.4f", gMoveScale );

	float moveScale = gFrameTime * gMoveScale;
	
	if ( Input_KeyPressed( EBinding_Viewport_Slow ) )
	{
		moveScale *= view_move_slow;
	}

	if ( Input_KeyPressed( EBinding_Viewport_Sprint ) )
	{
		moveScale *= view_move_fast;
	}

	const float forwardSpeed = view_move_speed * moveScale;
	const float sideSpeed    = view_move_speed * moveScale;
	const float upSpeed      = view_move_speed * moveScale;
	// apMove->aMaxSpeed        = max_speed * moveScale;

	if ( Input_KeyPressed( EBinding_Viewport_MoveForward ) )
		gEditorData.aMove[ W_FORWARD ] = forwardSpeed;

	if ( Input_KeyPressed( EBinding_Viewport_MoveBack ) )
		gEditorData.aMove[ W_FORWARD ] += -forwardSpeed;

	if ( Input_KeyPressed( EBinding_Viewport_MoveLeft ) )
		gEditorData.aMove[ W_RIGHT ] = -sideSpeed;

	if ( Input_KeyPressed( EBinding_Viewport_MoveRight ) )
		gEditorData.aMove[ W_RIGHT ] += sideSpeed;

	if ( Input_KeyPressed( EBinding_Viewport_MoveUp ) )
		gEditorData.aMove[ W_UP ] = upSpeed;

	if ( Input_KeyPressed( EBinding_Viewport_MoveDown ) )
		gEditorData.aMove[ W_UP ] -= upSpeed;

	// make sure to only allow selecting other entities if no axis is selected
	if ( gEditorData.gizmo.selectedAxis == EGizmoAxis_None )
	{
		if ( Input_KeyJustPressed( EBinding_Viewport_SelectMulti ) )
		{
			UpdateSelectionRenderables();
		}
		else if ( Input_KeyJustPressed( EBinding_Viewport_SelectSingle ) )
		{
			gClearSelection = true;
			UpdateSelectionRenderables();
		}
	}
}


void EditorView_UpdateView( EditorContext_t* spContext )
{
	PROF_SCOPE();

	// move viewport
	for ( int i = 0; i < 3; i++ )
	{
		// spContext->aView.aPos[ i ] += spContext->aView.aForward[ i ] * gEditorData.aMove.x + spContext->aView.aRight[ i ] * gEditorData.aMove[ W_RIGHT ];
		spContext->aView.aPos[ i ] += spContext->aView.aForward[ i ] * gEditorData.aMove.x + spContext->aView.aRight[ i ] * gEditorData.aMove[ W_RIGHT ] + spContext->aView.aUp[ i ] * gEditorData.aMove.z;
	}

	glm::mat4 viewMatrixZ;

	{
		Util_ToViewMatrixZ( viewMatrixZ, spContext->aView.aPos, spContext->aView.aAng );
		Util_GetViewMatrixZDirection( viewMatrixZ, spContext->aView.aForward, spContext->aView.aRight, spContext->aView.aUp );
	}

	// Transform transformViewTmp = GetEntitySystem()->GetWorldTransform( info->aCamera );
	// Transform transformView    = transformViewTmp;

	//transformView.aAng[ PITCH ] = transformViewTmp.aAng[ YAW ];
	//transformView.aAng[ YAW ]   = transformViewTmp.aAng[ PITCH ];

	//transformView.aAng.Edit()[ YAW ] *= -1;

	//Transform transformView = transform;
	//transformView.aAng += move.aViewAngOffset;

	// if ( cl_thirdperson.GetBool() )
	// {
	// 	Transform thirdPerson = {
	// 		.aPos = { cl_cam_x, cl_cam_y, cl_cam_z }
	// 	};
	// 
	// 	// thirdPerson.aPos = {cl_cam_x, cl_cam_y, cl_cam_z};
	// 
	// 	glm::mat4 viewMatrixZ;
	// 	Util_ToViewMatrixZ( viewMatrixZ, transformView.aPos, transformView.aAng );
	// 
	// 	glm::mat4 viewMat = thirdPerson.ToMatrix( false ) * viewMatrixZ;
	// 
	// 	if ( info->aIsLocalPlayer )
	// 	{
	// 		ViewportShader_t* viewport = graphics->GetViewportData( 0 );
	// 
	// 		if ( viewport )
	// 			viewport->aViewPos = thirdPerson.aPos;
	// 
	// 		// TODO: PERF: this also queues the viewport data
	// 		Game_SetView( viewMat );
	// 		// audio->SetListenerTransform( thirdPerson.aPos, transformView.aAng );
	// 	}
	// 
	// 	Util_GetViewMatrixZDirection( viewMat, camDir->aForward.Edit(), camDir->aRight.Edit(), camDir->aUp.Edit() );
	// }
	// else
	{
		glm::mat4 viewMat;
		Util_ToViewMatrixZ( viewMat, spContext->aView.aPos, spContext->aView.aAng );

		// wtf broken??
		// audio->SetListenerTransform( transformView.aPos, transformView.aAng );

		// ViewportShader_t* viewport = graphics->GetViewportData( spContext->aView.aViewportIndex );
		ViewportShader_t* viewport = graphics->GetViewportData( gMainViewport );

		if ( viewport )
			viewport->aViewPos = spContext->aView.aPos;

		// TODO: PERF: this also queues the viewport data
		Game_SetView( viewMat );

		Util_GetViewMatrixZDirection( viewMatrixZ, spContext->aView.aForward, spContext->aView.aRight, spContext->aView.aUp );
	}

	// temp
	//graphics->DrawAxis( transformView.aPos, transformView.aAng, { 40.f, 40.f, 40.f } );
	//graphics->DrawAxis( transform->aPos, transform->aAng, { 40.f, 40.f, 40.f } );

	Game_UpdateProjection();

	// i feel like there's gonna be a lot more here in the future...
	Skybox_SetAng( spContext->aView.aAng );

	//glm::vec3 listenerAng{
	//	transformView.aAng.Get().x,
	//	transformView.aAng.Get().z,
	//	transformView.aAng.Get().y,
	//};
	//
	//audio->SetListenerTransform( transformView.aPos, listenerAng );

#if AUDIO_OPENAL
	//audio->SetListenerVelocity( rigidBody->aVel );
	// audio->SetListenerOrient( camDir->aForward, camDir->aUp );
#endif

	if ( editor_show_pos )
	{
		gui->DebugMessage( "Pos: %s", ch_vec3_to_str( spContext->aView.aPos ).c_str() );
		gui->DebugMessage( "Ang: %s", ch_vec3_to_str( spContext->aView.aAng ).c_str() );
	}
}


inline float DegreeConstrain( float num )
{
	num = std::fmod( num, 360.0f );
	return ( num < 0.0f ) ? num += 360.0f : num;
}


inline void ClampAngles( glm::vec3& srAng )
{
	srAng[ YAW ]   = DegreeConstrain( srAng[ YAW ] );
	srAng[ PITCH ] = std::clamp( srAng[ PITCH ], -90.0f, 90.0f );
};


static void CenterMouseOnScreen( EditorContext_t* context )
{
	int x = ( context->aView.aResolution.x / 2.f ) + context->aView.aOffset.x;
	int y = ( context->aView.aResolution.y / 2.f ) + context->aView.aOffset.y;

	// int x = context->aView.aOffset.x;
	// int y = context->aView.aOffset.y;

	SDL_WarpMouseInWindow( gToolData.window, x, y );
}


void EditorView_CheckSelectionResult( EditorContext_t* context )
{
	if ( gClearSelection )
		context->aEntitiesSelected.clear();

	gClearSelection = false;

	u8 selectColor[ 3 ];
	if ( !renderOld->GetSelectionResult( selectColor[ 0 ], selectColor[ 1 ], selectColor[ 2 ] ) )
		return;

	// scan entities to find a matching color
	for ( ch_handle_t entHandle : context->aMap.aMapEntities )
	{
		Entity_t* ent = Entity_GetData( entHandle );

		if ( !ent )
		{
			continue;
		}

		// Entity Color Selection
		if ( ent->aSelectColor[ 0 ] == selectColor[ 0 ] && ent->aSelectColor[ 1 ] == selectColor[ 1 ] && ent->aSelectColor[ 2 ] == selectColor[ 2 ] )
		{
			EntEditor_AddToSelection( context, entHandle );
			break;
		}

		// Multi-Texture Select
		for ( u32 i = 0; i < ent->aMaterialColors.size(); i++ )
		{
			Color3& color = ent->aMaterialColors[ i ];
			if ( color.r == selectColor[ 0 ] && color.g == selectColor[ 1 ] && color.b == selectColor[ 2 ] )
			{
				EntEditor_AddToSelection( context, entHandle );
				break;
			}
		}
	}
}


void EditorView_Update()
{
	EditorContext_t* context = Editor_GetContext();

	if ( !context )
		return;

	// If we ran a selection texture render last frame, then get the result of it here
	if ( gCheckSelectionResult )
	{
		gCheckSelectionResult = false;
		EditorView_CheckSelectionResult( context );
	}

	if ( !gEditorData.aMouseCaptured )
		gEditorData.aMouseInView = EditorView_IsMouseInView();

	// ch_handle_t Inputs

	ImGuiContext* imContext = ImGui::GetCurrentContext();

	ImGuiID       activeID = imContext->ActiveId;

	if ( !activeID )
		EditorView_UpdateInputs( gEditorData.aMouseInView );

	bool centerMouse = false;

	if ( gEditorData.aMouseInView && Input_KeyJustPressed( EBinding_Viewport_MouseLook ) )
	{
		SDL_SetRelativeMouseMode( SDL_TRUE );
		gEditorData.aMouseCaptured = true;

		// clear any focused text input
		ImGui::ClearActiveID();
	}

	if ( gEditorData.aMouseCaptured )
	{
		if ( Input_KeyPressed( EBinding_Viewport_MouseLook ) )
		{
			// not sure why i need to do this after updating imgui
			SDL_ShowCursor( SDL_FALSE );

			// ch_handle_t Mouse Input
			const glm::vec2 mouse = Input_GetMouseDelta();

			// transform.aAng[PITCH] = -mouse.y;
			context->aView.aAng[ PITCH ] += mouse.y * m_pitch;
			context->aView.aAng[ YAW ] += mouse.x * m_yaw;

			ClampAngles( context->aView.aAng );

			centerMouse = true;
		}
		else
		{
			SDL_SetRelativeMouseMode( SDL_FALSE );
			SDL_ShowCursor( SDL_TRUE );

			if ( gEditorData.aMouseCaptured )
				centerMouse = true;

			gEditorData.aMouseInView   = false;
			gEditorData.aMouseCaptured = false;
		}
	}

	// if ( Input_KeyReleased( EBinding_Viewport_MouseLook ) )
	// {
	// 	SDL_SetRelativeMouseMode( SDL_FALSE );
	// 	//SDL_ShowCursor( SDL_TRUE );
	// }

	// ch_handle_t View
	EditorView_UpdateView( context );

	// Update Renderable List
	static ChVector< ch_handle_t > renderList;
	renderList.clear();

#if 0
	renderList.reserve( context->aMap.aMapEntities.size() * 2 );

	for ( ch_handle_t entHandle : context->aMap.aMapEntities )
	{
		Entity_t* ent = Entity_GetData( entHandle );

		if ( !ent )
			continue;

		if ( ent->aHidden )
			continue;

		if ( ent->aRenderable )
			renderList.push_back( ent->aRenderable );

		if ( ent->aLightRenderable )
			renderList.push_back( ent->aLightRenderable );
	}
#else
	// TODO: Make a better render list setup that accounts for hiding entities
	// internally in set viewport renderlist, it checks for vis right now, we need to do that ourselves eventually
	// something like
	// ch_handle_t* outList = graphics->DoRenderableFrustumCulling( viewport, renderList.data(), renderList.size(), outSize );
	renderList.reserve( graphics->GetRenderableCount() );

	for ( u32 i = 0; i < graphics->GetRenderableCount(); i++ )
	{
		renderList.push_back( graphics->GetRenderableByIndex( i ) );
	}
#endif

	// TODO: should only update this when the viewport is actually changed
	graphics->SetViewportRenderList( gMainViewport, renderList.data(), renderList.size() );

	if ( centerMouse )
		CenterMouseOnScreen( context );
}

