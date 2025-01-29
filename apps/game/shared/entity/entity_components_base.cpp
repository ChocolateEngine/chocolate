#include "main.h"
#include "game_shared.h"
#include "entity.h"
#include "core/util.h"
#include "player.h"  // TEMP - for CPlayerMoveData

#include "entity.h"
#include "entity_systems.h"
#include "mapmanager.h"
#include "igui.h"

#include "igraphics.h"

#include "game_physics.h"  // just for IPhysicsShape* and IPhysicsObject*


// ====================================================================================================
// Base Components
// 
// TODO:
//  - add an option to define what components and component variables get saved to a map/scene
//  - also a random thought - maybe you could use a map/scene to create a preset entity prefab to create?
//      like a preset player entity to spawn in when a player loads
// ====================================================================================================


void Ent_RegisterVarHandlers()
{
}


CH_STRUCT_REGISTER_COMPONENT( CRigidBody, rigidBody, EEntComponentNetType_Both, ECompRegFlag_None )
{
	EntComp_RegisterComponentVarEx< TYPE, glm::vec3 >( EEntNetField_Vec3, "vel", offsetof( TYPE, aVel ), ECompRegFlag_None );

	//CH_REGISTER_COMPONENT_VAR2( EEntNetField_Vec3, glm::vec3, aVel, vel, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Vec3, glm::vec3, aAccel, accel, ECompRegFlag_None );
}


CH_STRUCT_REGISTER_COMPONENT( CDirection, direction, EEntComponentNetType_Both, ECompRegFlag_None )
{
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Vec3, glm::vec3, aForward, forward, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Vec3, glm::vec3, aUp, up, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Vec3, glm::vec3, aRight, right, ECompRegFlag_None );
}


// TODO: use a "protocol system", so an internally created model path would be this:
// "internal://model_0"
// and a file on the disk to load will be this:
// "file://path/to/asset.glb"
CH_STRUCT_REGISTER_COMPONENT( CRenderable, renderable, EEntComponentNetType_Both, ECompRegFlag_None )
{
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_StdString, std::string, aPath, path, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Bool, bool, aTestVis, testVis, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Bool, bool, aCastShadow, castShadow, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Bool, bool, aVisible, visible, ECompRegFlag_None );
	
	CH_REGISTER_COMPONENT_SYS2( EntSys_Renderable, gEntSys_Renderable );
}


// Probably should be in graphics?
CH_STRUCT_REGISTER_COMPONENT( CLight, light, EEntComponentNetType_Both, ECompRegFlag_None )
{
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_S32, ELightType, aType, type, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Color4, glm::vec4, color, color, ECompRegFlag_None );

	// TODO: these 2 should not be here
	// it should be attached to it's own entity that can be parented
	// and that entity needs to contain the transform (or transform small) component
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Vec3, glm::vec3, aPos, pos, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Quat, glm::quat, aRot, rot, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aInnerFov, innerFov, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aOuterFov, outerFov, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aRadius, radius, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aLength, length, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Bool, bool, aShadow, shadow, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Bool, bool, aEnabled, enabled, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Bool, bool, aUseTransform, useTransform, ECompRegFlag_None );
	
	CH_REGISTER_COMPONENT_SYS2( LightSystem, gLightEntSystems );
}


CH_STRUCT_REGISTER_COMPONENT( CHealth, health, EEntComponentNetType_Both, ECompRegFlag_None )
{
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_S32, int, aHealth, health, ECompRegFlag_None );
}


void Ent_RegisterBaseComponents()
{
	PROF_SCOPE();

	Ent_RegisterVarHandlers();

	// Setup Types, only used for registering variables without specifing the VarType
	GetEntComponentRegistry().aVarTypes[ typeid( bool ).hash_code() ]        = EEntNetField_Bool;
	GetEntComponentRegistry().aVarTypes[ typeid( float ).hash_code() ]       = EEntNetField_Float;
	GetEntComponentRegistry().aVarTypes[ typeid( double ).hash_code() ]      = EEntNetField_Double;

	GetEntComponentRegistry().aVarTypes[ typeid( s8 ).hash_code() ]          = EEntNetField_S8;
	GetEntComponentRegistry().aVarTypes[ typeid( s16 ).hash_code() ]         = EEntNetField_S16;
	GetEntComponentRegistry().aVarTypes[ typeid( s32 ).hash_code() ]         = EEntNetField_S32;
	GetEntComponentRegistry().aVarTypes[ typeid( s64 ).hash_code() ]         = EEntNetField_S64;

	GetEntComponentRegistry().aVarTypes[ typeid( u8 ).hash_code() ]          = EEntNetField_U8;
	GetEntComponentRegistry().aVarTypes[ typeid( u16 ).hash_code() ]         = EEntNetField_U16;
	GetEntComponentRegistry().aVarTypes[ typeid( u32 ).hash_code() ]         = EEntNetField_U32;
	GetEntComponentRegistry().aVarTypes[ typeid( u64 ).hash_code() ]         = EEntNetField_U64;

	// probably overrides type have of u64, hmmm
	// GetEntComponentRegistry().aVarTypes[ typeid( Entity ).hash_code() ]      = EEntNetField_Entity;
	GetEntComponentRegistry().aVarTypes[ typeid( std::string ).hash_code() ] = EEntNetField_StdString;

	GetEntComponentRegistry().aVarTypes[ typeid( glm::vec2 ).hash_code() ]   = EEntNetField_Vec2;
	GetEntComponentRegistry().aVarTypes[ typeid( glm::vec3 ).hash_code() ]   = EEntNetField_Vec3;
	GetEntComponentRegistry().aVarTypes[ typeid( glm::vec4 ).hash_code() ]   = EEntNetField_Vec4;
	GetEntComponentRegistry().aVarTypes[ typeid( glm::quat ).hash_code() ]   = EEntNetField_Quat;

	// Now Register Base Components
	EntComp_RegisterComponent< CTransform >(
	  "transform", EEntComponentNetType_Both,
	  [ & ]()
	  {
		  auto transform           = new CTransform;
		  transform->aScale.Edit() = { 1.f, 1.f, 1.f };
		  return transform;
	  },
	  [ & ]( void* spData )
	  { delete (CTransform*)spData; } );

	EntComp_RegisterComponentVar< CTransform, glm::vec3 >( "pos", offsetof( CTransform, aPos ), 0 );
	EntComp_RegisterComponentVar< CTransform, glm::vec3 >( "ang", offsetof( CTransform, aAng ), 0 );
	EntComp_RegisterComponentVar< CTransform, glm::vec3 >( "scale", offsetof( CTransform, aScale ), 0 );
	CH_REGISTER_COMPONENT_SYS( CTransform, EntSys_Transform, gEntSys_Transform );

	// CH_REGISTER_COMPONENT_RW( CRigidBody, rigidBody, true );
	// CH_REGISTER_COMPONENT_VAR( CRigidBody, glm::vec3, aVel, vel );
	// CH_REGISTER_COMPONENT_VAR( CRigidBody, glm::vec3, aAccel, accel );

	// CH_REGISTER_COMPONENT_RW( CDirection, direction, true );
	// CH_REGISTER_COMPONENT_VAR( CDirection, glm::vec3, aForward, forward );
	// CH_REGISTER_COMPONENT_VAR( CDirection, glm::vec3, aUp, up );
	// // CH_REGISTER_COMPONENT_VAR( CDirection, glm::vec3, aRight, right );
	// CH_REGISTER_COMP_VAR_VEC3( CDirection, aRight, right );

	// CH_REGISTER_COMPONENT_RW( CGravity, gravity, true );
	// CH_REGISTER_COMP_VAR_VEC3( CGravity, aForce, force );

	// might be a bit weird
	// HACK HACK: DONT OVERRIDE CLIENT VALUE, IT WILL NEVER BE UPDATED
	CH_REGISTER_COMPONENT_RW( CCamera, camera, ECompRegFlag_DontOverrideClient );
	CH_REGISTER_COMPONENT_VAR( CCamera, float, aFov, fov, true );
	
	// CH_REGISTER_COMPONENT_FL( CMap, map, EEntComponentNetType_Both, ECompRegFlag_None );
}


// Helper Functions
ch_handle_t Ent_GetRenderableHandle( Entity sEntity )
{
	auto renderComp = Ent_GetComponent< CRenderable >( sEntity, "renderable" );

	if ( !renderComp )
	{
		Log_Error( "Failed to get renderable component\n" );
		return CH_INVALID_HANDLE;
	}

	return renderComp->aRenderable;
}


Renderable_t* Ent_GetRenderable( Entity sEntity )
{
	auto renderComp = Ent_GetComponent< CRenderable >( sEntity, "renderable" );

	if ( !renderComp )
	{
		Log_Error( "Failed to get renderable component\n" );
		return nullptr;
	}

	return graphics->GetRenderableData( renderComp->aRenderable );
}


// Requires the entity to have renderable component with a model path set
Renderable_t* Ent_CreateRenderable( Entity sEntity )
{
	auto renderComp = Ent_GetComponent< CRenderable >( sEntity, "renderable" );

	if ( !renderComp )
	{
		Log_Error( "Failed to get renderable component\n" );
		return nullptr;
	}

	if ( renderComp->aRenderable == CH_INVALID_HANDLE )
	{
		if ( renderComp->aModel == CH_INVALID_HANDLE )
		{
			renderComp->aModel = graphics->LoadModel( renderComp->aPath );
			if ( renderComp->aModel == CH_INVALID_HANDLE )
			{
				Log_Error( "Failed to load model for renderable\n" );
				return nullptr;
			}
		}

		renderComp->aRenderable = graphics->CreateRenderable( renderComp->aModel );
		if ( renderComp->aRenderable == CH_INVALID_HANDLE )
		{
			Log_Error( "Failed to create renderable\n" );
			return nullptr;
		}
	}

	return graphics->GetRenderableData( renderComp->aRenderable );
}

