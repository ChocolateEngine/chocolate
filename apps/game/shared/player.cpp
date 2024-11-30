#include "main.h"
#include "player.h"
#include "core/util.h"

#include "igui.h"
#include "iinput.h"
#include "render/irender.h"
#include "igraphics.h"

#include "game_shared.h"
#include "game_physics.h"
#include "mapmanager.h"
#include "testing.h"
#include "entity/entity_systems.h"
#include "button_inputs.h"

#include "imgui/imgui.h"

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtx/compatibility.hpp>
#include <algorithm>
#include <cmath>


#if CH_CLIENT
#include "skybox.h"
#include "inputsystem.h"
#include "cl_main.h"
#else
#include "sv_main.h"
#endif


PlayerManager players;

#if CH_SERVER
static PlayerSpawnManager playerSpawn;
#else
extern u32 gMainViewportIndex;
#endif

constexpr float DEFAULT_SPEED = 6.34325f;

CONVAR_FLOAT( sv_sprint_mult, 2.4, CVARF_DEF_SERVER_REPLICATED, "Player Sprint Speed Multiplier" );
CONVAR_FLOAT( sv_duck_mult, 0.5, CVARF_DEF_SERVER_REPLICATED, "Player Duck Speed Multiplier" );

CONVAR_FLOAT( sv_forward_speed, DEFAULT_SPEED, CVARF_DEF_SERVER_REPLICATED, "Player Forward Speed" );
CONVAR_FLOAT( sv_side_speed, DEFAULT_SPEED, CVARF_DEF_SERVER_REPLICATED, "Player Side Speed" );  // 350.f
CONVAR_FLOAT( sv_max_speed, DEFAULT_SPEED, CVARF_DEF_SERVER_REPLICATED, "Player Max Speed" );    // 320.f

CONVAR_FLOAT( sv_accel_speed, 10, CVARF_DEF_SERVER_REPLICATED, "Player Acceleration" );
CONVAR_FLOAT( sv_accel_speed_air, 1, CVARF_DEF_SERVER_REPLICATED, "Player Acceleration When in the Air" );
CONVAR_FLOAT( sv_jump_force, DEFAULT_SPEED, CVARF_DEF_SERVER_REPLICATED, "Player Jump Power" );
CONVAR_FLOAT( sv_stop_speed, 2, CVARF_DEF_SERVER_REPLICATED, "Player Stop Speed" );

CONVAR_FLOAT( sv_friction, 8, CVARF_DEF_SERVER_REPLICATED, "Player Ground Friction" );  // 4.f
CONVAR_BOOL( sv_friction_enable, 1, CVARF_DEF_SERVER_REPLICATED );

CONVAR_FLOAT( phys_friction_player, 0.01, CVARF_DEF_SERVER_REPLICATED );

// lerp the friction maybe?
//CONVAR_FLOAT( sv_new_movement, 1 );
//CONVAR_FLOAT( sv_friction_new, 8 );  // 4.f

// CONVAR_FLOAT( sv_gravity, 800, CVARF_DEF_SERVER_REPLICATED );
CONVAR_FLOAT( phys_gravity_player, -( CH_GRAVITY_SPEED * 2.f ), CVARF_DEF_SERVER_REPLICATED, "Player Gravity, Different from phys_gravity" );

CONVAR_FLOAT( cl_stepspeed, 8 );
CONVAR_FLOAT( cl_steptime, 0.25 );
CONVAR_FLOAT( cl_stepduration, 0.22 );

CONVAR_FLOAT( sv_view_height, 1.7, CVARF_DEF_SERVER_REPLICATED, "View Height in Meters" );              // 67
CONVAR_FLOAT( sv_view_height_duck, 0.888, CVARF_DEF_SERVER_REPLICATED, "View Duck Height in Meters" );  // 35
CONVAR_FLOAT( sv_view_height_lerp, 15, CVARF_DEF_SERVER_REPLICATED );                                   // 0.015

CONVAR_FLOAT( player_model_scale, 0.025373, "yeah" );

CONVAR_BOOL( cl_thirdperson, 0 );
CONVAR_BOOL( cl_playermodel_enable, 1 );
CONVAR_BOOL( cl_playermodel_shadow_local, 1 );
CONVAR_BOOL( cl_playermodel_shadow, 0 );
CONVAR_BOOL( cl_playermodel_cam_ang, 1 );

CONVAR_VEC3( cl_cam_offset, 0, 0, -3.f );

CONVAR_FLOAT( cl_show_player_stats, 0 );

CONVAR_BOOL( phys_dbg_player, 0 );

CONVAR_FLOAT( r_fov, 106.f, CVARF_ARCHIVE, "Player FOV" );
CONVAR_FLOAT( r_nearz, 0.01f, "Player Near Z" );
CONVAR_FLOAT( r_farz, 1000.f, "Player Far Z" );

CONVAR_FLOAT( cl_zoom_fov, 40 );
CONVAR_FLOAT( cl_zoom_duration, 0.4 );

CONVAR_FLOAT( cl_duck_time, 0.4 );

CONVAR_BOOL( sv_land_smoothing, 1 );
CONVAR_FLOAT( sv_land_max_speed, 30 );
CONVAR_FLOAT( sv_land_vel_scale, 1 );  // 0.01
CONVAR_FLOAT( sv_land_power_scale, 2 );  // 0.01
CONVAR_FLOAT( sv_land_time_scale, 2 );

CONVAR_BOOL( cl_bob_enabled, 1 );
CONVAR_FLOAT( cl_bob_magnitude, 0.5 );
CONVAR_FLOAT( cl_bob_freq, 45 );
CONVAR_FLOAT( cl_bob_speed_scale, 0.013 );
CONVAR_FLOAT( cl_bob_exit_lerp, 0.1 );
CONVAR_FLOAT( cl_bob_exit_threshold, 0.1 );
CONVAR_FLOAT( cl_bob_sound_threshold, 0.1 );
CONVAR_FLOAT( cl_bob_offset, 0.25 );
CONVAR_FLOAT( cl_bob_time_offset, -0.6 );
CONVAR_BOOL( cl_bob_debug, 0 );

CONVAR_FLOAT( r_flashlight_brightness, 10.f );
CONVAR_BOOL( r_flashlight_lock, 0 );

CONVAR_VEC3( r_flashlight_offset, -0.1f, -0.1f, -0.1f );
CONVAR_VEC3( r_flashlight_color, 1, 1, 1 );

// CONVAR_FLOAT_EXT( m_yaw );
// CONVAR_FLOAT_EXT( m_pitch );
extern const float &m_yaw, &m_pitch;

// 2.069860757751118
constexpr float         PLAYER_MASS            = 10.503715f;


static IPhysicsShape* gPlayerShapeStanding   = nullptr;
static IPhysicsShape* gPlayerShapeCrouch     = nullptr;

// constexpr u32         gPlayerPhysHeight      = 72;
// constexpr u32         gPlayerPhysHeightDuck  = 40;
// constexpr u32         gPlayerPhysHeightDuck2 = 32;  // why ?????

constexpr float         gPlayerPhysHeight      = 1.826f;
constexpr float         gPlayerPhysHeightDuck  = 1.0149f;
constexpr float         gPlayerPhysHeightDuck2 = 0.8119f;  // why ?????


#if CH_SERVER
CONCMD_VA( respawn, CVARF( CL_EXEC ) )
{
	Entity player = SV_GetCommandClientEntity();

	players.Respawn( player );
}


CONCMD_VA( reset_velocity, CVARF( CL_EXEC ) )
{
	Entity player  = SV_GetCommandClientEntity();
	auto rigidBody = GetRigidBody( player );

	if ( !rigidBody )
		return;

	rigidBody->aVel.Edit() = { 0, 0, 0 };
}
#endif


static void CmdSetPlayerMoveType( Entity sPlayer, EPlayerMoveType sMoveType )
{
	// auto& move = Entity_GetComponent< CPlayerMoveData >( sPlayer );
	// SetMoveType( move, sMoveType );

	auto move = GetPlayerMoveData( sPlayer );

	if ( !move )
	{
		Log_Error( "Failed to find player move data component\n" );
		return;
	}

	if ( !players.SetCurrentPlayer( sPlayer ) )
		return;

	// Toggle between the desired move type and walking
	if ( move->aMoveType == sMoveType )
		players.apMove->SetMoveType( *move, EPlayerMoveType_Walk );
	else
		players.apMove->SetMoveType( *move, sMoveType );
}



#if CH_SERVER
CONCMD_VA( noclip, CVARF( CL_EXEC ) )
{
	Entity player = SV_GetCommandClientEntity();

	if ( !player )
		return;

	CmdSetPlayerMoveType( player, EPlayerMoveType_NoClip );
}


CONCMD_VA( fly, CVARF( CL_EXEC ) )
{
	Entity player = SV_GetCommandClientEntity();

	if ( !player )
		return;

	CmdSetPlayerMoveType( player, EPlayerMoveType_Fly );
}
#endif


// glm::normalize doesn't return a float
// move to util.cpp?
float vec3_norm(glm::vec3& v)
{
	float length = sqrt(glm::dot(v, v));

	if (length)
		v *= 1/length;

	return length;
}


// ============================================================


PlayerManager::PlayerManager()
{
	apMove = new PlayerMovement;
}

PlayerManager::~PlayerManager()
{
	if ( apMove )
		delete apMove;

	apMove = nullptr;
}


CH_STRUCT_REGISTER_COMPONENT( CPlayerMoveData, playerMoveData, EEntComponentNetType_Both, ECompRegFlag_None )
{
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_S32, EPlayerMoveType, aMoveType, moveType, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_U8, unsigned char, aPlayerFlags, playerFlags, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_U8, unsigned char, aPrevPlayerFlags, prevPlayerFlags, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aMaxSpeed, maxSpeed, ECompRegFlag_None );

	// View Bobbing
	//CH_REGISTER_COMPONENT_VAR( EEntNetField_Float, float, aWalkTime, walkTime, true );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aBobOffsetAmount, bobOffsetAmount, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aPrevViewTilt, prevViewTilt, ECompRegFlag_None );

	// Smooth Land
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aLandPower, landPower, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aLandTime, landTime, ECompRegFlag_None );

	// Smooth Duck
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aPrevViewHeight, prevViewHeight, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aTargetViewHeight, targetViewHeight, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aOutViewHeight, outViewHeight, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aDuckDuration, duckDuration, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aDuckTime, duckTime, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aLastStepTime, lastStepTime, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Vec3, glm::vec3, aPrevVel, prevVel, ECompRegFlag_None );
}


CH_STRUCT_REGISTER_COMPONENT( CPlayerInfo, playerInfo, EEntComponentNetType_Both, ECompRegFlag_None )
{
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Entity, Entity, aCamera, camera, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Entity, Entity, aFlashlight, flashlight, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_SYS2( PlayerManager, players );
}


#if CH_SERVER
CH_STRUCT_REGISTER_COMPONENT( CPlayerSpawn, playerSpawn, EEntComponentNetType_Server, ECompRegFlag_None )
{
	CH_REGISTER_COMPONENT_SYS2( PlayerSpawnManager, playerSpawn );
}
#endif


void PlayerManager::RegisterComponents()
{
	// CH_REGISTER_COMPONENT_FL( CPlayerInfo, playerInfo, EEntComponentNetType_Both, ECompRegFlag_None );
	// // CH_REGISTER_COMPONENT_VAR( CPlayerInfo, std::string, name, name );
	// CH_REGISTER_COMPONENT_VAR_EX( CPlayerInfo, EEntNetField_Entity, Entity, aCamera, camera, ECompRegFlag_None );
	// CH_REGISTER_COMPONENT_VAR_EX( CPlayerInfo, EEntNetField_Entity, Entity, aFlashlight, flashlight, ECompRegFlag_None );
	// CH_REGISTER_COMPONENT_VAR( CPlayerInfo, bool, aIsLocalPlayer, isLocalPlayer, ECompRegFlag_None );  // don't mess with this

	CH_REGISTER_COMPONENT_RW( CPlayerZoom, playerZoom, ECompRegFlag_DontOverrideClient );
	CH_REGISTER_COMPONENT_VAR( CPlayerZoom, float, aOrigFov, origFov, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR( CPlayerZoom, float, aNewFov, newFov, ECompRegFlag_None );
	// CH_REGISTER_COMPONENT_VAR( CPlayerZoom, float, aZoomChangeFov, zoomChangeFov );
	// CH_REGISTER_COMPONENT_VAR( CPlayerZoom, float, aZoomTime, zoomTime );
	// CH_REGISTER_COMPONENT_VAR( CPlayerZoom, float, aZoomDuration, zoomDuration );
	CH_REGISTER_COMPONENT_VAR( CPlayerZoom, bool, aWasZoomed, wasZoomed, ECompRegFlag_None );

	// Entity_RegisterComponent< Model* >();
	// Entity_RegisterComponent< Model >();
	//Entity_RegisterComponent<PhysicsObject*>();
}


void PlayerManager::ComponentAdded( Entity sEntity, void* spData )
{
	Create( sEntity );
}


void PlayerManager::ComponentUpdated( Entity sEntity, void* spData )
{
	CPlayerInfo* playerInfo = static_cast< CPlayerInfo* >( spData );

	if ( playerInfo->aCamera != CH_ENT_INVALID )
	{
#if CH_SERVER
		// Parent the Entities to the Player Entity
		Entity_ParentEntity( playerInfo->aCamera, sEntity );
#else
		Entity_SetComponentPredicted( playerInfo->aCamera, "transform", true );
		Entity_SetComponentPredicted( playerInfo->aCamera, "direction", true );
		Entity_SetComponentPredicted( playerInfo->aCamera, "camera", true );
#endif
	}
}


bool PlayerManager::SetCurrentPlayer( Entity player )
{
	CH_ASSERT( apMove );

	apMove->aPlayer = player;

#if CH_CLIENT
	apMove->apUserCmd = &gClientUserCmd;
#else
	SV_Client_t* client = SV_GetClientFromEntity( player );
	if ( !client )
		return false;

	apMove->apUserCmd = &client->aUserCmd;
#endif

	CPlayerInfo* playerInfo = GetPlayerInfo( player );

	if ( !playerInfo )
	{
		Log_Error( "playerInfo component not found on player entity\n" );
		return false;
	}

	CH_ASSERT( playerInfo );
	CH_ASSERT( playerInfo->aCamera );

	apMove->apCamTransform = GetTransform( playerInfo->aCamera );
	apMove->apCamDir       = GetComp_Direction( playerInfo->aCamera );
	apMove->apCamera       = GetCamera( playerInfo->aCamera );

	apMove->apDir          = Ent_GetComponent< CDirection >( player, "direction" );
	apMove->apRigidBody    = GetRigidBody( player );
	apMove->apTransform    = GetTransform( player );
	apMove->apCharacter    = apMove->apCharacter;

	CH_ASSERT( apMove->apDir );
	CH_ASSERT( apMove->apRigidBody );
	CH_ASSERT( apMove->apTransform );
	CH_ASSERT( apMove->apCharacter );

	CH_ASSERT( apMove->apCamTransform );
	CH_ASSERT( apMove->apCamDir );
	CH_ASSERT( apMove->apCamera );

	return true;
}


void PlayerManager::Init()
{
	// apMove = Entity_RegisterSystem<PlayerMovement>();
}


void PlayerManager::Create( Entity player )
{
	CPlayerInfo* playerInfo = Ent_GetComponent< CPlayerInfo >( player, "playerInfo" );
	CH_ASSERT( playerInfo );

#if CH_CLIENT
	playerInfo->aIsLocalPlayer = player == gLocalPlayer;
	Log_Msg( "Client Creating Local Player\n" );
#endif

#if CH_SERVER

	SV_Client_t* client = SV_GetClientFromEntity( player );

	CH_ASSERT( client );

	if ( !client )
		return;

	// ------------------------------------------------------------------------------
	// Setup Player Entity Components

	auto playerMove = Ent_AddComponent< CPlayerMoveData >( player, "playerMoveData" );

	Ent_AddComponent( player, "rigidBody" );
	Ent_AddComponent( player, "direction" );

	auto renderable   = Ent_AddComponent< CRenderable >( player, "renderable" );
	renderable->aPath = DEFAULT_PROTOGEN_PATH;

	auto flashlight   = Ent_AddComponent< CLight >( player, "light" );
	auto zoom         = Ent_AddComponent< CPlayerZoom >( player, "playerZoom" );
	auto transform    = Ent_AddComponent< CTransform >( player, "transform" );
	auto health       = Ent_AddComponent< CHealth >( player, "health" );
	//auto suit         = Ent_AddComponent< CSuit >( player, "suit" );

	CH_ASSERT( flashlight );
	CH_ASSERT( zoom );
	CH_ASSERT( transform );
	CH_ASSERT( health );
	//CH_ASSERT( suit );

	health->aHealth = 100;

	Log_MsgF( "Server Creating Player Entity: \"%s\"\n", client->name.c_str() );

	// Lets create local entities for the camera and the flashlight, so they have their own unique transform in local space
	// And we parent them to the player
	playerInfo->aCamera = Entity_CreateEntity();
	// playerInfo->aFlashlight = Entity_CreateEntity( true );

	// Parent the Entities to the Player Entity
	Entity_ParentEntity( playerInfo->aCamera, player );
	// Entity_ParentEntity( playerInfo->aFlashlight, player );

	Ent_AddComponent( playerInfo->aCamera, "transform" );
	Ent_AddComponent( playerInfo->aCamera, "direction" );

	CCamera* camera = Ent_AddComponent< CCamera >( playerInfo->aCamera, "camera" );
	camera->aFov    = r_fov;

	zoom->aOrigFov             = r_fov;
	zoom->aNewFov              = r_fov;

	// ------------------------------------------------------------------------------
	// Setup Flashlight

	flashlight->aEnabled       = false;
	flashlight->aType          = ELightType_Cone;
	flashlight->aInnerFov      = 0.f;
	flashlight->aOuterFov      = 45.f;
	// flashlight->color    = { r_flashlight_brightness, r_flashlight_brightness, r_flashlight_brightness };
	flashlight->color.Edit()  = { r_flashlight_color.x, r_flashlight_color.y, r_flashlight_color.z, r_flashlight_brightness };

	// ------------------------------------------------------------------------------
	// Setup Physics Shapes

	PhysicsShapeInfo charShapeInfo( PhysShapeType::Cylinder );
	charShapeInfo.aBounds                  = { gPlayerPhysHeight, 0.377, 1 };

	gPlayerShapeStanding  = GetPhysEnv()->CreateShape( charShapeInfo );

	PhysicsShapeInfo duckShapeInfo( PhysShapeType::Cylinder );
	duckShapeInfo.aBounds = { gPlayerPhysHeightDuck, 0.377, 1 };

	gPlayerShapeCrouch    = GetPhysEnv()->CreateShape( duckShapeInfo );

	// ------------------------------------------------------------------------------
	// Setup Virtual Character Physics Object

	PhysVirtualCharacterSettings charSettings{};
	charSettings.shape                     = gPlayerShapeStanding;
	charSettings.up                        = vec_up;
	charSettings.mass                      = PLAYER_MASS;
	charSettings.maxSlopeAngle             = 35;
	//charSettings.predictiveContactDistance = 20.f;
	//charSettings.characterPadding    = 0.2f;
	//charSettings.collisionTolerance  = 0.2f;

	// charSettings.maxCollisionIterations    = 25;
	// charSettings.maxConstraintIterations   = 50;

	IPhysVirtualCharacter* character = GetPhysEnv()->CreateVirtualCharacter( charSettings );

	playerMove->apCharacter          = character;

	character->SetRotation( AngToQuat( glm::radians( glm::vec3( 90.f, 0.f, 0.f ) ) ) );
	character->SetShapeOffset( { 0, gPlayerPhysHeight / 2.f, 0 } );

	// auto compPhysShape        = Ent_AddComponent< CPhysShape >( player, "physShape" );
	// compPhysShape->aShapeType = PhysShapeType::Cylinder;
	// compPhysShape->aBounds    = glm::vec3( 72, 16, 1 );
	// 
	// Phys_CreatePhysShapeComponent( compPhysShape );
	// 
	// IPhysicsShape* physShape = compPhysShape->apShape;
	// 
	// CH_ASSERT( physShape );
	// 
	// PhysicsObjectInfo physInfo;
	// physInfo.aMotionType = PhysMotionType::Dynamic;
	// physInfo.aPos        = transform->aPos;
	// physInfo.aAng        = transform->aAng;
	// 
	// physInfo.aCustomMass = true;
	// physInfo.aMass       = PLAYER_MASS;
	// 
	// CPhysObject* physObj = Phys_CreateObject( player, physShape, physInfo );
	// 
	// CH_ASSERT( physObj );
#endif
}


void PlayerManager::Spawn( Entity player )
{
	apMove->OnPlayerSpawn( player );

	Respawn( player );
}


void PlayerManager::Respawn( Entity player )
{
	CPlayerInfo* playerInfo = GetPlayerInfo( player );

	if ( !playerInfo )
	{
		Log_Error( "playerInfo component not found on player entity\n" );
		return;
	}

	auto         rigidBody  = GetRigidBody( player );
	auto         transform  = GetTransform( player );
	auto         zoom       = GetPlayerZoom( player );
	auto         playerMove = GetPlayerMoveData( player );

	CH_ASSERT( playerMove );
	CH_ASSERT( playerInfo );
	CH_ASSERT( playerInfo->aCamera );

	auto camTransform = GetTransform( playerInfo->aCamera );

	CH_ASSERT( rigidBody );
	CH_ASSERT( transform );
	CH_ASSERT( camTransform );
	CH_ASSERT( zoom );

#if CH_SERVER
	Transform playerSpawnSpot = playerSpawn.SelectSpawnTransform();

	transform->aPos               = playerSpawnSpot.aPos;
	transform->aAng.Edit()        = { 0, playerSpawnSpot.aAng.y, 0 };
	rigidBody->aVel.Edit()        = { 0, 0, 0 };
	rigidBody->aAccel.Edit()      = { 0, 0, 0 };

	camTransform->aAng.Edit()     = playerSpawnSpot.aAng;
#endif

	zoom->aOrigFov                = r_fov;
	zoom->aNewFov                 = r_fov;

	// physObjComp->aTransformMode   = EPhysTransformMode_None;

	playerMove->apCharacter->SetLinearVelocity( { 0, 0, 0 } );

#if 0
	physObj->SetAllowSleeping( false );
	physObj->SetMotionQuality( PhysMotionQuality::LinearCast );
	physObj->SetLinearVelocity( { 0, 0, 0 } );
	physObj->SetAngularVelocity( { 0, 0, 0 } );

	Phys_SetMaxVelocities( physObj );

	physObj->SetFriction( phys_friction_player );

	// Don't allow any rotation on this
	physObj->SetInverseMass( 1.f / PLAYER_MASS );
	physObj->SetInverseInertia( { 0, 0, 0 }, { 1, 0, 0, 0 } );

	// rotate 90 degrees
	// physObj->SetAng( { 90, transform->aAng.Get().y, 0 } );
	physObj->SetAng( { 90, 0, 0 } );
#endif

	apMove->OnPlayerRespawn( player );
}


void Player_UpdateFlashlight( Entity player, bool sToggle )
{
	PROF_SCOPE();

	CPlayerInfo* playerInfo = GetPlayerInfo( player );

	if ( !playerInfo )
	{
		Log_Error( "playerInfo component not found on player entity\n" );
		return;
	}

	CH_ASSERT( playerInfo->aCamera );

	CTransform* transform    = GetTransform( player );
	CLight*     flashlight   = Ent_GetComponent< CLight >( player, "light" );

	CTransform* camTransform = GetTransform( playerInfo->aCamera );
	auto        camDir       = Ent_GetComponent< CDirection >( playerInfo->aCamera, "direction" );

	CH_ASSERT( transform );
	CH_ASSERT( camTransform );
	CH_ASSERT( camDir );
	CH_ASSERT( flashlight );

	auto UpdateTransform = [ & ]()
	{
		// weird stuff to get the angle of the light correct

		glm::vec3 ang{};
		ang.x                   = -camTransform->aAng.Get().x + 90.f;
		ang.z                   = -camTransform->aAng.Get().y;

		// HACK HACK
		glm::mat4 flashlightMat = Util_ToMatrix( nullptr, &ang );
		flashlight->aRot.Edit() = Util_GetMatrixRotation( flashlightMat );

		flashlight->aPos        = transform->aPos.Get() + camTransform->aPos.Get();

		flashlight->aPos += r_flashlight_offset * camDir->aUp.Get();
	};

	// Toggle flashlight on or off
	if ( sToggle )
	{
		flashlight->aEnabled = !flashlight->aEnabled;

		if ( !flashlight->aEnabled )
		{
			// graphics->UpdateLight( flashlight );
		}
		else
		{
			UpdateTransform();
		}
	}

	if ( flashlight->aEnabled )
	{
		// flashlight->color = { r_flashlight_brightness, r_flashlight_brightness, r_flashlight_brightness };
		flashlight->color.Edit() = { r_flashlight_color.x, r_flashlight_color.y, r_flashlight_color.z, r_flashlight_brightness };

		if ( !r_flashlight_lock )
		{
			UpdateTransform();
		}

		// graphics->UpdateLight( flashlight );
	}
}


inline float DegreeConstrain( float num )
{
	num = std::fmod(num, 360.0f);
	return (num < 0.0f) ? num += 360.0f : num;
}


inline void ClampAngles( CTransform* spTransform )
{
	if ( !spTransform )
		return;

	spTransform->aAng.Edit()[ YAW ]   = DegreeConstrain( spTransform->aAng.Get()[ YAW ] );
	spTransform->aAng.Edit()[ PITCH ] = std::clamp( spTransform->aAng.Get()[ PITCH ], -90.0f, 90.0f );
};


void PlayerManager::Update( float frameTime )
{
	PROF_SCOPE();

#if CH_SERVER
	for ( Entity player: aEntities )
	{
		SV_Client_t* client = SV_GetClientFromEntity( player );
		if ( !client )
			continue;

		PROF_SCOPE_NAMED( "Player" );
		CH_PROF_ZONE_TEXT( client->name.c_str(), client->name.size() );

		UserCmd_t& userCmd    = client->aUserCmd;

		auto playerInfo = GetPlayerInfo( player );
		// auto camera     = GetCamera( player );

		CH_ASSERT( playerInfo );
		CH_ASSERT( playerInfo->aCamera );

		if ( !Game_IsPaused() )
		{
			// Update Client UserCmd
			auto transform    = GetTransform( player );
			auto camTransform = GetTransform( playerInfo->aCamera );

			CH_ASSERT( transform );
			CH_ASSERT( camTransform );

			// transform.aAng[PITCH] = -mouse.y;
			camTransform->aAng.Edit()[ PITCH ] = userCmd.aAng[ PITCH ];
			camTransform->aAng.Edit()[ YAW ]   = userCmd.aAng[ YAW ];
			camTransform->aAng.Edit()[ ROLL ]  = userCmd.aAng[ ROLL ];

			transform->aAng.Set( { 0.f, DegreeConstrain( userCmd.aAng[ YAW ] ), 0.f } );

			ClampAngles( camTransform );

			apMove->MovePlayer( player, &userCmd );
			Player_UpdateFlashlight( player, userCmd.aFlashlight );
		}

		UpdateView( playerInfo, player );
	}
#endif
}


void PlayerManager::UpdateLocalPlayer()
{
	PROF_SCOPE();

#if CH_CLIENT
	CH_ASSERT( apMove );

	auto userCmd = gClientUserCmd;

	// We still need to do client updating of players actually, so

	for ( Entity player : aEntities )
	{
		PROF_SCOPE();

		auto playerInfo = GetPlayerInfo( player );
		CH_ASSERT( playerInfo );
		CH_ASSERT( playerInfo->aCamera );

		auto     playerMove = GetPlayerMoveData( player );
		auto     transform  = GetTransform( player );
		CLight*  flashlight = Ent_GetComponent< CLight >( player, "light" );

		auto     camTransform = GetTransform( playerInfo->aCamera );
		// auto     camera     = GetCamera( playerInfo->aCamera );

		CH_ASSERT( playerMove );
		CH_ASSERT( camTransform );
		//CH_ASSERT( camera );
		CH_ASSERT( transform );
		CH_ASSERT( flashlight );

		apMove->SetPlayer( player );

		// TODO: prediction
		// apMove->MovePlayer( gLocalPlayer, &userCmd );
	
		// Player_UpdateFlashlight( player, &userCmd );
	
		// TEMP
		camTransform->aPos.Edit()[ W_UP ] = playerMove->aOutViewHeight;

		// bool wasOnGround = playerMove->aPrevPlayerFlags & PlyOnGround && !( playerMove->aPlayerFlags & PlyOnGround );
		bool wasOnGround = playerMove->aPrevPlayerFlags & PlyOnGround;

		if ( playerMove->aMoveType == EPlayerMoveType_Walk )
		{
			apMove->DoSmoothLand( wasOnGround );
			apMove->DoViewBob();
			apMove->DoViewTilt();
		}
		else
		{
			// Reset View Tilt
			camTransform->aAng.Edit()[ ROLL ] = {};
			playerMove->aPrevViewTilt.Edit()  = {};
		}

		UpdateView( playerInfo, player );

		// if ( ( cl_thirdperson && cl_playermodel_enable ) || !playerInfo->aIsLocalPlayer )
		if ( cl_playermodel_enable && ( cl_thirdperson || !playerInfo->aIsLocalPlayer ) )
		{
			auto renderComp = Ent_GetComponent< CRenderable >( player, "renderable" );

			// I hate this so much
			if ( renderComp->aRenderable == CH_INVALID_HANDLE )
			{
				if ( renderComp->aPath.Get().empty() )
					continue;

				if ( renderComp->aModel == CH_INVALID_HANDLE )
				{
					renderComp->aModel = graphics->LoadModel( renderComp->aPath );
					if ( renderComp->aModel == CH_INVALID_HANDLE )
						continue;
				}

				renderComp->aRenderable = graphics->CreateRenderable( renderComp->aModel );
				if ( renderComp->aRenderable == CH_INVALID_HANDLE )
					continue;
			}
			
			Renderable_t* renderData = graphics->GetRenderableData( renderComp->aRenderable );

			if ( !renderData )
				continue;

			renderComp->aVisible = true;
			renderData->aVisible = true;

			if ( cl_playermodel_shadow )
				renderData->aCastShadow = cl_playermodel_shadow_local ? true : player != gLocalPlayer;
			else
				renderData->aCastShadow = false;

			float     scaleBase = player_model_scale;

			// This is to squish the model when the player crouches
			// TODO: HANDLE WHEN THE PLAYER IS IN THE AIR !!!! NETWORK THAT DATA !!!!
			float     scaleMult = playerMove->aOutViewHeight / sv_view_height;
			
			glm::vec3 scale( scaleBase, scaleBase, scaleBase );
			scale.z *= scaleMult;

			// HACK HACK
			glm::vec3 ang{};

			if ( cl_playermodel_cam_ang )
			{
				ang[ ROLL ]  = -camTransform->aAng.Get()[ YAW ];
				ang[ YAW ]   = 0.f;
				ang[ PITCH ] = camTransform->aAng.Get()[ PITCH ] * -1.f;
			}
			else
			{
				ang[ ROLL ]  = -transform->aAng.Get()[ YAW ];
				ang[ YAW ]   = 0.f;
				ang[ PITCH ] = transform->aAng.Get()[ PITCH ] * -1.f;
			}

			// Util_ToMatrix( renderData->aModelMatrix, transform->aPos, ang, scale );

			// Is something wrong with my Util_ToMatrix function?
			// Do i have applying scale and rotation in the wrong order?
			// If so, why is everything else using it seemingly working fine so far?

			renderData->aModelMatrix = glm::translate( transform->aPos.Get() );

			renderData->aModelMatrix *= glm::eulerAngleZYX(
			  glm::radians( ang.z ),
			  glm::radians( ang.y ),
			  glm::radians( ang.x ) );
			
			renderData->aModelMatrix = glm::scale( renderData->aModelMatrix, scale );

			graphics->UpdateRenderableAABB( renderComp->aRenderable );
		}
		else
		{
			// Make sure this thing is hidden
			auto renderComp = Ent_GetComponent< CRenderable >( player, "renderable" );
			if ( renderComp->aRenderable == CH_INVALID_HANDLE )
				continue;

			Renderable_t* renderData = graphics->GetRenderableData( renderComp->aRenderable );
			if ( !renderData )
				continue;

			renderComp->aVisible = false;
			renderData->aVisible = false;
		}
	}
#endif
}


void PlayerManager::DoMouseLook( Entity player )
{
#if CH_CLIENT
	CPlayerInfo* playerInfo = GetPlayerInfo( player );
	CH_ASSERT( playerInfo );
	CH_ASSERT( playerInfo->aCamera );

	if ( !playerInfo )
	{
		Log_Error( "playerInfo compoment not found when applying mouse look\n" );
		return;
	}

	CTransform*     camTransform = GetTransform( playerInfo->aCamera );
	CTransform*     transform    = GetTransform( player );

	const glm::vec2 mouse = Input_GetMouseDelta();

	// transform.aAng[PITCH] = -mouse.y;
	camTransform->aAng.Edit()[ PITCH ] += mouse.y * m_pitch;
	camTransform->aAng.Edit()[ YAW ] += mouse.x * m_yaw;
	transform->aAng.Edit()[ YAW ] += mouse.x * m_yaw;

	ClampAngles( camTransform );
	ClampAngles( transform );
#endif
}


// TODO: Move this to some generel math file or whatever

float Lerp_GetDuration( float max, float min, float current )
{
	return (current - min) / (max - min);
}

float Lerp_GetDuration( float max, float min, float current, float mult )
{
	return ((current - min) / (max - min)) * mult;
}

// inverted version of it
float Lerp_GetDurationIn( float max, float min, float current )
{
	return 1 - ((current - min) / (max - min));
}

float Lerp_GetDurationIn( float max, float min, float current, float mult )
{
	return (1 - ((current - min) / (max - min))) * mult;
}


float Math_EaseOutExpo( float x )
{
	return x == 1 ? 1 : 1 - pow( 2, -10 * x );
}

float Math_EaseOutQuart( float x )
{
	return 1 - pow( 1 - x, 4 );
}


void CalcZoom( CCamera* camera, Entity player )
{
	UserCmd_t* userCmd = nullptr;

#if CH_CLIENT
	if ( player == gLocalPlayer )
		userCmd = &gClientUserCmd;
#else
	SV_Client_t* client = SV_GetClientFromEntity( player );

	if ( !client )
		return;

	userCmd = &client->aUserCmd;

	CH_ASSERT( userCmd );
#endif

	CPlayerZoom* zoom = GetPlayerZoom( player );

	CH_ASSERT( zoom );

	// If we have a usercmd to process on either client or server, process it
#if 1
	if ( userCmd )
	{
		if ( zoom->aOrigFov != r_fov )
		{
			zoom->aZoomTime = 0.f;  // idk lol
			zoom->aOrigFov  = r_fov;
		}

		if ( Game_IsPaused() )
		{
			camera->aFov = zoom->aNewFov;
			return;
		}

		float lerpTarget = 0.f;

		if ( userCmd && userCmd->aButtons & EBtnInput_Zoom )
		{
			if ( !zoom->aWasZoomed )
			{
				zoom->aZoomChangeFov = camera->aFov;

				// scale duration by how far zoomed in we are compared to the target zoom level
				zoom->aZoomDuration  = Lerp_GetDurationIn( cl_zoom_fov, zoom->aOrigFov, camera->aFov, cl_zoom_duration );
				zoom->aZoomTime      = 0.f;
				zoom->aWasZoomed     = true;
			}

			lerpTarget = cl_zoom_fov;
		}
		else
		{
			if ( zoom->aWasZoomed || zoom->aZoomDuration == 0.f )
			{
				zoom->aZoomChangeFov = camera->aFov;

				// scale duration by how far zoomed in we are compared to the target zoom level
				zoom->aZoomDuration  = Lerp_GetDuration( cl_zoom_fov, zoom->aOrigFov, camera->aFov, cl_zoom_duration );
				zoom->aZoomTime      = 0.f;
				zoom->aWasZoomed     = false;
			}

			lerpTarget = zoom->aOrigFov;
		}

		zoom->aZoomTime += gFrameTime;

		if ( zoom->aZoomDuration >= zoom->aZoomTime )
		{
			float time = (zoom->aZoomTime / zoom->aZoomDuration);

			// smooth cosine lerp
			// float timeCurve = (( cos((zoomLerp * M_PI) - M_PI) ) + 1) * 0.5;
			float timeCurve = Math_EaseOutQuart( time );

			zoom->aNewFov = std::lerp( zoom->aZoomChangeFov, lerpTarget, timeCurve );
		}
	}
#else
	zoom->aOrigFov = r_fov;
	zoom->aNewFov  = r_fov;
#endif

#if CH_CLIENT
	if ( player == gLocalPlayer )
	{
		// scale mouse delta
		float fovScale = (zoom->aNewFov / zoom->aOrigFov);
		Input_SetMouseDeltaScale( { fovScale, fovScale } );
	}
#endif

	camera->aFov = zoom->aNewFov;
}


void PlayerManager::UpdateView( CPlayerInfo* info, Entity player )
{
	PROF_SCOPE();

	CH_ASSERT( info );
	CH_ASSERT( info->aCamera );

	CTransform* camTransform = GetTransform( info->aCamera );
	CTransform* transform    = GetTransform( player );
	auto        playerMove   = GetPlayerMoveData( player );

	auto        dir          = GetComp_Direction( player );

	auto        rigidBody    = GetRigidBody( player );
	auto        camera       = GetCamera( info->aCamera );
	auto        camDir       = GetComp_Direction( info->aCamera );

	CH_ASSERT( transform );

	ClampAngles( camTransform );

	if ( transform->aPos.aIsDirty || transform->aAng.aIsDirty )
	{
		glm::mat4 viewMatrixZ;
		Util_ToViewMatrixZ( viewMatrixZ, transform->aPos, transform->aAng );
		Util_GetViewMatrixZDirection( viewMatrixZ, dir->aForward.Edit(), dir->aRight.Edit(), dir->aUp.Edit() );
	}

	// MOVE ME ELSEWHERE IDK, MAYBE WHEN AN HEV SUIT COMPONENT IS MADE
	CalcZoom( camera, player );

	// TEMP UNTIL GetWorldMatrix works properly
	CTransform transformView = *transform;
	transformView.aPos += camTransform->aPos;
	transformView.aAng = camTransform->aAng;

	// Transform transformViewTmp = Entity_GetWorldTransform( info->aCamera );
	// Transform transformView    = transformViewTmp;

	//transformView.aAng[ PITCH ] = transformViewTmp.aAng[ YAW ];
	//transformView.aAng[ YAW ]   = transformViewTmp.aAng[ PITCH ];

	//transformView.aAng.Edit()[ YAW ] *= -1;

	//Transform transformView = transform;
	//transformView.aAng += move.aViewAngOffset;

	if ( cl_thirdperson )
	{
		Transform thirdPerson = {
			.aPos = cl_cam_offset
		};

		// thirdPerson.aPos = {cl_cam_x, cl_cam_y, cl_cam_z};

		glm::mat4 viewMatrixZ;
		Util_ToViewMatrixZ( viewMatrixZ, transformView.aPos, transformView.aAng );

		glm::mat4 viewMat = thirdPerson.ToMatrix( false ) * viewMatrixZ;

#if CH_CLIENT
		if ( info->aIsLocalPlayer )
		{
			ViewportShader_t* viewport = graphics->GetViewportData( gMainViewportIndex );

			if ( viewport )
				viewport->aViewPos = thirdPerson.aPos;

			// TODO: PERF: this also queues the viewport data
			Game_SetView( viewMat );
			// audio->SetListenerTransform( thirdPerson.aPos, transformView.aAng );
		}
#endif

		Util_GetViewMatrixZDirection( viewMat, camDir->aForward.Edit(), camDir->aRight.Edit(), camDir->aUp.Edit() );
	}
	else
	{
		glm::mat4 viewMat;
		Util_ToViewMatrixZ( viewMat, transformView.aPos, transformView.aAng );

#if CH_CLIENT
		if ( info->aIsLocalPlayer )
		{
			// wtf broken??
			// audio->SetListenerTransform( transformView.aPos, transformView.aAng );
			
			ViewportShader_t* viewport = graphics->GetViewportData( gMainViewportIndex );

			if ( viewport )
				viewport->aViewPos = transformView.aPos;

			// TODO: PERF: this also queues the viewport data
			Game_SetView( viewMat );
		}
#endif

		Util_GetViewMatrixZDirection( viewMat, camDir->aForward.Edit(), camDir->aRight.Edit(), camDir->aUp.Edit() );
	}

	// temp
	//graphics->DrawAxis( transformView.aPos, transformView.aAng, { 40.f, 40.f, 40.f } );
	//graphics->DrawAxis( transform->aPos, transform->aAng, { 40.f, 40.f, 40.f } );
	// graphics->DrawAxis( transform->aPos, {}, { 40.f, 40.f, 40.f } );

#if CH_SERVER
	glm::mat4 physTransform = playerMove->apCharacter->GetCenterOfMassTransform();

	glm::vec3 physPos       = Util_GetMatrixPosition( physTransform );

	graphics->DrawAxis( physPos, {}, { 40.f, 40.f, 40.f } );
#endif


#if CH_CLIENT
	if ( info->aIsLocalPlayer )
	// if ( player == gLocalPlayer )
	{
		// scale the nearz and farz
		gView.aFarZ  = r_farz;
		gView.aNearZ = r_nearz;
		gView.aFOV   = camera->aFov;
		Game_UpdateProjection();

		// i feel like there's gonna be a lot more here in the future...
		Skybox_SetAng( transformView.aAng );

		glm::vec3 listenerAng{
			transformView.aAng.Get().x,
			transformView.aAng.Get().z,
			transformView.aAng.Get().y,
		};

		audio->SetListenerTransform( transformView.aPos, listenerAng );
		audio->SetListenerVelocity( rigidBody->aVel );
		// audio->SetListenerOrient( camDir->aForward, camDir->aUp );
	}
#endif
}


// ============================================================


void PlayerMovement::EnsureUserCmd( Entity player )
{
#if CH_CLIENT
	apUserCmd = &gClientUserCmd;

#else
	apUserCmd           = nullptr;
	SV_Client_t* client = SV_GetClientFromEntity( player );

	if ( !client )
		return;

	apUserCmd = &client->aUserCmd;
#endif

	CH_ASSERT( apUserCmd );
}


void PlayerMovement::SetPlayer( Entity player )
{
	PROF_SCOPE();

	aPlayer     = player;

	CPlayerInfo* playerInfo = GetPlayerInfo( player );

	if ( !playerInfo )
	{
		Log_Error( "playerInfo component not found on player entity\n" );
		return;
	}

	CH_ASSERT( playerInfo );
	CH_ASSERT( playerInfo->aCamera );

	apCamTransform = GetTransform( playerInfo->aCamera );
	apCamDir       = GetComp_Direction( playerInfo->aCamera );
	apCamera       = GetCamera( playerInfo->aCamera );

	apMove         = GetPlayerMoveData( player );
	apRigidBody    = GetRigidBody( player );
	apTransform    = GetTransform( player );
	apDir          = Ent_GetComponent< CDirection >( player, "direction" );

#if CH_SERVER
	apCharacter = apMove->apCharacter;
	CH_ASSERT( apCharacter );
#endif

	CH_ASSERT( apCamTransform );
	CH_ASSERT( apCamDir );
	CH_ASSERT( apCamera );

	CH_ASSERT( apMove );
	CH_ASSERT( apRigidBody );
	CH_ASSERT( apTransform );
	CH_ASSERT( apDir );
}


void PlayerMovement::OnPlayerSpawn( Entity player )
{
	EnsureUserCmd( player );

	CPlayerInfo* playerInfo = GetPlayerInfo( player );
	CH_ASSERT( playerInfo );
	CH_ASSERT( playerInfo->aCamera );

	if ( !playerInfo )
	{
		Log_Error( "playerInfo compoment not found in OnPlayerSpawn\n" );
		return;
	}

	CTransform* camTransform = GetTransform( playerInfo->aCamera );
	auto        move         = GetPlayerMoveData( player );

	CH_ASSERT( move );
	CH_ASSERT( camTransform );

	SetMoveType( *move, EPlayerMoveType_Walk );

	camTransform->aPos.Edit() = { 0, 0, sv_view_height };
}


void PlayerMovement::OnPlayerRespawn( Entity player )
{
	EnsureUserCmd( player );

	auto move      = GetPlayerMoveData( player );
	auto transform = GetTransform( player );

	CH_ASSERT( move );
	CH_ASSERT( transform );

	//auto& physObj = Entity_GetComponent< PhysicsObject* >( player );

	move->apCharacter->SetPosition( transform->aPos );

	// Init Smooth Duck
	move->aTargetViewHeight = GetViewHeight();
	move->aOutViewHeight    = GetViewHeight();
}


void PlayerMovement::MovePlayer( Entity player, UserCmd_t* spUserCmd )
{
	PROF_SCOPE();

	SetPlayer( player );

	apUserCmd         = spUserCmd;

	apCharacter->SetAllowDebugDraw( phys_dbg_player );

	// update velocity
	apRigidBody->aVel = apCharacter->GetLinearVelocity();

	//apPhysObj->SetSleepingThresholds( 0, 0 );
	//apPhysObj->SetAngularFactor( 0 );
	//apPhysObj->SetAngularVelocity( {0, 0, 0} );
	// apCharacter->SetFriction( phys_friction_player );

	UpdateInputs();

	// Needed before smooth duck
	CalcOnGround();

	// TODO: what if we automatically duck when we jump

	if ( apMove->aMoveType == EPlayerMoveType_Walk )
	{
		// If we are now ducking
		if ( apMove->aPlayerFlags & PlyInDuck && !( apMove->aPrevPlayerFlags & PlyInDuck ) )
		{
			SetPhysicsShape( true );
		}
		else if ( !( apMove->aPlayerFlags & PlyInDuck ) && ( apMove->aPrevPlayerFlags & PlyInDuck ) )
		{
			SetPhysicsShape( false );
		}
	}

	// should be in WalkMove only, but i need this here when toggling noclip mid-duck
	DoSmoothDuck();

	DetermineMoveType();

	switch ( apMove->aMoveType )
	{
		case EPlayerMoveType_Walk:    WalkMove();     break;
		case EPlayerMoveType_Fly:     FlyMove();      break;
		case EPlayerMoveType_NoClip:  NoClipMove();   break;
	}

	// HACK FOR IMPACT SOUND ON CLIENT
	apMove->aPrevVel = apRigidBody->aVel;
}


void PlayerMovement::UpdatePhysicsShape()
{
}


void PlayerMovement::SetPhysicsShape( bool ducked )
{
	if ( ducked )
	{
		switch ( apMove->apCharacter->GetGroundState() )
		{
			// case EPhysGroundState_NotSupported:
			case EPhysGroundState_InAir:
				// offset player position so our view height stays the same
				glm::vec3 pos = apMove->apCharacter->GetPosition();
				pos.z += gPlayerPhysHeightDuck2;

				apMove->apCharacter->SetPosition( pos );
				apTransform->aPos = pos;
				break;
		}

		apMove->apCharacter->SetShapeOffset( { 0, gPlayerPhysHeightDuck / 2.f, 0 } );
		GetPhysEnv()->SetVirtualCharacterShape( apMove->apCharacter, gPlayerShapeCrouch, FLT_MAX );
	}
	else 
	{
		switch ( apMove->apCharacter->GetGroundState() )
		{
			// case EPhysGroundState_NotSupported:
			case EPhysGroundState_InAir:
				// offset player position so our view height stays the same
				glm::vec3 pos = apMove->apCharacter->GetPosition();
				pos.z -= gPlayerPhysHeightDuck2;

				apMove->apCharacter->SetPosition( pos );
				apTransform->aPos = pos;
				break;
		}

		apMove->apCharacter->SetShapeOffset( { 0, gPlayerPhysHeight / 2.f, 0 } );
		GetPhysEnv()->SetVirtualCharacterShape( apMove->apCharacter, gPlayerShapeStanding, FLT_MAX );
	}
}


float PlayerMovement::GetViewHeight()
{
	if ( !apUserCmd )
		return sv_view_height;

	return ( apUserCmd->aButtons & EBtnInput_Duck ) ? sv_view_height_duck : sv_view_height;
}


void PlayerMovement::DetermineMoveType()
{
	// TODO: MULTIPLAYER
	// if ( gHostMoveType != apMove->aMoveType )
	// {
	// 	apMove->aMoveType = gHostMoveType;
	// 	SetMoveType( *apMove, gHostMoveType );
	// }
}


void PlayerMovement::SetMoveType( CPlayerMoveData& move, EPlayerMoveType type )
{
	move.aMoveType = type;

	switch ( type )
	{
		case EPlayerMoveType_NoClip:
		case EPlayerMoveType_Fly:
		{
			EnableGravity( false );

			// Remove the OnGround flag
			move.aPlayerFlags.Edit() &= ~PlyOnGround;

			// Remove the Ground Entity
			apMove->apGroundObj = nullptr;

			break;
		}
	}

	switch (type)
	{
		case EPlayerMoveType_NoClip:
		{
			SetCollisionEnabled( false );
			break;
		}

		case EPlayerMoveType_Fly:
		{
			SetCollisionEnabled( true );
			break;
		}

		case EPlayerMoveType_Walk:
		{
			// update physics shape
			SetPhysicsShape( apMove->aPlayerFlags & PlyInDuck );
			EnableGravity( true );
			SetCollisionEnabled( true );
			break;
		}
	}
}


void PlayerMovement::SetCollisionEnabled( bool enable )
{
	//apPhysObjComp->aEnableCollision = enable;
	apCharacter->EnableCollision( enable );
}


void PlayerMovement::EnableGravity( bool enabled )
{
	//apPhysObjComp->aGravity = enabled;
	//apCharacter->SetGravityEnabled( enabled );
}

// ============================================================


#if CH_CLIENT
void PlayerMovement::DisplayPlayerStats( Entity player ) const
{
	if ( !cl_show_player_stats )
		return;

	CPlayerInfo* playerInfo = GetPlayerInfo( player );
	CH_ASSERT( playerInfo );

	if ( !playerInfo )
	{
		Log_ErrorF( "playerInfo compoment not found in %s\n", CH_FUNC_NAME_CLASS);
		return;
	}

	CH_ASSERT( playerInfo->aCamera );

	// auto& move = Entity_GetComponent< CPlayerMoveData >( player );
	auto rigidBody = GetRigidBody( player );
	auto transform = GetTransform( player );

	CTransform* camTransform = GetTransform( playerInfo->aCamera );
	CCamera*    camera       = GetCamera( playerInfo->aCamera );

	auto        flashlight   = Ent_GetComponent< CLight >( player, "light" );

	float speed        = glm::length( glm::vec2( rigidBody->aVel.Get().x, rigidBody->aVel.Get().y ) );

	gui->DebugMessage( "Player Pos:    %s", ch_vec3_to_str(transform->aPos).c_str() );
	gui->DebugMessage( "Player Ang:    %s", ch_vec3_to_str(transform->aAng).c_str() );
	gui->DebugMessage( "Player Vel:    %s", ch_vec3_to_str(rigidBody->aVel).c_str() );
	gui->DebugMessage( "Player Speed:  %.4f", speed );

	gui->DebugMessage( "Camera FOV:    %.4f", camera->aFov.Get() );
	gui->DebugMessage( "Camera Pos:    %s", ch_vec3_to_str(camTransform->aPos.Get()).c_str() );
	gui->DebugMessage( "Camera Ang:    %s", ch_vec3_to_str(camTransform->aAng.Get()).c_str() );

	gui->DebugMessage( "Flashlight:    %s", flashlight->aEnabled ? "Enabled" : "Disabled" );
}
#endif


void PlayerMovement::SetPos( const glm::vec3& origin )
{
	apTransform->aPos = origin;
}

const glm::vec3& PlayerMovement::GetPos() const
{
	return apTransform->aPos;
}

void PlayerMovement::SetAng( const glm::vec3& angles )
{
	apTransform->aAng = angles;
}

const glm::vec3& PlayerMovement::GetAng() const
{
	return apTransform->aAng;
}


void PlayerMovement::UpdateInputs()
{
	PROF_SCOPE();

	apRigidBody->aAccel.Set( { 0, 0, 0 } );

	float moveScale = 1.0f;

	apMove->aPrevPlayerFlags = apMove->aPlayerFlags;
	PlayerFlags newFlags     = PlyNone;

	if ( apUserCmd->aButtons & EBtnInput_Duck )
	{
		newFlags |= PlyInDuck;
		moveScale = sv_duck_mult;
	}

	else if ( apUserCmd->aButtons & EBtnInput_Sprint )
	{
		newFlags |= PlyInSprint;
		moveScale = sv_sprint_mult;
	}

	const float forwardSpeed = sv_forward_speed * moveScale;
	const float sideSpeed    = sv_side_speed * moveScale;
	apMove->aMaxSpeed        = sv_max_speed * moveScale;

	if ( apUserCmd->aButtons & EBtnInput_Forward ) apRigidBody->aAccel.Edit()[ W_FORWARD ] = forwardSpeed;
	if ( apUserCmd->aButtons & EBtnInput_Back )    apRigidBody->aAccel.Edit()[ W_FORWARD ] += -forwardSpeed;
	if ( apUserCmd->aButtons & EBtnInput_Left )    apRigidBody->aAccel.Edit()[ W_RIGHT ] = -sideSpeed;
	if ( apUserCmd->aButtons & EBtnInput_Right )   apRigidBody->aAccel.Edit()[ W_RIGHT ] += sideSpeed;

	if ( CalcOnGround() && apUserCmd->aButtons & EBtnInput_Jump )
	{
		apRigidBody->aVel.Edit()[ W_UP ] = sv_jump_force;
	}

	apMove->aPlayerFlags.Set( newFlags );
}


class PlayerCollisionCheck : public PhysCollisionCollector
{
  public:
	explicit PlayerCollisionCheck( const glm::vec3& srGravity, const glm::vec3& srVelocity, CPlayerMoveData* moveData ) :
		aGravity( srGravity ),
		aVelocity( srVelocity ),
		apMove( moveData )
	{
		apMove->apGroundObj = nullptr;
		// apMove->aGroundPosition = sPhysResult.aContactPointOn2;
		// apMove->aGroundNormal = normal;
	}

	void AddResult( const PhysCollisionResult& sPhysResult ) override
	{
		glm::vec3 normal = -glm::normalize( sPhysResult.aPenetrationAxis );

		float dot = glm::dot( normal, aGravity );
		if ( dot < aBestDot ) // Find the hit that is most opposite to the gravity
		{
			apMove->apGroundObj = sPhysResult.apPhysObj2;
			// mGroundBodySubShapeID = inResult.mSubShapeID2;
			apMove->aGroundPosition = sPhysResult.aContactPointOn2;
			apMove->aGroundNormal = normal;

			aBestDot = dot;
		}
	}

	glm::vec3 GetDirection()
	{
		return aVelocity;
	}

	CPlayerMoveData*        apMove;

private:
	glm::vec3               aGravity;
	glm::vec3               aVelocity;
	float                   aBestDot = 0.f;
};


CONVAR_FLOAT( phys_player_max_sep_dist, 1 );

// Post Physics Simulation Update
void PlayerMovement::UpdatePosition( Entity player )
{
	apMove        = GetPlayerMoveData( player );
	apTransform   = GetTransform( player );
	apRigidBody   = GetRigidBody( player );
	apCharacter   = apMove->apCharacter;

	CH_ASSERT( apMove );
	CH_ASSERT( apTransform );
	CH_ASSERT( apRigidBody );
	CH_ASSERT( apCharacter );

	//auto& physObj = Entity_GetComponent< PhysicsObject* >( player );

	//if ( aMoveType == MoveType::Fly )
		//aTransform = apPhysObj->GetWorldTransform();
	//transform.aPos = physObj->GetWorldTransform().aPos;
	apTransform->aPos = apCharacter->GetPosition();
	// apTransform->aPos.Edit().z -= phys_player_offset;

	if ( apMove->aMoveType != EPlayerMoveType_NoClip )
	{
		// PlayerCollisionCheck playerCollide( GetPhysEnv()->GetGravity(), apRigidBody->aVel, apMove );
		// apPhysObj->CheckCollision( phys_player_max_sep_dist, &playerCollide );
	}

	// um
	if ( apMove->aMoveType == EPlayerMoveType_Walk )
	{
		WalkMovePostPhys();
	}
}


float Math_EaseInOutCubic( float x )
{
	return x < 0.5 ? 4 * x * x * x : 1 - pow( -2 * x + 2, 3 ) / 2;
}


// VERY BUGGY STILL
void PlayerMovement::DoSmoothDuck()
{
	PROF_SCOPE();

	CPlayerInfo* playerInfo = GetPlayerInfo( aPlayer );
	CH_ASSERT( playerInfo );
	CH_ASSERT( playerInfo->aCamera );

	if ( !playerInfo )
	{
		Log_Error( "playerInfo compoment not found in OnPlayerSpawn\n" );
		return;
	}

	CTransform* camTransform = GetTransform( playerInfo->aCamera );

	if ( !camTransform )
	{
		Log_Error( "transform compoment not found on playerInfo->camera\n" );
		return;
	}

	if ( apMove->aMoveType != EPlayerMoveType_Walk )
	{
		apMove->aDuckTime += gFrameTime;

		if ( apMove->aDuckDuration >= apMove->aDuckTime )
		{
			float time             = ( apMove->aDuckTime / apMove->aDuckDuration );
			float timeCurve        = Math_EaseOutQuart( time );

			apMove->aOutViewHeight = std::lerp( apMove->aPrevViewHeight, apMove->aTargetViewHeight, timeCurve );
		}

		return;
	}
	
	EPhysGroundState groundState = apMove->apCharacter->GetGroundState();

	// no smooth duck in air, imagine your only moving your legs in the air, not your view
	if ( groundState == EPhysGroundState_InAir || groundState == EPhysGroundState_NotSupported )
	{
		apMove->aOutViewHeight            = GetViewHeight();
		camTransform->aPos.Edit()[ W_UP ] = apMove->aOutViewHeight;
		return;
	}

#if CH_SERVER
	// if ( IsOnGround() && apMove->aMoveType == EPlayerMoveType_Walk )
	if ( apMove->aMoveType == EPlayerMoveType_Walk )
	{
		if ( apMove->aTargetViewHeight != GetViewHeight() )
		{
			apMove->aPrevViewHeight = apMove->aOutViewHeight;
			apMove->aTargetViewHeight = GetViewHeight();
			apMove->aDuckTime = 0.f;

			apMove->aDuckDuration = Lerp_GetDuration( sv_view_height, sv_view_height_duck, apMove->aPrevViewHeight );

			// this is stupid
			if ( apMove->aTargetViewHeight == sv_view_height )
				apMove->aDuckDuration = 1 - apMove->aDuckDuration;

			apMove->aDuckDuration *= cl_duck_time;
		}
	}
	else // if ( WasOnGround() )
	{
		apMove->aPrevViewHeight = apMove->aOutViewHeight;
		apMove->aDuckDuration = Lerp_GetDuration( sv_view_height, sv_view_height_duck, apMove->aPrevViewHeight, cl_duck_time );
		apMove->aDuckTime = 0.f;
	}

	apMove->aDuckTime += gFrameTime;

	if ( apMove->aDuckDuration >= apMove->aDuckTime )
	{
		float time = (apMove->aDuckTime / apMove->aDuckDuration);
		float timeCurve = Math_EaseOutQuart( time );

		apMove->aOutViewHeight = std::lerp( apMove->aPrevViewHeight, apMove->aTargetViewHeight, timeCurve );
	}
#endif

	camTransform->aPos.Edit()[ W_UP ] = apMove->aOutViewHeight;
}


// The maximum angle of slope that character can still walk on
CONVAR_FLOAT( phys_player_max_slope_ang, 40, "The maximum angle of slope that character can still walk on" );


bool PlayerMovement::CalcOnGround( bool sSetFlag )
{
	PROF_SCOPE();

	EPhysGroundState groundState = apMove->apCharacter->GetGroundState();
	bool             onGround    = groundState == EPhysGroundState_OnGround;

	if ( sSetFlag )
	{
		if ( onGround )
			apMove->aPlayerFlags.Edit() |= PlyOnGround;
		else
			apMove->aPlayerFlags.Edit() &= ~PlyOnGround;
	}

	return onGround;

#if 0
	if ( apMove->aMoveType != EPlayerMoveType_Walk )
		return false;

	// static float maxSlopeAngle = cos( apMove->aMaxSlopeAngle );
	
	// The maximum angle of slope that character can still walk on (radians and put in cos())
	float maxSlopeAngle = cos( phys_player_max_slope_ang * (M_PI / 180.f) );

	if ( apMove->apGroundObj == nullptr )
		return false;

	glm::vec3 up       = -glm::normalize( GetPhysEnv()->GetGravity() );

	bool      onGround = ( glm::dot( apMove->aGroundNormal, up ) > maxSlopeAngle );

	if ( sSetFlag )
	{
		if ( onGround )
			apMove->aPlayerFlags.Edit() |= PlyOnGround;
		else
			apMove->aPlayerFlags.Edit() &= ~PlyOnGround;
	}

	return onGround;
#endif
}


bool PlayerMovement::IsOnGround()
{
	return apMove->aPlayerFlags & PlyOnGround;
}


bool PlayerMovement::WasOnGround()
{
	return apMove->aPrevPlayerFlags & PlyOnGround;
}


float PlayerMovement::GetMoveSpeed( glm::vec3 &wishdir, glm::vec3 &wishvel )
{
	wishdir = wishvel;
	float wishspeed = vec3_norm( wishdir );

	if ( wishspeed > apMove->aMaxSpeed )
	{
		wishvel = wishvel * apMove->aMaxSpeed.Get() / wishspeed;
		wishspeed = apMove->aMaxSpeed;
	}

	return wishspeed;
}


float PlayerMovement::GetMaxSpeed()
{
	return apMove->aMaxSpeed;
}


float PlayerMovement::GetMaxSpeedBase()
{
	return sv_max_speed;
}


float PlayerMovement::GetMaxSprintSpeed()
{
	return GetMaxSpeedBase() * sv_sprint_mult;
}


float PlayerMovement::GetMaxDuckSpeed()
{
	return GetMaxSpeedBase() * sv_duck_mult;
}


CONVAR_FLOAT( cl_step_sound_speed_vol, 0.001 );
CONVAR_FLOAT( cl_step_sound_speed_offset, 1 );
CONVAR_FLOAT( cl_step_sound_gravity_scale, 4 );
CONVAR_FLOAT( cl_step_sound_min_speed, 0.075 );
CONVAR_FLOAT( cl_step_sound, 1 );
CONVAR_FLOAT( cl_impact_sound, 1 );


#if CH_CLIENT
// old system
/*std::string PlayerMovement::PreloadStepSounds()
{
	char soundName[128];
	int soundIndex = (rand() / (RAND_MAX / 40.0f)) + 1;
	snprintf( soundName, 128, "sound/footsteps/running_dirt_%s%d.ogg", soundIndex < 10 ? "0" : "", soundIndex );

	return soundName;
}*/


ch_handle_t PlayerMovement::GetStepSound()
{
	char soundName[128];
	int soundIndex = (rand() / (RAND_MAX / 40.0f)) + 1;
	snprintf( soundName, 128, "sound/footsteps/running_dirt_%s%d.ogg", soundIndex < 10 ? "0" : "", soundIndex );

	// audio system should auto free it i think?
	if ( ch_handle_t stepSound = audio->OpenSound( soundName ) )
		// return audio->PreloadSound( stepSound ) ? stepSound : nullptr;
		return stepSound;

	return CH_INVALID_HANDLE;
}


void PlayerMovement::StopStepSound( bool force )
{
	/*if ( apMove->apStepSound && apMove->apStepSound->Valid() )
	{
		if ( game->aCurTime - apMove->aLastStepTime > cl_stepduration || force )
		{
			audio->FreeSound( apMove->apStepSound );
			//gui->DebugMessage( 7, "Freed Step Sound" );
		}
	}*/
}


void PlayerMovement::PlayStepSound()
{
	if ( !cl_step_sound )
		return;

	//float vel = glm::length( glm::vec2(aVelocity.x, aVelocity.y) ); 
	float vel         = glm::length( glm::vec3( apRigidBody->aVel.Get().x, apRigidBody->aVel.Get().y, apRigidBody->aVel.Get().z * cl_step_sound_gravity_scale ) ); 
	float speedFactor = glm::min( glm::log( vel * cl_step_sound_speed_vol + cl_step_sound_speed_offset ), 1.f );
	
	if ( speedFactor < cl_step_sound_min_speed )
		return;

	if ( gCurTime - apMove->aLastStepTime < cl_stepduration )
		return;

	StopStepSound( true );

	// if ( apMove->apStepSound = audio->LoadSound( GetStepSound().c_str() ) )
	if ( ch_handle_t stepSound = GetStepSound() )
	{
		audio->SetVolume( stepSound, speedFactor );
		audio->PlaySound( stepSound );

		// TODO: SPECTATING
		if ( aPlayer != gLocalPlayer )
		{
			audio->AddEffects( stepSound, AudioEffect_World );
			audio->SetEffectData( stepSound, EAudio_World_Pos, apTransform->aPos.Get() );
		}

		apMove->aLastStepTime = gCurTime;
	}
}


// really need to be able to play multiple impact sounds at a time
void PlayerMovement::StopImpactSound()
{
	/*if ( apMove->apImpactSound && apMove->apImpactSound->Valid() )
	{
		audio->FreeSound( apMove->apImpactSound );
	}*/
}


void PlayerMovement::PlayImpactSound()
{
	if ( !cl_impact_sound )
		return;

	//float vel = glm::length( glm::vec2(aVelocity.x, aVelocity.y) ); 
	float vel         = glm::length( glm::vec3( apMove->aPrevVel.Get().x, apMove->aPrevVel.Get().y, apMove->aPrevVel.Get().z * cl_step_sound_gravity_scale ) );
	float speedFactor = glm::min( glm::log( vel * cl_step_sound_speed_vol + cl_step_sound_speed_offset ), 1.f );

	if ( speedFactor < cl_step_sound_min_speed )
		return;

	StopImpactSound();

	/*if ( apMove->apImpactSound = audio->LoadSound(GetStepSound().c_str()) )
	{
		apMove->apImpactSound->vol = speedFactor;

		audio->PlaySound( apMove->apImpactSound );
	}*/

	if ( ch_handle_t impactSound = GetStepSound() )
	{
		audio->SetVolume( impactSound, speedFactor );
		audio->PlaySound( impactSound );
	}
}
#endif


void PlayerMovement::BaseFlyMove()
{
	glm::vec3 wishvel( 0, 0, 0 );
	glm::vec3 wishdir( 0, 0, 0 );

	// forward and side movement
	for ( int i = 0; i < 3; i++ )
		wishvel[ i ] = apCamDir->aForward.Get()[ i ] * apRigidBody->aAccel.Get().x + apCamDir->aRight.Get()[ i ] * apRigidBody->aAccel.Get()[ W_RIGHT ];

	float wishspeed = GetMoveSpeed( wishdir, wishvel );

	AddFriction();
	Accelerate( wishspeed, wishdir );
}


void PlayerMovement::NoClipMove()
{
	BaseFlyMove();
	apCharacter->SetLinearVelocity( apRigidBody->aVel );
}


void PlayerMovement::FlyMove()
{
	BaseFlyMove();
	apCharacter->SetLinearVelocity( apRigidBody->aVel );
}


CONVAR_FLOAT( cl_land_sound_threshold, 0.1 );


class PlayerStairsCheck : public PhysCollisionCollector
{
  public:
	explicit PlayerStairsCheck( const glm::vec3& srDir )
	{
		aDir = glm::normalize( srDir );
	}

	void AddResult( const PhysCollisionResult& srPhysResult ) override
	{
		glm::vec3 normal = -glm::normalize( srPhysResult.aPenetrationAxis );

		float     dot    = glm::dot( normal, aDir );
		if ( dot < aBestDot )  // Find the hit that is most opposite to the gravity
		{
			aBestResult = srPhysResult;
			aNormal     = normal;
			aBestDot    = dot;
		}
	}
	
	glm::vec3 GetDirection()
	{
		return aDir;
	}

	PhysCollisionResult aBestResult;
	glm::vec3           aNormal;

	glm::vec3           aDir;
	float               aBestDot = 0.f;
};


void PlayerMovement::WalkMove()
{
	PROF_SCOPE();

	glm::vec3 wishvel = apDir->aForward * apRigidBody->aAccel.Get().x + apDir->aRight * apRigidBody->aAccel.Get()[ W_RIGHT ];
	wishvel[W_UP] = 0.f;

	glm::vec3 wishdir(0,0,0);
	float wishspeed = GetMoveSpeed( wishdir, wishvel );

	bool wasOnGround = WasOnGround();
	bool onGround = CalcOnGround();

	if ( onGround )
	{
		// physics engine friction is kinda meh
		if ( sv_friction_enable )
			AddFriction();

		Accelerate( wishspeed, wishdir, false );
	}
	else
	{	// not on ground, so little effect on velocity
		Accelerate( wishspeed, wishvel, true );
	}

	// -------------------------------------------------
	// Try checking for stairs

	// PlayerStairsCheck playerStairsForward( { apRigidBody->aVel.x, apRigidBody->aVel.y, 0 } );
	// apPhysObj->CheckCollision( phys_player_max_sep_dist, &playerStairsForward );

	// if ( playerStairsForward.aBestResult.apPhysObj2 )
	// {
	// 	Log_Msg( "FOUND STAIRS\n" );
	// }

	// -------------------------------------------------

#if CH_CLIENT
	StopStepSound();

	if ( IsOnGround() && !onGround )
	{
		PlayImpactSound();

		//glm::vec2 vel( apRigidBody->aVel.x, apRigidBody->aVel.y );
		//if ( glm::length( vel ) < cl_land_sound_threshold )
		//	PlayStepSound();
	}
#endif

	DoSmoothLand( wasOnGround );

	// Manually apply gravity
	EPhysGroundState groundState = apCharacter->GetGroundState();

	if ( groundState != EPhysGroundState_OnGround )
	{
		// glm::vec3 gravity = GetPhysEnv()->GetGravity();
		glm::vec3 gravity( 0.f, 0.f, phys_gravity_player );
		apRigidBody->aVel += gravity * gFrameTime;
	}
	else
	{
		glm::vec3 newVel = apRigidBody->aVel;

		// ch_handle_t Jumping
		if ( CalcOnGround() && apUserCmd->aButtons & EBtnInput_Jump )
		{
			newVel.z = sv_jump_force;
		}
		else
		{
			// have slight velocity downward (TODO: make this just gravity clamped from 0 to 1)
			// this allows the velocity arrows to show in jolt phys debug view
			newVel.z = -0.01f;
		}

		apRigidBody->aVel.Set( newVel );
	}

	// uhhh
	apCharacter->SetLinearVelocity( apRigidBody->aVel );
	apRigidBody->aVel = apCharacter->GetLinearVelocity();


#if 0 // CH_SERVER
	// YEAH !!!!!
	switch ( groundState )
	{
		case EPhysGroundState_OnGround:
			Log_Msg( "GroundState: OnGround\n" );
			break;

		case EPhysGroundState_OnSteepGround:
			Log_Msg( "GroundState: OnSteepGround\n" );
			break;

		case EPhysGroundState_NotSupported:
			Log_Msg( "GroundState: NotSupported\n" );
			break;

		case EPhysGroundState_InAir:
			Log_Msg( "GroundState: InAir\n" );
			break;
	}
#endif
}


void PlayerMovement::WalkMovePostPhys()
{
	bool onGroundPrev = IsOnGround();
	bool onGround = CalcOnGround( false );

#if CH_CLIENT
	if ( onGround && !onGroundPrev )
		PlayImpactSound();
#endif

	if ( onGround )
		apRigidBody->aVel.Edit()[ W_UP ] = 0.f;
}


void PlayerMovement::DoSmoothLand( bool wasOnGround )
{
	PROF_SCOPE();

#if CH_SERVER
	if ( sv_land_smoothing )
	{
		// NOTE: this doesn't work properly when jumping mid duck and landing
		// meh, works well enough with the current values for now
		if ( CalcOnGround() && !wasOnGround )
		// if ( CalcOnGround() && !WasOnGround() )
		{
			float baseLandVel  = abs( apRigidBody->aVel.Get()[ W_UP ] * sv_land_vel_scale ) / sv_land_max_speed;
			float landVel      = std::clamp( baseLandVel * M_PI, 0.0, M_PI );

			apMove->aLandPower = ( -cos( landVel ) + 1 ) / 2;

			apMove->aLandTime  = 0.f;

			// This is shoddy and is only here to be funny
			auto health        = Ent_GetComponent< CHealth >( aPlayer, "health" );
			health->aHealth.Edit() -= ( landVel * 50 );
		}

		apMove->aLandTime += gFrameTime * sv_land_time_scale;
	}
	else
	{
		apMove->aLandPower = 0.f;
		apMove->aLandTime  = 0.f;
	}
#else
	// Log_Msg( "Smooth Land\n" );

	// if ( CalcOnGround() && !wasOnGround )
	if ( IsOnGround() && !wasOnGround )
	{
		PlayImpactSound();
	}
#endif

	if ( sv_land_smoothing )
	{
		if ( apMove->aLandPower > 0.f )
			// apCamera->aTransform.aPos[W_UP] += (- landPower * sin(landTime / landPower / 2) / exp(landTime / landPower)) * cl_land_power_scale;
			apCamTransform->aPos.Edit()[ W_UP ] += ( -apMove->aLandPower * sin( apMove->aLandTime / apMove->aLandPower ) / exp( apMove->aLandTime / apMove->aLandPower ) ) * sv_land_power_scale;

	}
}


// TODO: doesn't smoothly transition out of viewbob still
// TODO: maybe make a minimum speed threshold and start lerping to 0
//  or some check if your movements changed compared to the previous frame
//  so if you start moving again faster than min speed, or movements change,
//  you restart the view bob, or take a new step right there
void PlayerMovement::DoViewBob()
{
	if ( !cl_bob_enabled )
		return;

	//static glm::vec3 prevMove = aMove;
	//static bool inExit = false;

	if ( !IsOnGround() /*|| aMove != prevMove || inExit*/ )
	{
		// lerp back to 0 to not snap view the offset (not good enough) and reset input
		apMove->aWalkTime        = 0.f;
		apMove->aBobOffsetAmount = glm::mix( apMove->aBobOffsetAmount.Get(), 0.f, cl_bob_exit_lerp );
		apCamTransform->aPos.Edit()[ W_UP ] += apMove->aBobOffsetAmount;
		//inExit = aBobOffsetAmount > 0.01;
		//prevMove = aMove;
		return;
	}

	static bool playedStepSound = false;

	float       vel             = glm::length( glm::vec2( apRigidBody->aVel.Get().x, apRigidBody->aVel.Get().y ) ); 

	float speedFactor = glm::log( vel * cl_bob_speed_scale + 1 );

	// scale by speed
	apMove->aWalkTime += gFrameTime * speedFactor * cl_bob_freq;

	// never reaches 0 so do this to get it to 0
	// static float sinMessFix = sin(cos(tan(cos(0))));
	float sinMessFix = sin(cos(tan(cos(0)))) + cl_bob_offset;
	
	// using this math function mess instead of abs(sin(x)) and lerping it, since it gives a similar result
	// mmmmmmm cpu cycles go brrrrrr
	// apMove->aBobOffsetAmount = cl_bob_magnitude * speedFactor * (sin(cos(tan(cos(apMove->aWalkTime)))) - sinMessFix);
	apMove->aBobOffsetAmount = cl_bob_magnitude * speedFactor * (sin(cos(tan(cos(apMove->aWalkTime + cl_bob_time_offset)))) - sinMessFix);

	// apMove->aBobOffsetAmount = cl_bob_magnitude * speedFactor * ( 0.6*(1-cos( 2*sin(apMove->aWalkTime) )) );

	// reset input
	if ( apMove->aBobOffsetAmount == 0.f )
		apMove->aWalkTime = 0.f;

	// TODO: change this to be time based (and maybe a separate component? idk)
	if ( apMove->aBobOffsetAmount <= cl_bob_sound_threshold )
	{
		if ( !playedStepSound && vel > cl_land_sound_threshold )
		{
#if CH_CLIENT
			PlayStepSound();
#endif
			playedStepSound = true;
		}
	}
	else
	{
		playedStepSound = false;
	}

	apCamTransform->aPos.Edit()[ W_UP ] += apMove->aBobOffsetAmount;

#if CH_CLIENT
	if ( cl_bob_debug )
	{
		gui->DebugMessage( "Walk Time * Speed:  %.8f", apMove->aWalkTime.Get() );
		gui->DebugMessage( "View Bob Offset:    %.4f", apMove->aBobOffsetAmount.Get() );
		gui->DebugMessage( "View Bob Speed:     %.6f", speedFactor );
	}
#endif
}


CONVAR_FLOAT( cl_tilt, 1, CVARF_ARCHIVE );
CONVAR_FLOAT( cl_tilt_speed, 0.1 );
CONVAR_FLOAT( cl_tilt_threshold, 200 );

CONVAR_INT( cl_tilt_type, 1 );
CONVAR_FLOAT( cl_tilt_lerp, 5 );
CONVAR_FLOAT( cl_tilt_lerp_new, 10 );
CONVAR_FLOAT( cl_tilt_speed_scale, 1.7 );
CONVAR_FLOAT( cl_tilt_scale, 0.2 );
CONVAR_FLOAT( cl_tilt_threshold_new, 12 );


void PlayerMovement::DoViewTilt()
{
	if ( cl_tilt == false )
		return;

	float        output   = glm::dot( apRigidBody->aVel.Get(), apDir->aRight.Get() );
	float side = output < 0 ? -1 : 1;

	if ( cl_tilt_type == 1.f )
	{
		float speedFactor = glm::max(0.f, glm::log( glm::max(0.f, (fabs(output) * cl_tilt_speed_scale + 1) - cl_tilt_threshold_new) ));

		/* Now Lerp the tilt angle with the previous angle to make a smoother transition. */
		output = glm::mix( apMove->aPrevViewTilt.Get(), speedFactor * side * cl_tilt_scale, cl_tilt_lerp_new * gFrameTime );
	}
	else // type 0
	{
		output = glm::clamp( glm::max(0.f, fabs(output) - cl_tilt_threshold) / GetMaxSprintSpeed(), 0.f, 1.f ) * side;

		/* Now Lerp the tilt angle with the previous angle to make a smoother transition. */
		output = glm::mix( apMove->aPrevViewTilt.Get(), output * cl_tilt, cl_tilt_lerp * gFrameTime );
	}

	apCamTransform->aAng.Edit()[ ROLL ] = output;
	apMove->aPrevViewTilt.Edit()                    = output;
}


CONVAR_FLOAT( sv_friction_idk, 16 );

CONVAR_FLOAT( sv_friction2, 800 );

/*CONVAR_FLOAT( sv_friction_scale, 0.005 );
CONVAR_FLOAT( sv_friction_scale2, 10 );
CONVAR_FLOAT( sv_friction_offset, 6 );
CONVAR_FLOAT( sv_friction_lerp, 5 );

CONVAR_FLOAT( sv_friction_stop_lerp, 2 );
CONVAR_FLOAT( sv_friction_scale3, 0.01 );
CONVAR_FLOAT( sv_friction_scaleeee, 1 );

CONVAR_FLOAT( sv_friction_power, 2 );*/


// TODO: make your own version of this that uses this to find the friction value:
// log(-playerSpeed) + 1
void PlayerMovement::AddFriction()
{
#if 0

	glm::vec3 vel = apRigidBody->aVel;

	float speed = vel.length();
	if ( !speed )
		return;

	vel /= sv_friction2;
	apRigidBody->aVel = vel;

#else
	glm::vec3	start(0, 0, 0), stop(0, 0, 0);
	float	friction;
	//trace_t	trace;

	glm::vec3 vel = apRigidBody->aVel;

	float speed = sqrt(vel[0]*vel[0] + vel[W_RIGHT]*vel[W_RIGHT] + vel[W_UP]*vel[W_UP]);
	if (!speed)
		return;

	float idk = sv_friction_idk;

	// if the leading edge is over a dropoff, increase friction
	start.x = stop.x = GetPos().x + vel.x / speed * idk;
	start[ W_RIGHT ] = stop[ W_RIGHT ] = GetPos()[ W_RIGHT ] + vel[ W_RIGHT ] / speed * idk;
	start[ W_UP ] = stop[ W_UP ] = GetPos()[ W_UP ] + vel[ W_UP ] / speed * idk;
	// start.z = GetPos().z + sv_player->v.mins.z;

	//trace = SV_Move (start, vec3_origin, vec3_origin, stop, true, sv_player);

	//if (trace.fraction == 1.0)
	//	friction = sv_friction * sv_edgefriction.value;
	//else
		friction = sv_friction;

	// apply friction
		float control            = speed < sv_stop_speed ? sv_stop_speed : speed;
	float newspeed = glm::max( 0.f, speed - gFrameTime * control * friction );

	newspeed /= speed;
	apRigidBody->aVel = vel * newspeed;
#endif
}


void PlayerMovement::Accelerate( float wishSpeed, glm::vec3 wishDir, bool inAir )
{
	//float baseWishSpeed = inAir ? glm::min( 30.f, vec3_norm( wishDir ) ) : wishSpeed;
	float baseWishSpeed = inAir ? glm::min( sv_accel_speed_air, vec3_norm( wishDir ) ) : wishSpeed;

	float currentspeed  = glm::dot( apRigidBody->aVel.Get(), wishDir );
	float addspeed      = baseWishSpeed - currentspeed;

	if ( addspeed <= 0.f )
		return;

	addspeed = glm::min( addspeed, sv_accel_speed * gFrameTime * wishSpeed );

	for ( int i = 0; i < 3; i++ )
		apRigidBody->aVel.Edit()[ i ] += addspeed * wishDir[ i ];
}


#if CH_SERVER
Transform PlayerSpawnManager::SelectSpawnTransform()
{
	if ( aEntities.empty() )
	{
		Log_Error( "No playerSpawn entities found, returning origin for spawn position\n" );
		return {};
	}

	Transform spot{};
	size_t    index = 0;

	// If we have more than one playerSpawn entity, pick a random one
	// TODO: maybe make some priority thing, or master spawn like in source engine? idk
	if ( aEntities.size() > 1 )
		index = rand_u64( 0, aEntities.size() - 1 );
	
	auto transform = Ent_GetComponent< CTransform >( aEntities[ index ], "transform" );

	if ( !transform )
	{
		Log_Error( "Entity with playerSpawn does not have a transform component, returning origin for spawn position\n" );
		return spot;
	}

	spot.aPos = transform->aPos.Get();
	spot.aAng = transform->aAng.Get();

	return spot;
}
#endif

