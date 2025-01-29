#include "main.h"
#include "game_shared.h"
#include "entity.h"
#include "entity_systems.h"
#include "core/util.h"
#include "player.h"  // TEMP - for CPlayerMoveData

#include "mapmanager.h"
#include "igui.h"

#include "igraphics.h"

#include "game_physics.h"  // just for IPhysicsShape* and IPhysicsObject*


LOG_CHANNEL( Entity );


// returns "CLIENT", "SERVER", or "GAME" depending on what we are processing
inline const char* GetProcessingName()
{
#if CH_CLIENT
	return "CLIENT";
#else
	return "SERVER";
#endif
}


EntityComponentPool::EntityComponentPool()
{
	// aCount = 0;
}


EntityComponentPool::~EntityComponentPool()
{
	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );

	while ( aMapEntityToComponent.size() )
	{
		auto it = aMapEntityToComponent.begin();
		Remove( it->first );
	}
}


bool EntityComponentPool::Init( const char* spName )
{
	PROF_SCOPE();

	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );

	apName  = spName;

	// Get Creation and Free functions
	auto it = GetEntComponentRegistry().aComponentNames.find( apName );

	if ( it == GetEntComponentRegistry().aComponentNames.end() )
	{
		Log_FatalF( gLC_Entity, "Component not registered: \"%s\"\n", apName );
		return false;
	}

	CH_ASSERT( it->second->aFuncNew );
	CH_ASSERT( it->second->aFuncFree );

	aFuncNew  = it->second->aFuncNew;
	aFuncFree = it->second->aFuncFree;

	apData    = it->second;

	return true;
}


// Get Component Registry Data
EntComponentData_t* EntityComponentPool::GetRegistryData()
{
	PROF_SCOPE();

	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );

	return GetEntComponentRegistry().aComponentNames[ apName ];
}


// Does this pool contain a component for this entity?
bool EntityComponentPool::Contains( Entity entity )
{
	PROF_SCOPE();

	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );

	auto it = aMapEntityToComponent.find( entity );
	return ( it != aMapEntityToComponent.end() );
}


void EntityComponentPool::EntityDestroyed( Entity entity )
{
	PROF_SCOPE();

	auto it = aMapEntityToComponent.find( entity );

	if ( it != aMapEntityToComponent.end() )
	{
		Remove( entity );
	}

	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );
}


// Adds This component to the entity
void* EntityComponentPool::Create( Entity entity )
{
	PROF_SCOPE();

	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );

	// Is this a client or server component pool?
	// TODO: Make sure this component can be created on it

	// Check if the component already exists
	auto it = aMapEntityToComponent.find( entity );

	if ( it != aMapEntityToComponent.end() )
	{
		Log_ErrorF( gLC_Entity, "Component already exists on entity - \"%s\"\n", apName );
		return aComponents[ it->second.aIndex ];
	}

	ComponentID_t newID;
	newID.aIndex = aComponentIDs.size();
	auto it2     = aMapComponentToEntity.find( newID );

	if ( it2 != aMapComponentToEntity.end() )
	{
		Log_ErrorF( gLC_Entity, "what - component \"%s\"\n", apName );
	}

	aMapEntityToComponent[ entity ] = newID;
	aMapComponentToEntity[ newID ]  = entity;

	void* data                      = aFuncNew();
	aComponentFlags[ newID ]        = EEntityFlag_Created;
	aComponents[ newID.aIndex ]     = data;

	aComponentIDs.push_back( newID );
	aNewComponents.push_front( newID );

	// Add it to system
	if ( apComponentSystem )
		apComponentSystem->aEntities.push_back( entity );

	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );

	Log_DevF( gLC_Entity, 2, "%s - Added Component To Entity %zd - %s\n", GetProcessingName(), entity, apName );

	// TODO: use malloc and use that pointer in the constructor for this
	// use placement new
	// https://stackoverflow.com/questions/2494471/c-is-it-possible-to-call-a-constructor-directly-without-new

	return data;
}


// Removes this component from the entity
void EntityComponentPool::Remove( Entity entity )
{
	PROF_SCOPE();

	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );

	auto it = aMapEntityToComponent.find( entity );

	if ( it == aMapEntityToComponent.end() )
	{
		Log_ErrorF( gLC_Entity, "Failed to remove component from entity - \"%s\"\n", apName );
		return;
	}

	ComponentID_t index = it->second;
	void* data = aComponents[ index.aIndex ];
	CH_ASSERT( data );

	// Remove it from systems
	// for ( auto system : aComponentSystems )
	if ( apComponentSystem )
	{
		apComponentSystem->ComponentRemoved( entity, data );
		vec_remove( apComponentSystem->aEntities, entity );
	}

	aMapComponentToEntity.erase( index );
	aMapEntityToComponent.erase( it );

	aComponentFlags.erase( index );

	aFuncFree( data );

	vec_remove( aComponentIDs, index );

	Log_DevF( gLC_Entity, 2, "%s - Removed Component From Entity %zd - %s\n", GetProcessingName(), entity, apName );

	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );
}


// Removes this component by index
void EntityComponentPool::RemoveByID( ComponentID_t sID )
{
	PROF_SCOPE();

	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );

	auto it = aMapComponentToEntity.find( sID );

	if ( it == aMapComponentToEntity.end() )
	{
		Log_ErrorF( gLC_Entity, "Failed to remove component from entity - \"%s\"\n", apName );
		return;
	}

	Entity entity = it->second;

	void*  data   = aComponents[ sID.aIndex ];
	CH_ASSERT( data );

	// Remove it from the system
	if ( apComponentSystem )
	{
		apComponentSystem->ComponentRemoved( entity, data );
		vec_remove( apComponentSystem->aEntities, entity );
	}

	aMapComponentToEntity.erase( sID );
	aMapEntityToComponent.erase( entity );

	aComponentFlags.erase( sID );

	aFuncFree( data );

	vec_remove( aComponentIDs, sID );
	// aCount--;

	Log_DevF( gLC_Entity, 2, "%s - Removed Component From Entity %zd - %s\n", GetProcessingName(), entity, apName );

	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );
}


// Removes this component from the entity later
void EntityComponentPool::RemoveQueued( Entity entity )
{
	PROF_SCOPE();

	auto it = aMapEntityToComponent.find( entity );

	if ( it == aMapEntityToComponent.end() )
	{
		Log_ErrorF( gLC_Entity, "Failed to remove component from entity - \"%s\"\n", apName );
		return;
	}

	ComponentID_t index = it->second;

	// Mark Component as Destroyed
	aComponentFlags[ index ] |= EEntityFlag_Destroyed;

	Log_DevF( gLC_Entity, 2, "%s - Marked Component to be removed From Entity %zd - %s\n", GetProcessingName(), entity, apName );
}


// Removes components queued for deletion
void EntityComponentPool::RemoveAllQueued()
{
	PROF_SCOPE();

	std::vector< ComponentID_t > toRemove;

	// for ( auto& [ index, state ] : aComponentFlags )
	for ( auto it = aComponentFlags.begin(); it != aComponentFlags.end(); it++ )
	{
		if ( it->second & EEntityFlag_Destroyed )
		{
			toRemove.push_back( it->first );
			continue;
		}
	}

	for ( ComponentID_t entity : toRemove )
	{
		RemoveByID( entity );
	}
}


void EntityComponentPool::InitCreatedComponents()
{
	PROF_SCOPE();

	if ( !apComponentSystem )
	{
		aNewComponents.clear();
		aComponentsUpdated.clear();
		return;
	}

	for ( ComponentID_t id : aNewComponents )
	{
		aComponentFlags[ id ] &= ~EEntityFlag_Created;
		apComponentSystem->ComponentAdded( aMapComponentToEntity.at( id ), aComponents.at( id.aIndex ) );
	}

	for ( ComponentID_t id : aComponentsUpdated )
	{
		apComponentSystem->ComponentUpdated( aMapComponentToEntity.at( id ), aComponents.at( id.aIndex ) );
	}

	aNewComponents.clear();
	aComponentsUpdated.clear();
}


// Gets the data for this component
void* EntityComponentPool::GetData( Entity entity )
{
	PROF_SCOPE();

	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );

	auto it = aMapEntityToComponent.find( entity );

	if ( it != aMapEntityToComponent.end() )
	{
		size_t index = it->second.aIndex;
		// return &aComponents[ index ];
		return aComponents[ index ];
	}

	return nullptr;
}


void* EntityComponentPool::GetData( ComponentID_t sComponentID )
{
	PROF_SCOPE();

	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );

	if ( sComponentID.aIndex > aComponentIDs.size() )
	{
		Log_ErrorF( "Invalid Component ID: %zd\n", sComponentID.aIndex );
		return nullptr;
	}

	return aComponents[ sComponentID.aIndex ];
}


// Marks this component as predicted
void EntityComponentPool::SetPredicted( Entity entity, bool sPredicted )
{
	CH_ASSERT( aMapComponentToEntity.size() == aMapEntityToComponent.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentFlags.size() );
	CH_ASSERT( aMapComponentToEntity.size() == aComponentIDs.size() );

	auto it = aMapEntityToComponent.find( entity );

	if ( it == aMapEntityToComponent.end() )
	{
		Log_ErrorF( gLC_Entity, "Failed to mark Entity Component as predicted, Entity does not have this component - \"%s\"\n", apName );
		return;
	}

	if ( sPredicted )
	{
		// We want this component predicted, add it to the prediction set
		aComponentFlags[ it->second ] |= EEntityFlag_Predicted;
	}
	else
	{
		// We don't want this component predicted, remove the prediction flag from it
		aComponentFlags[ it->second ] &= ~EEntityFlag_Predicted;
	}
}


bool EntityComponentPool::IsPredicted( Entity entity )
{
	PROF_SCOPE();

	auto it = aMapEntityToComponent.find( entity );

	if ( it == aMapEntityToComponent.end() )
	{
		Log_ErrorF( gLC_Entity, "Failed to get Entity Component Flags, Entity does not have this component - \"%s\"\n", apName );
		return false;
	}

	return aComponentFlags.at( it->second ) & EEntityFlag_Predicted;
}


bool EntityComponentPool::IsPredicted( ComponentID_t sComponentID )
{
	PROF_SCOPE();

	if ( sComponentID.aIndex > aComponentIDs.size() )
	{
		Log_ErrorF( "Invalid Component ID: %zd\n", sComponentID.aIndex );
		return false;
	}

	return aComponentFlags.at( sComponentID ) & EEntityFlag_Predicted;
}


// How Many Components are in this Pool?
size_t EntityComponentPool::GetCount()
{
	return aComponentIDs.size();
}

