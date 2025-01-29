#include "testing.h"
#include "main.h"
#include "igraphics.h"
#include "steam.h"

#if CH_CLIENT
#include "../client/cl_main.h"
#else
#include "../server/sv_main.h"
#endif

#include "sound.h"

#include "imgui/imgui.h"

// ---------------------------------------------------------------------------
// This file is dedicated for random stuff and experiments with the game
// and will be quite messy


// INPUT_CONVAR( in_proto_spam, "" );

CONVAR_FLOAT( vrcmdl_scale, 40, "" );
CONVAR_BOOL( proto_look, 1, "" );
CONVAR_BOOL( proto_follow, 0, "" );
CONVAR_FLOAT( proto_follow_speed, 400, "" );
CONVAR_FLOAT( proto_swap_target_sec_min, 1.0, "" );
CONVAR_FLOAT( proto_swap_target_sec_max, 10.0, "" );
CONVAR_FLOAT( proto_swap_turn_time_min, 0.2, "" );
CONVAR_FLOAT( proto_swap_turn_time_max, 3.0, "" );
CONVAR_BOOL( proto_look_at_entities, 1, "" );

CONVAR_FLOAT( snd_cube_scale, 0.05, "" );

CONVAR_FLOAT_EXT( phys_friction );
CONVAR_FLOAT_EXT( cl_view_height );

// Audio Channels
ch_handle_t                        hAudioMusic = CH_INVALID_HANDLE;

// testing
std::vector< Entity >         gAudioTestEntitiesCl{};
std::vector< Entity >         gAudioTestEntitiesSv{};


#if CH_CLIENT
CONCMD( help_quick )
{
	Log_Msg(
	  "convars/commands you can mess around with:\n"
	  "\tcl_username - your username, steam username will override this if the steam dll is compiled and steam is running, can be turned off though\n"
	  "\tsnd_test - pass in a path to an ogg to create an entity playing that sound where the player was\n"
	  "\tcreate_proto\n"
	  "\tcreate_look_entity - pass in a model path\n"
	  "\tmap - load a map\n"
	  // "\ttool_ent_editor - entity editor\n"
	  // "\ttool_light_editor - light editor - will be removed soon when fully merged into entity editor\n"
	  "\tm_sensitivity - change mouse sensitivity\n"
	  "\tbind - bind a key to a command or input convar\n"
	  "\tskybox_set - set the skybox material by path, only works if the material is using the skybox shader\n"
	  "\tsnd_* - sound commands\n"
	  "\tr_debug_draw - globally enables debug drawing, check out the other r_debug_* convars\n"
	);
}
#endif


// Was for an old multithreading test
#if 0
struct ProtoLookData_t
{
	std::vector< Entity > aProtos;
	Transform*            aPlayerTransform;
};
#endif


// Protogen Component
// This acts a tag to put all entities here in a list automatically
struct CProtogen
{
};


// funny temp
struct ProtoTurn_t
{
	double aNextTime;
	float  aLastRand;
	float  aRand;
};


// Information for protogen smooth turning to look toward targets
struct ProtoLookData_t
{
	glm::vec3 aStartAng{};
	glm::vec3 aGoalAng{};

	float     aTurnCurTime   = 0.f;
	float     aTurnEndTime   = 0.f;

	float     aTimeToDuel    = 0.f;
	float     aTimeToDuelCur = 0.f;

	Entity    aLookTarget    = CH_ENT_INVALID;
};


#define CH_PROTO_SV 0
#define CH_PROTO_CL 1


class ProtogenSystem : public IEntityComponentSystem
{
  public:
	ProtogenSystem()
	{
	}

	~ProtogenSystem()
	{
	}

	void ComponentAdded( Entity sEntity, void* spData ) override
	{
#if CH_CLIENT
		// Add a renderable component
		auto renderable = Ent_GetComponent< CRenderable >( sEntity, "renderable" );

		if ( !renderable )
			return;

		if ( renderable->aModel )
		{
			renderable->aRenderable = graphics->CreateRenderable( renderable->aModel );
		}
		else
		{
			renderable->aRenderable = CH_INVALID_HANDLE;
		}
#endif
	}

	void ComponentRemoved( Entity sEntity, void* spData ) override
	{
#if CH_CLIENT
		aTurnMap.erase( sEntity );
		aLookMap.erase( sEntity );

		// Entity_RemoveComponent( sEntity, "renderable" );
#endif
	}

	void ComponentUpdated( Entity sEntity, void* spData ) override
	{
#if CH_CLIENT
		aUpdated.push_back( sEntity );
#endif
	}

	void Update() override
	{
#if CH_CLIENT
		TEST_CL_UpdateProtos( gFrameTime );
#else
		TEST_SV_UpdateProtos( gFrameTime );
#endif
	}

	std::vector< Entity >                         aUpdated;

	std::unordered_map< Entity, ProtoTurn_t >     aTurnMap;
	std::unordered_map< Entity, ProtoLookData_t > aLookMap;
};


static ProtogenSystem gProtoSystem;

// ===========================================================================


void CreateProtogen_f( const std::string& path )
{
#if CH_SERVER
	Entity player = SV_GetCommandClientEntity();

	if ( !player )
		return;

	Entity proto = Entity_CreateEntity();

	Ent_AddComponent( proto, "protogen" );

	// ch_handle_t       model          = graphics->LoadModel( path );

	CRenderable* renderable     = Ent_AddComponent< CRenderable >( proto, "renderable" );
	renderable->aPath           = path;
	// renderable->aModel          = model;

	// CRenderable_t* renderComp  = Ent_AddComponent< CRenderable_t >( proto, "renderable" );
	// renderComp->aHandle        = graphics->CreateRenderable( model );

	CTransform* transform       = Ent_AddComponent< CTransform >( proto, "transform" );

	auto        playerTransform = Ent_GetComponent< CTransform >( player, "transform" );

	transform->aPos            = playerTransform->aPos;
	transform->aScale.Set( { vrcmdl_scale, vrcmdl_scale, vrcmdl_scale } );
#endif
}

void CreateModelEntity( const std::string& path )
{
#if 0
	Entity physEnt = Entity_CreateEntity();

	Model* model   = graphics->LoadModel( path );
	Entity_AddComponent< Model* >( physEnt, model );
	Transform& transform = Entity_AddComponent< Transform >( physEnt );

	Transform& playerTransform = Entity_GetComponent< Transform >( game->aLocalPlayer );
	transform = playerTransform;

	g_staticEnts.push_back( physEnt );
#endif
}

void CreatePhysEntity( const std::string& path )
{
#if 0
	Entity         physEnt    = Entity_CreateEntity();

	ch_handle_t         model      = graphics->LoadModel( path );

	CRenderable_t& renderComp = Entity_AddComponent< CRenderable_t >( physEnt );
	renderComp.aHandle        = graphics->CreateRenderable( model );

	PhysicsShapeInfo shapeInfo( PhysShapeType::Convex );

	Phys_GetModelVerts( model, shapeInfo.aConvexData );

	IPhysicsShape* shape = physenv->CreateShape( shapeInfo );

	if ( shape == nullptr )
	{
		Log_ErrorF( "Failed to create physics shape for model: \"%s\"\n", path.c_str() );
		return;
	}

	Transform&        transform = Entity_GetComponent< Transform >( gLocalPlayer );

	PhysicsObjectInfo physInfo;
	physInfo.aPos         = transform.aPos;  // NOTE: THIS IS THE CENTER OF MASS
	physInfo.aAng         = transform.aAng;
	physInfo.aMass        = 1.f;
	physInfo.aMotionType  = PhysMotionType::Dynamic;
	physInfo.aStartActive = true;

	IPhysicsObject* phys  = physenv->CreateObject( shape, physInfo );
	phys->SetFriction( phys_friction );

	Phys_SetMaxVelocities( phys );

	Entity_AddComponent< IPhysicsShape* >( physEnt, shape );
	Entity_AddComponent< IPhysicsObject* >( physEnt, phys );

	g_otherEnts.push_back( physEnt );
#endif
}


#if CH_SERVER
CONCMD_VA( create_proto, CVARF( CL_EXEC ) )
{
	CreateProtogen_f( DEFAULT_PROTOGEN_PATH );
}


static void model_dropdown(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command line the user has typed in
  std::vector< std::string >&       results )     // results to populate the dropdown list with
{
	const std::vector< ch_string > materials = FileSys_ScanDir( "materials", ReadDir_AllPaths | ReadDir_NoDirs | ReadDir_Recursive );

	for ( const auto& file : materials )
	{
		if ( ch_str_ends_with( file, "..", 2 ) )
			continue;

		if ( args.size() && !( ch_str_ends_with( file, args[ 0 ].data(), args[ 0 ].size() ) ) )
			continue;

		// make sure it's a format we can open (TODO: CHECK HEADERS)
		if ( ch_str_ends_with( file, ".obj", 4 ) || ch_str_ends_with( file, ".glb", 4 ) || ch_str_ends_with( file, ".gltf", 5 ) )
			results.emplace_back( file.data, file.size );
	}
}


CONCMD_DROP_VA( create_look_entity, model_dropdown, CVARF( CL_EXEC ) )
{
	if ( args.size() )
		CreateProtogen_f( args[ 0 ] );
}


CONCMD_VA( delete_protos, CVARF( CL_EXEC ) )
{
	// while ( gProtoSystem.aEntities.size() )
	for ( auto& proto : gProtoSystem.aEntities )
	{
		// graphics->FreeModel( Entity_GetComponent< Model* >( proto ) );
		// graphics->FreeRenderable( Entity_GetComponent< CRenderable_t >( proto ).aHandle );
		Entity_DeleteEntity( proto );
	}
}


	#if 0
CONCMD_VA( create_phys_test, CVARF( CL_EXEC ) )
{
	CreatePhysEntity( "materials/models/riverhouse/riverhouse.obj" );
}


CONCMD_VA( create_phys_proto, CVARF( CL_EXEC ) )
{
	CreatePhysEntity( "materials/models/protogen_wip_25d/protogen_wip_25d_big.obj" );
}


CONCMD_VA( create_phys_ent, CVARF( CL_EXEC ) )
{
	if ( args.size() )
		CreatePhysEntity( args[ 0 ] );
}
	#endif
#endif


void TEST_Init()
{
	CH_REGISTER_COMPONENT_FL( CProtogen, protogen, EEntComponentNetType_Both, ECompRegFlag_DontOverrideClient );
	CH_REGISTER_COMPONENT_SYS( CProtogen, ProtogenSystem, gProtoSystem );

	if ( IsSteamLoaded() )
	{
		steam->RequestAvatarImage( ESteamAvatarSize_Large, steam->GetSteamID() );
	}
}


void TEST_Shutdown()
{
	// for ( int i = 0; i < g_physEnts.size(); i++ )
	// {
	// 	if ( g_physEnts[ i ] )
	// 		delete g_physEnts[ i ];
	// 
	// 	g_physEnts[ i ] = nullptr;
	// }
}


static ImVec2 ImVec2Mul( ImVec2 vec, float sScale )
{
	vec.x *= sScale;
	vec.y *= sScale;
	return vec;
}

static ImVec2 ImVec2MulMin( ImVec2 vec, float sScale, float sMin = 1.f )
{
	vec.x *= sScale;
	vec.y *= sScale;

	vec.x = glm::max( sMin, vec.x );
	vec.y = glm::max( sMin, vec.y );

	return vec;
}

static void ScaleImGui( float sScale )
{
	static ImGuiStyle baseStyle     = ImGui::GetStyle();

	ImGuiStyle&       style         = ImGui::GetStyle();

	style.ChildRounding             = baseStyle.ChildRounding * sScale;
	style.WindowRounding            = baseStyle.WindowRounding * sScale;
	style.PopupRounding             = baseStyle.PopupRounding * sScale;
	style.FrameRounding             = baseStyle.FrameRounding * sScale;
	style.IndentSpacing             = baseStyle.IndentSpacing * sScale;
	style.ColumnsMinSpacing         = baseStyle.ColumnsMinSpacing * sScale;
	style.ScrollbarSize             = baseStyle.ScrollbarSize * sScale;
	style.ScrollbarRounding         = baseStyle.ScrollbarRounding * sScale;
	style.GrabMinSize               = baseStyle.GrabMinSize * sScale;
	style.GrabRounding              = baseStyle.GrabRounding * sScale;
	style.LogSliderDeadzone         = baseStyle.LogSliderDeadzone * sScale;
	style.TabRounding               = baseStyle.TabRounding * sScale;
	style.MouseCursorScale          = baseStyle.MouseCursorScale * sScale;
	style.TabMinWidthForCloseButton = ( baseStyle.TabMinWidthForCloseButton != FLT_MAX ) ? ( baseStyle.TabMinWidthForCloseButton * sScale ) : FLT_MAX;

	style.WindowPadding             = ImVec2Mul( baseStyle.WindowPadding, sScale );
	style.WindowMinSize             = ImVec2MulMin( baseStyle.WindowMinSize, sScale );
	style.FramePadding              = ImVec2Mul( baseStyle.FramePadding, sScale );
	style.ItemSpacing               = ImVec2Mul( baseStyle.ItemSpacing, sScale );
	style.ItemInnerSpacing          = ImVec2Mul( baseStyle.ItemInnerSpacing, sScale );
	style.CellPadding               = ImVec2Mul( baseStyle.CellPadding, sScale );
	style.TouchExtraPadding         = ImVec2Mul( baseStyle.TouchExtraPadding, sScale );
	style.DisplayWindowPadding      = ImVec2Mul( baseStyle.DisplayWindowPadding, sScale );
	style.DisplaySafeAreaPadding    = ImVec2Mul( baseStyle.DisplaySafeAreaPadding, sScale );
}


void TEST_EntUpdate()
{
#if 0
	PROF_SCOPE();

	// blech
	for ( auto& ent : g_otherEnts )
	{
		CRenderable_t& renderComp = Entity_GetComponent< CRenderable_t >( ent );

		if ( Renderable_t* renderable = graphics->GetRenderableData( renderComp.aHandle ) )
		{
			if ( renderable->aModel == CH_INVALID_HANDLE )
				continue;

			// Model *physObjList = &Entity_GetComponent< Model >( ent );

			// Transform& transform = Entity_GetComponent< Transform >( ent );
			IPhysicsObject* phys = Entity_GetComponent< IPhysicsObject* >( ent );

			if ( !phys )
				continue;

			phys->SetFriction( phys_friction );
			Util_ToMatrix( renderable->aModelMatrix, phys->GetPos(), phys->GetAng() );
		}
	}

#if 0
	// what
	if ( ImGui::Begin( "What" ) )
	{
		ImGuiStyle& style = ImGui::GetStyle();

		static float imguiScale = 1.f;

		if ( ImGui::SliderFloat( "Scale", &imguiScale, 0.25f, 4.f, "%.4f", 1.f ) )
		{
			ScaleImGui( imguiScale );
		}
	}

	ImGui::End();
#endif
#endif
}


#if 0
void TaskUpdateProtoLook( ftl::TaskScheduler *taskScheduler, void *arg )
{
	float protoScale = vrcmdl_scale;

	ProtoLookData_t* lookData = (ProtoLookData_t*)arg;
	
	// TODO: maybe make this into some kind of "look at player" component? idk lol
	// also could thread this as a test
	for ( auto& proto : lookData->aProtos )
	{
		DefaultRenderable* renderable = (DefaultRenderable*)Entity_GetComponent< RenderableHandle_t >( proto );
		auto& protoTransform = Entity_GetComponent< Transform >( proto );

		bool matrixChanged = false;

		if ( proto_look )
		{
			matrixChanged = true;

			glm::vec3 forward{}, right{}, up{};
			//AngleToVectors( protoTransform.aAng, forward, right, up );
			AngleToVectors( lookData->aPlayerTransform->aAng, forward, right, up );

			glm::vec3 protoView = protoTransform.aPos;
			//protoView.z += cl_view_height;

			glm::vec3 direction = (protoView - lookData->aPlayerTransform->aPos);
			// glm::vec3 rotationAxis = VectorToAngles( direction );
			glm::vec3 rotationAxis = VectorToAngles( direction, up );

			protoTransform.aAng = rotationAxis;
			protoTransform.aAng[PITCH] = 0.f;
			protoTransform.aAng[YAW] -= 90.f;
			protoTransform.aAng[ROLL] = (-rotationAxis[PITCH]) + 90.f;
			//protoTransform.aAng[ROLL] = 90.f;
		}

		if ( protoTransform.aScale.x != protoScale )
		{
			// protoTransform.aScale = {protoScale, protoScale, protoScale};
			protoTransform.aScale = glm::vec3( protoScale );
			matrixChanged = true;
		}

		if ( matrixChanged )
			renderable->aMatrix = protoTransform.ToMatrix();
	}
}
#endif


CONVAR_FLOAT( r_proto_line_dist, 32.f, "" );
CONVAR_FLOAT( r_proto_line_dist2, 32.f, "" );


void TEST_CL_UpdateProtos( float frameTime )
{
	PROF_SCOPE();

	// TEMP
	for ( auto& proto : gProtoSystem.aEntities )
	{
		// auto protoTransform = Ent_GetComponent< CTransform >( proto, "transform" );
		// 
		// CH_ASSERT( protoTransform );
		// 
		// // Only update the renderable model matrix if the transform component is dirty
		// bool transformDirty = false;
		// transformDirty |= protoTransform->aPos.aIsDirty;
		// transformDirty |= protoTransform->aAng.aIsDirty;
		// transformDirty |= protoTransform->aScale.aIsDirty;
		// 
		// if ( !transformDirty )
		// 	continue;

		auto renderComp = Ent_GetComponent< CRenderable >( proto, "renderable" );

		CH_ASSERT( renderComp );

		if ( !renderComp )
			continue;

		// I have to do this here, and not in ComponentAdded(), because modelPath may not added yet
		if ( renderComp->aRenderable == CH_INVALID_HANDLE )
		{
			if ( renderComp->aModel == CH_INVALID_HANDLE )
			{
				if ( renderComp->aPath.Get().empty() )

				renderComp->aModel = graphics->LoadModel( renderComp->aPath );
				if ( renderComp->aModel == CH_INVALID_HANDLE )
					continue;
			}

			ch_handle_t        renderable = graphics->CreateRenderable( renderComp->aModel );
			Renderable_t* renderData = graphics->GetRenderableData( renderable );

			if ( !renderData )
			{
				Log_Error( "scream\n" );
				continue;
			}

			renderComp->aRenderable = renderable;
		}

		Renderable_t* renderData = graphics->GetRenderableData( renderComp->aRenderable );

		if ( renderData )
		{
			Entity_GetWorldMatrix( renderData->aModelMatrix, proto );
			graphics->UpdateRenderableAABB( renderComp->aRenderable );
		}

		// glm::vec3 modelForward, modelRight, modelUp;
		// Util_GetMatrixDirection( protoTransform->ToMatrix(), &modelForward, &modelRight, &modelUp );
		// graphics->DrawLine( protoTransform->aPos, protoTransform->aPos + ( modelForward * r_proto_line_dist ), { 1.f, 0.f, 0.f } );
		// graphics->DrawLine( protoTransform->aPos, protoTransform->aPos + ( modelRight * r_proto_line_dist ), { 0.f, 1.f, 0.f } );
		// graphics->DrawLine( protoTransform->aPos, protoTransform->aPos + ( modelUp * r_proto_line_dist ), { 0.f, 0.f, 1.f } );
	}

	gProtoSystem.aUpdated.clear();
}


extern float Lerp_GetDuration( float max, float min, float current );
extern float Lerp_GetDuration( float max, float min, float current, float mult );

// inverted version of it
extern float Lerp_GetDurationIn( float max, float min, float current );
extern float Lerp_GetDurationIn( float max, float min, float current, float mult );

extern float Math_EaseOutExpo( float x );
extern float Math_EaseOutQuart( float x );
extern float Math_EaseInOutCubic( float x );


void TEST_SV_UpdateProtos( float frameTime )
{
#if CH_SERVER
	PROF_SCOPE();

	// Protogens have to have look targets, if there are no players, don't bother executing this code
	// As no player will even see the protogens looking around
	if ( gServerData.aClients.empty() )
		return;

	// ?????
	float protoScale = vrcmdl_scale;

	// TODO: maybe make this into some kind of "look at player" component? idk lol
	// also could thread this as a test
	for ( auto& proto : gProtoSystem.aEntities )
	{
		ProtoLookData_t& protoLook     = gProtoSystem.aLookMap[ proto ];
		bool             targetChanged = false;

		protoLook.aTimeToDuelCur += frameTime;

		if ( protoLook.aTimeToDuelCur > protoLook.aTimeToDuel )
		{
			if ( proto_look_at_entities )
			{
				// Reset it back to 0 to enter the while loop of not picking the world or itself
				protoLook.aLookTarget = 0;

				// Don't look at yourself or the world, only pick the world if there are no other entities to pick
				if ( Entity_GetEntityCount() > 2 )
				{
					while ( protoLook.aLookTarget == 0 || protoLook.aLookTarget == proto )
						protoLook.aLookTarget = rand_int( 0, Entity_GetEntityCount() - 1 );
				}
			}
			else
			{
				auto randIt           = std::next( std::begin( gServerData.aClientIDs ), rand_int( 0, gServerData.aClientIDs.size() - 1 ) );
				protoLook.aLookTarget = SV_GetPlayerEnt( randIt->first );

				if ( protoLook.aLookTarget == CH_ENT_INVALID )
					continue;
			}

			protoLook.aTimeToDuel    = rand_float( proto_swap_target_sec_min, proto_swap_target_sec_max );
			protoLook.aTimeToDuelCur = 0.f;
			targetChanged            = true;
		}

		auto playerTransform = Ent_GetComponent< CTransform >( protoLook.aLookTarget, "transform" );

		// CH_ASSERT( playerTransform );

		if ( !playerTransform )
		{
			// Find something else to try to look at
			protoLook.aTimeToDuel = 0.f;
			continue;
		}

		auto protoTransform = Ent_GetComponent< CTransform >( proto, "transform" );

		// glm::length( renderable->aModelMatrix ) == 0.f

		// TESTING: BROKEN
		if ( proto_look == 2.f )
		{
			glm::vec3 forward, right, up;
			//AngleToVectors( protoTransform.aAng, forward, right, up );
			// Util_GetDirectionVectors( playerTransform.aAng, &forward, &right, &up );
			glm::mat4 playerMatrix;
			Util_ToMatrix( playerMatrix, playerTransform->aPos, playerTransform->aAng );
			Util_GetMatrixDirection( playerMatrix, &forward, &right, &up );

			// graphics->DrawLine( protoTransform.aPos, protoTransform.aPos + ( forward * r_proto_line_dist2 ), { 1.f, 0.f, 0.f } );
			// graphics->DrawLine( protoTransform.aPos, protoTransform.aPos + ( right * r_proto_line_dist2 ), { 0.f, 1.f, 0.f } );
			// graphics->DrawLine( protoTransform.aPos, protoTransform.aPos + ( up * r_proto_line_dist2 ), { 0.f, 0.f, 1.f } );

			glm::vec3 protoView    = protoTransform->aPos;
			//protoView.z += cl_view_height;

			glm::vec3 direction    = ( protoView - playerTransform->aPos.Get() );
			glm::vec3 rotationAxis = Util_VectorToAngles( direction, up );

			glm::mat4 protoViewMat = glm::lookAt( protoView, playerTransform->aPos.Get(), up );

			glm::vec3 vForward, vRight, vUp;
			Util_GetViewMatrixZDirection( protoViewMat, vForward, vRight, vUp );
		}
		else if ( proto_look && !Game_IsPaused() )
		{
			glm::vec3 up;
			Util_GetDirectionVectors( playerTransform->aAng, nullptr, nullptr, &up );

			glm::vec3 protoView    = protoTransform->aPos;
			glm::vec3 direction    = protoView - playerTransform->aPos.Get();
			glm::vec3 rotationAxis = Util_VectorToAngles( direction, up );

			if ( targetChanged )
			{
				protoLook.aStartAng    = protoTransform->aAng;
				protoLook.aTurnEndTime = rand_float( proto_swap_turn_time_min, proto_swap_turn_time_max );
				protoLook.aTurnCurTime = 0.f;

				// Make sure we don't suddenly want to look at something else while midway through turning to look at something
				// Though if you want this behavior, you would need to have them slow down looking, and slowly start to look at that
				// Like a turn velocity or something lol
				protoLook.aTurnEndTime = std::min( protoLook.aTurnEndTime, protoLook.aTimeToDuel );
			}

			// this is weird
			protoLook.aGoalAng          = rotationAxis;
			protoLook.aGoalAng[ PITCH ] = -rotationAxis[ PITCH ] + 90.f;
			protoLook.aGoalAng[ YAW ]   = 0.f;
			protoLook.aGoalAng[ ROLL ]  = rotationAxis[ YAW ] - 90.f;

			protoLook.aTurnCurTime += frameTime;

			if ( protoLook.aTurnEndTime >= protoLook.aTurnCurTime )
			{
				float time           = protoLook.aTurnCurTime / protoLook.aTurnEndTime;
				float timeCurve      = Math_EaseInOutCubic( time );
				protoTransform->aAng = glm::mix( protoLook.aStartAng, protoLook.aGoalAng, timeCurve );
			}
			else
			{
				protoTransform->aAng = protoLook.aGoalAng;
			}
		}

		if ( protoTransform->aScale.Get().x != protoScale )
		{
			protoTransform->aScale = glm::vec3( protoScale );
		}

		if ( proto_follow && !Game_IsPaused() )
		{
			glm::vec3 modelUp, modelRight, modelForward;

			glm::mat4 protoMatrix;
			Util_ToMatrix( protoMatrix, protoTransform->aPos, protoTransform->aAng, protoTransform->aScale );

			Util_GetMatrixDirection( protoMatrix, &modelForward, &modelRight, &modelUp );
		
			ProtoTurn_t& protoTurn = gProtoSystem.aTurnMap[ proto ];
		
			if ( protoTurn.aNextTime <= gCurTime )
			{
				protoTurn.aNextTime = gCurTime + ( rand() / ( RAND_MAX / 8.f ) );
				protoTurn.aLastRand = protoTurn.aRand;
				// protoTurn.aRand     = ( rand() / ( RAND_MAX / 40.0f ) ) - 20.f;
				protoTurn.aRand     = ( rand() / ( RAND_MAX / 360.0f ) ) - 180.f;
			}
		
			float randTurn       = glm::mix( protoTurn.aLastRand, protoTurn.aRand, protoTurn.aNextTime - gCurTime );
		
			protoTransform->aPos = protoTransform->aPos + ( modelUp * proto_follow_speed * gFrameTime );
		
			// protoTransform.aAng    = protoTransform.aAng + ( modelForward * randTurn * proto_follow_speed * gFrameTime );
			protoTransform->aAng = protoTransform->aAng + ( modelRight * randTurn * proto_follow_speed * gFrameTime );
		}
	}
#endif
}


CONVAR_FLOAT( snd_test_vol, 0.25, "" );
CONVAR_FLOAT( snd_test_falloff, 1.f, "" );


// constexpr const char* SND_TEST_PATH = "sound/endymion_mono.ogg";
constexpr const char* SND_TEST_PATH = "sound/fiery_no_ext.ogg";


static void cmd_sound_test( const std::string& srPath, bool sServer )
{
#if CH_CLIENT
	if ( !audio )
		return;

	ch_handle_t soundHandle = audio->OpenSound( srPath );

	if ( soundHandle == CH_INVALID_HANDLE )
		return;
#endif

#if CH_CLIENT
	auto playerTransform = Ent_GetComponent< CTransform >( gLocalPlayer, "transform" );
#else
	Entity player = SV_GetCommandClientEntity();

	if ( player == CH_ENT_INVALID )
		return;

	auto playerTransform = Ent_GetComponent< CTransform >( player, "transform" );
#endif

	if ( !playerTransform )
		return;

	Entity soundEnt = Entity_CreateEntity( !sServer );

	if ( soundEnt == CH_ENT_INVALID )
	{
		Log_Error( "Failed to create sound test entity\n" );
		return;
	}

	if ( sServer )
		gAudioTestEntitiesSv.push_back( soundEnt );
	else
		gAudioTestEntitiesCl.push_back( soundEnt );

	auto transform           = Ent_AddComponent< CTransform >( soundEnt, "transform" );
	auto sound               = Ent_AddComponent< CSound >( soundEnt, "sound" );

	transform->aPos          = playerTransform->aPos;
	transform->aAng          = playerTransform->aAng;

	// purely debugging
	transform->aScale.Edit() = { 20.f, 20.f, 20.f };

	sound->aPath             = srPath;
	sound->aVolume           = snd_test_vol;
	sound->aFalloff          = snd_test_falloff;
	sound->aStartPlayback    = true;

#if CH_CLIENT
	sound->aHandle           = soundHandle;
#endif

	sound->aEffects.Edit() |= AudioEffect_Loop | AudioEffect_World;
}


#if CH_CLIENT
CONCMD( snd_test_cl )
{
	cmd_sound_test( args.size() ? args[ 0 ] : SND_TEST_PATH, false );
}
#else
CONCMD_VA( snd_test_sv, CVARF( CL_EXEC ) )
{
	cmd_sound_test( args.size() ? args[ 0 ] : SND_TEST_PATH, true );
}
#endif


#if CH_CLIENT
CONCMD( snd_test_cl_clear )
{
	if ( !audio )
		return;

	for ( Entity entity : gAudioTestEntitiesCl )
	{
		CSound* sound = Ent_GetComponent< CSound >( entity, "sound" );

		if ( sound )
		{
			if ( audio->IsValid( sound->aHandle ) )
			{
				audio->FreeSound( sound->aHandle );
			}
		}

		Entity_DeleteEntity( entity );
	}

	gAudioTestEntitiesCl.clear();
}

#else

CONCMD_VA( snd_test_sv_clear, CVARF( CL_EXEC ) )
{
	for ( Entity entity : gAudioTestEntitiesSv )
	{
		Entity_DeleteEntity( entity );
	}

	gAudioTestEntitiesSv.clear();
}

#endif

