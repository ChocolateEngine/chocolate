#include "graphics_int.h"
#include "debug_draw.h"
#include "types/transform.h"

// --------------------------------------------------------------------------------------
// Debug Drawing


// static MeshBuilder                               gDebugLineBuilder;
static ch_handle_t                   gDebugLineModel    = CH_INVALID_HANDLE;
static ch_handle_t                   gDebugLineDraw     = CH_INVALID_HANDLE;
static ch_handle_t                   gDebugLineMaterial = CH_INVALID_HANDLE;
ChVector< Shader_VertexData_t > gDebugLineVerts;
static size_t                   gDebugLineBufferSize = 0;


// --------------------------------------------------------------------------------------

// CONVAR_BOOL( r_debug_draw, 0 );
CONVAR_BOOL( r_debug_aabb, 0, "", 0 );
CONVAR_BOOL( r_debug_frustums, 0, "", 0 );
CONVAR_BOOL( r_debug_normals, 0, "", 0 );
CONVAR_FLOAT( r_debug_normals_len, 0.15, "", 0 );
CONVAR_FLOAT( r_debug_normals_len_face, 8, "", 0 );


const bool& r_debug_draw   = Con_Register_Bool( "r_debug_draw", "Enable or Disable All Debug Drawing", true );


// --------------------------------------------------------------------------------------


bool Graphics_DebugDrawInit()
{
	return true;
}


void Graphics_DebugDrawNewFrame()
{
	PROF_SCOPE();

	gDebugLineVerts.clear();

	if ( !r_debug_draw )
	{
		if ( gDebugLineModel )
		{
			gGraphics.FreeModel( gDebugLineModel );
			gDebugLineModel = CH_INVALID_HANDLE;
		}

		if ( gDebugLineDraw )
		{
			Renderable_t* renderable = gGraphics.GetRenderableData( gDebugLineDraw );

			if ( !renderable )
				return;

			// gGraphics.FreeModel( renderable->aModel );
			gGraphics.FreeRenderable( gDebugLineDraw );
			gDebugLineDraw = CH_INVALID_HANDLE;
		}

		return;
	}

	if ( !gDebugLineModel )
	{
		Model* model    = nullptr;
		gDebugLineModel = gGraphics.CreateModel( &model );

		if ( !gDebugLineModel )
		{
			Log_Error( gLC_ClientGraphics, "Failed to create Debug Line Model\n" );
			return;
		}

		model->aMeshes.resize( 1 );

		model->apVertexData = new VertexData_t;
		model->apBuffers    = new ModelBuffers_t;

		// gpDebugLineModel->apBuffers->aVertex.resize( 2, true );
		model->apVertexData->aData.resize( 2, true );
		model->apVertexData->aData[ 0 ].aAttrib = VertexAttribute_Position;
		model->apVertexData->aData[ 1 ].aAttrib = VertexAttribute_Color;

		gDebugLineMaterial                      = gGraphics.CreateMaterial( "__debug_line_mat", gGraphics.GetShader( "debug_line" ) );

		gGraphics.Model_SetMaterial( gDebugLineModel, 0, gDebugLineMaterial );

		if ( gDebugLineDraw )
		{
			if ( Renderable_t* renderable = gGraphics.GetRenderableData( gDebugLineDraw ) )
			{
				renderable->aModel = gDebugLineDraw;
			}
		}
	}

	if ( !gDebugLineDraw )
	{
		gDebugLineDraw = gGraphics.CreateRenderable( gDebugLineModel );

		if ( !gDebugLineDraw )
			return;

		Renderable_t* renderable = gGraphics.GetRenderableData( gDebugLineDraw );

		if ( !renderable )
			return;

		renderable->aTestVis    = false;
		renderable->aCastShadow = false;
		renderable->aVisible    = true;

		gGraphics.SetRenderableDebugName( gDebugLineDraw, "Debug Draw Renderable" );
	}
}


// Update Debug Draw Buffers
// TODO: replace this system with instanced drawing
void Graphics_UpdateDebugDraw()
{
	PROF_SCOPE();

	if ( !r_debug_draw )
		return;

	if ( r_debug_aabb || r_debug_normals )
	{
		for ( auto& [ viewHandle, viewRenderList ] : gGraphicsData.aViewRenderLists )
		{
			glm::mat4 lastMatrix = glm::mat4( 0.f );
			glm::mat4 invMatrix  = glm::mat4( 1.f );

			for ( auto& [ shader, modelList ] : viewRenderList.aRenderLists )
			{
				if ( r_debug_normals )
				{
					u32 total_amount_of_indices = 0;
					for ( SurfaceDraw_t& surfaceDraw : modelList )
					{
						// hack to not draw this AABB multiple times, need to change this render list system
						if ( surfaceDraw.aSurface != 0 )
							continue;

						Renderable_t* renderable        = gGraphics.GetRenderableData( surfaceDraw.aRenderable );

						if ( !renderable )
						{
							Log_Warn( gLC_ClientGraphics, "Draw Data does not exist for renderable!\n" );
							return;
						}

						Model*        model             = gGraphics.GetModelData( renderable->aModel );

						for ( size_t s = 0; s < model->aMeshes.size(); s++ )
						{
							Mesh& mesh = model->aMeshes[ s ];
							total_amount_of_indices += mesh.aIndexCount;
						}
					}

					// gGraphics.DebugDrawReserve( total_amount_of_indices * 3 );
				}

				for ( SurfaceDraw_t& surfaceDraw : modelList )
				{
					// hack to not draw this AABB multiple times, need to change this render list system
					if ( surfaceDraw.aSurface != 0 )
						continue;

					Renderable_t* renderable = gGraphics.GetRenderableData( surfaceDraw.aRenderable );

					if ( !renderable )
					{
						Log_Warn( gLC_ClientGraphics, "Draw Data does not exist for renderable!\n" );
						return;
					}

					if ( r_debug_aabb )
						gGraphics.DrawBBox( renderable->aAABB.aMin, renderable->aAABB.aMax, { 1.0, 0.5, 1.0 } );

					if ( r_debug_normals )
					{
						if ( lastMatrix != renderable->aModelMatrix )
						{
							lastMatrix = renderable->aModelMatrix;
							invMatrix  = glm::inverse( renderable->aModelMatrix );
						}

						gGraphics.DrawNormals( renderable->aModel, renderable->aModelMatrix );
					}

					// ModelBBox_t& bbox = gModelBBox[ renderable->apDraw->aModel ];
					// gGraphics.DrawBBox( bbox.aMin, bbox.aMax, { 1.0, 0.5, 1.0 } );
				}
			}
		}
	}

	if ( gDebugLineDraw == CH_INVALID_HANDLE )
	{
		return;
	}

	Renderable_t* renderable = gGraphics.GetRenderableData( gDebugLineDraw );

	if ( !renderable )
		return;

	if ( gDebugLineModel && gDebugLineVerts.size() )
	{
		renderable->aVisible = true;
		// Mesh& mesh = gDebugLineModel->aMeshes[ 0 ];

		Model* model = nullptr;
		if ( !gGraphicsData.aModels.Get( gDebugLineModel, &model ) )
		{
			Log_Error( gLC_ClientGraphics, "Failed to get Debug Draw Model!\n" );
			gDebugLineModel = CH_INVALID_HANDLE;
			return;
		}

		if ( !model->apVertexData )
			model->apVertexData = new VertexData_t;

		if ( !model->apBuffers )
			model->apBuffers = new ModelBuffers_t;

		// Is our current buffer size too small? If so, free the old ones
		if ( gDebugLineVerts.size() > gDebugLineBufferSize || model->apBuffers->aVertex == CH_INVALID_HANDLE )
		{
			if ( model->apBuffers && model->apBuffers->aVertex != CH_INVALID_HANDLE )
			{
				if ( model->apBuffers->aVertexHandle != UINT32_MAX )
				{
					Graphics_RemoveShaderBuffer( gGraphicsData.aVertexBuffers, model->apBuffers->aVertexHandle );
				}

				render->DestroyBuffer( model->apBuffers->aVertex );
				model->apBuffers->aVertex = CH_INVALID_HANDLE;
			}

			gDebugLineBufferSize = gDebugLineVerts.size();
		}

		size_t bufferSize = sizeof( Shader_VertexData_t ) * gDebugLineVerts.size();

		// Create new Buffers if needed
		if ( model->apBuffers->aVertex == CH_INVALID_HANDLE )
		{
			model->apBuffers->aVertex       = render->CreateBuffer( "DebugLine Vertex", bufferSize, EBufferFlags_Storage, EBufferMemory_Host );
			model->apBuffers->aVertexHandle = Graphics_AddShaderBuffer( gGraphicsData.aVertexBuffers, model->apBuffers->aVertex );
		}

		model->apVertexData->aCount = gDebugLineVerts.size();

		if ( model->aMeshes.empty() )
			model->aMeshes.resize( 1 );

		model->aMeshes[ 0 ].aIndexCount   = 0;
		model->aMeshes[ 0 ].aVertexOffset = 0;
		model->aMeshes[ 0 ].aVertexCount  = gDebugLineVerts.size();
		
		renderable->aVertexBuffer         = model->apBuffers->aVertex;
		renderable->aVertexIndex          = model->apBuffers->aVertexHandle;

		// Update the Buffers
		render->BufferWrite( model->apBuffers->aVertex, bufferSize, gDebugLineVerts.data() );
	}
	else
	{
		renderable->aVisible = false;
	}
}


// ---------------------------------------------------------------------------------------
// Debug Rendering Functions


void Graphics::DrawLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec3& sColor )
{
	PROF_SCOPE();

	if ( !r_debug_draw || !gDebugLineModel )
		return;

	size_t index = gDebugLineVerts.size();
	gDebugLineVerts.resize( gDebugLineVerts.size() + 2 );

#if 0
	gDebugLineVerts[ index ].aPos     = sX;
	gDebugLineVerts[ index ].color.x = sColor.x;
	gDebugLineVerts[ index ].color.y = sColor.y;
	gDebugLineVerts[ index ].color.z = sColor.z;

	index++;
	gDebugLineVerts[ index ].aPos     = sY;
	gDebugLineVerts[ index ].color.x = sColor.x;
	gDebugLineVerts[ index ].color.y = sColor.y;
	gDebugLineVerts[ index ].color.z = sColor.z;

#else

	gDebugLineVerts[ index ].aPosNormX.x = sX.x;
	gDebugLineVerts[ index ].aPosNormX.y = sX.y;
	gDebugLineVerts[ index ].aPosNormX.z = sX.z;

	gDebugLineVerts[ index ].color.x    = sColor.x;
	gDebugLineVerts[ index ].color.y    = sColor.y;
	gDebugLineVerts[ index ].color.z    = sColor.z;
	gDebugLineVerts[ index ].color.w    = 1.f;

	index++;
	gDebugLineVerts[ index ].aPosNormX.x = sY.x;
	gDebugLineVerts[ index ].aPosNormX.y = sY.y;
	gDebugLineVerts[ index ].aPosNormX.z = sY.z;

	gDebugLineVerts[ index ].color.x    = sColor.x;
	gDebugLineVerts[ index ].color.y    = sColor.y;
	gDebugLineVerts[ index ].color.z    = sColor.z;
	gDebugLineVerts[ index ].color.w    = 1.f;
#endif
}


void Graphics::DrawLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec3& sColorX, const glm::vec3& sColorY )
{
	PROF_SCOPE();

	if ( !r_debug_draw || !gDebugLineModel )
		return;

	size_t index = gDebugLineVerts.size();
	gDebugLineVerts.resize( gDebugLineVerts.size() + 2 );

	gDebugLineVerts[ index ].aPosNormX.x = sX.x;
	gDebugLineVerts[ index ].aPosNormX.y = sX.y;
	gDebugLineVerts[ index ].aPosNormX.z = sX.z;

	gDebugLineVerts[ index ].color.x    = sColorX.x;
	gDebugLineVerts[ index ].color.y    = sColorX.y;
	gDebugLineVerts[ index ].color.z    = sColorX.z;
	gDebugLineVerts[ index ].color.w    = 1.f;

	index++;
	gDebugLineVerts[ index ].aPosNormX.x = sY.x;
	gDebugLineVerts[ index ].aPosNormX.y = sY.y;
	gDebugLineVerts[ index ].aPosNormX.z = sY.z;

	gDebugLineVerts[ index ].color.x    = sColorY.x;
	gDebugLineVerts[ index ].color.y    = sColorY.y;
	gDebugLineVerts[ index ].color.z    = sColorY.z;
	gDebugLineVerts[ index ].color.w    = 1.f;
}


void Graphics::DrawLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec4& sColor )
{
	PROF_SCOPE();

	if ( !r_debug_draw || !gDebugLineModel )
		return;

	size_t index = gDebugLineVerts.size();
	gDebugLineVerts.resize( gDebugLineVerts.size() + 2 );

	gDebugLineVerts[ index ].aPosNormX.x = sX.x;
	gDebugLineVerts[ index ].aPosNormX.y = sX.y;
	gDebugLineVerts[ index ].aPosNormX.z = sX.z;
	gDebugLineVerts[ index ].color      = sColor;

	index++;
	gDebugLineVerts[ index ].aPosNormX.x = sY.x;
	gDebugLineVerts[ index ].aPosNormX.y = sY.y;
	gDebugLineVerts[ index ].aPosNormX.z = sY.z;
	gDebugLineVerts[ index ].color      = sColor;
}


CONVAR_FLOAT( r_debug_axis_scale, 1, "", 0 );


void Graphics::DrawAxis( const glm::vec3& sPos, const glm::vec3& sAng, const glm::vec3& sScale )
{
	if ( !r_debug_draw || !gDebugLineModel )
		return;

	glm::vec3 forward, right, up;
	// Util_GetDirectionVectors( sAng, &forward, &right, &up );
	glm::mat4 mat = Util_ToMatrix( &sPos, &sAng, &sScale );
	Util_GetMatrixDirection( mat, &forward, &right, &up );

	gGraphics.DrawLine( sPos, sPos + ( forward * sScale.x * r_debug_axis_scale ), { 1.f, 0.f, 0.f } );
	gGraphics.DrawLine( sPos, sPos + ( right * sScale.y * r_debug_axis_scale ), { 0.f, 1.f, 0.f } );
	gGraphics.DrawLine( sPos, sPos + ( up * sScale.z * r_debug_axis_scale ), { 0.f, 0.f, 1.f } );
}


void Graphics::DrawAxis( const glm::mat4& sMat, const glm::vec3& sScale )
{
	if ( !r_debug_draw || !gDebugLineModel )
		return;

	glm::vec3 forward, right, up;
	Util_GetMatrixDirection( sMat, &forward, &right, &up );
	glm::vec3 pos = Util_GetMatrixPosition( sMat );

	gGraphics.DrawLine( pos, pos + ( forward * sScale.x * r_debug_axis_scale ), { 1.f, 0.f, 0.f } );
	gGraphics.DrawLine( pos, pos + ( right * sScale.y * r_debug_axis_scale ), { 0.f, 1.f, 0.f } );
	gGraphics.DrawLine( pos, pos + ( up * sScale.z * r_debug_axis_scale ), { 0.f, 0.f, 1.f } );
}


void Graphics::DrawAxis( const glm::mat4& sMat )
{
	if ( !r_debug_draw || !gDebugLineModel )
		return;

	glm::vec3 forward, right, up;
	Util_GetMatrixDirection( sMat, &forward, &right, &up );
	glm::vec3 pos   = Util_GetMatrixPosition( sMat );
	glm::vec3 scale = Util_GetMatrixScale( sMat );

	gGraphics.DrawLine( pos, pos + ( forward * scale.x * r_debug_axis_scale ), { 1.f, 0.f, 0.f } );
	gGraphics.DrawLine( pos, pos + ( right * scale.y * r_debug_axis_scale ), { 0.f, 1.f, 0.f } );
	gGraphics.DrawLine( pos, pos + ( up * scale.z * r_debug_axis_scale ), { 0.f, 0.f, 1.f } );
}


void Graphics::DrawBBox( const glm::vec3& sMin, const glm::vec3& sMax, const glm::vec3& sColor )
{
	PROF_SCOPE();

	if ( !r_debug_draw || !gDebugLineModel )
		return;

	gDebugLineVerts.reserve( gDebugLineVerts.size() + 24 );

	// bottom
	gGraphics.DrawLine( sMin, glm::vec3( sMax.x, sMin.y, sMin.z ), sColor );
	gGraphics.DrawLine( sMin, glm::vec3( sMin.x, sMax.y, sMin.z ), sColor );
	gGraphics.DrawLine( glm::vec3( sMin.x, sMax.y, sMin.z ), glm::vec3( sMax.x, sMax.y, sMin.z ), sColor );
	gGraphics.DrawLine( glm::vec3( sMax.x, sMin.y, sMin.z ), glm::vec3( sMax.x, sMax.y, sMin.z ), sColor );

	// top
	gGraphics.DrawLine( sMax, glm::vec3( sMin.x, sMax.y, sMax.z ), sColor );
	gGraphics.DrawLine( sMax, glm::vec3( sMax.x, sMin.y, sMax.z ), sColor );
	gGraphics.DrawLine( glm::vec3( sMax.x, sMin.y, sMax.z ), glm::vec3( sMin.x, sMin.y, sMax.z ), sColor );
	gGraphics.DrawLine( glm::vec3( sMin.x, sMax.y, sMax.z ), glm::vec3( sMin.x, sMin.y, sMax.z ), sColor );

	// sides
	gGraphics.DrawLine( sMin, glm::vec3( sMin.x, sMin.y, sMax.z ), sColor );
	gGraphics.DrawLine( sMax, glm::vec3( sMax.x, sMax.y, sMin.z ), sColor );
	gGraphics.DrawLine( glm::vec3( sMax.x, sMin.y, sMin.z ), glm::vec3( sMax.x, sMin.y, sMax.z ), sColor );
	gGraphics.DrawLine( glm::vec3( sMin.x, sMax.y, sMin.z ), glm::vec3( sMin.x, sMax.y, sMax.z ), sColor );
}


void Graphics::DrawProjView( const glm::mat4& srProjView )
{
	PROF_SCOPE();

	if ( !r_debug_draw || !gDebugLineModel )
		return;

	glm::mat4 inv = glm::inverse( srProjView );

	// Calculate Frustum Points
	glm::vec3 v[ 8u ];
	for ( int i = 0; i < 8; i++ )
	{
		glm::vec4 ff = inv * gFrustumFaceData[ i ];
		v[ i ].x     = ff.x / ff.w;
		v[ i ].y     = ff.y / ff.w;
		v[ i ].z     = ff.z / ff.w;
	}

	gDebugLineVerts.reserve( gDebugLineVerts.size() + 24 );

	gGraphics.DrawLine( v[ 0 ], v[ 1 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( v[ 0 ], v[ 2 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( v[ 3 ], v[ 1 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( v[ 3 ], v[ 2 ], glm::vec3( 1, 1, 1 ) );

	gGraphics.DrawLine( v[ 4 ], v[ 5 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( v[ 4 ], v[ 6 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( v[ 7 ], v[ 5 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( v[ 7 ], v[ 6 ], glm::vec3( 1, 1, 1 ) );

	gGraphics.DrawLine( v[ 0 ], v[ 4 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( v[ 1 ], v[ 5 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( v[ 3 ], v[ 7 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( v[ 2 ], v[ 6 ], glm::vec3( 1, 1, 1 ) );
}


void Graphics::DrawFrustum( const Frustum_t& srFrustum )
{
	PROF_SCOPE();

	if ( !r_debug_draw || !gDebugLineModel || !r_debug_frustums )
		return;

	gDebugLineVerts.reserve( gDebugLineVerts.size() + 24 );

	// Draw the Near Plane of the Frustum
	gGraphics.DrawLine( srFrustum.aPoints[ 0 ], srFrustum.aPoints[ 1 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( srFrustum.aPoints[ 0 ], srFrustum.aPoints[ 2 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( srFrustum.aPoints[ 3 ], srFrustum.aPoints[ 1 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( srFrustum.aPoints[ 3 ], srFrustum.aPoints[ 2 ], glm::vec3( 1, 1, 1 ) );

	// Draw the lines conecting the near plane to the far plane
	gGraphics.DrawLine( srFrustum.aPoints[ 4 ], srFrustum.aPoints[ 5 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( srFrustum.aPoints[ 4 ], srFrustum.aPoints[ 6 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( srFrustum.aPoints[ 7 ], srFrustum.aPoints[ 5 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( srFrustum.aPoints[ 7 ], srFrustum.aPoints[ 6 ], glm::vec3( 1, 1, 1 ) );
	
	// Draw the Far Plane of the Frustum
	gGraphics.DrawLine( srFrustum.aPoints[ 0 ], srFrustum.aPoints[ 4 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( srFrustum.aPoints[ 1 ], srFrustum.aPoints[ 5 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( srFrustum.aPoints[ 3 ], srFrustum.aPoints[ 7 ], glm::vec3( 1, 1, 1 ) );
	gGraphics.DrawLine( srFrustum.aPoints[ 2 ], srFrustum.aPoints[ 6 ], glm::vec3( 1, 1, 1 ) );
}


void Graphics::DrawNormals( ch_handle_t sModel, const glm::mat4& srMatrix )
{
#if 1
	PROF_SCOPE();

	if ( !r_debug_draw || !gDebugLineModel || !r_debug_normals )
		return;

	glm::mat4 inverse           = glm::inverse( srMatrix );

	Model*    model = gGraphics.GetModelData( sModel );

//	size_t    total_index_count = 0;
//
//	for ( size_t s = 0; s < model->aMeshes.size(); s++ )
//	{
//		Mesh& mesh = model->aMeshes[ s ];
//		total_index_count += mesh.aIndexCount;
//	}
//
//	gDebugLineVerts.reserve( gDebugLineVerts.size() + ( total_index_count * 3 ) );

	// TODO: use this for physics materials later on
	for ( size_t s = 0; s < model->aMeshes.size(); s++ )
	{
		Mesh&      mesh     = model->aMeshes[ s ];

		auto&      vertData = model->apVertexData;

		glm::vec3* pos      = nullptr;
		glm::vec3* normals  = nullptr;

		for ( auto& attrib : vertData->aData )
		{
			if ( attrib.aAttrib == VertexAttribute_Position )
			{
				pos = static_cast< glm::vec3* >( attrib.apData );
			}
			if ( attrib.aAttrib == VertexAttribute_Normal )
			{
				normals = static_cast< glm::vec3* >( attrib.apData );
			}

			if ( pos && normals )
				break;
		}

		if ( pos == nullptr || normals == nullptr )
		{
			// Log_Error( "gGraphics.DrawNormals(): Position Vertex Data not found?\n" );
			return;
		}

		u32 j = 0;
		for ( u32 i = 0; i < mesh.aIndexCount; )
		{
			u32       idx[ 3 ];
			glm::vec3 v[ 3 ];
			glm::vec3 n[ 3 ];

			idx[ 0 ] = vertData->aIndices[ mesh.aIndexOffset + i++ ];
			idx[ 1 ] = vertData->aIndices[ mesh.aIndexOffset + i++ ];
			idx[ 2 ] = vertData->aIndices[ mesh.aIndexOffset + i++ ];

			v[ 0 ]   = pos[ idx[ 0 ] ];
			v[ 1 ]   = pos[ idx[ 1 ] ];
			v[ 2 ]   = pos[ idx[ 2 ] ];

			n[ 0 ]   = normals[ idx[ 0 ] ];
			n[ 1 ]   = normals[ idx[ 1 ] ];
			n[ 2 ]   = normals[ idx[ 2 ] ];

			// Calculate face normal
			glm::vec3 normal = glm::cross( ( v[ 1 ] - v[ 0 ] ), ( v[ 2 ] - v[ 0 ] ) );
			float     len    = glm::length( normal );

			// Make sure we don't have any 0 lengths
			if ( len == 0.f )
			{
				// Log_Warn( "gGraphics.DrawNormals(): Face Normal of 0?\n" );
				continue;
			}

			glm::vec3 posX[ 3 ];
			glm::vec3 posY[ 3 ];

			// Draw Vertex Normals
			for ( int vi = 0; vi < 3; vi++ )
			{
				glm::vec4 v4( v[ vi ], 1 );
				glm::vec4 n4( n[ vi ], 1 );

				posX[ vi ]           = srMatrix * v4;
				glm::vec3 normal_dir = glm::normalize( n4 * inverse );

				posY[ vi ]           = posX[ vi ] + ( normal_dir * r_debug_normals_len );

				// protoTransform.aPos, protoTransform.aPos + ( forward * r_proto_line_dist2 )

				// gGraphics.DrawLine( posX, posY, {0.9, 0.1, 0.1, 1.f} );
			}

			gGraphics.DrawLine( posX[ 0 ], posY[ 0 ], { 0.9, 0.1, 0.1, 1.f } );
			gGraphics.DrawLine( posX[ 1 ], posY[ 1 ], { 0.1, 0.9, 0.1, 1.f } );
			gGraphics.DrawLine( posX[ 2 ], posY[ 2 ], { 0.1, 0.1, 0.9, 1.f } );

			// Draw Face Normal
			// glm::vec4 normal4( normal, 1 );
			// glm::vec3 posX = normal4 * srMatrix;
			// glm::vec3 posY = posX * r_debug_normals_len_face;
			// 
			// gGraphics.DrawLine( posX, posY, normal );
			j++;
		}
	}
#endif
}


void Graphics::DebugDrawReserve( u32 count )
{
	gDebugLineVerts.reserve( gDebugLineVerts.size() + count );
}

