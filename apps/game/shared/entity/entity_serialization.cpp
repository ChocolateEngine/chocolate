#include "main.h"
#include "game_shared.h"
#include "entity.h"
#include "core/util.h"
#include "player.h"  // TEMP - for CPlayerMoveData

#include "entity_systems.h"
#include "mapmanager.h"
#include "igui.h"

#include "igraphics.h"

#include "game_physics.h"  // just for IPhysicsShape* and IPhysicsObject*


LOG_CHANNEL( Entity );

CONVAR_BOOL( ent_always_full_update, 0, "For debugging, always send a full update" );
CONVAR_BOOL( ent_show_component_net_updates, 0, "Show Component Network Updates" );


// Read and write from the network
void Entity_ReadEntityUpdates( const NetMsg_EntityUpdates* spMsg )
{
	PROF_SCOPE();

	auto entityUpdateList = spMsg->update_list();

	if ( !entityUpdateList )
		return;

	for ( size_t i = 0; i < entityUpdateList->size(); i++ )
	{
		const NetMsg_EntityUpdate* entityUpdate = entityUpdateList->Get( i );

		if ( !entityUpdate )
			continue;

		Entity entId = entityUpdate->id();

		if ( entityUpdate->destroyed() )
		{
			Entity entity = Entity_TranslateEntityID( entId, false );
			if ( entity != CH_ENT_INVALID )
				Entity_DeleteEntity( entity );
			else
				Log_Error( gLC_Entity, "Trying to delete entity not in translation list\n" );

			continue;
		}
		else
		{
			Entity entity = Entity_TranslateEntityID( entId, true );

			if ( !Entity_EntityExists( entity ) )
			{
				Log_Error( gLC_Entity, "wtf entity in translation list doesn't actually exist?\n" );
				continue;
			}
			else
			{
				// Check for an entity parent
				Entity parent = Entity_TranslateEntityID( entityUpdate->parent(), true );

				if ( parent == CH_ENT_INVALID )
					continue;

				Entity_ParentEntity( entity, parent );
			}
		}
	}
}


// TODO: redo this by having it loop through component pools, and not entitys
// right now, it's doing a lot of entirely unnecessary checks
// we can avoid those if we loop through the pools instead
void Entity_WriteEntityUpdates( flatbuffers::FlatBufferBuilder& srBuilder )
{
	PROF_SCOPE();

	CH_ASSERT( Entity_GetEntityCount() == EntSysData().aEntityFlags.size() );

	std::vector< flatbuffers::Offset< NetMsg_EntityUpdate > > updateOut;
	updateOut.reserve( Entity_GetEntityCount() );

	for ( auto& [ entity, flags ] : EntSysData().aEntityFlags )
	{
		// Make sure this and all the parents are networked
		if ( !Entity_IsNetworked( entity, flags ) )
			continue;

		// NetMsg_EntityUpdateBuilder& update = updateBuilderList.emplace_back( srBuilder );
		NetMsg_EntityUpdateBuilder update( srBuilder );

		update.add_id( entity );

		// Get Entity State
		if ( flags & EEntityFlag_Destroyed )
		{
			update.add_destroyed( true );
		}
		else
		{
			// doesn't matter if it returns CH_ENT_INVALID
			if ( flags & EEntityFlag_Parented )
				update.add_parent( Entity_GetParent( entity ) );
			else
				update.add_parent( CH_ENT_INVALID );
		}

		updateOut.push_back( update.Finish() );
		// srBuilder.Finish( update.Finish() );
	}

	auto                        vec = srBuilder.CreateVector( updateOut );

	NetMsg_EntityUpdatesBuilder updatesBuilder( srBuilder );
	updatesBuilder.add_update_list( vec );

	srBuilder.Finish( updatesBuilder.Finish() );
}


// Read and write from the network
//void Entity_ReadComponentUpdates( capnp::MessageReader& srReader )
//{
//}


void ReadComponent( flexb::Reference& spSrc, EntComponentData_t* spRegData, void* spData )
{
	PROF_SCOPE();

	// Get the vector i guess
	auto   vector    = spSrc.AsVector();
	size_t i         = 0;
	size_t curOffset = 0;

	for ( const auto& [ offset, var ] : spRegData->aVars )
	{
		if ( var.aFlags & ECompRegFlag_LocalVar )
		{
			curOffset += var.aSize + offset;
			continue;
		}

		// size_t offset = EntComp_GetVarDirtyOffset( (char*)spData, var.aType );
		void* data = ( (char*)spData ) + offset;
		// curOffset += var.aSize + offset;

		// Check these first
		switch ( var.aType )
		{
			case EEntNetField_Invalid:
				continue;

			case EEntNetField_Bool:
			{
				bool* value = static_cast< bool* >( data );
				*value      = vector[ i++ ].AsBool();
				continue;
			}
		}

		// Now, check if we wrote data for this var
		bool wroteVar = vector[ i++ ].AsBool();

		if ( !wroteVar )
			continue;

		CH_ASSERT( var.aType != EEntNetField_Invalid );
		CH_ASSERT( var.aType != EEntNetField_Bool );

		switch ( var.aType )
		{
			default:
				break;

			case EEntNetField_Float:
			{
				auto value = (float*)( data );
				*value     = vector[ i++ ].AsFloat();
				break;
			}
			case EEntNetField_Double:
			{
				auto value = (double*)( data );
				*value     = vector[ i++ ].AsDouble();
				break;
			}

			case EEntNetField_S8:
			{
				auto value = (s8*)( data );
				*value     = vector[ i++ ].AsInt8();
				break;
			}
			case EEntNetField_S16:
			{
				auto value = (s16*)( data );
				*value     = vector[ i++ ].AsInt16();
				break;
			}
			case EEntNetField_S32:
			{
				auto value = (s32*)( data );
				*value     = vector[ i++ ].AsInt32();
				break;
			}
			case EEntNetField_S64:
			{
				auto value = (s64*)( data );
				*value     = vector[ i++ ].AsInt64();
				break;
			}

			case EEntNetField_U8:
			{
				auto value = (u8*)( data );
				*value     = vector[ i++ ].AsUInt8();
				break;
			}
			case EEntNetField_U16:
			{
				auto value = (u16*)( data );
				*value     = vector[ i++ ].AsUInt16();
				break;
			}
			case EEntNetField_U32:
			{
				auto value = (u32*)( data );
				*value     = vector[ i++ ].AsUInt32();
				break;
			}
			case EEntNetField_U64:
			{
				auto value = (u64*)( data );
				*value     = vector[ i++ ].AsUInt64();
				break;
			}

			case EEntNetField_Entity:
			{
				auto value      = (Entity*)( data );
				auto recvEntity = (Entity)( vector[ i++ ].AsUInt64() );

				if ( recvEntity == CH_ENT_INVALID )
				{
					*value = CH_ENT_INVALID;
					break;
				}

				Entity convertEntity = Entity_TranslateEntityID( recvEntity );

				if ( convertEntity == CH_ENT_INVALID )
				{
					Log_Error( gLC_Entity, "Can't find Networked Entity ID\n" );
				}
				else
				{
					*value = convertEntity;
				}

				break;
			}

			case EEntNetField_StdString:
			{
				auto value = (std::string*)( data );
				*value     = vector[ i++ ].AsString().str();
				break;
			}

				// Will have a special case for these once i have each value in a vecX marked dirty

			case EEntNetField_Vec2:
			{
				auto value = (glm::vec2*)( data );
				value->x   = vector[ i++ ].AsFloat();
				value->y   = vector[ i++ ].AsFloat();
				break;
			}
			case EEntNetField_Color3:
			case EEntNetField_Vec3:
			{
				auto value = (glm::vec3*)( data );

				// auto typedVector = vector[ i++ ].AsTypedVector();
				// CH_ASSERT( typedVector.size() == 3 );
				// 
				// value->x = typedVector[ 0 ].AsFloat();
				// value->y = typedVector[ 1 ].AsFloat();
				// value->z = typedVector[ 2 ].AsFloat();

				value->x   = vector[ i++ ].AsFloat();
				value->y   = vector[ i++ ].AsFloat();
				value->z   = vector[ i++ ].AsFloat();
				break;
			}
			case EEntNetField_Color4:
			case EEntNetField_Vec4:
			{
				auto value = (glm::vec4*)( data );
				value->x   = vector[ i++ ].AsFloat();
				value->y   = vector[ i++ ].AsFloat();
				value->z   = vector[ i++ ].AsFloat();
				value->w   = vector[ i++ ].AsFloat();
				break;
			}

			case EEntNetField_Quat:
			{
				auto value = (glm::quat*)( data );
				value->x   = vector[ i++ ].AsFloat();
				value->y   = vector[ i++ ].AsFloat();
				value->z   = vector[ i++ ].AsFloat();
				value->w   = vector[ i++ ].AsFloat();
				break;
			}
		}
	}
}


bool WriteComponent( flexb::Builder& srBuilder, EntComponentData_t* spRegData, const void* spData, bool sFullUpdate )
{
	PROF_SCOPE();

	bool   wroteData = false;
	size_t curOffset = 0;
	size_t flexVec   = srBuilder.StartVector();

	for ( const auto& [ offset, var ] : spRegData->aVars )
	{
		if ( var.aFlags & ECompRegFlag_LocalVar )
			continue;

		PROF_SCOPE();
		CH_PROF_ZONE_NAME( var.apName, var.aNameLen );

		auto IsVarDirty = [ & ]( bool isBool = false )
		{
			// We always write this if it's a full update
			if ( sFullUpdate )
			{
				wroteData = true;

				if ( !isBool )
					srBuilder.Bool( true );

				return true;
			}

			char* dataChar = (char*)spData;
			bool isDirty  = *reinterpret_cast< bool* >( dataChar + var.aSize + offset );
			
			wroteData |= isDirty;

			if ( isBool )
				return true;

			{
				srBuilder.Bool( isDirty );
				return isDirty;
			}

		};

		void* data = ( (char*)spData ) + offset;

		switch ( var.aType )
		{
			default:
				srBuilder.Bool( false );
				break;

			case EEntNetField_Invalid:
				break;

			case EEntNetField_Bool:
			{
				if ( IsVarDirty( true ) )
				{
					auto value = *(bool*)( data );
					srBuilder.Bool( value );
					wroteData = true;
				}
				break;
			}

			case EEntNetField_Float:
			{
				if ( IsVarDirty() )
				{
					auto value = *(float*)( data );
					srBuilder.Float( value );
				}
				
				break;
			}
			case EEntNetField_Double:
			{
				if ( IsVarDirty() )
				{
					auto value = *(double*)( data );
					srBuilder.Double( value );
				}
				
				break;
			}

			case EEntNetField_S8:
			{
				// FLEX BUFFERS STORES ALL INTS AND UINTS AS INT64 AND UINT64, WHAT A WASTE OF SPACE
				if ( IsVarDirty() )
				{
					auto value = *(s8*)( data );
					srBuilder.Add( value );
				}

				break;
			}
			case EEntNetField_S16:
			{
				if ( IsVarDirty() )
				{
					auto value = *(s16*)( data );
					srBuilder.Add( value );
				}

				break;
			}
			case EEntNetField_S32:
			{
				if ( IsVarDirty() )
				{
					auto value = *(s32*)( data );
					srBuilder.Add( value );
				}

				break;
			}
			case EEntNetField_S64:
			{
				if ( IsVarDirty() )
				{
					auto value = *(s64*)( data );
					srBuilder.Add( value );
				}

				break;
			}

			case EEntNetField_U8:
			{
				// FLEX BUFFERS STORES ALL INTS AND UINTS AS INT64 AND UINT64, WHAT A WASTE OF SPACE
				if ( IsVarDirty() )
				{
					auto value = *(u8*)( data );
					srBuilder.Add( value );
				}

				break;
			}
			case EEntNetField_U16:
			{
				if ( IsVarDirty() )
				{
					auto value = *(u16*)( data );
					srBuilder.Add( value );
				}

				break;
			}
			case EEntNetField_U32:
			{
				if ( IsVarDirty() )
				{
					auto value = *(u32*)( data );
					srBuilder.Add( value );
				}

				break;
			}
			case EEntNetField_U64:
			{
				if ( IsVarDirty() )
				{
					auto value = *(u64*)( data );
					srBuilder.Add( value );
				}

				break;
			}

			case EEntNetField_Entity:
			{
				if ( IsVarDirty() )
				{
					auto value = *(Entity*)( data );
					srBuilder.Add( value );
				}

				break;
			}

			case EEntNetField_StdString:
			{
				if ( IsVarDirty() )
				{
					auto value = (const std::string*)( data );
					srBuilder.Add( value->c_str() );
				}

				break;
			}

			// Will have a special case for these once i have each value in a vecX marked dirty

			case EEntNetField_Vec2:
			{
				if ( IsVarDirty() )
				{
					const glm::vec2* value = (const glm::vec2*)( data );
					srBuilder.Add( value->x );
					srBuilder.Add( value->y );
				}

				break;
			}
			case EEntNetField_Color3:
			case EEntNetField_Vec3:
			{
				if ( IsVarDirty() )
				{
					const glm::vec3* value = (const glm::vec3*)( data );

					// no idea if this is better or worse
					// std::vector< float > float3;
					// float3.reserve( 3 );
					// float3.push_back( value->x );
					// float3.push_back( value->y );
					// float3.push_back( value->z );
					// srBuilder.Add( float3 );

					srBuilder.Add( value->x );
					srBuilder.Add( value->y );
					srBuilder.Add( value->z );
				}

				break;
			}
			case EEntNetField_Color4:
			case EEntNetField_Vec4:
			{
				if ( IsVarDirty() )
				{
					const glm::vec4* value = (const glm::vec4*)( data );
					srBuilder.Add( value->x );
					srBuilder.Add( value->y );
					srBuilder.Add( value->z );
					srBuilder.Add( value->w );
				}

				break;
			}

			case EEntNetField_Quat:
			{
				if ( IsVarDirty() )
				{
					const glm::quat* value = (const glm::quat*)( data );
					srBuilder.Add( value->x );
					srBuilder.Add( value->y );
					srBuilder.Add( value->z );
					srBuilder.Add( value->w );
				}

				break;
			}
		}
	}

	if ( !wroteData )
		return false;

	{
		PROF_SCOPE_NAMED( "EndVector" );
		srBuilder.EndVector( flexVec, false, false );
	}

	return wroteData;
}


void Entity_WriteComponentUpdates( fb::FlatBufferBuilder& srRootBuilder, bool sFullUpdate )
{
	PROF_SCOPE();

	std::vector< fb::Offset< NetMsg_ComponentUpdate > > componentsBuilt;
	componentsBuilt.reserve( EntSysData().aComponentPools.size() );

	size_t i = 0;
	size_t poolCount = 0;

	flexb::Builder flexBuilder;

	for ( auto& [ poolName, pool ] : EntSysData().aComponentPools )
	{
		// If there are no components in existence, don't even bother to send anything here
		if ( !pool->aMapComponentToEntity.size() )
			continue;

		PROF_SCOPE_NAMED( "Pool" );

		EntComponentData_t* regData = pool->GetRegistryData();
		CH_PROF_ZONE_NAME( regData->apName, regData->aNameLen );

		// if ( regData->aNetType != EEntComponentNetType_Both || regData->aNetType != EEntComponentNetType_Server )
		if ( regData->aNetType != EEntComponentNetType_Both )
			continue;

		poolCount++;

		std::vector< fb::Offset< NetMsg_ComponentUpdateData > > componentDataBuilt;

		bool                                                    builtUpdateList = false;
		bool                                                    wroteData       = false;

		componentDataBuilt.reserve( pool->aMapComponentToEntity.size() );

		size_t compListI = 0;
		for ( auto& [ componentID, entity ] : pool->aMapComponentToEntity )
		{
			PROF_SCOPE_NAMED( "Entity" );

			EEntityFlag entFlags = EntSysData().aEntityFlags.at( entity );

			// skip the IsNetworked or CanSaveToMap call
			if ( entFlags & EEntityFlag_Destroyed )
			{
				// Don't bother sending data if we're about to be destroyed
				compListI++;
				continue;
			}

			EEntityFlag compFlags           = pool->aComponentFlags.at( componentID );

			bool        shouldSkipComponent = false;

			// check if the entity itself will be destroyed
			// shouldSkipComponent |= ( (entFlags & EEntityFlag_Destroyed) && !sFullUpdate );

			// check if the entity isn't networked
			shouldSkipComponent |= !Entity_IsNetworked( entity, entFlags );
			shouldSkipComponent |= compFlags & EEntityFlag_Local;

			if ( !sFullUpdate && !( compFlags & EEntityFlag_Created ) )
				shouldSkipComponent |= regData->aVars.empty();

			// Have we determined we should skip this component?
			if ( shouldSkipComponent )
			{
				compListI++;
				continue;
			}

			fb::Offset< fb::Vector< u8 > > dataVector;
			void*                          data = pool->aComponents[ componentID.aIndex ];

			// Constructing flexBuilder is slow, so only do that if we have variables on this component
			// Also make sure the component isn't being destroyed
			if ( regData->aVars.size() && !( compFlags & EEntityFlag_Destroyed ) )
			{
				PROF_SCOPE_NAMED( "FlexBuilder" )

				// Write Component Data
				flexBuilder.Clear();
				wroteData = WriteComponent( flexBuilder, regData, data, ent_always_full_update ? true : sFullUpdate );

				if ( wroteData )
				{
					flexBuilder.Finish();
					dataVector = srRootBuilder.CreateVector( flexBuilder.GetBuffer().data(), flexBuilder.GetSize() );

#if CH_SERVER
					if ( ent_show_component_net_updates )
					{
						Log_DevF( gLC_Entity, 2, "Sending Component Write Update to Clients: \"%s\" - %zd bytes", regData->apName, flexBuilder.GetSize() );
					}
#endif
				}
			}

			// Now after creating the data vector, we can make the update data builder
			{
				PROF_SCOPE_NAMED( "ComponentUpdateData" );

				// NetMsg_ComponentUpdateDataBuilder& compDataBuilder = componentDataBuilders.emplace_back( srRootBuilder );
				NetMsg_ComponentUpdateDataBuilder compDataBuilder( srRootBuilder );

				compDataBuilder.add_id( entity );

				// Set Destroyed
				if ( compFlags & EEntityFlag_Destroyed )
					compDataBuilder.add_destroyed( true );

				if ( wroteData )
					compDataBuilder.add_values( dataVector );

				builtUpdateList = true;
				componentDataBuilt.push_back( compDataBuilder.Finish() );
			}

			// Reset Component Var Dirty Values
			if ( !ent_always_full_update )
			{
				for ( const auto& [ offset, var ] : regData->aVars )
				{
					if ( var.aFlags & ECompRegFlag_LocalVar )
						continue;

					char* dataChar = static_cast< char* >( data );
					// EntComp_ResetVarDirty( dataChar + offset, var.aType );

					bool* aIsDirty = reinterpret_cast< bool* >( dataChar + var.aSize + offset );
					*aIsDirty      = false;
				}
			}

			compListI++;
		}

		if ( componentDataBuilt.size() && builtUpdateList )
		{
			PROF_SCOPE_NAMED( "Building Component Update" );

			// oh my god
			fb::Offset< fb::Vector< fb::Offset< NetMsg_ComponentUpdateData > > > compVector;

			//if ( wroteData )
				compVector = srRootBuilder.CreateVector( componentDataBuilt.data(), componentDataBuilt.size() );

			auto                          compNameOffset = srRootBuilder.CreateString( poolName );

			NetMsg_ComponentUpdateBuilder compUpdate( srRootBuilder );
			compUpdate.add_name( compNameOffset );
			// compUpdate.add_hash( regData->aHash );

			//if ( wroteData )
				compUpdate.add_components( compVector );

			componentsBuilt.push_back( compUpdate.Finish() );

#if CH_SERVER
			if ( ent_show_component_net_updates )
			{
				Log_DevF( gLC_Entity, 2, "Size of Component Write Update for \"%s\": %zd bytes", pool->apName, srRootBuilder.GetSize() );
			}
#endif
		}
		else
		{
			// NetMsg_ComponentUpdateBuilder& compUpdate = componentBuilders.emplace_back( srRootBuilder );
			// componentsBuilt.push_back( compUpdate.Finish() );
		}

		// if ( Game_IsClient() && ent_show_component_net_updates )
		// {
		// 	gui->DebugMessage( "Full Component Write Update: \"%s\" - %zd bytes", regData->apName, componentDataBuilt.size() * sizeof( flatbuffers::Offset< NetMsg_ComponentUpdateData > ) );
		// }

		i++;
	}

	auto                           updateListOut = srRootBuilder.CreateVector( componentsBuilt.data(), componentsBuilt.size() );

	NetMsg_ComponentUpdatesBuilder root( srRootBuilder );
	root.add_update_list( updateListOut );

	srRootBuilder.Finish( root.Finish() );

#if CH_SERVER
	if ( ent_show_component_net_updates )
	{
		Log_DevF( gLC_Entity, 2, "Sending \"%zd\" Component Types", poolCount );
		Log_DevF( gLC_Entity, 2, "Total Size of Component Write Update: %zd bytes", srRootBuilder.GetSize() );
	}
#endif
}


void Entity_ReadComponentUpdates( const NetMsg_ComponentUpdates* spReader )
{
	PROF_SCOPE();

	// First, reset all dirty variables
	for ( auto& [ name, pool ] : EntSysData().aComponentPools )
	{
		EntComponentData_t* regData = pool->GetRegistryData();

		for ( auto& [entity, compIndex] : pool->aMapEntityToComponent )
		{
			void* componentData = pool->aComponents[ compIndex.aIndex ];

			// Reset Component Var Dirty Values
			for ( const auto& [ offset, var ] : regData->aVars )
			{
				if ( var.aFlags & ECompRegFlag_LocalVar )
					continue;

				char* dataChar = static_cast< char* >( componentData );
				bool* isDirty  = reinterpret_cast< bool* >( dataChar + var.aSize + offset );
				*isDirty       = false;
			}
		}
	}

	auto componentUpdateList = spReader->update_list();

	if ( !componentUpdateList )
		return;

	for ( size_t i = 0; i < componentUpdateList->size(); i++ )
	{
		PROF_SCOPE();

		const NetMsg_ComponentUpdate* componentUpdate = componentUpdateList->Get( i );

		if ( !componentUpdate )
			continue;

		if ( !componentUpdate->name() )
			continue;

		// This shouldn't even be networked if we don't have any components
		if ( !componentUpdate->components() )
			continue;

		std::string_view     componentName = componentUpdate->name()->string_view();

		EntityComponentPool* pool          = Entity_GetComponentPool( componentName );

		if ( CH_IF_ASSERT_MSG( pool, "Failed to find component pool" ) )
		{
			Log_ErrorF( "Failed to find component pool for component: \"%s\"\n", componentName.data() );
			continue;
		}

		CH_PROF_ZONE_NAME_STR( componentName );

		IEntityComponentSystem* system  = pool->apComponentSystem;
		EntComponentData_t*     regData = pool->GetRegistryData();

		CH_ASSERT_MSG( regData, "Failed to find component registry data" );

		// if ( componentUpdate->hash() != regData->aHash )
		// {
		// 	Log_ErrorF( "Hash of component \"%s\" differs from what we have (got %zd, expected %zd)\n",
		// 		componentName.data(), componentUpdate->hash(), regData->aHash );
		// 	continue;
		// }

		for ( size_t c = 0; c < componentUpdate->components()->size(); c++ )
		{
			PROF_SCOPE();

			const NetMsg_ComponentUpdateData* componentUpdateData = componentUpdate->components()->Get( c );

			if ( !componentUpdateData )
				continue;

			// Is this entity in the translation system?
			Entity entity = Entity_TranslateEntityID( componentUpdateData->id(), false );
			if ( entity == CH_ENT_INVALID )
			{
				Log_Error( gLC_Entity, "Failed to find entity while updating components from server\n" );
				continue;
			}

			// std::string scopeTest = vstring( "Entity %zd", entity );
			// CH_PROF_ZONE_NAME( scopeTest.c_str(), scopeTest.size() );

			// Check the component state, do we need to remove it, or add it to the entity?
			if ( componentUpdateData->destroyed() )
			{
				// We can just remove the component right now, no need to queue it,
				// as this is before all client game processing
				pool->Remove( entity );
				continue;
			}

			// We don't use the component pool functions to reduce calls to aMapEntityToComponent
			// We only need to find the componentID once (or twice when creating the component), and gain some speed up
			ComponentID_t componentID{ SIZE_MAX };

			void*         componentData = nullptr;
			auto          entToCompIt   = pool->aMapEntityToComponent.find( entity );

			if ( entToCompIt != pool->aMapEntityToComponent.end() )
			{
				componentID = entToCompIt->second;
				componentData = pool->aComponents[ entToCompIt->second.aIndex ];
			}
			else
			{
				// Create the component
				componentData = pool->Create( entity );

				if ( componentData == nullptr )
				{
					Log_ErrorF( "Failed to create component\n" );
					continue;
				}

				componentID = pool->aMapEntityToComponent.at( entity );
			}

#if CH_CLIENT
			// Now, update component data
			// NOTE: i could try to check if it's predicted here and get rid of aOverrideClient
			if ( ( regData->aFlags & ECompRegFlag_DontOverrideClient ) && entity == gLocalPlayer )
				continue;

			// a bit of a hack and not implemented properly
			if ( pool->aComponentFlags.at( componentID ) & EEntityFlag_Predicted )
				continue;
#endif

			if ( componentUpdateData->values() )
			{
				auto             values   = componentUpdateData->values();
				flexb::Reference flexRoot = flexb::GetRoot( values->data(), values->size() );

				ReadComponent( flexRoot, regData, componentData );
				// regData->apRead( componentVerifier, values->data(), componentData );

				Log_DevF( gLC_Entity, 3, "Parsed component data for entity \"%zd\" - \"%s\"\n", entity, componentName );

				if ( system )
				{
					pool->aComponentsUpdated.push_front( componentID );
				}
			}
		}
	}
}

