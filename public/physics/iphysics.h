#pragma once

/*
* Interface for Engine Implementation of Jolt Physics
*/

#include <glm/vec3.hpp>
#include "types/transform.h"
#include "system.h"


// blech
class Model;


// TODO: how tf will you do vehicles lol


enum class PhysShapeType
{
	Invalid = 0,

	Sphere,             // A sphere centered around zero.
	Box,                // A box centered around zero.
	Capsule,            // A capsule centered around zero.
	TaperedCapsule,     // NOT IMPLEMENTED - A capsule with different radii at the bottom and top.
	Cylinder,           // A cylinder shape. Note that cylinders are the least stable of all shapes, so use another shape if possible.
	Convex,             // A convex hull defined by a set of points.
	Mesh,               // A shape consisting of triangles. Any body that uses this shape needs to be static.

	// NOT IMPLEMENTED BELOW - ON TODO LIST !!!

	StaticCompound,     // A shape containing other shapes. This shape is constructed once and cannot be changed afterwards.
						// Child shapes are organized in a tree to speed up collision detection.

	MutableCompound,    // A shape containing other shapes. This shape can be constructed/changed at runtime and trades construction time for runtime performance.
						// Child shapes are organized in a list to make modification easy.

	HeightField,        // A shape consisting of NxN points that define the height at each point, very suitable for representing hilly terrain.
						// Any body that uses this shape needs to be static.

	Max = HeightField
};


// Jolt also has these decorator shapes???
// no idea how these work, so i'll just leave it here for now
// they "change the behavior of their children", so idk
// maybe i could figure out scaling for player ducking
// and use the offset one for setting the origin to the bottom of the object? idk
enum class PhysDecoratorShape
{
	Invalid = 0,

	Scaled,             // This shape can uniformly scale a child shape.
						// Note that if a shape is rotated first and then scaled,
						// you can introduce shearing which is not supported by the library.

	RotatedTranslated,  // This shape can rotate and translate a child shape, it can e.g. be used to offset a sphere from the origin.

	OffsetCenterOfMass, // This shape does not change its child shape but it does shift the calculated center of mass for that shape.
						// It allows you to e.g. shift the center of mass of a vehicle down to improve its handling.

	Max = OffsetCenterOfMass
};


enum class PhysMotionType
{
	Static,     // not simulating
	Kinematic,  // moved by velocities only
	Dynamic,    // moved by forces
};

// emum class PhysCollisionType


struct PhysicsShapeInfo
{
	PhysicsShapeInfo( PhysShapeType shapeType ):
		aShapeType( shapeType )
	{
	}

	~PhysicsShapeInfo()
	{
	}

	// Sets the shape we want the physics engine to create
	PhysShapeType           aShapeType = PhysShapeType::Invalid;

	union
	{
		// use if ShapeType is Concave or Convex
		struct
		{
			// Only used for when making a Convex or a Concave collision mesh
			// i don't like this being a Model*, but idk
			// maybe i can use IMesh** apMeshes and size_t aMeshCount?
			Model*                      apModel = nullptr;

			// IMesh*              mesh = nullptr;
			// std::vector< vertex_3d_t >  aVertices;
			// std::vector< uint32_t >     aIndices;

			// Tell the physics engine to optimize this convex collision mesh
			bool                        aOptimizeConvex = true;
		} aMeshData;

		// Otherwise, use this (maybe make a separate thing? idk)
		glm::vec3           aBounds = {0, 0, 0};
	};
};



struct PhysicsObjectInfo
{
	PhysicsObjectInfo()
	{
	}

	~PhysicsObjectInfo()
	{
	}

	glm::vec3           aPos{};
	glm::vec3           aAng{};

	bool                aStartActive = false;

	PhysMotionType      aMotionType = PhysMotionType::Static;

	bool                aHasMass = false;
	float               aMass;
};


// um, maybe use handles for this and store in some keyvalues file? idk
class IPhysicsMaterial
{
public:
};


// Shapes are refcounted (internally) and can be shared between bodies
class IPhysicsShape
{
public:
	virtual ~IPhysicsShape() = default;
};


class PhysCollisionCollector;


// this could technically be a Handle, but i mean, that would add a lot of functions so
class IPhysicsObject
{
public:
	virtual ~IPhysicsObject() = default;

	virtual glm::vec3             GetPos() = 0;
	virtual glm::vec3             GetAng() = 0;

	virtual void                  SetPos( const glm::vec3& pos, bool activate = true ) = 0;
	virtual void                  SetAng( const glm::vec3& ang, bool activate = true ) = 0;

	virtual void                  Activate( bool active ) = 0;
	virtual void                  SetAlwaysActive( bool alwaysActive ) = 0;
	virtual void                  SetCollisionEnabled( bool enable ) = 0;
	virtual void                  SetContinuousCollisionEnabled( bool enable ) = 0;

	virtual void                  SetScale( const glm::vec3& scale ) = 0;

	virtual void                  SetMaxLinearVelocity( float velocity ) = 0;
	virtual float                 GetMaxLinearVelocity() = 0;

	virtual void                  SetMaxAngularVelocity( float velocity ) = 0;
	virtual float                 GetMaxAngularVelocity() = 0;

	virtual void                  SetLinearVelocity( const glm::vec3& velocity ) = 0;
	virtual void                  SetAngularVelocity( const glm::vec3& velocity ) = 0;

	virtual glm::vec3             GetLinearVelocity() = 0;
	virtual glm::vec3             GetAngularVelocity() = 0;

	virtual void                  SetAllowSleeping( bool allow ) = 0;
	virtual bool                  GetAllowSleeping() = 0;

	virtual void                  SetFriction( float val ) = 0;
	virtual float                 GetFriction() = 0;

	virtual void                  SetGravityEnabled( bool enabled ) = 0;
	virtual bool                  GetGravityEnabled() = 0;

	virtual void                  AddForce( const glm::vec3& force ) = 0;
	virtual void                  AddImpulse( const glm::vec3& impulse ) = 0;

	virtual int                   ContactTest() = 0;

	virtual PhysShapeType         GetShapeType() = 0;

	// A sensor will receive collision callbacks, but will not cause any collision responses and can be used as a trigger volume.
	virtual bool                  IsSensor() = 0;

	// ===============================================================
	// Part of Motion Properties

	virtual void                  SetInverseMass( float inverseMass ) = 0;
	virtual float                 GetInverseMass() = 0;

	// Set the inverse inertia tensor in local space by setting the diagonal and the rotation: \f$I_{body}^{-1} = R \: D \: R^{-1}\f$
	virtual void                  SetInverseInertia( const glm::vec3& diagonal, const glm::quat& rot ) = 0;

	// ===============================================================
	// Physics World Interactions

	virtual void                  CheckCollision( float sMaxSeparationDist, PhysCollisionCollector* spPhysCollector ) = 0;
	virtual void                  CheckCollision( IPhysicsShape* spShape, float sMaxSeparationDist, PhysCollisionCollector* spPhysCollector ) = 0;

	// TODO: make some CastRay function here and in IPhysicsEnvironment
};


class PhysObjectFilter
{
public:
	virtual                    ~PhysObjectFilter() = default;

	virtual bool                ShouldCollide( const IPhysicsObject* spPhysObj )
	{
		return true;
	}
};


struct PhysCollisionResult
{
	// using Face = std::array< glm::vec3, 32 >;

	glm::vec3               aContactPointOn1;           // Contact point on shape 1 (in world space)
	glm::vec3               aContactPointOn2;           // Contact point on shape 2 (in world space)
	glm::vec3               aPenetrationAxis;           // Direction to move shape 2 out of collision along the shortest path (magnitude is meaningless, in world space)
	float                   aPenetrationDepth;          // Penetration depth (move shape 2 by this distance to resolve the collision)

	// um
	// IPhysicsShape*          apSubShape1;                // Sub shape ID that identifies the face on shape 1
	// IPhysicsShape*          apSubShape2;                // Sub shape ID that identifies the face on shape 2

	IPhysicsObject*         apPhysObj2;                 // BodyID to which shape 2 belongs to

	// Face                    aShape1Face;                // Colliding face on shape 1 (optional result, in world space)
	// Face                    aShape2Face;                // Colliding face on shape 2 (optional result, in world space)
};


class PhysCollisionCollector
{
public:
	virtual                    ~PhysCollisionCollector() = default;

	virtual void                AddResult( const PhysCollisionResult& sPhysResult ) = 0;

	// virtual bool                SetContext( const IPhysicsObject* spPhysObj ) = 0;
	// virtual IPhysicsObject*     GetContext() = 0;
};


class IPhysicsEnvironment
{
public:
	virtual ~IPhysicsEnvironment() = default;

	virtual void                            Init() = 0;
	virtual void                            Shutdown() = 0;
	virtual void                            Simulate( float sDT ) = 0;

	// deprecated
	// virtual IPhysicsObject*                 CreatePhysicsObject( PhysicsObjectInfo& physInfo ) = 0;
	// virtual void                            DeletePhysicsObject( IPhysicsObject *body ) = 0;

	// ----------------------------------------------------------------------------
	// Physics Object/Shape Creation

	// Create a shape that can be shared between bodies
	virtual IPhysicsShape*                  CreateShape( const PhysicsShapeInfo& physInfo ) = 0;
	virtual void                            DestroyShape( IPhysicsShape *body ) = 0;

	// Create a Physics Object from a shape
	virtual IPhysicsObject*                 CreateObject( IPhysicsShape* spShape, const PhysicsObjectInfo& physInfo ) = 0;
	virtual void                            DestroyObject( IPhysicsObject* spObj ) = 0;
	
	// ----------------------------------------------------------------------------
	// Environment Controls

	virtual void                            SetGravity( const glm::vec3& gravity ) = 0;
	virtual void                            SetGravityY( float gravity ) = 0;  // convenience functions
	virtual void                            SetGravityZ( float gravity ) = 0;
	virtual glm::vec3                       GetGravity() = 0;

	// ----------------------------------------------------------------------------
	// Tools

	// virtual bool                            RegisterCollisionCollector( PhysCollisionCollector* spCollector ) = 0;

	// virtual bool                            SetCollisionCollectorContext( PhysCollisionCollector* spCollector, const IPhysicsObject* spPhysObj ) = 0;
	// virtual const IPhysicsObject*           GetCollisionCollectorContext( PhysCollisionCollector* spCollector ) = 0;

	// virtual void                            RayTest( const glm::vec3& from, const glm::vec3& to, std::vector< RayHit* >& hits );
};


class Ch_IPhysics : public BaseSystem
{
public:
	virtual IPhysicsEnvironment*           CreatePhysEnv() = 0;
	virtual void                           DestroyPhysEnv( IPhysicsEnvironment* pEnv ) = 0;
};
