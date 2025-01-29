#pragma once

// Game Implementation of Physics, mainly for helper stuff and loading physics

#include "physics/iphysics.h"
#include "entity/entity.h"

using Entity = size_t;


enum EPhysTransformMode : u8
{
	// Don't do anything with the transform component
	EPhysTransformMode_None,

	// Override the transform component's values with what we got from the physics object
	EPhysTransformMode_Update,

	// Have the physics object use the values from the transform component
	EPhysTransformMode_Inherit,
};


// Physics Shape Component
struct CPhysShape
{
	ComponentNetVar< PhysShapeType > aShapeType;

	// Used for Concave and Convex Meshes
	ComponentNetVar< std::string >   aPath;

	// Used for everything else
	ComponentNetVar< glm::vec3 >     aBounds;

	// Used for making a Static Compound
	// ComponentNetVar< Entity > aEntity;

	ch_handle_t                           aModel  = CH_INVALID_HANDLE;

	IPhysicsShape*                   apShape = nullptr;
};


struct CPhysObjectEvent_ContactStart
{
};


struct CPhysObjectEvent_TransformUpdated
{
};


struct CPhysObject
{
	ComponentNetVar< bool >               aStartActive        = false;
	ComponentNetVar< bool >               aAllowSleeping      = true;

	ComponentNetVar< float >              aMaxLinearVelocity  = 500.0f;                // Maximum linear velocity that this body can reach (m/s)
	ComponentNetVar< float >              aMaxAngularVelocity = 0.25f * M_PI * 60.0f;  // Maximum angular velocity that this body can reach (rad/s)

	ComponentNetVar< PhysMotionType >     aMotionType         = PhysMotionType::Static;
	ComponentNetVar< PhysMotionQuality >  aMotionQuality      = PhysMotionQuality::LinearCast;

	ComponentNetVar< bool >               aIsSensor           = false;

	ComponentNetVar< bool >               aCustomMass         = false;
	ComponentNetVar< float >              aMass               = 0.f;

	// Custom stuff
	ComponentNetVar< bool >               aGravity            = true;
	ComponentNetVar< bool >               aEnableCollision    = true;
	ComponentNetVar< EPhysTransformMode > aTransformMode{};

	IPhysicsObject*                       apObj = nullptr;
};


class EntSys_PhysShape : public IEntityComponentSystem
{
  public:
	EntSys_PhysShape() {}
	~EntSys_PhysShape() {}

	void ComponentAdded( Entity sEntity, void* spData ) override;
	void ComponentRemoved( Entity sEntity, void* spData ) override;
	void ComponentUpdated( Entity sEntity, void* spData ) override;
	void Update() override;
};


class EntSys_PhysObject : public IEntityComponentSystem
{
  public:
	EntSys_PhysObject() {}
	~EntSys_PhysObject() {}

	void ComponentAdded( Entity sEntity, void* spData ) override;
	void ComponentRemoved( Entity sEntity, void* spData ) override;
	void ComponentUpdated( Entity sEntity, void* spData ) override;
	void Update() override;
};


extern EntSys_PhysShape   gEntSys_PhysShape;
extern EntSys_PhysObject  gEntSys_PhysObject;


IPhysicsEnvironment*      GetPhysEnv();

// TODO: when physics materials are implemented, split up models by their physics material
void                      Phys_GetModelVerts( ch_handle_t sModel, PhysDataConvex_t& srData );
void                      Phys_GetModelTris( ch_handle_t sModel, std::vector< PhysTriangle_t >& srTris );
void                      Phys_GetModelInd( ch_handle_t sModel, PhysDataConcave_t& srData );

void                      Phys_Init();
void                      Phys_Shutdown();

void                      Phys_CreateEnv();
void                      Phys_DestroyEnv();

// Simulate This Physics Environment
void                      Phys_Simulate( IPhysicsEnvironment* spPhysEnv, float sFrameTime );
void                      Phys_SetMaxVelocities( IPhysicsObject* spPhysObj );

// Helper functions for creating physics shapes/objects in the physics engine and adding a component wrapper to the entity
// CPhysShape*            Phys_CreateShape( Entity sEntity, PhysicsShapeInfo& srShapeInfo );
CPhysObject*              Phys_CreateObject( Entity sEntity, PhysicsObjectInfo& srObjectInfo );
CPhysObject*              Phys_CreateObject( Entity sEntity, IPhysicsShape* spShape, PhysicsObjectInfo& srObjectInfo );

bool                      Phys_CreatePhysShapeComponent( CPhysShape* compPhysShape );

// Networking
// void                 Phys_NetworkRead();
// void                 Phys_NetworkWrite();

extern Ch_IPhysics*       ch_physics;

// Helper functions for getting the wrapper physics components
inline auto GetComp_PhysShape( Entity ent )  { return Ent_GetComponent< CPhysShape >( ent, "physShape" ); }
inline auto GetComp_PhysObject( Entity ent ) { return Ent_GetComponent< CPhysObject >( ent, "physObject" ); }

inline IPhysicsShape* GetComp_PhysShapePtr( Entity ent )
{
	if ( auto physShape = GetComp_PhysShape( ent ) )
		return physShape->apShape;

	return nullptr;
}

inline IPhysicsObject* GetComp_PhysObjectPtr( Entity ent )
{
	if ( auto physObject = GetComp_PhysObject( ent ) )
		return physObject->apObj;

	return nullptr;
}

