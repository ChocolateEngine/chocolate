#include "main.h"
#include "game_physics.h"
#include "game_shared.h"
#include "entity/entity.h"
#include "render/irender.h"
#include "igraphics.h"
#include "mesh_builder.h"


extern IRender*          render;

// IPhysicsEnvironment*     physenv    = nullptr;
IPhysicsEnvironment*     physenv    = nullptr;

Ch_IPhysics*             ch_physics = nullptr;

static Phys_DebugFuncs_t gPhysDebugFuncs;

IPhysicsEnvironment*     GetPhysEnv()
{
	CH_ASSERT( physenv );
	return physenv;
}

// CONVAR_CMD_EX( phys_gravity, -20.2984, CVARF( SERVER ) | CVARF( REPLICATED ), "Physics Engine Gravity" )
CONVAR_FLOAT_CMD( phys_gravity, -CH_GRAVITY_SPEED, CVARF( SERVER ) | CVARF( REPLICATED ), "Physics Engine Gravity" )
{
	// if ( Game_GetCommandSource() != ECommandSource_Console )
	// 	return;

	// TODO: maybe make some convar permission system here, and check if the client has permissions to execute this command?

	GetPhysEnv()->SetGravityZ( phys_gravity );
}

bool&                                       r_debug_draw = Con_GetConVarData_Bool( "r_debug_draw", false );


// constexpr glm::vec3 vec3_default( 255, 255, 255 );
// constexpr glm::vec4 vec4_default( 255, 255, 255, 255 );

constexpr glm::vec3                         vec3_default( 1, 1, 1 );
constexpr glm::vec4                         vec4_default( 1, 1, 1, 1 );

static ch_handle_t                               gShader_Debug     = CH_INVALID_HANDLE;
static ch_handle_t                               gShader_DebugLine = CH_INVALID_HANDLE;

static ch_handle_t                               gMatSolid         = CH_INVALID_HANDLE;
static ch_handle_t                               gMatWire          = CH_INVALID_HANDLE;

static std::unordered_map< ch_handle_t, ch_handle_t > gPhysRenderables;


// ==============================================================

//#define CH_COMPONENT_EDITOR_FUNC() static void ASDKASOPDOPKEOPFGK9oieGFKJIOPEGIOPESJFi()
//#define CH_REGISTER_COMPONENT_EDITOR()

// CH_COMPONENT_EDITOR_FUNC()
// {
// }


CH_STRUCT_REGISTER_COMPONENT( CPhysShape, physShape, EEntComponentNetType_Both, ECompRegFlag_None )
{
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_S32, PhysShapeType, aShapeType, shapeType, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_StdString, std::string, aPath, path, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Vec3, glm::vec3, aBounds, bounds, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_SYS2( EntSys_PhysShape, gEntSys_PhysShape );

	// TODO: add a read and write function for the entity editor
	// we need to have physShape and physObject not be created instantly
	// so what we're going to do instead is add a button called "Create Phys Shape" and "Create Phys Object", that creates or recreates them manually
	// would probably cause less issues
	// CH_REGISTER_COMPONENT_EDITOR();
}


struct CPlayerUse_Event_t
{
	glm::vec3 aPos;
	glm::vec3 aAng;
};


CH_STRUCT_REGISTER_COMPONENT( CPhysObject, physObject, EEntComponentNetType_Both, ECompRegFlag_None )
{
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Bool, bool, aStartActive, startActive, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Bool, bool, aAllowSleeping, allowSleeping, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aMaxLinearVelocity, maxLinearVelocity, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aMaxAngularVelocity, maxAngularVelocity, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_VAR2( EEntNetField_S32, PhysMotionType, aMotionType, motionType, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_S32, PhysMotionQuality, aMotionQuality, motionQuality, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Bool, bool, aIsSensor, isSensor, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Bool, bool, aCustomMass, customMass, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aMass, mass, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Bool, bool, aGravity, gravity, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_U8, EPhysTransformMode, aTransformMode, transformMode, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_SYS2( EntSys_PhysObject, gEntSys_PhysObject );



	// Events
	// CH_REGISTER_COMPONENT_EVENT( CPhysObjectEvent_ContactStart, ContactStart );
	// CH_REGISTER_COMPONENT_EVENT( CPhysObjectEvent_TransformUpdated, TransformUpdated );
	// 
	// CH_REGISTER_COMPONENT_EVENT( CPlayerUse_Event_t, Use );
	// 
	// // example
	// CPhysObjectEvent_OnContactStart onContactStartData{};
	// Entity_FireEvent( physObject, "ContactStart", onContactStartData );
	// 
	// // Event Firing Example
	// CPlayerUse_Event_t* onUseEvent = new CPlayerUse_Event_t;
	// onUseEvent->aPos               = PLAYER_VIEW_POS;
	// onUseEvent->aAng               = PLAYER_VIEW_ANG;
	// 
	// Entity_FireEvent( playerEntity, "playerUse", "Use", onUseEvent );
	// 
	// // somewhere else (thinking of having the component name here to avoid event name clashing)
	// Entity_AddEventListener( physObjectEnt, "physObject", "ContactStart", CallbackFunction );
	// Entity_AddEventListener( physObjectEnt, "physObject", "TransformUpdated", CallbackFunction2 );
	// 
	// 
	// // On a renderable component
	// Entity_AddEventListener( physObjectEnt, "physObject", "TransformUpdated", CallbackFunction );
	// 
	// // On a physics component - pass in CH_ENT_INVALID to listen for this event on all entities
	// Entity_AddEventListener( CH_ENT_INVALID, "playerUse", "Use", CallbackFunction );

	

	// GameRules()->SetBool( "canSpawnNPCs", true );
	// 
	// bool canSpawnNPCs = GameRules()->GetBool( "canSpawnNPCs" );
}


// ==============================================================


bool Phys_CreatePhysShapeComponent( CPhysShape* compPhysShape )
{
	PhysicsShapeInfo shapeInfo( compPhysShape->aShapeType );

	switch ( compPhysShape->aShapeType )
	{
		default:
		{
			// Free a model if we have one
			if ( compPhysShape->aModel != CH_INVALID_HANDLE )
				graphics->FreeModel( compPhysShape->aModel );

			compPhysShape->aModel = CH_INVALID_HANDLE;

			break;
		}

		case PhysShapeType::Convex:
		case PhysShapeType::Mesh:
		{
			//if ( compPhysShape->aModel == CH_INVALID_HANDLE )
			//{
			//	compPhysShape->aModel = graphics->LoadModel( compPhysShape->aPath.Get() );
			//}
			//
			//if ( compPhysShape->aModel == CH_INVALID_HANDLE )
			//{
			//	Log_ErrorF( "Failed to load physics model: \"%s\"\n", compPhysShape->aPath.Get().c_str() );
			//	return false;
			//}

			break;
		}
	}

	IPhysicsShape* shape = nullptr;

	switch ( compPhysShape->aShapeType )
	{
		default:
			break;

		case PhysShapeType::Convex:
		case PhysShapeType::Mesh:
		case PhysShapeType::StaticCompound:
		case PhysShapeType::MutableCompound:
			shape = GetPhysEnv()->LoadShape( compPhysShape->aPath.Get().data(), compPhysShape->aPath.Get().size(), compPhysShape->aShapeType );
			break;

		// Uses the path for this
		// case PhysShapeType::Convex:
		// 	Phys_GetModelVerts( compPhysShape->aModel, shapeInfo.aConvexData );
		// 	break;
		// 
		// case PhysShapeType::Mesh:
		// 	Phys_GetModelInd( compPhysShape->aModel, shapeInfo.aConcaveData );
		// 	break;

		case PhysShapeType::Sphere:
		case PhysShapeType::Box:
		case PhysShapeType::Capsule:
		case PhysShapeType::TaperedCapsule:
		case PhysShapeType::Cylinder:
			shapeInfo.aBounds = compPhysShape->aBounds;
			shape = GetPhysEnv()->CreateShape( shapeInfo );
			break;
	}


	if ( !shape )
	{
		Log_Error( "Failed to create physics shape\n" );
		return false;
	}

	compPhysShape->apShape = shape;
	return true;
}


static void OnCreatePhysShape( Entity sEntity, CPhysShape* compPhysShape )
{
	auto physObject = Ent_GetComponent< CPhysObject >( sEntity, "physObject" );

	if ( !physObject || !physObject->apObj )
		return;

	physObject->apObj->SetShape( compPhysShape->apShape, true, true );
}


void EntSys_PhysShape::ComponentAdded( Entity sEntity, void* spData )
{
	auto compPhysShape = static_cast< CPhysShape* >( spData );

	Phys_CreatePhysShapeComponent( compPhysShape );
}


void EntSys_PhysShape::ComponentRemoved( Entity sEntity, void* spData )
{
	auto compPhysShape = static_cast< CPhysShape* >( spData );

	if ( !compPhysShape->apShape )
		return;

	GetPhysEnv()->DestroyShape( compPhysShape->apShape );
}


void EntSys_PhysShape::ComponentUpdated( Entity sEntity, void* spData )
{
	auto physShape = static_cast< CPhysShape* >( spData );

	if ( !physShape->apShape )
		return;

	if ( physShape->aShapeType.aIsDirty )
	{
		if ( physShape->apShape )
			GetPhysEnv()->DestroyShape( physShape->apShape );

		physShape->apShape = nullptr;
		Phys_CreatePhysShapeComponent( physShape );
		OnCreatePhysShape( sEntity, physShape );
	}
}


void EntSys_PhysShape::Update()
{
	for ( Entity entity : aEntities )
	{
		auto physShape  = Ent_GetComponent< CPhysShape >( entity, "physShape" );

		CH_ASSERT( physShape );

		if ( !physShape )
			continue;

		if ( !physShape->apShape )
		{
			Phys_CreatePhysShapeComponent( physShape );
			continue;
		}
	}
}


// ==============================================================


static void CreatePhysObjectComponent( Entity sEntity, CPhysShape* srCompShape, CPhysObject* srCompObject )
{
	if ( !srCompShape->apShape )
	{
		if ( !Phys_CreatePhysShapeComponent( srCompShape ) )
			return;
	}

	PhysicsObjectInfo physObjectInfo;
	physObjectInfo.aStartActive        = srCompObject->aStartActive;
	physObjectInfo.aAllowSleeping      = srCompObject->aAllowSleeping;

	physObjectInfo.aMaxLinearVelocity  = srCompObject->aMaxLinearVelocity;
	physObjectInfo.aMaxAngularVelocity = srCompObject->aMaxAngularVelocity;

	physObjectInfo.aMotionType         = srCompObject->aMotionType;
	physObjectInfo.aMotionQuality      = srCompObject->aMotionQuality;

	physObjectInfo.aIsSensor           = srCompObject->aIsSensor;

	physObjectInfo.aCustomMass         = srCompObject->aCustomMass;
	physObjectInfo.aMass               = srCompObject->aMass;

	auto transform                     = Ent_GetComponent< CTransform >( sEntity, "transform" );

	if ( transform )
	{
		physObjectInfo.aPos = transform->aPos;
		physObjectInfo.aAng = transform->aAng;
	}

	srCompObject->apObj = GetPhysEnv()->CreateObject( srCompShape->apShape, physObjectInfo );
}


void EntSys_PhysObject::ComponentAdded( Entity sEntity, void* spData )
{
}


void EntSys_PhysObject::ComponentRemoved( Entity sEntity, void* spData )
{
	auto physObject = static_cast< CPhysObject* >( spData );

	if ( !physObject->apObj )
		return;

	GetPhysEnv()->DestroyObject( physObject->apObj );
}


void EntSys_PhysObject::ComponentUpdated( Entity sEntity, void* spData )
{
	PROF_SCOPE();

	auto physObject = static_cast< CPhysObject* >( spData );

	if ( !physObject->apObj )
		return;

	if ( physObject->aMotionType != PhysMotionType::Static )
	{
		if ( physObject->aGravity.aIsDirty )
			physObject->apObj->SetGravityEnabled( physObject->aGravity );

		if ( physObject->aEnableCollision.aIsDirty )
			physObject->apObj->SetCollisionEnabled( physObject->aEnableCollision );
	}

	if ( physObject->aIsSensor.aIsDirty )
		physObject->apObj->SetSensor( physObject->aIsSensor );
}


void EntSys_PhysObject::Update()
{
	PROF_SCOPE();

	for ( Entity entity : aEntities )
	{
		auto physShape = Ent_GetComponent< CPhysShape >( entity, "physShape" );
		auto physObject = Ent_GetComponent< CPhysObject >( entity, "physObject" );

		CH_ASSERT( physShape );
		CH_ASSERT( physObject );

		if ( !physShape || !physObject )
			continue;

		if ( !physObject->apObj )
		{
			CreatePhysObjectComponent( entity, physShape, physObject );
			continue;
		}

		if ( physObject->aMotionType != PhysMotionType::Static )
		{
			if ( physObject->aGravity.aIsDirty )
				physObject->apObj->SetGravityEnabled( physObject->aGravity );

			if ( physObject->aEnableCollision.aIsDirty )
				physObject->apObj->SetCollisionEnabled( physObject->aEnableCollision );
		}

		if ( physObject->aIsSensor.aIsDirty )
			physObject->apObj->SetSensor( physObject->aIsSensor );

		auto transform = Ent_GetComponent< CTransform >( entity, "transform" );

		if ( !transform )
			continue;

		physObject->apObj->SetScale( transform->aScale );

		if ( physObject->aTransformMode.Get() == EPhysTransformMode_Update )
		{
			transform->aPos = physObject->apObj->GetPos();
			transform->aAng = physObject->apObj->GetAng();
		}
		else if ( physObject->aTransformMode.Get() == EPhysTransformMode_Inherit )
		{
			physObject->apObj->SetPos( transform->aPos );
			physObject->apObj->SetAng( transform->aAng );
		}
	}
}


// ==============================================================


EntSys_PhysShape  gEntSys_PhysShape;
EntSys_PhysObject gEntSys_PhysObject;


// ==============================================================


void Phys_DebugInit()
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
	ch_handle_t      handle = graphics->CreateModel( &model );

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
	ch_handle_t      handle = graphics->CreateModel( &model );

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
  ch_handle_t           sGeometry,
  EPhysCullMode    sCullMode,
  bool             sCastShadow,
  bool             sWireframe )
{
	if ( !r_debug_draw )
		return;

	ch_handle_t mat = graphics->Model_GetMaterial( sGeometry, 0 );

	// graphics->Mat_SetVar( mat, "color", srColor );
	graphics->Mat_SetVar( mat, "color", {1, 0.25, 0, 1} );

	ch_handle_t renderHandle = gPhysRenderables[ sGeometry ];

	if ( renderHandle == CH_INVALID_HANDLE )
	{
		renderHandle                  = graphics->CreateRenderable( sGeometry );
		gPhysRenderables[ sGeometry ] = renderHandle;
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
				size_t    i0   = vertData->aIndices[ i + 0 ] * 3;
				size_t    i1   = vertData->aIndices[ i + 1 ] * 3;
				size_t    i2   = vertData->aIndices[ i + 2 ] * 3;

				auto& tri = srTris.emplace_back();

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
				auto&     tri  = srTris.emplace_back();

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
		Mesh&  mesh     = model->aMeshes[ s ];

		auto&  vertData = model->apVertexData;

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


#if 0
IPhysicsShape* Phys_CreateShape( Entity sEntity, PhysicsShapeInfo& srShapeInfo )
{
	IPhysicsShape* shape = GetPhysEnv()->CreateShape( srShapeInfo );

	if ( !shape )
	{
		Log_Error( "Failed to create physics shape\n" );
		return nullptr;
	}

	// Attach it to the Entity
	auto shapeWrapper     = static_cast< CPhysShape* >( Entity_AddComponent( sEntity, "physShape" ) );
	shapeWrapper->apShape = shape;
	shapeWrapper->aBounds = srShapeInfo.aBounds;

	return shape;
}
#endif


CPhysObject* Phys_CreateObject( Entity sEntity, PhysicsObjectInfo& srObjectInfo )
{
	// Get the physics shape from the entity
	CPhysShape* shape = GetComp_PhysShape( sEntity );

	if ( !shape )
	{
		Log_Error( "Failed to find physics shape on entity\n" );
		return nullptr;
	}

	return Phys_CreateObject( sEntity, shape->apShape, srObjectInfo );
}


CPhysObject* Phys_CreateObject( Entity sEntity, IPhysicsShape* spShape, PhysicsObjectInfo& srObjectInfo )
{
	IPhysicsObject* object = GetPhysEnv()->CreateObject( spShape, srObjectInfo );

	if ( !object )
	{
		Log_Error( "Failed to create physics object\n" );
		return nullptr;
	}

	CPhysObject* objWrapper         = static_cast< CPhysObject* >( Entity_AddComponent( sEntity, "physObject" ) );
	objWrapper->apObj               = object;

	objWrapper->aStartActive        = srObjectInfo.aStartActive;
	objWrapper->aAllowSleeping      = srObjectInfo.aAllowSleeping;

	objWrapper->aMaxLinearVelocity  = srObjectInfo.aMaxLinearVelocity;
	objWrapper->aMaxAngularVelocity = srObjectInfo.aMaxAngularVelocity;

	objWrapper->aMotionType         = srObjectInfo.aMotionType;
	objWrapper->aMotionQuality      = srObjectInfo.aMotionQuality;

	objWrapper->aIsSensor           = srObjectInfo.aIsSensor;

	objWrapper->aCustomMass         = srObjectInfo.aCustomMass;
	objWrapper->aMass               = srObjectInfo.aMass;

	return objWrapper;
}

