#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <type_traits>
#include <set>
#include <array>
#include <forward_list>

#include "types/transform.h"
#include "iaudio.h"

// TODO: split in 2 parts
// - A magic number to send across client and server, if this number is 0, it's local to client or server and not networked
// - Client and Server give their own entity index to this,
//     so we don't have client only and server only entity indexes conflict
using Entity = size_t;

// AAA
#include "game_shared.h"
#include "flatbuffers/sidury_generated.h"
#include "igraphics.h"

#include "iaudio.h"


template< typename T >
struct ComponentNetVar;
struct EntComponentData_t;

class EntitySystem;
class EntityComponentPool;
class IEntityComponentSystem;


// ====================================================================================================
// Base Entity and Component Stuff
// ====================================================================================================


constexpr Entity CH_MAX_ENTITIES = 8192;
constexpr Entity CH_ENT_INVALID  = SIZE_MAX;

constexpr bool   CH_ENT_SAVE_TO_MAP      = true;
constexpr bool   CH_ENT_DONT_SAVE_TO_MAP = false;


struct ComponentID_t
{
	size_t aIndex;

	bool   operator==( const ComponentID_t& srOther )
	{
		return aIndex == srOther.aIndex;
	};
};


// maybe do this?
#define SOURCE_DECLARE_POINTER_HANDLE( name ) struct name##__{}; typedef struct name##__ * name

// why do i need this here and i can't have it in the class?????
inline bool operator==( const ComponentID_t& srSelf, const ComponentID_t& srOther )
{
	return srSelf.aIndex == srOther.aIndex;
};


// Hashing Support for ComponentID_t
namespace std
{
	template<  > struct hash< ComponentID_t >
	{
		size_t operator()( ComponentID_t const& srID ) const
		{
			size_t value = ( hash< size_t >()( srID.aIndex ) );
			return value;
		}
	};
}


// Functions for creating and freeing components
using FEntComp_New          = std::function< void*() >;
using FEntComp_Free         = std::function< void( void* spData ) >;

// Functions for creating component systems
using FEntComp_NewSys       = std::function< IEntityComponentSystem*() >;

// Callback Function for when a new component is registered at runtime
// Used for creating a new component pool for client and/or server entity system
using FEntComp_Register     = void( const char* spName );

// Used for custom defined variables
using FEntComp_VarRead      = void( flexb::Reference& spSrc, EntComponentData_t* spRegData, void* spData );
using FEntComp_VarWrite     = bool( flexb::Builder& srBuilder, EntComponentData_t* spRegData, const void* spData, bool sFullUpdate );

using FEntSys_EventListener = void( Entity sEntity, void* spData );


// TODO: What is the purpose of this again?
enum EEntNetField : u8
{
	EEntNetField_Invalid,

	EEntNetField_Bool,

	EEntNetField_Float,
	EEntNetField_Double,

	EEntNetField_S8,   // signed char
	EEntNetField_S16,  // signed short
	EEntNetField_S32,  // signed int
	EEntNetField_S64,  // signed long long

	EEntNetField_U8,   // unsigned char
	EEntNetField_U16,  // unsigned short
	EEntNetField_U32,  // unsigned int
	EEntNetField_U64,  // unsigned long long

	EEntNetField_Entity,

	EEntNetField_StdString,  // std::string

	EEntNetField_Vec2,  // glm::vec2
	EEntNetField_Vec3,  // glm::vec3
	EEntNetField_Vec4,  // glm::vec4
	EEntNetField_Quat,  // glm::quat

	// Behaves the same as Vec3 and Vec4 for now, but has a different ui in the entity editor
	EEntNetField_Color3,  // glm::vec3
	EEntNetField_Color4,  // glm::vec4

	EEntNetField_Custom,  // Custom Type, must define your own read and write function for this type

	EEntNetField_Count,
};


enum EEntComponentNetType
{
	// This component is on both client and server
	EEntComponentNetType_Both,

	// This component is only on the client
	EEntComponentNetType_Client,

	// This component is only on the server
	EEntComponentNetType_Server,
};


using EEntityFlag = int;
enum EEntityFlag_ : EEntityFlag
{
	// Entity still exists
	EEntityFlag_None           = 0,

	// Entity was just created
	EEntityFlag_Created        = ( 1 << 0 ),

	// Entity was destroyed and is queued for removal
	EEntityFlag_Destroyed      = ( 1 << 1 ),

	// Entity is not networked and is local only
	EEntityFlag_Local          = ( 1 << 2 ),

	// Entity is Parented to another Entity
	EEntityFlag_Parented       = ( 1 << 3 ),

	// Entity is Predicted
	EEntityFlag_Predicted      = ( 1 << 4 ),

	// Ignore data from the server on the client, useful for first person camera entity
	// Or just change how EEntComponentNetType works, maybe what we pass is the default value, but you can override it?
	EEntityFlag_IgnoreOnClient = ( 1 << 5 ), 
};


using ECompRegFlag = int;

// Entity Component Registry Flags
enum ECompRegFlag_ : ECompRegFlag
{
	ECompRegFlag_None               = 0,

	// Hack
	ECompRegFlag_DontOverrideClient = ( 1 << 0 ),

	// This variable is a network variable
	ECompRegFlag_LocalVar           = ( 1 << 1 ),
};


// ====================================================================================================
// Entity Component Database
//
// A shared system between client and server that stores info on each component
// this will not talk to anything, only things to talk to it
// ====================================================================================================


// Var Data for a component
struct EntComponentVarData_t
{
	EEntNetField aType;
	ECompRegFlag aFlags;
	size_t       aSize;
	const char*  apName;
	size_t       aNameLen;
};


struct EntComponentVarNetData_t
{
	EEntNetField aType;
	ECompRegFlag aFlags;
	size_t       aSize;
	const char*  apName;
	size_t       aNameLen;
};


struct EntComponentNetData_t
{
};


// Data for a specific component
struct EntComponentData_t
{
	const char*                               apName;
	size_t                                    aNameLen;

	// [Var Offset] = Var Data
	std::map< size_t, EntComponentVarData_t > aVars;

	size_t                                    aSize;

	ECompRegFlag                              aFlags;
	EEntComponentNetType                      aNetType;

	FEntComp_New                              aFuncNew;
	FEntComp_Free                             aFuncFree;

	IEntityComponentSystem*                   apSystem;
};


struct EntComponentRegistry_t
{
	// [type hash of component] = Component Data
	std::unordered_map< size_t, EntComponentData_t >            aComponents;

	// Component Name to Component Data
	std::unordered_map< std::string_view, EntComponentData_t* > aComponentNames;

	// [type hash of var] = Var Type Enum
	std::unordered_map< size_t, EEntNetField >                  aVarTypes;

	// [type hash of var] = Var Read/Write Function
	// std::unordered_map< size_t, FEntComp_VarRead* >             aVarRead;
	// std::unordered_map< size_t, FEntComp_VarWrite* >            aVarWrite;
};


struct EntCompVarTypeToEnum_t
{
	size_t               aHashCode;
	EEntNetField aType;
};


// it would be funny if we changed this to "TheEntityComponentRegistry"
inline EntComponentRegistry_t &GetEntComponentRegistry()
{
	static EntComponentRegistry_t entComponentRegistry;
	return entComponentRegistry;
}

// void* EntComponentRegistry_Create( std::string_view sName );
// void* EntComponentRegistry_GetVarHandler();

const char* EntComp_NetTypeToStr( EEntComponentNetType sNetType );
const char* EntComp_VarTypeToStr( EEntNetField sVarType );
std::string EntComp_GetStrValueOfVar( void* spData, EEntNetField sVarType );
std::string EntComp_GetStrValueOfVarOffset( size_t sOffset, void* spData, EEntNetField sVarType );

void        Ent_RegisterBaseComponents();

void        Entity_CreateComponentPool( const char* spName );


template< typename T >
inline void EntComp_RegisterComponent(
  const char*          spName,
  EEntComponentNetType sNetType,
  FEntComp_New         sFuncNew,
  FEntComp_Free        sFuncFree,
  ECompRegFlag         sFlags = 0 )
{
	CH_ASSERT( spName );

	if ( !spName )
		return;

	size_t typeHash = typeid( T ).hash_code();
	auto   it       = GetEntComponentRegistry().aComponents.find( typeHash );

	if ( it != GetEntComponentRegistry().aComponents.end() )
	{
		Log_ErrorF( "Component already registered: \"%s\" - \"%s\"\n", typeid( T ).name(), spName );
		return;
	}

	EntComponentData_t& data                            = GetEntComponentRegistry().aComponents[ typeHash ];
	data.apName                                         = spName;
	data.aNameLen                                       = strlen( spName );
	data.aSize                                          = sizeof( T );
	data.aFlags                                         = sFlags;
	data.aNetType                                       = sNetType;
	data.aFuncNew                                       = sFuncNew;
	data.aFuncFree                                      = sFuncFree;

	GetEntComponentRegistry().aComponentNames[ spName ] = &data;

	Entity_CreateComponentPool( spName );
}


template< typename T >
inline void EntComp_RegisterComponentSystem( IEntityComponentSystem* spSystem )
{
	size_t typeHash = typeid( T ).hash_code();

	auto   it       = GetEntComponentRegistry().aComponents.find( typeHash );

	if ( it == GetEntComponentRegistry().aComponents.end() )
	{
		Log_ErrorF( "Component not registered, can't set system creation function: \"%s\"\n", typeid( T ).name() );
		return;
	}

	EntComponentData_t& data = it->second;
	data.apSystem            = spSystem;
}


template< typename COMPONENT_TYPE, typename VAR_TYPE >
inline void EntComp_RegisterComponentVarEx( EEntNetField sVarType, const char* spName, size_t sOffset, ECompRegFlag sFlags = 0 )
{
	CH_ASSERT( spName );

	if ( !spName )
		return;

	size_t typeHash = typeid( COMPONENT_TYPE ).hash_code();
	auto   it       = GetEntComponentRegistry().aComponents.find( typeHash );

	if ( it == GetEntComponentRegistry().aComponents.end() )
	{
		Log_ErrorF( "Component not registered, can't add var: \"%s\" - \"%s\"\n", typeid( COMPONENT_TYPE ).name() );
		return;
	}

	EntComponentData_t& data    = it->second;
	auto                varFind = data.aVars.find( sOffset );

	if ( varFind != data.aVars.end() )
	{
		Log_ErrorF( "Component Var already registered: \"%s::%s\" - \"%s\"\n", typeid( COMPONENT_TYPE ).name(), typeid( VAR_TYPE ).name(), spName );
		return;
	}

	EntComponentVarData_t& varData = data.aVars[ sOffset ];
	varData.apName                 = spName;
	varData.aNameLen               = strlen( spName );
	varData.aSize                  = sizeof( VAR_TYPE );
	varData.aType                  = sVarType;
	varData.aFlags                 = sFlags;

	// TODO: really should have this be done once, but im not adding a new function for this to add to every single component
	// data.aHash = 0;
	// for ( const auto& [ offset, var ] : data.aVars )
	// {
	// 	data.aHash ^= varData.aHash;
	// }
}


#if 0
template< typename T >
inline void EntComp_RegisterComponentVarEx2( EEntNetField sVarType, const char* spVarName, const char* spName, size_t sOffset, size_t sVarHash )
{
	size_t typeHash = typeid( T ).hash_code();
	auto   it       = GetEntComponentRegistry().aComponents.find( typeHash );

	if ( it == GetEntComponentRegistry().aComponents.end() )
	{
		Log_ErrorF( "Component not registered, can't add var: \"%s\" - \"%s\"\n", typeid( T ).name() );
		return;
	}

	EntComponentData_t& data    = it->second;
	auto                varFind = data.aVars.find( sOffset );

	if ( varFind != data.aVars.end() )
	{
		Log_ErrorF( "Component Var already registered: \"%s::%s\" - \"%s\"\n", typeid( T ).name(), typeid( VAR_TYPE ).name(), spName );
		return;
	}

	auto& varData     = data.aVars[ sOffset ];
	varData.apVarName = spVarName;
	varData.apName    = spName;
	varData.aSize     = sizeof( VAR_TYPE );
	varData.aType     = sVarType;

	// Assert( sVarHash == typeid( ComponentNetVar< VAR_TYPE > ).hash_code() );

	if ( sVarHash != typeid( ComponentNetVar< VAR_TYPE > ).hash_code() )
	{
		varData.aIsNetVar = false;
		// Log_ErrorF( "Not Registering Component Var, is not a ComponentNetVar Type: \"%s\" - \"%s\"\n", typeid( VAR_TYPE ).name(), spVarName );
		// return;
	}
	else
	{
		varData.aIsNetVar = true;
	}
}
#endif


template< typename COMPONENT_TYPE, typename VAR_TYPE >
inline void EntComp_RegisterComponentVar( const char* spName, size_t sOffset, ECompRegFlag sFlags = 0 )
{
	// Get Var Type
	size_t varTypeHash = typeid( VAR_TYPE ).hash_code();
	auto   findEnum    = GetEntComponentRegistry().aVarTypes.find( varTypeHash );

	if ( findEnum == GetEntComponentRegistry().aVarTypes.end() )
	{
		Log_ErrorF( "Component Var Type not registered in EEntComponentVarType: \"%s\"\n", typeid( VAR_TYPE ).name() );
		return;
	}

	EntComp_RegisterComponentVarEx< COMPONENT_TYPE, VAR_TYPE >( findEnum->second, spName, sOffset, sFlags );
}


#if 0
template< typename T >
inline void EntComp_RegisterVarReadWrite( FEntComp_ReadFunc* spRead, FEntComp_WriteFunc* spWrite )
{
	size_t typeHash = typeid( T ).hash_code();
	auto   it       = GetEntComponentRegistry().aComponents.find( typeHash );

	if ( it == GetEntComponentRegistry().aComponents.end() )
	{
		Log_ErrorF( "Component not registered, can't add read and write function: \"%s\" - \"%s\"\n", typeid( T ).name() );
		return;
	}

	EntComponentData_t& data = it->second;
	data.apRead              = spRead;
	data.apWrite             = spWrite;
}
#endif


#if 0
template< typename T >
// void EntComp_RegisterComponentReadWrite( FEntComp_ReadFunc< T >* spRead, FEntComp_WriteFunc< T >* spWrite )
inline void EntComp_RegisterComponentReadWrite( FEntComp_ReadFunc* spRead, FEntComp_WriteFunc* spWrite )
{
	size_t typeHash = typeid( T ).hash_code();
	auto   it       = GetEntComponentRegistry().aComponents.find( typeHash );

	if ( it == GetEntComponentRegistry().aComponents.end() )
	{
		Log_ErrorF( "Component not registered, can't add read and write function: \"%s\" - \"%s\"\n", typeid( T ).name() );
		return;
	}

	EntComponentData_t& data = it->second;
	data.apRead              = spRead;
	data.apWrite             = spWrite;
}
#endif


template< typename T >
inline void EntComp_RegisterVarHandler()
{
}


// template< typename T >
// inline void EntComp_RegisterComponentRead( FEntComp_ReadFunc* spRead )
// {
// 	size_t typeHash = typeid( T ).hash_code();
// }
// 
// template< typename T >
// inline void EntComp_RegisterComponentWrite( FEntComp_WriteFunc* spWrite )
// {
// 	size_t typeHash = typeid( T ).hash_code();
// }


// ====================================================================================================
// Entity Component Pool
// Stores all data for a specific component for each entity
// ====================================================================================================


class EntityComponentPool
{
  public:
	EntityComponentPool();
	~EntityComponentPool();

	// Called whenever an entity is destroyed on all Component Pools
	bool                                      Init( const char* spName );

	// Get Component Registry Data
	EntComponentData_t*                       GetRegistryData();

	// Does this pool contain a component for this entity?
	bool                                      Contains( Entity entity );

	// Called whenever an entity is destroyed on all Component Pools
	void                                      EntityDestroyed( Entity entity );

	// Adds This component to the entity
	void*                                     Create( Entity entity );

	// Removes this component from the entity
	void                                      Remove( Entity entity );

	// Removes this component by ID
	void                                      RemoveByID( ComponentID_t sID );

	// Removes this component from the entity later
	void                                      RemoveQueued( Entity entity );

	// Removes components queued for deletion
	void                                      RemoveAllQueued();

	// Initializes Created Components
	void                                      InitCreatedComponents();

	// Gets the data for this component
	void*                                     GetData( Entity entity );
	void*                                     GetData( ComponentID_t sComponentID );

	// Marks this component as predicted
	void                                      SetPredicted( Entity entity, bool sPredicted );

	// Is this component predicted for this Entity?
	bool                                      IsPredicted( Entity entity );
	bool                                      IsPredicted( ComponentID_t sComponentID );

	// How Many Components are in this Pool?
	size_t                                    GetCount();

	// ------------------------------------------------------------------

	// Map Component Index to Entity
	std::unordered_map< ComponentID_t, Entity >      aMapComponentToEntity;

	// Map Entity to Component Index
	std::unordered_map< Entity, ComponentID_t >      aMapEntityToComponent;

	// Memory Pool of Components
	// This is an std::array so that when a component is freed, it does not changes the index of each component
	// TODO: I DON'T LIKE THIS, TAKES UP TOO MUCH MEMORY
	// maybe use ch_handle_t's from ResourceList<> instead, as it uses a memory pool?
	std::array< void*, CH_MAX_ENTITIES >             aComponents{};

	// Component Flags, just uses Entity Flags for now
	// Key is an index into aComponents
	std::unordered_map< ComponentID_t, EEntityFlag > aComponentFlags;

	// Amount of Components we have allocated
	// size_t                                           aCount;
	std::vector < ComponentID_t >                    aComponentIDs;

	std::forward_list< ComponentID_t >               aNewComponents;
	std::forward_list< ComponentID_t >               aComponentsUpdated;

	// Component Name
	const char*                                      apName;

	// Component Creation and Free Func
	FEntComp_New                                     aFuncNew;
	FEntComp_Free                                    aFuncFree;

	EntComponentData_t*                              apData;

	// Component Systems that manage this component
	// NOTE: This may always just be one
	// std::set< IEntityComponentSystem* >  aComponentSystems;
	IEntityComponentSystem*                          apComponentSystem = nullptr;
};


// ====================================================================================================
// Entity Component System Interface
// stores a list of entities with that component type
// ====================================================================================================


class IEntityComponentSystem
{
  public:
	virtual ~IEntityComponentSystem()                        = default;

	// Called when the component is added to this entity
	virtual void          ComponentAdded( Entity sEntity, void* spData ){};

	// Called when the component is removed from this entity
	virtual void          ComponentRemoved( Entity sEntity, void* spData ){};

	// Called when the component data has been updated (ONLY ON CLIENT RIGHT NOW)
	virtual void          ComponentUpdated( Entity sEntity, void* spData ){};

	// Called when this component variable has been updated (NOT IMPLEMENTED)
	//virtual void          ComponentVarUpdated( Entity sEntity, void* spData ){};

	virtual void          Update(){};

	// What if we stored a pointer to the component pool here
	// Or changed this std::vector of entities to an unordered_map, or a tuple

	EntityComponentPool*  apPool = nullptr;
	std::vector< Entity > aEntities;
};


// ====================================================================================================
// Entity System
// ====================================================================================================


struct EntityEventListener_t
{
	Entity                 aEntity;
	std::string_view       aComponent;
	std::string_view       aEvent;
	FEntSys_EventListener* apCallback = nullptr;
};


// truct EntityData_t
// 
// 	EEntityFlag aFlags;
// 	Entity      aParent;
// 	std::string name;
// ;


struct EntitySystemData
{
	bool                                                         aActive   = false;

	// Queue of unused entity IDs
	// TODO: CHANGE BACK TO QUEUE
	// std::queue< Entity >                               aEntityPool{};
	std::vector< Entity >                                        aEntityPool{};

	// Used for converting a sent entity ID to what it actually is on the recieving end, so no conflicts occur
	// This is needed for client/server networking, the entity id on each end will be different, so we convert the id
	std::unordered_map< Entity, Entity >                         aEntityIDConvert;

	// Entity Flags
	std::unordered_map< Entity, EEntityFlag >                    aEntityFlags;

	// Entity Parents
	std::unordered_map< Entity, Entity >                         aEntityParents;

	// Entity Names (needs to be ordered because of parenting)
	std::map< Entity, std::string >                              aEntityNames;

	// Component Pools - Pool of all of this type of component in existence
	std::unordered_map< std::string_view, EntityComponentPool* > aComponentPools;

	// All Component Systems, key is the type_hash() of the system
	// NOTE: it's a bit strange to have them be stored here and one in each component pool
	std::unordered_map< size_t, IEntityComponentSystem* >        aComponentSystems;

	// Event Listeners
	// std::vector< EntityEventListener_t >                         aEventListeners;
	// ResourceList< EntityEventListener_t >                        aEventListeners;
};


// Get Entity System Data
EntitySystemData&       EntSysData();


bool                    Entity_Init();
void                    Entity_Shutdown();

void                    Entity_UpdateSystems();

// Update Entity and Component States, and Delete Queued Entities
void                    Entity_UpdateStates();
void                    Entity_InitCreatedComponents();

void                    Entity_CreateComponentPools();
void                    Entity_CreateComponentPool( const char* spName );

// Get a system for managing this component
IEntityComponentSystem* Entity_GetComponentSystem( const char* spName );

// if sLocal is true, then the entity is only created on client or server, and is never networked
// Useful for client or server only entities, if we ever have those
Entity                  Entity_CreateEntity( bool sLocal = false );
void                    Entity_DeleteEntity( Entity ent );
void                    Entity_DeleteQueuedEntities();
Entity                  Entity_GetEntityCount();

bool                    Entity_EntityExists( Entity sDesiredId );

// Parents an entity to another one
// TODO: tackle parenting with physics objects
void                    Entity_ParentEntity( Entity sSelf, Entity sParent );
Entity                  Entity_GetParent( Entity sEntity );
bool                    Entity_IsParented( Entity sEntity );

// Get the highest level parent for this entity, returns self if not parented
Entity                  Entity_GetRootParent( Entity sEntity );

// Recursively get all entities attached to this one (SLOW)
void                    Entity_GetChildrenRecurse( Entity sEntity, ChVector< Entity >& srChildren );

// Returns a Model Matrix with parents applied in world space IF we have a transform component
bool                    Entity_GetWorldMatrix( glm::mat4& srMat, Entity sEntity );

// Same as GetWorldMatrix, but returns in a Transform struct
Transform               Entity_GetWorldTransform( Entity sEntity );

// having local versions of these functions are useless, that's just the transform component
// GetWorldMatrix
// GetWorldTransform
// GetWorldPosition
// GetWorldAngles
// GetWorldScale

// Read and write from the network
void                    Entity_ReadEntityUpdates( const NetMsg_EntityUpdates* spMsg );
void                    Entity_WriteEntityUpdates( flatbuffers::FlatBufferBuilder& srBuilder );

void                    Entity_ReadComponentUpdates( const NetMsg_ComponentUpdates* spReader );
void                    Entity_WriteComponentUpdates( flatbuffers::FlatBufferBuilder& srBuilder, bool sFullUpdate );

// Add a component to an entity
void*                   Entity_AddComponent( Entity entity, std::string_view sName );

// Does this entity have this component?
bool                    Entity_HasComponent( Entity entity, std::string_view sName );

// Get a component from an entity
void*                   Entity_GetComponent( Entity entity, std::string_view sName );

// Remove a component from an entity
void                    Entity_RemoveComponent( Entity entity, std::string_view sName );

#if CH_CLIENT
// Sets Prediction on this component
void                    Entity_SetComponentPredicted( Entity entity, std::string_view sName, bool sPredicted );

// Is this component predicted for this Entity?
bool                    Entity_IsComponentPredicted( Entity entity, std::string_view sName );
#endif

// Enables/Disables Networking on this Entity
void                    Entity_SetNetworked( Entity entity, bool sNetworked = true );

// Is this Entity Networked?
bool                    Entity_IsNetworked( Entity sEntity );

// Faster version of IsNetworked with the option to pass in the entity flags if you already have them
bool                    Entity_IsNetworked( Entity sEntity, EEntityFlag sFlags );

// Get the Component Pool for this Component
EntityComponentPool*    Entity_GetComponentPool( std::string_view sName );

// Used for converting a sent entity ID to what it actually is on the recieving end, so no conflicts occur
// This is needed for client/server networking, the entity id on each end will be different, so we convert the id
Entity                  Entity_TranslateEntityID( Entity sEntity, bool sCreate = false );

// ---------------------------------------------------------
// Entity Names

void                    Entity_SetName( Entity sEntity, std::string_view sName );
std::string_view        Entity_GetName( Entity sEntity );

// Multiple Entities can have the same name
void                    Entity_GetEntitiesByName( std::string_view sName, ChVector< Entity >& sEntities );

// ---------------------------------------------------------
// Entity Event System

// void                    Entity_ProcessEventQueue();
//
// // Fire an event (Server Only, Event's will be networked to the client)
// // We also pass in the component name to avoid event name clashing
// void                    Entity_FireEvent( Entity sEntity, std::string_view sComponent, std::string_view sName, void* spData = nullptr );
//
// // Add a listener to listen for an event
// // If CH_ENT_INVALID is passed in, it will listen globally for this event
// // If an entity is passed in, it will only listen for events from that entity
// // We also pass in the component name to avoid event name clashing
// ch_handle_t                  Entity_AddEventListener( Entity sEntity, std::string_view sComponent, std::string_view sEvent, FEntSys_EventListener* spCallback );
//
// // Same as above, but with an entity name
// ch_handle_t                  Entity_AddEventListener( std::string_view sEntityName, std::string_view sComponent, std::string_view sEvent, FEntSys_EventListener* spCallback );
//
// // how tf will this work, will AddEventListener return a handle? idfk
// void                    Entity_RemoveEventListener( ch_handle_t sListenerHandle );


inline void*  Ent_AddComponent( Entity sEnt, const char* spName )
{
	return Entity_AddComponent( sEnt, spName );
}

template< typename T >
inline T* Ent_AddComponent( Entity sEnt, const char* spName )
{
	return ch_pointer_cast< T >( Entity_AddComponent( sEnt, spName ) );
}


inline void* Ent_GetComponent( Entity sEnt, const char* spName )
{
	return Entity_GetComponent( sEnt, spName );
}

// Gets a component and static cast's it to the desired type
template< typename T >
inline T* Ent_GetComponent( Entity sEnt, const char* spName )
{
	return ch_pointer_cast< T >( Entity_GetComponent( sEnt, spName ) );
}


bool Entity_Init();
void Entity_Shutdown();


// ==========================================
// Network Var
// ==========================================


template< typename T >
struct ComponentNetVar
{
	using Type = T;

	T    aValue{};
	bool aIsDirty = true;

	ComponentNetVar() :
		aIsDirty( true ), aValue()
	{
	}

	template< typename VAR_TYPE = int >
	ComponentNetVar( VAR_TYPE var ) :
		aIsDirty( true ), aValue( var )
	{
	}

	~ComponentNetVar()
	{
	}

	const T& Set( const T* spValue )
	{
		// if ( aValue != *spValue )
		if ( memcmp( &aValue, spValue, sizeof( T ) ) != 0 )
		{
			aIsDirty = true;
			aValue   = *spValue;
		}

		return aValue;
	}

	const T& Set( const T& srValue )
	{
		if ( memcmp( &aValue, &srValue, sizeof( T ) ) != 0 )
		{
			aIsDirty = true;
			aValue   = srValue;
		}

		return aValue;
	}

	// const T& Set( const T& srValue )
	// {
	// 	if ( aValue != srValue )
	// 	{
	// 		aIsDirty = true;
	// 		aValue   = srValue;
	// 	}
	// }

	const T& Get() const
	{
		return aValue;
	}

	T& Edit()
	{
		aIsDirty = true;
		return aValue;
	}

	// -------------------------------------------------
	// Operators

	operator const T&() const
	{
		return aValue;
	}

	// T* operator->() const
	// {
	// 	return &aValue;
	// }

	// T* operator()() const
	// {
	// 	return &aValue;
	// }

	const T& operator=( const T* spValue )
	{
		return Set( spValue );
	}

	// const T& operator=( const T& srValue )
	// {
	// 	return Set( srValue );
	// }

	bool operator==( const T& srValue ) const
	{
		return aValue == srValue;
	}

	bool operator!=( const T& srValue ) const
	{
		return aValue != srValue;
	}

	const T& operator+=( const T* spValue )
	{
		aIsDirty = true;
		aValue += *spValue;
		return aValue;
	}

	const T& operator+=( const T& srValue )
	{
		aIsDirty = true;
		aValue += srValue;
		return aValue;
	}

	const T& operator*=( const T* spValue )
	{
		aIsDirty = true;
		aValue *= *spValue;
		return aValue;
	}

	const T& operator*=( const T& srValue )
	{
		aIsDirty = true;
		aValue *= srValue;
		return aValue;
	}

	T operator*( const T& srValue ) const
	{
		return aValue * srValue;
	}

	T operator/( const T& srValue ) const
	{
		return aValue / srValue;
	}

	T operator+( const T& srValue ) const
	{
		return aValue + srValue;
	}

	T operator-( const T& srValue ) const
	{
		return aValue - srValue;
	}

	template< typename OTHER_TYPE = int >
	T operator*( const OTHER_TYPE& srValue ) const
	{
		return aValue * srValue;
	}

#if 0
	template< typename OTHER_TYPE = int >
	T operator/( const OTHER_TYPE& srValue ) const
	{
		return aValue / srValue;
	}

	template< typename OTHER_TYPE = int >
	T operator+( const OTHER_TYPE& srValue ) const
	{
		return aValue + srValue;
	}

	template< typename OTHER_TYPE = int >
	T operator-( const OTHER_TYPE& srValue ) const
	{
		return aValue - srValue;
	}

	// Index into the buffer
	template< typename INDEX_TYPE = int >
	T& operator[]( INDEX_TYPE sIndex )
	{
		return aValue[ sIndex ];
	}
#endif

	// -------------------------------------------------
	// C++ class essentials

	// copying
	void assign( const ComponentNetVar& other )
	{
		Set( &other.aValue );
	}

	// moving
	void assign( ComponentNetVar&& other )
	{
		Set( &other.aValue );

		// swap the values of this one with that one
		// std::swap( aIsDirty, other.aIsDirty );
		// std::swap( aValue, other.aValue );
	}

#if 1
	ComponentNetVar& operator=( const ComponentNetVar& other )
	{
		assign( other );
		return *this;
	}

	ComponentNetVar& operator=( ComponentNetVar&& other )
	{
		assign( std::move( other ) );
		return *this;
	}
#endif

	// copying
	ComponentNetVar( const ComponentNetVar& other )
	{
		assign( other );
	}

	// moving
	ComponentNetVar( ComponentNetVar&& other )
	{
		assign( std::move( other ) );
	}
};


enum ENetFlagVec : char
{
	ENetFlagVec_None = 0,
	ENetFlagVec_HasX = ( 1 << 0 ),
	ENetFlagVec_HasY = ( 1 << 1 ),
	ENetFlagVec_HasZ = ( 1 << 2 ),
	ENetFlagVec_HasW = ( 1 << 3 ),
};


// TODO: copy stuff from ComponentNetVar
template< typename T >
struct ComponentNetVecBase
{
	T           aVec = {};
	ENetFlagVec aDirty;
};


using ComponentNetVec2 = ComponentNetVecBase< glm::vec2 >;
using ComponentNetVec3 = ComponentNetVecBase< glm::vec3 >;
using ComponentNetVec4 = ComponentNetVecBase< glm::vec4 >;


// ==========================================
// Some Base Types here for now


// idea for FlexBuffers
enum ETransformNetFlag : short
{
	ETransformNetFlag_None      = 0,
	ETransformNetFlag_HasPos    = ( 1 << 0 ),
	ETransformNetFlag_HasAng    = ( 1 << 1 ),
	ETransformNetFlag_HasScale  = ( 1 << 2 ),

	ETransformNetFlag_HasPosX   = ( 1 << 3 ),
	ETransformNetFlag_HasPosY   = ( 1 << 4 ),
	ETransformNetFlag_HasPosZ   = ( 1 << 5 ),

	ETransformNetFlag_HasAngX   = ( 1 << 6 ),
	ETransformNetFlag_HasAngY   = ( 1 << 7 ),
	ETransformNetFlag_HasAngZ   = ( 1 << 8 ),

	ETransformNetFlag_HasScaleX = ( 1 << 9 ),
	ETransformNetFlag_HasScaleY = ( 1 << 10 ),
	ETransformNetFlag_HasScaleZ = ( 1 << 11 ),
};




struct CTransform
{
	ComponentNetVar< glm::vec3 > aPos   = {};
	ComponentNetVar< glm::vec3 > aAng   = {};
	ComponentNetVar< glm::vec3 > aScale = {};

	// -------------------------------------------------
	// C++ class essentials

	CTransform()
	{
		aScale.aValue = { 1.f, 1.f, 1.f };
	}

	~CTransform()
	{
	}

	// copying
	void assign( const CTransform& other )
	{
		aPos   = std::move( other.aPos );
		aAng   = std::move( other.aAng );
		aScale = std::move( other.aScale );
	}

	// moving
	void assign( CTransform&& other )
	{
		// swap the values of this one with that one
		std::swap( aPos, other.aPos );
		std::swap( aAng, other.aAng );
		std::swap( aScale, other.aScale );
	}

	CTransform& operator=( const CTransform& other )
	{
		assign( other );
		return *this;
	}

	CTransform& operator=( CTransform&& other )
	{
		assign( std::move( other ) );
		return *this;
	}

	// copying
	CTransform( const CTransform& other )
	{
		assign( other );
	}

	// moving
	CTransform( CTransform&& other ) noexcept
	{
		assign( std::move( other ) );
	}
};


struct CRigidBody
{
	ComponentNetVar< glm::vec3 > aVel = {};
	ComponentNetVar< glm::vec3 > aAccel = {};
};


// struct CGravity
// {
// 	ComponentNetVar< glm::vec3 > aForce = {};
// };


// Direction Vectors
struct CDirection
{
	ComponentNetVar< glm::vec3 > aForward = {};
	ComponentNetVar< glm::vec3 > aUp = {};
	ComponentNetVar< glm::vec3 > aRight = {};
};


// TODO: add viewport info here
// maybe have a field for an output texture or material?
// and then with rendering certain surfaces you can reference that texture or material?
// no idea how camera's usually work in game engines
// in source, there seems to be a func_monitor brush, which takes in a camera name
// but i know the tv had a monitor on it at one point, hmm
struct CCamera
{
	ComponentNetVar< float > aFov = 90.f;
};


struct CCamera2
{
	ComponentNetVar< float >     aNear = 0.5f;
	ComponentNetVar< float >     aFar  = 10000.f;
	ComponentNetVar< float >     aFov  = 90.f;

	ComponentNetVar< glm::vec2 > aSize{};

	ComponentNetVar< glm::mat4 > aViewMat{};
	ComponentNetVar< glm::mat4 > aProjMat{};

	// projection matrix * view matrix
	ComponentNetVar< glm::mat4 > aProjViewMat{};
};


struct CRenderable
{
	// Path to model to load
	ComponentNetVar< std::string > aPath;

	ComponentNetVar< bool >        aTestVis    = true;
	ComponentNetVar< bool >        aCastShadow = true;
	ComponentNetVar< bool >        aVisible    = true;

	ch_handle_t                         aModel      = CH_INVALID_HANDLE;
	ch_handle_t                         aRenderable = CH_INVALID_HANDLE;
};


// wrapper for lights
struct CLight
{
	ComponentNetVar< ELightType > aType = ELightType_Directional;
	ComponentNetVar< glm::vec4 >  color{};
	ComponentNetVar< glm::vec3 >  aPos{};
	ComponentNetVar< glm::quat >  aRot{};
	ComponentNetVar< float >      aInnerFov     = 45.f;
	ComponentNetVar< float >      aOuterFov     = 45.f;
	ComponentNetVar< float >      aRadius       = 0.f;
	ComponentNetVar< float >      aLength       = 0.f;
	ComponentNetVar< bool >       aShadow       = true;
	ComponentNetVar< bool >       aEnabled      = true;
	ComponentNetVar< bool >       aUseTransform = false;  // Use a CTransform component instead, will eventually be required and aPos/aAng will be removed

	Light_t*                      apLight   = nullptr;  // var not registered
};


struct CHealth
{
	ComponentNetVar< int >        aHealth       = 0;
};


// convinence
// inline auto &GetDirection( Entity ent ) { return Entity_GetComponent< CDirection >( ent ); }


// Helper Macros
#define CH_REGISTER_COMPONENT( type, name, netType ) \
  EntComp_RegisterComponent< type >(                                             \
	#name, netType, [ & ]() { return new type; }, [ & ]( void* spData ) { delete (type*)spData; }, 0 )

#define CH_REGISTER_COMPONENT_FL( type, name, netType, flags ) \
  EntComp_RegisterComponent< type >(                                             \
	#name, netType, [ & ]() { return new type; }, [ & ]( void* spData ) { delete (type*)spData; }, flags )

#define CH_REGISTER_COMPONENT_SYS( type, systemClass, systemVar ) \
  EntComp_RegisterComponentSystem< type >( &systemVar )

#define CH_REGISTER_COMPONENT_NEWFREE( type, name, overrideClient, netType, newFunc, freeFunc ) \
  EntComp_RegisterComponent< type >( #name, overrideClient, netType, newFunc, freeFunc )



#define CH_REGISTER_COMPONENT_VAR( type, varType, varName, varStr, flags ) \
  EntComp_RegisterComponentVar< type, varType >( #varStr, offsetof( type, varName ), flags )

#define CH_REGISTER_COMPONENT_VAR_EX( type, netVarType, varType, varName, varStr, flags ) \
  EntComp_RegisterComponentVarEx< type, varType >( netVarType, #varStr, offsetof( type, varName ), flags )


//define CH_REGISTER_COMPONENT_VAR_EX( type, varType, varName, varStr ) \
// EntComp_RegisterComponentVar< type, varType >( #varName, #varStr, offsetof( type, varName ), typeid( type::varName ).hash_code() )



// #define CH_REGISTER_COMPONENT_RW_EX( type, read, write ) \
//   EntComp_RegisterComponentReadWrite< type >( read, write )

#define CH_REGISTER_COMPONENT_RW( type, name, flags ) \
  CH_REGISTER_COMPONENT_FL( type, name, EEntComponentNetType_Both, flags );



//#define CH_NET_VAR_WRITE( type, typeName ) \
//	bool _NetComp_Write_##typeName( flexb::Builder& srBuilder, EntComponentData_t* spRegData, const void* spData, bool sFullUpdate )
//
//#define CH_NET_VAR_READ( type, typeName ) \
//	bool _NetComp_Write_##typeName( flexb::Builder& srBuilder, EntComponentData_t* spRegData, const void* spData, bool sFullUpdate )
//
//
//#define CH_REGISTER_VAR_RW( type, typeName ) \
//	bool _NetComp_Write_##typeName( flexb::Builder& srBuilder, EntComponentData_t* spRegData, const void* spData, bool sFullUpdate )


#define CH_STRUCT_REGISTER_COMPONENT( type, name, netType, flags ) \
  struct __CompRegister_##type##_t                                 \
  {                                                                \
	using TYPE = type;                                             \
                                                                   \
	void RegisterVars();                                           \
	__CompRegister_##type##_t()                                    \
	{                                                              \
	  EntComp_RegisterComponent< TYPE >(                           \
		#name, netType, [ & ]() { return new TYPE; },              \
		[ & ]( void* spData )                                      \
		{ delete (type*)spData; },                                 \
		flags );                                                   \
                                                                   \
	  RegisterVars();                                              \
	}                                                              \
  };                                                               \
  static __CompRegister_##type##_t __CompRegister_##type;          \
  void                             __CompRegister_##type##_t::RegisterVars()


#define CH_REGISTER_COMPONENT_VAR2( compVarType, varType, varName, varStr, flags ) \
  EntComp_RegisterComponentVarEx< TYPE, varType >( compVarType, #varStr, offsetof( TYPE, varName ), flags )

#define CH_REGISTER_COMPONENT_SYS2( systemClass, systemVar ) \
	EntComp_RegisterComponentSystem< TYPE >( &systemVar )


#define CH_NET_WRITE_VEC2( varName, var ) \
  if ( var.aIsDirty || sFullUpdate )                 \
  {                                                  \
	Vec2Builder vec2Build( srBuilder );              \
	NetHelper_WriteVec2( vec2Build, var );           \
	varName##Offset = vec2Build.Finish();                \
  }

//#define CH_NET_WRITE_VEC3( varName, var ) \
//  if ( var.aIsDirty || sFullUpdate )                 \
//  {                                                  \
//	Vec3Builder vec3Build( srBuilder );              \
//	NetHelper_WriteVec3( vec3Build, var );           \
//	varName##Offset = vec3Build.Finish();                \
//  }

#define CH_NET_WRITE_VEC3( builder, varName, var ) \
  if ( var.aIsDirty || sFullUpdate )                 \
  {                                                  \
	Vec3 vec( var.Get().x, var.Get().y, var.Get().z ); \
	builder.add_##varName( &vec );               \
  }

#define CH_NET_WRITE_VEC4( varName, var ) \
  if ( var.aIsDirty || sFullUpdate )                 \
  {                                                  \
	Vec4Builder vec4Build( srBuilder );              \
	NetHelper_WriteVec4( vec4Build, var );           \
	varName##Offset = vec4Build.Finish();                \
  }

// #define CH_NET_WRITE_OFFSET( builder, var ) \
//   if ( !var##Offset.IsNull() )              \
// 	builder.add_##var( var##Offset ) 

#define CH_NET_WRITE_OFFSET( builder, var ) 


#define CH_VAR_DIRTY( var ) var.aIsDirty || sFullUpdate


// Helper Macros for Registering Standard Var Types

#define CH_REGISTER_COMP_VAR_VEC3( type, varName, varStr ) \
  CH_REGISTER_COMPONENT_VAR( type, glm::vec3, varName, varStr )

#define CH_REGISTER_COMPT_VAR_FL( type, varName, varStr ) \
  CH_REGISTER_COMPONENT_VAR( type, float, varName, varStr )


// Helper Functions
ch_handle_t        Ent_GetRenderableHandle( Entity sEntity );

Renderable_t* Ent_GetRenderable( Entity sEntity );

// Requires the entity to have renderable component with a model path set
Renderable_t* Ent_CreateRenderable( Entity sEntity );

#if 0
// This version has an option to enter a model handle
inline Renderable_t* Ent_CreateRenderable( Entity sEntity, ch_handle_t sModel )
{
	auto renderComp = Ent_GetComponent< CRenderable >( sEntity, "renderable" );

	if ( !renderComp )
	{
		Log_Error( "Failed to get renderable component\n" );
		return nullptr;
	}

	// TODO: CHECK IF renderComp->aModel is different, and handle that

	// I hate this so much
	if ( renderComp->aRenderable == CH_INVALID_HANDLE )
	{
		renderComp->aModel      = sModel;
		renderComp->aRenderable = Graphics_CreateRenderable( sModel );
		if ( renderComp->aRenderable == CH_INVALID_HANDLE )
		{
			Log_Error( "Failed to create renderable\n" );
			return nullptr;
		}
	}

	return Graphics_GetRenderableData( renderComp->aRenderable );
}
#endif

