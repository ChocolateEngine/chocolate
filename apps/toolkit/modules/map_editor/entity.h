#pragma once

#include "types/transform.h"
#include "igraphics.h"
#include "physics/iphysics.h"


#include <unordered_set>


// using Entity = size_t;


struct Color3
{
	u8 r;
	u8 g;
	u8 b;
};


// not sure i really need much of a component system for an editor
struct Entity_t
{
	ch_string          name;

	Transform          aTransform;

	// Rendering
	ch_handle_t         aModel;
	ch_handle_t         aRenderable;
	bool               aHidden;

	// Physics
	char*              apPhysicsModel;
	IPhysicsObject*    apPhysicsObject;

	// Lighting
	bool               aLightEnabled;
	Light_t*           apLight;
	ch_handle_t         aLightRenderable;

	// Audio

	// Color for Selecting with the cursor
	// IDEA: eventually you might need to select renderables based on material
	// so this would need to be an array of colors, with the same length as the material count on the current renderable
	u8                 aSelectColor[ 3 ];

	// This is used for selecting individual materials
	ChVector< Color3 > aMaterialColors;

	// List of Components with general data
};


bool                                                Entity_Init();
void                                                Entity_Shutdown();
void                                                Entity_Update();

ch_handle_t                                          Entity_Create();
void                                                Entity_Delete( ch_handle_t sHandle );

Entity_t*                                           Entity_GetData( ch_handle_t sHandle );
const std::vector< ch_handle_t >&                    Entity_GetHandleList();

void                                                Entity_SetName( ch_handle_t sHandle, const char* name, s64 nameLen = -1 );

void                                                Entity_SetEntityVisible( ch_handle_t sEntity, bool sVisible );
void                                                Entity_SetEntitiesVisible( ch_handle_t* sEntities, u32 sCount, bool sVisible );
void                                                Entity_SetEntitiesVisibleNoChild( ch_handle_t* sEntities, u32 sCount, bool sVisible );

// Do an update on these entities
void                                                Entity_SetEntitiesDirty( ch_handle_t* sEntities, u32 sCount );

// Get the highest level parent for this entity, returns self if not parented
ch_handle_t                                          Entity_GetRootParent( ch_handle_t sSelf );

// Recursively get all entities attached to this one (SLOW)
void                                                Entity_GetChildrenRecurse( ch_handle_t sEntity, ChVector< ch_handle_t >& srChildren );
void                                                Entity_GetChildrenRecurse( ch_handle_t sEntity, std::unordered_set< ch_handle_t >& srChildren );

// Get child entities attached to this one (SLOW)
void                                                Entity_GetChildren( ch_handle_t sEntity, ChVector< ch_handle_t >& srChildren );

// bool                          Entity_IsParented( ch_handle_t sEntity );
ch_handle_t                                          Entity_GetParent( ch_handle_t sEntity );
void                                                Entity_SetParent( ch_handle_t sEntity, ch_handle_t sParent );

// [ child ] = parent
const std::unordered_map< ch_handle_t, ch_handle_t >& Entity_GetParentMap();

// Returns a Model Matrix with parents applied in world space
void                                                Entity_GetWorldMatrix( glm::mat4& srMat, ch_handle_t sEntity );


