#include "main.h"
#include "entity_editor.h"
#include "mesh_builder.h"
#include "inputsystem.h"
#include "gizmos.h"


static ch_handle_t gGizmoTranslationModel = CH_INVALID_HANDLE;

constexpr float   CH_GIZMO_SCALE         = 0.33f;
constexpr float   CH_GIZMO_LINE_LEN      = 45.f;
constexpr float   CH_GIZMO_ARROW_SIZE    = 6.f;
constexpr float   CH_GIZMO_ARROW_LEN     = 15.f;
constexpr float   CH_GIZMO_ARROW_END_POS = CH_GIZMO_LINE_LEN + CH_GIZMO_ARROW_LEN;

CONVAR_BOOL_EXT( editor_gizmo_scale_enabled );
CONVAR_FLOAT_EXT( editor_gizmo_scale );


static ch_handle_t CreateAxisMaterial( const char* name, ch_handle_t shader, glm::vec3 color )
{
	ch_handle_t mat = graphics->CreateMaterial( name, shader );

	if ( mat == CH_INVALID_HANDLE )
		return CH_INVALID_HANDLE;

	graphics->Mat_SetVar( mat, "color", { color.x, color.y, color.z, 1 } );
	return mat;
}


bool Gizmo_BuildTranslationMesh()
{
	ch_handle_t shader = graphics->GetShader( "gizmo" );

	if ( shader == CH_INVALID_HANDLE )
	{
		Log_FatalF( "Failed to find gizmo shader!\n" );
		return false;
	}

	Model* model           = nullptr;
	gGizmoTranslationModel = graphics->CreateModel( &model );

	ch_handle_t  matX       = CreateAxisMaterial( "gizmo_translate_x", shader, {1, 0, 0} );
	ch_handle_t  matY       = CreateAxisMaterial( "gizmo_translate_y", shader, {0, 1, 0} );
	ch_handle_t  matZ       = CreateAxisMaterial( "gizmo_translate_z", shader, {0, 0, 1} );

	if ( !matX || !matY || !matZ )
	{
		Log_ErrorF( "Failed to make axis material for translation gizmo!\n" );
		return false;
	}

	MeshBuilder meshBuilder( graphics );
	meshBuilder.Start( model, "gizmo_translation" );
	meshBuilder.SetSurfaceCount( 3 );

	auto CreateVert = [ & ]( const glm::vec3& pos )
	{
		meshBuilder.SetPos( pos * CH_GIZMO_SCALE );
		meshBuilder.NextVertex();
	};

	auto CreateTri = [ & ]( const glm::vec3& pos0, const glm::vec3& pos1, const glm::vec3& pos2 )
	{
		CreateVert( pos0 );
		CreateVert( pos1 );
		CreateVert( pos2 );
	};
	
	// -------------------------------------------------------------------------------------
	// X - AXIS
	
	meshBuilder.SetCurrentSurface( 0 );
	meshBuilder.SetMaterial( matX );

	// create the main rectangle mesh now
	// Create Bottom Face (-Z)
	CreateTri( { CH_GIZMO_LINE_LEN, 1, -1 }, { CH_GIZMO_LINE_LEN, -1, -1 }, { -1, -1, -1 } );
	CreateTri( { -1, 1, -1 }, { CH_GIZMO_LINE_LEN, 1, -1 }, { -1, -1, -1 } );

	// Create Top Face (+Z)
	CreateTri( { CH_GIZMO_LINE_LEN, 1, 1 }, { 1, 1, 1 }, { 1, -1, 1 } );
	CreateTri( { CH_GIZMO_LINE_LEN, -1, 1 }, { CH_GIZMO_LINE_LEN, 1, 1 }, { 1, -1, 1 } );

	// Create Left Face (-X)
	// CreateTri( { -1, CH_GIZMO_LINE_LEN, -1 }, { -1, -1, -1 }, { -1, -1, 1 } );
	// CreateTri( { -1, CH_GIZMO_LINE_LEN, 1 }, { -1, 1, -1 }, { -1, -1, 1 } );
	
	// Create Right Face (+X)
	// CreateTri( { CH_GIZMO_LINE_LEN, -1, -1 }, { CH_GIZMO_LINE_LEN, 1, -1 }, { CH_GIZMO_LINE_LEN, 1, 1 } );
	// CreateTri( { CH_GIZMO_LINE_LEN, -1, 1 }, { CH_GIZMO_LINE_LEN, -1, -1 }, { CH_GIZMO_LINE_LEN, 1, 1 } );
	
	// Create Back Face (+Y)
	CreateTri( { 1, 1, 1 }, { CH_GIZMO_LINE_LEN, 1, 1 }, { CH_GIZMO_LINE_LEN, 1, -1 } );
	CreateTri( { 1, 1, -1 }, { 1, 1, 1 }, { CH_GIZMO_LINE_LEN, 1, -1 } );
	
	// Create Front Face (-Y)
	CreateTri( { -1, -1, 1 }, { -1, -1, -1 }, { CH_GIZMO_LINE_LEN, -1, -1 } );
	CreateTri( { CH_GIZMO_LINE_LEN, -1, 1 }, { -1, -1, 1 }, { CH_GIZMO_LINE_LEN, -1, -1 } );

	// create the triangle
	// Create Bottom Face
	CreateTri( { CH_GIZMO_LINE_LEN, 6, -6 }, { CH_GIZMO_LINE_LEN, -6, -6 }, { CH_GIZMO_LINE_LEN, -6, 6 } );
	CreateTri( { CH_GIZMO_LINE_LEN, 6, 6 }, { CH_GIZMO_LINE_LEN, 6, -6 }, { CH_GIZMO_LINE_LEN, -6, 6 } );
	
	CreateTri( { CH_GIZMO_LINE_LEN, 6, 6 },   { CH_GIZMO_LINE_LEN, -6, 6 },  { CH_GIZMO_ARROW_END_POS, 0, 0 } );      // Front Triangle
	CreateTri( { CH_GIZMO_LINE_LEN, -6, -6 }, { CH_GIZMO_LINE_LEN,  6, -6 }, { CH_GIZMO_ARROW_END_POS, 0, 0 } );  // Back Triangle
	CreateTri( { CH_GIZMO_LINE_LEN, -6, 6 },  { CH_GIZMO_LINE_LEN, -6, -6 }, { CH_GIZMO_ARROW_END_POS, 0, 0 } );    // Left Triangle
	CreateTri( { CH_GIZMO_LINE_LEN, 6, -6 },  { CH_GIZMO_LINE_LEN,  6, 6 },  { CH_GIZMO_ARROW_END_POS, 0, 0 } );    // Right Triangle
	
	// -------------------------------------------------------------------------------------
	// Y - AXIS
	
	meshBuilder.SetCurrentSurface( 1 );
	meshBuilder.SetMaterial( matY );
	
	// create the main rectangle mesh now
	// Create Bottom Face (-Z)
	CreateTri( { 1, CH_GIZMO_LINE_LEN, -1 }, { 1, -1, -1 }, { -1, -1, -1 } );
	CreateTri( { -1, CH_GIZMO_LINE_LEN, -1 }, { 1, CH_GIZMO_LINE_LEN, -1 }, { -1, -1, -1 } );
	
	// Create Top Face (+Z)
	CreateTri( { 1, CH_GIZMO_LINE_LEN, 1 }, { -1, CH_GIZMO_LINE_LEN, 1 }, { -1, -1, 1 } );
	CreateTri( { 1, -1, 1 }, { 1, CH_GIZMO_LINE_LEN, 1 }, { -1, -1, 1 } );
	
	// Create Left Face (-X)
	CreateTri( { -1, CH_GIZMO_LINE_LEN, -1 }, { -1, -1, -1 }, { -1, -1, 1 } );
	CreateTri( { -1, CH_GIZMO_LINE_LEN, 1 }, { -1, CH_GIZMO_LINE_LEN, -1 }, { -1, -1, 1 } );
	
	// Create Right Face (+X)
	CreateTri( { 1, -1, -1 }, { 1, CH_GIZMO_LINE_LEN, -1 }, { 1, CH_GIZMO_LINE_LEN, 1 } );
	CreateTri( { 1, -1, 1 }, { 1, -1, -1 }, { 1, CH_GIZMO_LINE_LEN, 1 } );
	
	// Create Back Face (+Y)
	// CreateTri( { -1, CH_GIZMO_LINE_LEN, 1 }, { 1, CH_GIZMO_LINE_LEN, 1 }, { 1, CH_GIZMO_LINE_LEN, -1 } );
	// CreateTri( { -1, CH_GIZMO_LINE_LEN, -1 }, { -1, CH_GIZMO_LINE_LEN, 1 }, { 1, CH_GIZMO_LINE_LEN, -1 } );
	
	// Create Front Face (-Y)
	CreateTri( { -1, -1, 1 }, { -1, -1, -1 }, { 1, -1, -1 } );
	CreateTri( { 1, -1, 1 }, { -1, -1, 1 }, { 1, -1, -1 } );
	
	// create the triangle
	// Create Bottom Face
	CreateTri( { 6, CH_GIZMO_LINE_LEN, 6 }, { -6, CH_GIZMO_LINE_LEN, 6 }, { -6, CH_GIZMO_LINE_LEN, -6 } );
	CreateTri( { 6, CH_GIZMO_LINE_LEN, -6 }, { 6, CH_GIZMO_LINE_LEN, 6 }, { -6, CH_GIZMO_LINE_LEN, -6 } );
	
	CreateTri( { 6, CH_GIZMO_LINE_LEN, -6 }, { -6, CH_GIZMO_LINE_LEN, -6 }, { 0, CH_GIZMO_ARROW_END_POS, 0 } );  // Front Triangle
	CreateTri( { -6, CH_GIZMO_LINE_LEN, 6 }, { 6, CH_GIZMO_LINE_LEN, 6 }, { 0, CH_GIZMO_ARROW_END_POS, 0 } );  // Back Triangle
	CreateTri( { -6, CH_GIZMO_LINE_LEN, -6 }, { -6, CH_GIZMO_LINE_LEN, 6 }, { 0, CH_GIZMO_ARROW_END_POS, 0 } );  // Left Triangle
	CreateTri( { 6, CH_GIZMO_LINE_LEN, 6 }, { 6, CH_GIZMO_LINE_LEN, -6 }, { 0, CH_GIZMO_ARROW_END_POS, 0 } );  // Right Triangle

	// -------------------------------------------------------------------------------------
	// Z - AXIS

	meshBuilder.SetCurrentSurface( 2 );
	meshBuilder.SetMaterial( matZ );
	
	// create the main rectangle mesh now
	// Create Bottom Face (-Z)
	// CreateTri( { 1, 1, -1 }, { 1, -1, -1 }, { -1, -1, -1 } );
	// CreateTri( { -1, 1, -1 }, { 1, 1, -1 }, { -1, -1, -1 } );
	
	// Create Top Face (+Z)
	// // CreateTri( { 1, 1, CH_GIZMO_LINE_LEN }, { -1, 1, CH_GIZMO_LINE_LEN }, { -1, -1, CH_GIZMO_LINE_LEN } );
	// // CreateTri( { 1, -1, CH_GIZMO_LINE_LEN }, { 1, 1, CH_GIZMO_LINE_LEN }, { -1, -1, CH_GIZMO_LINE_LEN } );
	
	// Create Left Face (-X)
	CreateTri( { -1, 1, 1 }, { -1, -1, -1 }, { -1, -1, CH_GIZMO_LINE_LEN } );
	CreateTri( { -1, 1, CH_GIZMO_LINE_LEN }, { -1, 1, 1 }, { -1, -1, CH_GIZMO_LINE_LEN } );
	
	// Create Right Face (+X)
	CreateTri( { 1, -1, -1 }, { 1, 1, -1 }, { 1, 1, CH_GIZMO_LINE_LEN } );
	CreateTri( { 1, -1, CH_GIZMO_LINE_LEN }, { 1, -1, -1 }, { 1, 1, CH_GIZMO_LINE_LEN } );
	
	// Create Back Face (+Y)
	CreateTri( { -1, 1, CH_GIZMO_LINE_LEN }, { 1, 1, CH_GIZMO_LINE_LEN }, { 1, 1, -1 } );
	CreateTri( { -1, 1, -1 }, { -1, 1, CH_GIZMO_LINE_LEN }, { 1, 1, -1 } );
	
	// Create Front Face (-Y)
	CreateTri( { -1, -1, CH_GIZMO_LINE_LEN }, { -1, -1, -1 }, { 1, -1, -1 } );
	CreateTri( { 1, -1, CH_GIZMO_LINE_LEN }, { -1, -1, CH_GIZMO_LINE_LEN }, { 1, -1, -1 } );
	
	// create the triangle
	// Create Bottom Face
	CreateTri( { 6, -6, CH_GIZMO_LINE_LEN }, { -6, -6, CH_GIZMO_LINE_LEN }, { -6, 6, CH_GIZMO_LINE_LEN } );
	CreateTri( { 6, 6, CH_GIZMO_LINE_LEN }, { 6, -6, CH_GIZMO_LINE_LEN }, { -6, 6, CH_GIZMO_LINE_LEN } );
	
	CreateTri( { 6, 6, CH_GIZMO_LINE_LEN }, { -6, 6, CH_GIZMO_LINE_LEN }, { 0, 0, CH_GIZMO_ARROW_END_POS } );    // Front Triangle
	CreateTri( { -6, -6, CH_GIZMO_LINE_LEN }, { 6, -6, CH_GIZMO_LINE_LEN }, { 0, 0, CH_GIZMO_ARROW_END_POS } );  // Back Triangle
	CreateTri( { -6, 6, CH_GIZMO_LINE_LEN }, { -6, -6, CH_GIZMO_LINE_LEN }, { 0, 0, CH_GIZMO_ARROW_END_POS } );   // Left Triangle
	CreateTri( { 6, -6, CH_GIZMO_LINE_LEN }, { 6, 6, CH_GIZMO_LINE_LEN }, { 0, 0, CH_GIZMO_ARROW_END_POS } );   // Right Triangle

	// -------------------------------------------------------------------------------------
	
	// TODO: plane axis and center box for screen transform

	meshBuilder.End();

	gEditorRenderables.gizmoTranslation = graphics->CreateRenderable( gGizmoTranslationModel );

	if ( !gEditorRenderables.gizmoTranslation )
	{
		Log_Error( "Failed to Create Translation Gizmo Renderable!\n" );
		return false;
	}

	if ( Renderable_t* renderable = graphics->GetRenderableData( gEditorRenderables.gizmoTranslation ) )
	{
		graphics->SetRenderableDebugName( gEditorRenderables.gizmoTranslation, "gizmo_translation" );

		renderable->aCastShadow = false;
		renderable->aTestVis    = false;
	}

	// AABB's for selecting an axis
	// X Axis
	gEditorRenderables.baseTranslateAABB[ 0 ].min = { 1, 1, -1 };
	gEditorRenderables.baseTranslateAABB[ 0 ].max = { 18, -1, 1 };

	// Y Axis
	gEditorRenderables.baseTranslateAABB[ 1 ].min = { -1, 1, -1 };
	gEditorRenderables.baseTranslateAABB[ 1 ].max = { 1, 18, 1 };

	// Z Axis
	gEditorRenderables.baseTranslateAABB[ 2 ].min = { -1, -1, 1 };
	gEditorRenderables.baseTranslateAABB[ 2 ].max = { 1, 1, 18 };

	return true;
}


bool Gizmo_Init()
{
	if ( !Gizmo_BuildTranslationMesh() )
	{
		return false;
	}

	return true;
}


void Gizmo_Shutdown()
{
}


void Gizmo_UpdateSelectedAxis( EditorContext_t* context )
{
	Renderable_t* renderable = graphics->GetRenderableData( gEditorRenderables.gizmoTranslation );

	if ( Input_KeyReleased( EBinding_Viewport_SelectSingle ) )
	{
		for ( u32 i = 0; i < renderable->aMaterialCount; i++ )
		{
			graphics->Mat_SetVar( renderable->apMaterials[ i ], "selected", false );
		}

		gEditorData.gizmo.selectedAxis = EGizmoAxis_None;
		return;
	}

	// TEMP
	// gEditorData.gizmo.offset = { 0, 0, 1 };

	// calc distance between the current ray and last frame ray?
}


void Gizmo_UpdateTranslationInputs( EditorContext_t* context, Ray& ray, glm::mat4& entityWorldMatrix )
{
	// Reset hovered state on materials
	Renderable_t* renderable = graphics->GetRenderableData( gEditorRenderables.gizmoTranslation );

	for ( u32 i = 0; i < renderable->aMaterialCount; i++ )
	{
		graphics->Mat_SetVar( renderable->apMaterials[ i ], "hovered", false );
	}

	glm::vec3 entityPos = Util_GetMatrixPosition( entityWorldMatrix );

	AABB      aabbList[ 3 ] = {};

	for ( int i = 0; i < 3; i++ )
	{
		aabbList[ i ].min = gEditorRenderables.baseTranslateAABB[ i ].min;
		aabbList[ i ].max = gEditorRenderables.baseTranslateAABB[ i ].max;
	}

	// First do gizmo scaling if enabled
	if ( editor_gizmo_scale_enabled )
	{
		glm::vec3 camPos = context->aView.aPos;

		// Scale it based on distance so it always appears the same size, no matter how far or close you are
		float     dist  = glm::sqrt( powf( camPos.x - entityPos.x, 2 ) + powf( camPos.y - entityPos.y, 2 ) + powf( camPos.z - entityPos.z, 2 ) );

		glm::vec3 scale = { dist, dist, dist };
		scale *= editor_gizmo_scale;

		for ( int i = 0; i < 3; i++ )
		{
			aabbList[ i ].min *= scale;
			aabbList[ i ].max *= scale;
		}
	}

	// Now Apply Entity Position Offsets
	for ( int i = 0; i < 3; i++ )
	{
		aabbList[ i ].min += entityPos;
		aabbList[ i ].max += entityPos;
	}

	// graphics->DrawBBox( translateX.min, translateX.max, { 1, 0, 0 } );

	// do ray test against selection gizmos
	EGizmoAxis hoveredAxis = EGizmoAxis_None;

	glm::vec3  intersectionPoint;
	u32        aabbIndex = UINT32_MAX;
	if ( !Util_RayInteresectsWithAABBs( intersectionPoint, aabbIndex, ray, aabbList, 3 ) )
		return;

	ch_handle_t axisMatHandle = CH_INVALID_HANDLE;

	switch ( aabbIndex )
	{
		default:
			return;

		case 0:
			hoveredAxis   = EGizmoAxis_X;
			axisMatHandle = renderable->apMaterials[ 0 ];
			break;

		case 1:
			hoveredAxis   = EGizmoAxis_Y;
			axisMatHandle = renderable->apMaterials[ 1 ];
			break;

		case 2:
			hoveredAxis   = EGizmoAxis_Z;
			axisMatHandle = renderable->apMaterials[ 2 ];
			break;
	}

	graphics->Mat_SetVar( axisMatHandle, "hovered", true );

	// Check Selection
	if ( !Input_KeyJustPressed( EBinding_Viewport_SelectSingle ) )
		return;

	gEditorData.gizmo.selectedAxis          = hoveredAxis;
	gEditorData.gizmo.lastIntersectionPoint = intersectionPoint;
	graphics->Mat_SetVar( axisMatHandle, "selected", true );

	// Build Translation Plane
	glm::vec3 forward, right, up;

	if ( gEditorData.gizmo.isWorldSpace )
	{
		forward = vec_forward;
		right   = vec_right;
		up      = vec_up;
	}
	else  // local space
	{
		Util_GetMatrixDirectionNoScale( entityWorldMatrix, &forward, &right, &up );
	}

	glm::vec3 movePlaneNormal;

	switch ( hoveredAxis )
	{
		case EGizmoAxis_X:
		case EGizmoAxis_PlaneXY:
			movePlaneNormal = right;
			break;

		case EGizmoAxis_Y:
		case EGizmoAxis_PlaneXZ:
			movePlaneNormal = forward;
			break;

		case EGizmoAxis_Z:
		case EGizmoAxis_PlaneYZ:
			movePlaneNormal = up;
			break;

		case EGizmoAxis_Screen:
			movePlaneNormal = -context->aView.aForward;
			break;
	}

	// Creates a set of axis to draw a plane from based on camera angles, to interpret mouse movement properly (maybe idfk)
	switch ( hoveredAxis )
	{
		case EGizmoAxis_X:
		case EGizmoAxis_Y:
		case EGizmoAxis_Z:
		{
			glm::vec3 cameraToModelNormalized = glm::normalize( entityPos - context->aView.aPos );
			glm::vec3 orthoVector             = glm::cross( movePlaneNormal, cameraToModelNormalized );
			movePlaneNormal                   = glm::cross( movePlaneNormal, orthoVector );
			movePlaneNormal                   = glm::normalize( movePlaneNormal );
			break;
		}
	}
	
	gEditorData.gizmo.translationPlane       = Util_BuildPlane( entityPos, movePlaneNormal );
	float len                                = Util_RayPlaneIntersection( ray, gEditorData.gizmo.translationPlane );
	gEditorData.gizmo.translationPlaneOrigin = ray.origin + ray.dir * len;
	gEditorData.gizmo.matrixOrigin           = entityPos;
	gEditorData.gizmo.relativeOrigin         = ( gEditorData.gizmo.translationPlaneOrigin - entityPos ) * ( 1.f / gEditorData.gizmo.screenFactor );
}


void Gizmo_UpdateInputs( EditorContext_t* context, bool mouseInView )
{
	gEditorData.gizmo.offset = {};

	if ( context->aEntitiesSelected.empty() )
	{
		// Reset last frame ray data
		gEditorData.gizmo.lastFrameRay = {};
		gEditorData.gizmo.lastMousePos = {};
		return;
	}

	// Ray ray = Util_GetRayFromScreenSpace( input->GetMousePos(), context->aView.aPos, context->aView.aViewportIndex );
	Ray ray = Util_GetRayFromScreenSpace( input->GetMousePos(), context->aView.aPos, gMainViewport );

	if ( gEditorData.gizmo.selectedAxis != EGizmoAxis_None )
	{
		Gizmo_UpdateSelectedAxis( context );
	}

	gEditorData.gizmo.lastFrameRay = ray;
	gEditorData.gizmo.lastMousePos = input->GetMousePos();

	// The Rest of this code will only run if the mouse is in the viewport and not hovering over any imgui window
	if ( !mouseInView )
		return;

	glm::mat4 entityWorldMatrix( 1.f );
	Entity_GetWorldMatrix( entityWorldMatrix, context->aEntitiesSelected[ 0 ] );

	switch ( gEditorData.gizmo.mode )
	{
		default:
			return;

		case EGizmoMode_Translation:
			Gizmo_UpdateTranslationInputs( context, ray, entityWorldMatrix );
			break;

		case EGizmoMode_Rotation:
			break;

		case EGizmoMode_Scale:
			break;

		case EGizmoMode_SuperGizmo:
			break;
	}
}


void Gizmo_Draw()
{
}


EGizmoMode Gizmo_GetMode()
{
	return EGizmoMode_Translation;
}


void Gizmo_SetMode( EGizmoMode mode )
{
}


void Gizmo_SetMatrix( glm::mat4& mat )
{
}

