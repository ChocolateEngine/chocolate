#include "game_physics.h"

#include "../../main_window/main.h"
#include "entity.h"
#include "game_shared.h"
#include "igraphics.h"
#include "main.h"
#include "mesh_builder.h"
#include "render/irender.h"


extern IRender*          render;

static Phys_DebugFuncs_t gPhysDebugFuncs;

IPhysicsEnvironment*     GetPhysEnv()
{
	CH_ASSERT( physenv );
	return physenv;
}

CONVAR_FLOAT_CMD( phys_gravity, -1, 0, "Physics Engine Gravity" )
{
	// if ( Game_GetCommandSource() != ECommandSource_User )
	// 	return;

	// TODO: maybe make some convar permission system here, and check if the client has permissions to execute this command?

	GetPhysEnv()->SetGravityZ( phys_gravity );
}


// constexpr glm::vec3 vec3_default( 255, 255, 255 );
// constexpr glm::vec4 vec4_default( 255, 255, 255, 255 );

constexpr glm::vec3                                   vec3_default( 1, 1, 1 );
constexpr glm::vec4                                   vec4_default( 1, 1, 1, 1 );

static ch_handle_t                                    gShader_Debug     = CH_INVALID_HANDLE;
static ch_handle_t                                    gShader_DebugLine = CH_INVALID_HANDLE;

static ch_handle_t                                    gMatSolid         = CH_INVALID_HANDLE;
static ch_handle_t                                    gMatWire          = CH_INVALID_HANDLE;

static std::unordered_map< ch_handle_t, ch_handle_t > gPhysRenderables;


bool&                                                 r_debug_draw = Con_GetConVarData_Bool( "r_debug_draw", false );


// ==============================================================


void                                                  Phys_DebugInit()
{
	gShader_Debug     = graphics->GetShader( "debug" );
	gShader_DebugLine = graphics->GetShader( "debug_line" );

	gMatSolid         = graphics->CreateMaterial( "__phys_debug_solid", gShader_Debug );
	gMatWire          = graphics->CreateMaterial( "__phys_debug_wire", gShader_DebugLine );
}


void Phys_DrawLine( const glm::vec3& from, const glm::vec3& to, const glm::vec3& color )
{
	graphics->DrawLine( from, to, color );
}


void Phys_DrawTriangle(
  const glm::vec3& inV1,
  const glm::vec3& inV2,
  const glm::vec3& inV3,
  const glm::vec4& srColor )
{
	graphics->DrawLine( inV1, inV2, srColor );
	graphics->DrawLine( inV1, inV3, srColor );
	graphics->DrawLine( inV2, inV3, srColor );
}

// vertex_debug_t ToVertDBG( const JPH::DebugRenderer::Vertex& inVert )
// {
// 	return {
// 		.pos        = fromJolt(inVert.mPosition),
// 		.color      = fromJolt(inVert.mColor),
// 		// .texCoord   = fromJolt(inVert.mUV),
// 		// .normal     = fromJolt(inVert.mNormal),
// 	};
// }

ch_handle_t Phys_CreateTriangleBatch( const std::vector< PhysTriangle_t >& srTriangles )
{
	if ( srTriangles.empty() )
		return CH_INVALID_HANDLE;  // mEmptyBatch;

	Model*      model  = nullptr;
	ch_handle_t handle = graphics->CreateModel( &model );

	MeshBuilder meshBuilder( graphics );
	meshBuilder.Start( model, "_phys_triangle_batch" );
	meshBuilder.SetMaterial( gMatSolid );

	meshBuilder.PreallocateVertices( srTriangles.size() * 3 );

	// convert vertices
	// vert.resize( inTriangleCount * 3 );
	for ( int i = 0; i < srTriangles.size(); i++ )
	{
		meshBuilder.SetPos( srTriangles[ i ].aPos[ 0 ] );
		// meshBuilder.SetNormal( fromJolt( inTriangles[i].mV[0].mNormal ) );
		// meshBuilder.SetColor( srTriangles[ i ].aPos[ 0 ].mColor ) );
		// meshBuilder.SetTexCoord( fromJolt( inTriangles[i].mV[0].mUV ) );
		meshBuilder.NextVertex();

		meshBuilder.SetPos( srTriangles[ i ].aPos[ 1 ] );
		// meshBuilder.SetNormal( fromJolt( inTriangles[i].mV[1].mNormal ) );
		// meshBuilder.SetColor( fromJolt( inTriangles[ i ].mV[ 1 ].mColor ) );
		// meshBuilder.SetTexCoord( fromJolt( inTriangles[i].mV[1].mUV ) );
		meshBuilder.NextVertex();

		meshBuilder.SetPos( srTriangles[ i ].aPos[ 2 ] );
		// meshBuilder.SetNormal( fromJolt( inTriangles[i].mV[2].mNormal ) );
		// meshBuilder.SetColor( fromJolt( inTriangles[ i ].mV[ 2 ].mColor ) );
		// meshBuilder.SetTexCoord( fromJolt( inTriangles[i].mV[2].mUV ) );
		meshBuilder.NextVertex();

		// vert[i * 3 + 0] = ToVertDBG( inTriangles[i].mV[0] );
		// vert[i * 3 + 1] = ToVertDBG( inTriangles[i].mV[1] );
		// vert[i * 3 + 2] = ToVertDBG( inTriangles[i].mV[2] );
	}

	meshBuilder.End();

	graphics->CalcModelBBox( handle );

	gPhysRenderables[ handle ];
	return handle;
}


ch_handle_t Phys_CreateTriangleBatchInd(
  const std::vector< PhysVertex_t >& srVerts,
  const std::vector< u32 >&          srInd )
{
	if ( srVerts.empty() || srInd.empty() )
		return CH_INVALID_HANDLE;

	Model*      model  = nullptr;
	ch_handle_t handle = graphics->CreateModel( &model );

	MeshBuilder meshBuilder( graphics );
	meshBuilder.Start( model, "__phys_model" );
	meshBuilder.SetMaterial( gMatSolid );

	meshBuilder.PreallocateVertices( srInd.size() );

	// convert vertices
	for ( size_t i = 0; i < srInd.size(); i++ )
	{
		// pretty awful
		meshBuilder.SetPos( srVerts[ srInd[ i ] ].aPos );
		meshBuilder.SetNormal( srVerts[ srInd[ i ] ].aNorm );
		meshBuilder.SetColor( srVerts[ srInd[ i ] ].color );
		meshBuilder.SetTexCoord( srVerts[ srInd[ i ] ].aUV );
		meshBuilder.NextVertex();
	}

	meshBuilder.End();

	graphics->CalcModelBBox( handle );

	gPhysRenderables[ handle ];
	return handle;
}


void Phys_DrawGeometry(
  const glm::mat4& srModelMatrix,
  // const JPH::AABox& inWorldSpaceBounds,
  float            sLODScaleSq,
  const glm::vec4& srColor,
  ch_handle_t      sGeometry,
  EPhysCullMode    sCullMode,
  bool             sCastShadow,
  bool             sWireframe )
{
	if ( !r_debug_draw )
		return;

	ch_handle_t mat = graphics->Model_GetMaterial( sGeometry, 0 );

	// graphics->Mat_SetVar( mat, "color", srColor );
	graphics->Mat_SetVar( mat, "color", { 1, 0.25, 0, 1 } );

	ch_handle_t renderHandle = gPhysRenderables[ sGeometry ];

	if ( renderHandle == CH_INVALID_HANDLE )
	{
		renderHandle                  = graphics->CreateRenderable( sGeometry );
		gPhysRenderables[ sGeometry ] = renderHandle;

		graphics->SetRenderableDebugName( renderHandle, "Physics Model" );
	}

	if ( Renderable_t* renderable = graphics->GetRenderableData( renderHandle ) )
	{
		renderable->aTestVis     = false;
		renderable->aCastShadow  = false;
		renderable->aVisible     = true;
		renderable->aModel       = sGeometry;
		renderable->aModelMatrix = srModelMatrix;

		graphics->UpdateRenderableAABB( renderHandle );
	}
}


void Phys_DrawText(
  const glm::vec3&   srPos,
  const std::string& srText,
  const glm::vec3&   srColor,
  float              sHeight )
{
	if ( !r_debug_draw )
		return;
}


void Phys_GetModelVerts( ch_handle_t sModel, PhysDataConvex_t& srData )
{
	Model* model = graphics->GetModelData( sModel );

	// TODO: use this for physics materials later on
	// for ( size_t s = 0; s < model->aMeshes.size(); s++ )
	{
		// Mesh&  mesh     = model->aMeshes[ s ];

		auto&  vertData = model->apVertexData;

		float* data     = nullptr;
		for ( auto& attrib : vertData->aData )
		{
			if ( attrib.aAttrib == VertexAttribute_Position )
			{
				data = (float*)attrib.apData;
				break;
			}
		}

		if ( data == nullptr )
		{
			Log_Error( "Phys_GetModelVerts(): Position Vertex Data not found?\n" );
			return;
		}

		u32 newSize = static_cast< u32 >( vertData->aIndices.size() );
		if ( newSize == 0 )
			newSize = srData.aVertCount;

		u32 origSize = srData.aVertCount;

		if ( srData.apVertices )
			srData.apVertices = (glm::vec3*)realloc( srData.apVertices, ( origSize + newSize ) * sizeof( glm::vec3 ) );
		else
			srData.apVertices = (glm::vec3*)malloc( ( origSize + newSize ) * sizeof( glm::vec3 ) );

		if ( vertData->aIndices.size() )
		{
			for ( size_t i = 0; i < vertData->aIndices.size(); i++ )
			{
				size_t i0                           = vertData->aIndices[ i ] * 3;

				srData.apVertices[ origSize + i ].x = data[ i0 + 0 ];
				srData.apVertices[ origSize + i ].y = data[ i0 + 1 ];
				srData.apVertices[ origSize + i ].z = data[ i0 + 2 ];
			}

			srData.aVertCount += vertData->aIndices.size();
		}
		else
		{
			for ( size_t i = 0, j = 0; i < vertData->aCount * 3; j++ )
			{
				srData.apVertices[ origSize + j ].x = data[ i++ ];
				srData.apVertices[ origSize + j ].y = data[ i++ ];
				srData.apVertices[ origSize + j ].z = data[ i++ ];
			}

			srData.aVertCount += vertData->aCount;
		}
	}
}


void Phys_GetModelTris( ch_handle_t sModel, std::vector< PhysTriangle_t >& srTris )
{
	Model* model = graphics->GetModelData( sModel );

	// TODO: use this for physics materials later on
	// for ( size_t s = 0; s < model->aMeshes.size(); s++ )
	{
		// Mesh&  mesh     = model->aMeshes[ s ];

		auto&  vertData = model->apVertexData;

		float* data     = nullptr;
		for ( auto& attrib : vertData->aData )
		{
			if ( attrib.aAttrib == VertexAttribute_Position )
			{
				data = (float*)attrib.apData;
				break;
			}
		}

		// shouldn't be using this function if we have indices lol
		// could even calculate them in GetModelInd as well
		if ( vertData->aIndices.size() )
		{
			for ( size_t i = 0; i < vertData->aIndices.size(); i += 3 )
			{
				size_t i0     = vertData->aIndices[ i + 0 ] * 3;
				size_t i1     = vertData->aIndices[ i + 1 ] * 3;
				size_t i2     = vertData->aIndices[ i + 2 ] * 3;

				auto&  tri    = srTris.emplace_back();

				tri.aPos[ 0 ] = { data[ i0 ], data[ i0 + 1 ], data[ i0 + 2 ] };
				tri.aPos[ 1 ] = { data[ i1 ], data[ i1 + 1 ], data[ i1 + 2 ] };
				tri.aPos[ 2 ] = { data[ i2 ], data[ i2 + 1 ], data[ i2 + 2 ] };

				// srTris.emplace_back( vec0, vec1, vec2 );
			}
		}
		else
		{
			for ( size_t i = 0; i < vertData->aCount * 3; )
			{
				auto& tri     = srTris.emplace_back();

				tri.aPos[ 0 ] = { data[ i++ ], data[ i++ ], data[ i++ ] };
				tri.aPos[ 1 ] = { data[ i++ ], data[ i++ ], data[ i++ ] };
				tri.aPos[ 2 ] = { data[ i++ ], data[ i++ ], data[ i++ ] };

				// srTris.emplace_back( vec0, vec1, vec2 );
			}
		}
	}
}


void Phys_GetModelInd( ch_handle_t sModel, PhysDataConcave_t& srData )
{
	u32    origSize = srData.aVertCount;
	Model* model    = graphics->GetModelData( sModel );

	// TODO: use this for physics materials later on
	for ( size_t s = 0; s < model->aMeshes.size(); s++ )
	{
		Mesh&      mesh     = model->aMeshes[ s ];

		auto&      vertData = model->apVertexData;

		glm::vec3* data     = nullptr;
		for ( auto& attrib : vertData->aData )
		{
			if ( attrib.aAttrib == VertexAttribute_Position )
			{
				data = static_cast< glm::vec3* >( attrib.apData );
				break;
			}
		}

		if ( data == nullptr )
		{
			Log_Error( "Phys_GetModelInd(): Position Vertex Data not found?\n" );
			return;
		}

		origSize       = srData.aVertCount;
		void* vertsTmp = realloc( srData.apVertices, ( origSize + mesh.aVertexCount ) * sizeof( glm::vec3 ) );

		if ( !vertsTmp )
		{
			Log_ErrorF( "Failed to allocate memory for physics vertices: (%zd bytes)\n", ( origSize + mesh.aVertexCount ) * sizeof( glm::vec3 ) );
			free( srData.apVertices );
			return;
		}

		srData.apVertices = static_cast< glm::vec3* >( vertsTmp );

		// Zero the new memory
		memset( &srData.apVertices[ origSize ], 0, mesh.aVertexCount * sizeof( glm::vec3 ) );

		// faster
		memcpy( &srData.apVertices[ origSize ], &data[ mesh.aVertexOffset ], mesh.aVertexCount * sizeof( glm::vec3 ) );

		srData.aVertCount += mesh.aVertexCount;
		u32 indSize = srData.aTriCount;

		{
			void* trisTmp = realloc( srData.aTris, ( indSize + ( mesh.aIndexCount / 3 ) ) * sizeof( PhysIndexedTriangle_t ) );

			if ( !trisTmp )
			{
				Log_ErrorF( "Failed to allocate memory for physics indexed triangles: (%zd bytes)\n",
				            ( indSize + ( mesh.aIndexCount / 3 ) ) * sizeof( PhysIndexedTriangle_t ) );

				free( srData.aTris );
				return;
			}

			srData.aTris = static_cast< PhysIndexedTriangle_t* >( trisTmp );

			// Zero the new memory
			memset( &srData.aTris[ indSize ], 0, ( mesh.aIndexCount / 3 ) * sizeof( PhysIndexedTriangle_t ) );
		}

		u32 j = 0;
		for ( u32 i = 0; i < mesh.aIndexCount; )
		{
			u32       idx0   = vertData->aIndices[ mesh.aIndexOffset + i++ ];
			u32       idx1   = vertData->aIndices[ mesh.aIndexOffset + i++ ];
			u32       idx2   = vertData->aIndices[ mesh.aIndexOffset + i++ ];

			// Check Triangle Normal
			glm::vec3 v0     = srData.apVertices[ idx0 ];
			glm::vec3 v1     = srData.apVertices[ idx1 ];
			glm::vec3 v2     = srData.apVertices[ idx2 ];

			// Calculate triangle normal
			glm::vec3 normal = glm::cross( ( v1 - v0 ), ( v2 - v0 ) );
			float     len    = glm::length( normal );

			// Make sure we don't have any 0 lengths, that will crash the physics engine
			if ( len == 0.f )
			{
				Log_Warn( "Normal of 0?\n" );
				continue;
			}

			srData.aTris[ indSize + j ].aPos[ 0 ] = idx0;
			srData.aTris[ indSize + j ].aPos[ 1 ] = idx1;
			srData.aTris[ indSize + j ].aPos[ 2 ] = idx2;
			j++;
		}

		srData.aTriCount += j;

		// consolidate the memory just in case
		if ( j != mesh.aIndexCount / 3 )
		{
			void* trisTmp = realloc( srData.aTris, ( indSize + j ) * sizeof( PhysIndexedTriangle_t ) );

			if ( !trisTmp )
			{
				Log_ErrorF( "Failed to allocate memory for physics indexed triangles: (%zd bytes)\n",
				            ( indSize + j ) * sizeof( PhysIndexedTriangle_t ) );

				free( srData.aTris );
				return;
			}

			srData.aTris = static_cast< PhysIndexedTriangle_t* >( trisTmp );
		}
	}
}


void Phys_Init()
{
	Phys_CreateEnv();

	Phys_DebugInit();

	gPhysDebugFuncs.apDrawLine          = Phys_DrawLine;
	gPhysDebugFuncs.apDrawTriangle      = Phys_DrawTriangle;
	gPhysDebugFuncs.apCreateTriBatch    = Phys_CreateTriangleBatch;
	gPhysDebugFuncs.apCreateTriBatchInd = Phys_CreateTriangleBatchInd;
	gPhysDebugFuncs.apDrawGeometry      = Phys_DrawGeometry;
	gPhysDebugFuncs.apDrawText          = Phys_DrawText;

	ch_physics->SetDebugDrawFuncs( gPhysDebugFuncs );
}


void Phys_Shutdown()
{
	for ( auto& [ handle, renderable ] : gPhysRenderables )
	{
		if ( !renderable )
			continue;

		graphics->FreeRenderable( renderable );
	}

	gPhysRenderables.clear();
	Phys_DestroyEnv();
}


void Phys_CreateEnv()
{
	Phys_DestroyEnv();

	physenv = ch_physics->CreatePhysEnv();
	physenv->Init();
	physenv->SetGravityZ( phys_gravity );
}


void Phys_DestroyEnv()
{
	if ( physenv )
		ch_physics->DestroyPhysEnv( physenv );

	physenv = nullptr;
}


void Phys_Simulate( IPhysicsEnvironment* spPhysEnv, float sFrameTime )
{
	PROF_SCOPE();
	if ( !spPhysEnv )
		return;

	// reset all renderables
	// TODO: probably quite slow and inefficient, having to skip over chunks of data each loop
	// just to set visible to false
	for ( auto& [ handle, renderHandle ] : gPhysRenderables )
	{
		if ( !renderHandle )
			continue;

		if ( Renderable_t* renderable = graphics->GetRenderableData( renderHandle ) )
		{
			renderable->aVisible = false;
		}
	}

	spPhysEnv->Simulate( sFrameTime );
}


void Phys_SetMaxVelocities( IPhysicsObject* spPhysObj )
{
	if ( !spPhysObj )
		return;

	spPhysObj->SetMaxLinearVelocity( 50000.f );
	spPhysObj->SetMaxAngularVelocity( 50000.f );
}
