#include "main.h"
#include "game_shared.h"
#include "entity.h"
#include "core/util.h"
#include "player.h"  // TEMP - for CPlayerMoveData

#include "entity_systems.h"
#include "mapmanager.h"
#include "igui.h"

#include "game_physics.h"  // just for IPhysicsShape* and IPhysicsObject*


log_channel_h_t gLC_Entity = Log_RegisterChannel( "Entity - " CH_MODULE_NAME, ELogColor_Cyan );


EntitySystemData& EntSysData()
{
	static EntitySystemData entSysData;
	return entSysData;
}


CONVAR_BOOL( ent_show_translations, 0, "Show Entity ID Translations" );


// void* EntComponentRegistry_Create( std::string_view sName )
// {
// 	auto it = GetEntComponentRegistry().aComponentNames.find( sName );
// 
// 	if ( it == GetEntComponentRegistry().aComponentNames.end() )
// 	{
// 		return nullptr;
// 	}
// 
// 	EntComponentData_t* data = it->second;
// 	CH_ASSERT( data );
// 	return data->aCreate();
// }


EntCompVarTypeToEnum_t gEntCompVarToEnum[ EEntNetField_Count ] = {
	{ typeid( glm::vec3 ).hash_code(), EEntNetField_Vec3 },
};


const char* EntComp_NetTypeToStr( EEntComponentNetType sNetType )
{
	switch ( sNetType )
	{
		case EEntComponentNetType_Both:
			return "Both - This component is on both client and server";

		case EEntComponentNetType_Client:
			return "Client - This component is only on the Client";

		case EEntComponentNetType_Server:
			return "Server - This component is only on the Server";

		default:
			return "UNKNOWN";
	}
}


static const char* gEntVarTypeStr[] = {
	"INVALID",

	"bool",

	"float",
	"double",

#if 0
	"s8",
	"s16",
	"s32",
	"s64",

	"u8",
	"u16",
	"u32",
	"u64",
#else
	"char",
	"short",
	"int",
	"long long",

	"unsigned char",
	"unsigned short",
	"unsigned int",
	"unsigned long long",
#endif

	"Entity",
	"std::string",

	"glm::vec2",
	"glm::vec3",
	"glm::vec4",
	"glm::quat",

	"glm::vec3",
	"glm::vec4",

	"CUSTOM",
};


static_assert( CH_ARR_SIZE( gEntVarTypeStr ) == EEntNetField_Count );


const char* EntComp_VarTypeToStr( EEntNetField sVarType )
{
	if ( sVarType < 0 || sVarType > EEntNetField_Count )
		return gEntVarTypeStr[ EEntNetField_Invalid ];

	return gEntVarTypeStr[ sVarType ];
}


size_t EntComp_GetVarDirtyOffset( char* spData, EEntNetField sVarType )
{
	size_t offset = 0;
	switch ( sVarType )
	{
		default:
		case EEntNetField_Invalid:
			return 0;

		case EEntNetField_Bool:
			offset = sizeof( bool );
			break;

		case EEntNetField_Float:
			offset = sizeof( float );
			break;

		case EEntNetField_Double:
			offset = sizeof( double );
			break;


		case EEntNetField_S8:
			offset = sizeof( s8 );
			break;

		case EEntNetField_S16:
			offset = sizeof( s16 );
			break;

		case EEntNetField_S32:
			offset = sizeof( s32 );
			break;

		case EEntNetField_S64:
			offset = sizeof( s64 );
			break;


		case EEntNetField_U8:
			offset = sizeof( u8 );
			break;

		case EEntNetField_U16:
			offset = sizeof( u16 );
			break;

		case EEntNetField_U32:
			offset = sizeof( u32 );
			break;

		case EEntNetField_U64:
			offset = sizeof( u64 );
			break;


		case EEntNetField_Entity:
			offset = sizeof( Entity );
			break;

		case EEntNetField_StdString:
			offset = sizeof( std::string );
			break;

		case EEntNetField_Vec2:
			offset = sizeof( glm::vec2 );
			break;

		case EEntNetField_Color3:
		case EEntNetField_Vec3:
			offset = sizeof( glm::vec3 );
			break;

		case EEntNetField_Color4:
		case EEntNetField_Vec4:
			offset = sizeof( glm::vec4 );
			break;

		case EEntNetField_Quat:
			offset = sizeof( glm::quat );
			break;
	}

	return offset;
}


void EntComp_ResetVarDirty( char* spData, EEntNetField sVarType )
{
	size_t offset = EntComp_GetVarDirtyOffset( spData, sVarType );

	if ( offset )
	{
		bool* aIsDirty = reinterpret_cast< bool* >( spData + offset );
		*aIsDirty      = false;
	}
}


std::string EntComp_GetStrValueOfVar( void* spData, EEntNetField sVarType )
{
	switch ( sVarType )
	{
		default:
		{
			return "INVALID OR UNFINISHED";
		}
		case EEntNetField_Invalid:
		{
			return "INVALID";
		}
		case EEntNetField_Bool:
		{
			return *static_cast< bool* >( spData ) ? "TRUE" : "FALSE";
		}

		case EEntNetField_Float:
		{
			return ch_to_string( *static_cast< float* >( spData ) );
		}
		case EEntNetField_Double:
		{
			return ch_to_string( *static_cast< double* >( spData ) );
		}

		case EEntNetField_S8:
		{
			s8 value = *static_cast< s8* >( spData );
			return vstring( "%c", value );
		}
		case EEntNetField_S16:
		{
			s16 value = *static_cast< s16* >( spData );
			return vstring( "%d", value );
		}
		case EEntNetField_S32:
		{
			s32 value = *static_cast< s32* >( spData );
			return vstring( "%d", value );
		}
		case EEntNetField_S64:
		{
			s64 value = *static_cast< s64* >( spData );
			return vstring( "%lld", value );
		}

		case EEntNetField_U8:
		{
			u8 value = *static_cast< u8* >( spData );
			return vstring( "%uc", value );
		}
		case EEntNetField_U16:
		{
			u16 value = *static_cast< u16* >( spData );
			return vstring( "%ud", value );
		}
		case EEntNetField_U32:
		{
			u32 value = *static_cast< u32* >( spData );
			return vstring( "%ud", value );
		}
		case EEntNetField_U64:
		{
			u64 value = *static_cast< u64* >( spData );
			return vstring( "%zd", value );
		}

		case EEntNetField_Entity:
		{
			Entity value = *static_cast< Entity* >( spData );
			return vstring( "%zd", value );
		}
		case EEntNetField_StdString:
		{
			return *(const std::string*)spData;
		}

		case EEntNetField_Vec2:
		{
			const glm::vec2* value = (const glm::vec2*)spData;
			return vstring( "(%.4f, %.4f)", value->x, value->y );
		}
		case EEntNetField_Color3:
		case EEntNetField_Vec3:
		{
			return ch_vec3_to_str( *(const glm::vec3*)spData );
		}
		case EEntNetField_Color4:
		case EEntNetField_Vec4:
		{
			const glm::vec4* value = (const glm::vec4*)spData;
			return vstring( "(%.4f, %.4f, %.4f, %.4f)", value->x, value->y, value->z, value->w );
		}

		case EEntNetField_Quat:
		{
			const glm::quat* value = (const glm::quat*)spData;
			return vstring( "(%.4f, %.4f, %.4f, %.4f)", value->x, value->y, value->z, value->w );
		}
	}
}


std::string EntComp_GetStrValueOfVarOffset( size_t sOffset, void* spData, EEntNetField sVarType )
{
	char* data = static_cast< char* >( spData );
	return EntComp_GetStrValueOfVar( data + sOffset, sVarType );
}


// ===================================================================================
// Entity System
// ===================================================================================


bool Entity_Init()
{
	PROF_SCOPE();

	EntSysData().aActive = true;
	EntSysData().aEntityPool.clear();
	EntSysData().aComponentPools.clear();
	EntSysData().aEntityIDConvert.clear();

	// Initialize the queue with all possible entity IDs
	// for ( Entity entity = 0; entity < CH_MAX_ENTITIES; ++entity )
	// 	aEntityPool.push( entity );

	EntSysData().aEntityPool.resize( CH_MAX_ENTITIES );
	for ( Entity entity = CH_MAX_ENTITIES - 1, index = 0; entity > 0; --entity, ++index )
		EntSysData().aEntityPool[ index ] = entity;

	Entity_CreateComponentPools();

	return true;
}


void Entity_Shutdown()
{
	PROF_SCOPE();

	EntSysData().aActive = false;

	// Mark all entities as destroyed
	for ( auto& [ entity, flags ] : EntSysData().aEntityFlags )
	{
		flags |= EEntityFlag_Destroyed;
	}

	// Destroy entities marked as destroyed
	Entity_DeleteQueuedEntities();

	// Tell all component pools every entity was destroyed
	// for ( Entity entity : aUsedEntities )
	// {
	// 	for ( auto& [ name, pool ] : aComponentPools )
	// 	{
	// 		pool->EntityDestroyed( entity );
	// 	}
	// }

	// Free component pools
	for ( auto& [ name, pool ] : EntSysData().aComponentPools )
	{
		if ( pool->apComponentSystem )
		{
			EntSysData().aComponentSystems.erase( typeid( pool->apComponentSystem ).hash_code() );
			pool->apComponentSystem = nullptr;
		}

		delete pool;
	}

	EntSysData().aEntityPool.clear();
	EntSysData().aComponentPools.clear();
	EntSysData().aEntityIDConvert.clear();
}


void Entity_UpdateSystems()
{
	PROF_SCOPE();

	for ( auto& [ name, pool ] : EntSysData().aComponentPools )
	{
		if ( pool->apComponentSystem )
			pool->apComponentSystem->Update();
	}
}


void Entity_UpdateStates()
{
	PROF_SCOPE();

	// Remove Components Queued for Deletion
	for ( auto& [ name, pool ] : EntSysData().aComponentPools )
	{
		pool->RemoveAllQueued();
	}

	Entity_DeleteQueuedEntities();

	for ( auto& [ id, flags ] : EntSysData().aEntityFlags )
	{
		// Remove the created flag if it has that
		if ( flags & EEntityFlag_Created )
			EntSysData().aEntityFlags[ id ] &= ~EEntityFlag_Created;
	}
}


void Entity_InitCreatedComponents()
{
	PROF_SCOPE();

	// Remove Components Queued for Deletion
	for ( auto& [ name, pool ] : EntSysData().aComponentPools )
	{
		pool->InitCreatedComponents();
	}
}


void Entity_CreateComponentPools()
{
	PROF_SCOPE();

	// iterate through all registered components and create a component pool for them
	for ( auto& [ name, componentData ] : GetEntComponentRegistry().aComponentNames )
	{
		Entity_CreateComponentPool( name.data() );
	}
}


void Entity_CreateComponentPool( const char* spName )
{
	PROF_SCOPE();

	EntityComponentPool* pool = new EntityComponentPool;

	if ( !pool->Init( spName ) )
	{
		Log_ErrorF( gLC_Entity, "Failed to create component pool for \"%s\"", spName );
		delete pool;
		return;
	}
		
	EntSysData().aComponentPools[ spName ] = pool;

	// Create component system if it has one registered for it
	if ( !pool->apData->apSystem )
		return;

	pool->apComponentSystem = pool->apData->apSystem;

	if ( pool->apComponentSystem )
	{
		pool->apComponentSystem->apPool                                                 = pool;
		EntSysData().aComponentSystems[ typeid( pool->apComponentSystem ).hash_code() ] = pool->apComponentSystem;
	}
	else
	{
		Log_ErrorF( gLC_Entity, "Failed to create component system for component \"%s\"\n", spName );
	}
}


IEntityComponentSystem* Entity_GetComponentSystem( const char* spName )
{
	PROF_SCOPE();

	EntityComponentPool* pool = Entity_GetComponentPool( spName );

	if ( pool == nullptr )
	{
		Log_ErrorF( gLC_Entity, "Failed to get component system - no component pool found: \"%s\"\n", spName );
		return nullptr;
	}

	return pool->apComponentSystem;
}


Entity Entity_CreateEntity( bool sLocal )
{
	PROF_SCOPE();

	if ( CH_IF_ASSERT_MSG( Entity_GetEntityCount() < CH_MAX_ENTITIES, "Hit Entity Limit!" ) )
		return CH_ENT_INVALID;
	
	CH_ASSERT_MSG( Entity_GetEntityCount() + EntSysData().aEntityPool.size() == CH_MAX_ENTITIES, "Entity Count and Free Entities are out of sync!" );

	// Take an ID from the front of the queue
	Entity id = EntSysData().aEntityPool.back();
	EntSysData().aEntityPool.pop_back();

	// SANITY CHECK
	CH_ASSERT( !Entity_EntityExists( id ) );

	// Create Entity Flags for this and add the Created Flag to tit
	EntSysData().aEntityFlags[ id ] = EEntityFlag_Created;

	// If we want the entity to be local on the client or server, add that flag to it
	if ( sLocal )
		EntSysData().aEntityFlags[ id ] |= EEntityFlag_Local;

	Log_DevF( gLC_Entity, 2, "Created Entity %zd\n", id );

	return id;
}


void Entity_DeleteEntity( Entity sEntity )
{
	PROF_SCOPE();

	CH_ASSERT_MSG( sEntity < CH_MAX_ENTITIES, "Entity out of range" );

	CH_ASSERT_MSG( Entity_GetEntityCount() + EntSysData().aEntityPool.size() == CH_MAX_ENTITIES, "Entity Count and Free Entities are out of sync!" );

	EntSysData().aEntityFlags[ sEntity ] |= EEntityFlag_Destroyed;
	Log_DevF( gLC_Entity, 2, "Marked Entity to be Destroyed: %zd\n", sEntity );

	// Get all children attached to this entity
	ChVector< Entity > children;
	Entity_GetChildrenRecurse( sEntity, children );

	// Mark all of them as destroyed
	for ( Entity child : children )
	{
		EntSysData().aEntityFlags[ child ] |= EEntityFlag_Destroyed;
		Log_DevF( gLC_Entity, 2, "Marked Child Entity to be Destroyed (parent %zd): %zd\n", sEntity, child );
	}
}


void Entity_DeleteQueuedEntities()
{
	PROF_SCOPE();

	ChVector< Entity > deleteEntities;

	// well this sucks
	for ( auto& [ entity, flags ] : EntSysData().aEntityFlags )
	{
		// Check the entity's flags to see if it's marked as deleted
		if ( flags & EEntityFlag_Destroyed )
			deleteEntities.push_back( entity );
	}

	for ( auto entity : deleteEntities )
	{
		// Tell each Component Pool that this entity was destroyed
		for ( auto& [ name, pool ] : EntSysData().aComponentPools )
		{
			pool->EntityDestroyed( entity );
		}

		// Remove this entity from the translation list if it's in it
		for ( auto it = EntSysData().aEntityIDConvert.begin(); it != EntSysData().aEntityIDConvert.end(); it++ )
		{
			if ( it->second == entity )
			{
				EntSysData().aEntityIDConvert.erase( it );
				break;
			}
		}

		// Put the destroyed ID at the back of the queue
		
		// SANITY CHECK
		size_t sanityCheckPool = vec_index( EntSysData().aEntityPool, entity );
		CH_ASSERT( sanityCheckPool == SIZE_MAX );  // can't be in pool already

		// aEntityPool.push( ent );
		// aEntityPool.push_back( ent );
		EntSysData().aEntityPool.insert( EntSysData().aEntityPool.begin(), entity );

		EntSysData().aEntityFlags.erase( entity );

		Log_DevF( gLC_Entity, 2, "Destroyed Entity %zd\n", entity );
	}
}


Entity Entity_GetEntityCount()
{
	return EntSysData().aEntityFlags.size();
}


bool Entity_EntityExists( Entity desiredId )
{
	PROF_SCOPE();

	auto it = EntSysData().aEntityFlags.find( desiredId );

	// if the entity does not have any flags, it exists
	return it != EntSysData().aEntityFlags.end();
}


void Entity_ParentEntity( Entity sSelf, Entity sParent )
{
	if ( sSelf == CH_ENT_INVALID || sSelf == sParent )
		return;

	if ( sParent == CH_ENT_INVALID )
	{
		// Clear the parent
		EntSysData().aEntityFlags[ sSelf ] |= EEntityFlag_Parented;
		EntSysData().aEntityParents.erase( sSelf );
	}
	else
	{
		EntSysData().aEntityParents[ sSelf ] = sParent;
		EntSysData().aEntityFlags[ sSelf ] &= ~EEntityFlag_Parented;
	}
}


Entity Entity_GetParent( Entity sSelf )
{
	auto it = EntSysData().aEntityParents.find( sSelf );

	if ( it != EntSysData().aEntityParents.end() )
		return it->second;

	return CH_ENT_INVALID;
}


bool Entity_IsParented( Entity sSelf )
{
	auto it = EntSysData().aEntityFlags.find( sSelf );

	if ( it != EntSysData().aEntityFlags.end() )
		return it->second & EEntityFlag_Parented;

	return false;
}


// Get the highest level parent for this entity, returns self if not parented
Entity Entity_GetRootParent( Entity sSelf )
{
	PROF_SCOPE();

	auto it = EntSysData().aEntityParents.find( sSelf );

	if ( it != EntSysData().aEntityParents.end() )
		return Entity_GetRootParent( it->second );

	return sSelf;
}


// Recursively get all entities attached to this one (SLOW)
void Entity_GetChildrenRecurse( Entity sEntity, ChVector< Entity >& srChildren )
{
	PROF_SCOPE();

#pragma message( "could probably speed up GetChildrenRecurse() by instead iterating through aEntityParents, and checking the value for each one" )

	for ( auto& [ otherEntity, flags ] : EntSysData().aEntityFlags )
	{
		if ( !( flags & EEntityFlag_Parented ) )
			continue;

		Entity otherParent = Entity_GetParent( otherEntity );

		if ( otherParent == sEntity )
		{
			srChildren.push_back( otherEntity );
			Entity_GetChildrenRecurse( otherEntity, srChildren );

#pragma message( "CHECK IF THIS BREAKS THE CODE" )
			break;
		}
	}
}


// Returns a Model Matrix with parents applied in world space IF we have a transform component
bool Entity_GetWorldMatrix( glm::mat4& srMat, Entity sEntity )
{
	PROF_SCOPE();

	// Entity    parent = IsParented( sEntity ) ? GetParent( sEntity ) : CH_ENT_INVALID;
	Entity    parent = Entity_GetParent( sEntity );
	glm::mat4 parentMat( 1.f );

	if ( parent != CH_ENT_INVALID )
	{
		// Get the world matrix recursively
		Entity_GetWorldMatrix( parentMat, parent );
	}

	// Check if we have a transform component
	auto transform = static_cast< CTransform* >( Entity_GetComponent( sEntity, "transform" ) );

	if ( !transform )
	{
		// Fallback to the parent world matrix
		srMat = parentMat;
		return ( parent != CH_ENT_INVALID );
	}

	// is this all the wrong order?

	// NOTE: THIS IS PROBABLY WRONG
	srMat = glm::translate( transform->aPos.Get() );
	// srMat = glm::mat4( 1.f );

	glm::mat4 rotMat( 1.f );
	#define ROT_ANG( axis ) glm::vec3( rotMat[ 0 ][ axis ], rotMat[ 1 ][ axis ], rotMat[ 2 ][ axis ] )

	// glm::vec3 temp = glm::radians( transform->aAng.Get() );
	// glm::quat rotQuat = temp;

	#undef ROT_ANG

	// rotMat = glm::mat4_cast( rotQuat );

	// srMat *= rotMat;
	// srMat *= glm::eulerAngleYZX(
	//   glm::radians(transform->aAng.Get().x ),
	//   glm::radians(transform->aAng.Get().y ),
	//   glm::radians(transform->aAng.Get().z ) );
	
	srMat *= glm::eulerAngleZYX(
	  glm::radians( transform->aAng.Get()[ ROLL ] ),
	  glm::radians( transform->aAng.Get()[ YAW ] ),
	  glm::radians( transform->aAng.Get()[ PITCH ] ) );
	
	// srMat *= glm::yawPitchRoll(
	//   glm::radians( transform->aAng.Get()[ PITCH ] ),
	//   glm::radians( transform->aAng.Get()[ YAW ] ),
	//   glm::radians( transform->aAng.Get()[ ROLL ] ) );

	srMat = glm::scale( srMat, transform->aScale.Get() );

	srMat = parentMat * srMat;

	return true;
}


Transform Entity_GetWorldTransform( Entity sEntity )
{
	PROF_SCOPE();

	Transform final{};

	glm::mat4 matrix;
	if ( !Entity_GetWorldMatrix( matrix, sEntity ) )
		return final;

	final.aPos   = Util_GetMatrixPosition( matrix );
	final.aAng   = glm::degrees( Util_GetMatrixAngles( matrix ) );
	final.aScale = Util_GetMatrixScale( matrix );

	return final;
}


// Add a component to an entity
void* Entity_AddComponent( Entity entity, std::string_view sName )
{
	PROF_SCOPE();

	auto pool = Entity_GetComponentPool( sName );

	if ( pool == nullptr )
	{
		Log_ErrorF( gLC_Entity, "Failed to create component - no component pool found: \"%s\"\n", sName.data() );
		return nullptr;
	}

	return pool->Create( entity );
}


// Does this entity have this component?
bool Entity_HasComponent( Entity entity, std::string_view sName )
{
	PROF_SCOPE();

	auto pool = Entity_GetComponentPool( sName );

	if ( pool == nullptr )
	{
		Log_ErrorF( gLC_Entity, "Failed to get component - no component pool found: \"%s\"\n", sName.data() );
		return false;
	}

	return pool->Contains( entity );
}


// Get a component from an entity
void* Entity_GetComponent( Entity entity, std::string_view sName )
{
	PROF_SCOPE();

	auto pool = Entity_GetComponentPool( sName );

	if ( pool == nullptr )
	{
		Log_ErrorF( gLC_Entity, "Failed to get component - no component pool found: \"%s\"\n", sName.data() );
		return nullptr;
	}

	return pool->GetData( entity );
}


// Remove a component from an entity
void Entity_RemoveComponent( Entity entity, std::string_view sName )
{
	PROF_SCOPE();

	auto pool = Entity_GetComponentPool( sName );

	if ( pool == nullptr )
	{
		Log_ErrorF( gLC_Entity, "Failed to remove component - no component pool found: \"%s\"\n", sName.data() );
		return;
	}

	pool->RemoveQueued( entity );
}


#if CH_CLIENT
// Sets Prediction on this component
void Entity_SetComponentPredicted( Entity entity, std::string_view sName, bool sPredicted )
{
	auto pool = Entity_GetComponentPool( sName );

	if ( pool == nullptr )
	{
		Log_ErrorF( gLC_Entity, "Failed to set component prediction - no component pool found: \"%s\"\n", sName.data() );
		return;
	}

	pool->SetPredicted( entity, sPredicted );
}


// Is this component predicted for this Entity?
bool Entity_IsComponentPredicted( Entity entity, std::string_view sName )
{
	auto pool = Entity_GetComponentPool( sName );

	if ( pool == nullptr )
	{
		Log_FatalF( gLC_Entity, "Failed to get component prediction - no component pool found: \"%s\"\n", sName.data() );
		return false;
	}

	return pool->IsPredicted( entity );
}
#endif


// Enables/Disables Networking on this Entity
void Entity_SetNetworked( Entity entity, bool sNetworked )
{
	auto it = EntSysData().aEntityFlags.find( entity );
	if ( it == EntSysData().aEntityFlags.end() )
	{
		Log_Error( gLC_Entity, "Failed to set Entity Networked State - Entity not found\n" );
		return;
	}

	if ( sNetworked )
		it->second |= EEntityFlag_Local;
	else
		it->second &= ~EEntityFlag_Local;
}


// Is this Entity Networked?
bool Entity_IsNetworked( Entity sEntity )
{
	PROF_SCOPE();

	auto it = EntSysData().aEntityFlags.find( sEntity );
	if ( it == EntSysData().aEntityFlags.end() )
	{
		Log_Error( gLC_Entity, "Failed to get Entity Networked State - Entity not found\n" );
		return false;
	}

	// If we have the local flag, we aren't networked
	if ( it->second & EEntityFlag_Local )
		return false;

	// Check if we have a parent entity
	if ( !( it->second & EEntityFlag_Parented ) )
		return true;

	Entity parent = Entity_GetParent( sEntity );
	CH_ASSERT( parent != CH_ENT_INVALID );
	if ( parent == CH_ENT_INVALID )
		return true;

	// We make sure the parents are also networked before networking this one
	return Entity_IsNetworked( parent );
}


bool Entity_IsNetworked( Entity sEntity, EEntityFlag sFlags )
{
	PROF_SCOPE();

	// If we have the local flag, we aren't networked
	if ( sFlags & EEntityFlag_Local )
		return false;

	// Check if we have a parent entity
	if ( !( sFlags & EEntityFlag_Parented ) )
		return true;

	Entity parent = Entity_GetParent( sEntity );
	CH_ASSERT( parent != CH_ENT_INVALID );
	if ( parent == CH_ENT_INVALID )
		return true;

	// We make sure the parents are also networked before networking this one
	return Entity_IsNetworked( parent );
}


EntityComponentPool* Entity_GetComponentPool( std::string_view spName )
{
	PROF_SCOPE();

	auto it = EntSysData().aComponentPools.find( spName );

	if ( it == EntSysData().aComponentPools.end() )
		Log_FatalF( gLC_Entity, "Component not registered before use: \"%s\"\n", spName.data() );

	return it->second;
}


// Used for converting a sent entity ID to what it actually is on the recieving end, so no conflicts occur
// This is needed for client/server networking, the entity id on each end will be different, so we convert the id
Entity Entity_TranslateEntityID( Entity sEntity, bool sCreate )
{
	PROF_SCOPE();

	if ( sEntity == CH_ENT_INVALID )
		return CH_ENT_INVALID;

	auto it = EntSysData().aEntityIDConvert.find( sEntity );
	if ( it != EntSysData().aEntityIDConvert.end() )
	{
		// Make sure it actually exists
		if ( !Entity_EntityExists( it->second ) )
		{
			Log_ErrorF( gLC_Entity, "Failed to find entity while translating Entity ID: %zd -> %zd\n", sEntity, it->second );
			// remove it from the list
			EntSysData().aEntityIDConvert.erase( it );
			return CH_ENT_INVALID;
		}

		if ( ent_show_translations )
			Log_DevF( gLC_Entity, 3, "Translating Entity ID %zd -> %zd\n", sEntity, it->second );

		return it->second;
	}

	if ( !sCreate )
	{
		Log_ErrorF( gLC_Entity, "Entity not in translation list: %zd\n", sEntity );
		return CH_ENT_INVALID;
	}

	Entity entity = Entity_CreateEntity();

	if ( entity == CH_ENT_INVALID )
	{
		Log_Warn( gLC_Entity, "Failed to create networked entity\n" );
		return CH_ENT_INVALID;
	}

	Log_DevF( gLC_Entity, 2, "Added Translation of Entity ID %zd -> %zd\n", sEntity, entity );
	EntSysData().aEntityIDConvert[ sEntity ] = entity;
	return entity;
}


// ===================================================================================
// Console Commands
// ===================================================================================


GAME_CONCMD( ent_dump_registry )
{
	log_t group = Log_GroupBegin( gLC_Entity );

	Log_GroupF( group, "Entity Count: %zd\n", Entity_GetEntityCount() );
	Log_GroupF( group, "Registered Components: %zd\n", GetEntComponentRegistry().aComponents.size() );

	for ( const auto& [ name, regData ] : GetEntComponentRegistry().aComponentNames )
	{
		CH_ASSERT( regData );

		if ( !regData )
			continue;

		Log_GroupF( group, "\nComponent: %s\n", regData->apName );
		Log_GroupF( group, "   Override Client: %s\n", ( regData->aFlags & ECompRegFlag_DontOverrideClient ) ? "TRUE" : "FALSE" );
		Log_GroupF( group, "   Networking Type: %s\n", EntComp_NetTypeToStr( regData->aNetType ) );
		Log_GroupF( group, "   Variable Count:  %zd\n", regData->aVars.size() );

		for ( const auto& [ offset, var ] : regData->aVars )
		{
			Log_GroupF( group, "       %s - %s\n", EntComp_VarTypeToStr( var.aType ), var.apName );
		}
	}

	Log_GroupEnd( group );
}


GAME_CONCMD( ent_dump )
{
	log_t group = Log_GroupBegin( gLC_Entity );

	Log_GroupF( group, "Entity Count: %zd\n", Entity_GetEntityCount() );

	Log_GroupF( group, "Registered Components: %zd\n", GetEntComponentRegistry().aComponents.size() );

	for ( const auto& [ name, pool ] : EntSysData().aComponentPools )
	{
		CH_ASSERT( pool );

		if ( !pool )
			continue;

		Log_GroupF( group, "Component Pool: %s - %zd Components in Pool\n", name.data(), pool->GetCount() );
	}

	Log_GroupF( group, "Components: %zd\n", EntSysData().aComponentPools.size() );

	for ( auto& [ entity, flags ] : EntSysData().aEntityFlags )
	{
		// this is the worst thing ever
		std::vector< EntityComponentPool* > pools;

		// Find all component pools that contain this Entity
		// That means the Entity has the type of component that pool is for
		for ( auto& [ name, pool ] : EntSysData().aComponentPools )
		{
			if ( pool->Contains( entity ) )
				pools.push_back( pool );
		}

		Log_GroupF( group, "\nEntity %zd\n", entity );
		// Log_GroupF( group, "    Predicted: %s\n", GetEntitySystem() );

		for ( EntityComponentPool* pool : pools )
		{
			auto data    = pool->GetData( entity );
			auto regData = pool->GetRegistryData();

			Log_GroupF( group, "\n    Component: %s\n", regData->apName );
			Log_GroupF( group, "    Size: %zd bytes\n", regData->aSize );
			Log_GroupF( group, "    Predicted: %s\n", pool->IsPredicted( entity ) ? "TRUE" : "FALSE" );

			for ( const auto& [ offset, var ] : regData->aVars )
			{
				Log_GroupF( group, "        %s = %s\n", var.apName, EntComp_GetStrValueOfVarOffset( offset, data, var.aType ).c_str() );
			}
		}
	}

	Log_GroupEnd( group );
}


GAME_CONCMD( ent_mem )
{
	log_t group = Log_GroupBegin( gLC_Entity );

	size_t   componentPoolMapSize = ch_size_of_umap( EntSysData().aComponentPools );
	size_t componentPoolSize    = 0;

	for ( auto& [name, pool] : EntSysData().aComponentPools )
	{
		size_t              curSize       = sizeof( EntityComponentPool );
		size_t              compSize      = 0;
		size_t              compOtherSize = 0;

		EntComponentData_t* regData  = pool->GetRegistryData();

		for ( const auto& [ offset, var ] : regData->aVars )
		{
			// Check for special case std::string
			if ( var.aType == EEntNetField_StdString )
			{
				// We have to iterate through all components and get the amount of memory used by each std::string
				// compOtherSize += 0;
			}
		}

		compSize = regData->aSize * pool->GetCount();
		curSize += compSize + compOtherSize;

		Log_GroupF( group, "Component Pool \"%s\": %.6f KB\n", pool->apName, ch_bytes_to_kb( curSize ) );
		Log_GroupF( group, "    %zd bytes per component * %zd components\n\n", regData->aSize, pool->GetCount() );

		componentPoolSize += curSize;
	}

	Log_GroupF( group, "Component Pool Base Size: %.6f KB\n", ch_bytes_to_kb( sizeof( EntityComponentPool ) ) );
	Log_GroupF( group, "Component Pool Map Size: %.6f KB\n", ch_bytes_to_kb( componentPoolMapSize ) );
	Log_GroupF( group, "Component Pools: %.6f KB (%zd pools)\n", ch_bytes_to_kb( componentPoolSize ), EntSysData().aComponentPools.size() );

	Log_GroupEnd( group );
}

