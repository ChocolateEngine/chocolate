#include "entity.h"
#include "main.h"
#include "core/resource.h"

#define NAME_LEN 64


LOG_CHANNEL_REGISTER( Entity, ELogColor_DarkPurple );


static ResourceList< Entity_t >                      gEntityList;
static std::unordered_set< ch_handle_t >              gEntityDirtyList;
static std::unordered_map< ch_handle_t, glm::mat4 >   gEntityWorldMatrices;

// Entity Parents
// [ child ] = parent
static std::unordered_map< ch_handle_t, ch_handle_t >  gEntityParents;
// static std::unordered_set< glm::vec3 >               gUsedColors;


void Entity_CalcWorldMatrix( glm::mat4& srMat, ch_handle_t sEntity );


bool Entity_Init()
{
	return true;
}


void Entity_Shutdown()
{
	// TODO: delete all entities
}


void Entity_Update()
{
	// update renderables

	// for ( ch_handle_t entityHandle : gEntityList.aHandles )
	for ( ch_handle_t entityHandle : gEntityDirtyList )
	{
		Entity_t* ent = nullptr;
		if ( !gEntityList.Get( entityHandle, &ent ) )
			continue;

		glm::mat4 worldMatrix;
		Entity_CalcWorldMatrix( worldMatrix, entityHandle );

		// Update Light Position and Angle
		if ( ent->apLight )
		{
			if ( ent->aHidden )
				ent->apLight->aEnabled = false;
			else
				ent->apLight->aEnabled = ent->aLightEnabled;

			ent->apLight->aPos = Util_GetMatrixPosition( worldMatrix );
			ent->apLight->aRot = Util_GetMatrixRotation( worldMatrix );

			// blech
			graphics->UpdateLight( ent->apLight );
		}

		if ( !ent->aRenderable )
			continue;

		Renderable_t* renderable = graphics->GetRenderableData( ent->aRenderable );
		//renderable->aVisible = !ent->aHidden;
		renderable->aModelMatrix = worldMatrix;

		graphics->UpdateRenderableAABB( ent->aRenderable );
	}

	gEntityDirtyList.clear();
}


ch_handle_t Entity_Create()
{
	EditorContext_t* context = Editor_GetContext();

	if ( !context )
		return CH_INVALID_HANDLE;

	Entity_t* ent = nullptr;
	ch_handle_t entHandle = gEntityList.Create( &ent );

	if ( entHandle == CH_INVALID_HANDLE )
		return CH_INVALID_HANDLE;

	ent->aTransform.aScale.x = 1.f;
	ent->aTransform.aScale.y = 1.f;
	ent->aTransform.aScale.z = 1.f;

	context->aMap.aMapEntities.push_back( entHandle );

	ent->name = ch_str_copy_f( "Entity %zd", entHandle );
	if ( ent->name.data == nullptr )
	{
		Log_Error( "Failed to allocate memory for entity name\n" );
		Entity_Delete( entHandle );
		return CH_INVALID_HANDLE;
	}

	// Pick a random color for it
	ent->aSelectColor[ 0 ] = rand_u8( 0, 255 );
	ent->aSelectColor[ 1 ] = rand_u8( 0, 255 );
	ent->aSelectColor[ 2 ] = rand_u8( 0, 255 );

	ent->aLightEnabled     = true;

	Log_DevF( 2, "Created Entity With Selection Color of (%d, %d, %d)\n", ent->aSelectColor[ 0 ], ent->aSelectColor[ 1 ], ent->aSelectColor[ 2 ] );

	gEntityDirtyList.emplace( entHandle );
	gEntityWorldMatrices[ entHandle ] = glm::identity< glm::mat4 >();

	return entHandle;
}


void Entity_Delete( ch_handle_t sHandle )
{
	// TODO: queue this for deletion, like in the game? or is that not needed here

	Entity_t* ent = nullptr;

	if ( !gEntityList.Get( sHandle, &ent ) )
	{
		Log_ErrorF( "Invalid Entity: %d", sHandle );
		return;
	}

	// Check if this entity is in the dirty list
	auto it = std::find( gEntityDirtyList.begin(), gEntityDirtyList.end(), sHandle );

	if ( it != gEntityDirtyList.end() )
	{
		gEntityDirtyList.erase( it );
	}

	// remove the world matrix
	gEntityWorldMatrices.erase( sHandle );

	if ( ent->name.data )
	{
		ch_str_free( ent->name.data );
		ent->name.data = nullptr;
		ent->name.size = 0;
	}

	// Check if this entity has a light on it
	if ( ent->apLight )
	{
		graphics->DestroyLight( ent->apLight );
	}

	// Check if we have a renderable on this entity
	if ( ent->aRenderable != CH_INVALID_HANDLE )
	{
		graphics->FreeRenderable( ent->aRenderable );
	}

	// Check if we have a model on this entity
	if ( ent->aModel != CH_INVALID_HANDLE )
	{
		graphics->FreeModel( ent->aModel );
	}

	// Clear Parent and Children
	ChVector< ch_handle_t > children;
	Entity_GetChildrenRecurse( sHandle, children );

	// Mark all of them as destroyed
	for ( ch_handle_t child : children )
	{
		Entity_Delete( child );
		Log_DevF( gLC_Entity, 2, "Deleting Child Entity (parent %zd): %zd\n", sHandle, child );
	}

	// Search All Contexts for this entity

	u32              i   = 0;
	EditorContext_t* ctx = nullptr;
	for ( ; i < gEditorContexts.aHandles.size(); i++ )
	{
		if ( !gEditorContexts.Get( gEditorContexts.aHandles[ i ], &ctx ) )
			continue;

		u32 j = 0;
		for ( ; j < ctx->aMap.aMapEntities.size(); j++ )
		{
			if ( ctx->aMap.aMapEntities[ j ] == sHandle )
			{
				ctx->aMap.aMapEntities.erase( sHandle );
				break;
			}
		}

		if ( j != ctx->aMap.aMapEntities.size() )
			break;
	}

	gEntityList.Remove( sHandle );
}


Entity_t* Entity_GetData( ch_handle_t sHandle )
{
	Entity_t* ent = nullptr;

	if ( gEntityList.Get( sHandle, &ent ) )
	{
		return ent;
	}

	Log_ErrorF( "Invalid Entity: %d", sHandle );
	return nullptr;
}


const std::vector< ch_handle_t > &Entity_GetHandleList()
{
	return gEntityList.aHandles;
}


void Entity_SetName( ch_handle_t sHandle, const char* name, s64 nameLen )
{
	Entity_t* ent = nullptr;

	if ( !gEntityList.Get( sHandle, &ent ) )
	{
		Log_ErrorF( "Invalid Entity: %d", sHandle );
		return;
	}

	if ( name == nullptr )
		return;

	if ( nameLen == -1 )
		nameLen = strlen( name );

	if ( nameLen == 0 )
		return;

	ent->name = ch_str_realloc( ent->name.data, name, nameLen );
}


void Entity_SetEntityVisible( ch_handle_t sEntity, bool sVisible )
{
	Entity_t* ent = nullptr;

	if ( !gEntityList.Get( sEntity, &ent ) )
	{
		Log_ErrorF( "Invalid Entity: %d", sEntity );
		return;
	}

	ent->aHidden = !sVisible;

	if ( ent->apLight )
	{
		ent->apLight->aEnabled != sVisible;
		graphics->UpdateLight( ent->apLight );
	}

	// no renderable to hide
	if ( ent->aRenderable == CH_INVALID_HANDLE )
		return;

	Renderable_t* renderable = graphics->GetRenderableData( ent->aRenderable );
	
	if ( !renderable )
	{
		Log_ErrorF( gLC_Entity, "Entity Missing Renderable: %d", sEntity );
		ent->aRenderable = CH_INVALID_HANDLE;
		return;
	}

	renderable->aVisible = sVisible;

	gEntityDirtyList.emplace( sEntity );
}


struct EntityVisibleData_t
{
};


inline void Entity_SetEntitiesVisibleBase( Entity_t** entity_list, u32 count, bool visible )
{
	for ( u32 i = 0; i < count; i++ )
	{
		Entity_t* ent = entity_list[ i ];

		// TODO: put this entity in a different list
		// like entities_visible
		ent->aHidden = !visible;

		if ( ent->apLight )
		{
			ent->apLight->aEnabled = visible;
			graphics->UpdateLight( ent->apLight );
		}

		// no renderable to hide
		if ( ent->aRenderable == CH_INVALID_HANDLE )
			continue;

		// maybe make an array to store renderable handles, and then do a for loop after to mark all of them hidden quickly?
		// though you would need to allocate and free memory for this, hmmm
		Renderable_t* renderable = graphics->GetRenderableData( ent->aRenderable );

		if ( !renderable )
		{
			Log_ErrorF( gLC_Entity, "Entity Missing Renderable!\n" );
			ent->aRenderable = CH_INVALID_HANDLE;
			continue;
		}

		renderable->aVisible = visible;
	}
}


void Entity_SetEntitiesVisible( ch_handle_t* sEntities, u32 sCount, bool sVisible )
{
	Entity_t**                       entity_list = ch_malloc< Entity_t* >( sCount );
	u32                              new_count   = 0;
	std::unordered_set< ch_handle_t > child_entities; 

	for ( u32 i = 0; i < sCount; i++ )
	{
		if ( !gEntityList.Get( sEntities[ i ], &entity_list[ new_count ] ) )
		{
			Log_ErrorF( "Invalid Entity: %d", sEntities[ i ] );
			continue;
		}

		Entity_GetChildrenRecurse( sEntities[ i ], child_entities );
		new_count++;

		gEntityDirtyList.emplace( sEntities[ i ] );
	}

	if ( new_count == 0 )
	{
		free( entity_list );
		return;
	}

	if ( child_entities.size() )
	{
		Entity_t** entity_list_new = ch_realloc< Entity_t* >( entity_list, sCount + child_entities.size() );
		
		if ( entity_list_new == nullptr )
		{
			Log_ErrorF( "Failed to allocate memory for entities\n" );
			free( entity_list );
			return;
		}

		entity_list = entity_list_new;

		for ( const ch_handle_t& ent_handle : child_entities )
		{
			if ( !gEntityList.Get( ent_handle, &entity_list[ new_count ] ) )
			{
				Log_ErrorF( "Invalid Entity: %d", ent_handle );
				continue;
			}

			new_count++;

			gEntityDirtyList.emplace( ent_handle );
		}

		if ( new_count == 0 )
		{
			free( entity_list );
			return;
		}
	}

	Entity_SetEntitiesVisibleBase( entity_list, new_count, sVisible );

	free( entity_list );
}


void Entity_SetEntitiesVisibleNoChild( ch_handle_t* sEntities, u32 sCount, bool sVisible )
{
	// Entity_t** entity_list = ch_stack_alloc< Entity_t* >( sCount );
	Entity_t** entity_list = ch_malloc< Entity_t* >( sCount );
	u32        new_count   = 0;

	for ( u32 i = 0; i < sCount; i++ )
	{
		if ( !gEntityList.Get( sEntities[ i ], &entity_list[ new_count ] ) )
		{
			Log_ErrorF( "Invalid Entity: %d", sEntities[ i ] );
			continue;
		}

		new_count++;
	}

	if ( new_count == 0 )
	{
		ch_free( entity_list );
		return;
	}

	Entity_SetEntitiesVisibleBase( entity_list, new_count, sVisible );

	ch_free( entity_list );
}


void Entity_SetEntitiesDirty( ch_handle_t* sEntities, u32 sCount )
{
	for ( u32 i = 0; i < sCount; i++ )
	{
		// get child entities
		// Entity_GetChildrenRecurse( sEntities[ i ], gEntityDirtyList );

		gEntityDirtyList.emplace( sEntities[ i ] );
	}
}


// Get the highest level parent for this entity, returns self if not parented
ch_handle_t Entity_GetRootParent( ch_handle_t sSelf )
{
	PROF_SCOPE();

	auto it = gEntityParents.find( sSelf );

	if ( it != gEntityParents.end() )
		return Entity_GetRootParent( it->second );

	return sSelf;
}


// Recursively get all entities attached to this one (SLOW)
void Entity_GetChildrenRecurse( ch_handle_t sEntity, ChVector< ch_handle_t >& srChildren )
{
	PROF_SCOPE();

	for ( auto& [ child, parent ] : gEntityParents )
	{
		if ( parent != sEntity )
			continue;

		srChildren.push_back( child );
		Entity_GetChildrenRecurse( child, srChildren );
		break;
	}
}


// Recursively get all entities attached to this one (SLOW)
void Entity_GetChildrenRecurse( ch_handle_t sEntity, std::unordered_set< ch_handle_t >& srChildren )
{
	PROF_SCOPE();

	for ( auto& [ child, parent ] : gEntityParents )
	{
		if ( parent != sEntity )
			continue;

		srChildren.emplace( child );
		Entity_GetChildrenRecurse( child, srChildren );
		break;
	}
}


// Get child entities attached to this one (SLOW)
void Entity_GetChildren( ch_handle_t sEntity, ChVector< ch_handle_t >& srChildren )
{
	PROF_SCOPE();

	for ( auto& [ child, parent ] : gEntityParents )
	{
		if ( parent != sEntity )
			continue;

		srChildren.push_back( child );
	}
}


bool Entity_IsParented( ch_handle_t sEntity )
{
	if ( sEntity == CH_INVALID_HANDLE )
		return false;

	auto it = gEntityParents.find( sEntity );
	if ( it == gEntityParents.end() )
		return false;

	return true;
}


ch_handle_t Entity_GetParent( ch_handle_t sEntity )
{
	if ( sEntity == CH_INVALID_HANDLE )
		return CH_INVALID_HANDLE;

	auto it = gEntityParents.find( sEntity );
	if ( it == gEntityParents.end() )
		return CH_INVALID_HANDLE;

	return it->second;
}


void Entity_SetParent( ch_handle_t sEntity, ch_handle_t sParent )
{
	if ( sEntity == CH_INVALID_HANDLE || sEntity == sParent )
		return;

	// Make sure sParent isn't parented to sEntity
	auto it = gEntityParents.find( sParent );

	if ( it != gEntityParents.end() )
	{
		if ( it->second == sEntity )
		{
			Log_Error( gLC_Entity, "Trying to parent entity A to B, when B is already parented to A!!\n" );
			return;
		}
	}

	if ( sParent == CH_INVALID_HANDLE )
	{
		// Clear the parent
		gEntityParents.erase( sEntity );
	}
	else
	{
		gEntityParents[ sEntity ] = sParent;
	}

	gEntityDirtyList.emplace( sEntity );
}


const std::unordered_map< ch_handle_t, ch_handle_t >& Entity_GetParentMap()
{
	return gEntityParents;
}


// Returns a Model Matrix with parents applied in world space
// TODO: Cache this
void Entity_CalcWorldMatrix( glm::mat4& srMat, ch_handle_t sEntity )
{
	PROF_SCOPE();

	ch_handle_t parent = Entity_GetParent( sEntity );
	glm::mat4  parentMat( 1.f );

	if ( parent != CH_INVALID_HANDLE )
	{
		// Get the world matrix recursively
		Entity_CalcWorldMatrix( parentMat, parent );
	}

	Entity_t* entity = Entity_GetData( sEntity );

	if ( !entity )
	{
		Log_ErrorF( "Invalid Entity in GetWorldMatrix - %d\n", sEntity );
		srMat = glm::identity< glm::mat4 >();
		return;
	}

	// is this all the wrong order?

	// NOTE: THIS IS PROBABLY WRONG
	srMat = glm::translate( entity->aTransform.aPos );
	// srMat = glm::mat4( 1.f );

	glm::mat4 rotMat(1.f);

	// glm::vec3 temp = glm::radians( transform->aAng.Get() );
	// glm::quat rotQuat = temp;

// rotMat = glm::mat4_cast( rotQuat );

// srMat *= rotMat;
// srMat *= glm::eulerAngleYZX(
//   glm::radians(transform->aAng.Get().x ),
//   glm::radians(transform->aAng.Get().y ),
//   glm::radians(transform->aAng.Get().z ) );

	srMat *= glm::eulerAngleZYX(
		glm::radians( entity->aTransform.aAng[ ROLL ] ),
		glm::radians( entity->aTransform.aAng[ YAW ] ),
		glm::radians( entity->aTransform.aAng[ PITCH ] ) );

	// srMat *= glm::yawPitchRoll(
	//   glm::radians( transform->aAng.Get()[ PITCH ] ),
	//   glm::radians( transform->aAng.Get()[ YAW ] ),
	//   glm::radians( transform->aAng.Get()[ ROLL ] ) );

	srMat = glm::scale( srMat, entity->aTransform.aScale );

	srMat = parentMat * srMat;

	gEntityWorldMatrices[ sEntity ] = srMat;
}


void Entity_GetWorldMatrix( glm::mat4& srMat, ch_handle_t sEntity )
{
	// Is this entity in the dirty list?
	auto it = gEntityDirtyList.find( sEntity );

	if ( it != gEntityDirtyList.end() )
	{
		Entity_CalcWorldMatrix( srMat, sEntity );
		return;
	}

	// Check if we have a cached world matrix
	auto it2 = gEntityWorldMatrices.find( sEntity );

	if ( it2 != gEntityWorldMatrices.end() )
	{
		srMat = it2->second;
		return;
	}

	Log_ErrorF( "Entity %d does not have a cached world matrix, should never happen, but here we are\n", sEntity );
	Entity_CalcWorldMatrix( srMat, sEntity );
}


#if 0

ch_handle_t Entity_Create()
{
	EditorContext_t* context = Editor_GetContext();

	if ( !context )
		return CH_INVALID_HANDLE;

	Entity_t* ent = nullptr;
	return context->aEntities.Create( &ent );
}


void Entity_Delete( ch_handle_t sHandle )
{
	Entity_t*        ent = nullptr;

	// Search All Contexts for this entity

	u32              i   = 0;
	EditorContext_t* ctx = nullptr;
	for ( ; i < gEditorContexts.aHandles.size(); i++ )
	{
		if ( !gEditorContexts.Get( gEditorContexts.aHandles[ i ], &ctx ) )
			continue;

		if ( ctx->aEntities.Get( sHandle, &ent ) )
			break;
	}

	if ( i == gEditorContexts.aHandles.size() || ent == nullptr )
	{
		Log_ErrorF( "Invalid Entity: %d", sHandle );
		return;
	}

	// Check if we have a renderable on this entity
	if ( ent->aRenderable != CH_INVALID_HANDLE )
	{
		graphics->FreeRenderable( ent->aRenderable );
	}

	// Check if we have a model on this entity
	if ( ent->aModel != CH_INVALID_HANDLE )
	{
		graphics->FreeModel( ent->aModel );
	}

	ctx->aEntities.Remove( sHandle );
}

#endif

