#pragma once

#include "physics/iphysics.h"
#include "render/irender.h"

#ifndef JPH_DEBUG_RENDERER
	// Hack to compile DebugRenderer
	#define JPH_DEBUG_RENDERER 1
#endif

// workaround to not include chocolate's core/core.h, which should be renamed to ch_core/core.h
#include <JoltPhysics/Jolt/Core/Core.h>
#include <JoltPhysics/Jolt/Core/Profiler.h>

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt.h>

// Jolt includes
#include <RegisterTypes.h>
#include <Core/TempAllocator.h>
#include <Core/JobSystemThreadPool.h>
#include <Physics/PhysicsSettings.h>
#include <Physics/PhysicsSystem.h>

// shapes
#include <Physics/Collision/Shape/HeightFieldShape.h>
#include <Physics/Collision/Shape/MeshShape.h>
#include <Physics/Collision/Shape/SphereShape.h>
#include <Physics/Collision/Shape/BoxShape.h>
#include <Physics/Collision/Shape/ConvexHullShape.h>
#include <Physics/Collision/Shape/CapsuleShape.h>
#include <Physics/Collision/Shape/TaperedCapsuleShape.h>
#include <Physics/Collision/Shape/CylinderShape.h>
#include <Physics/Collision/Shape/TriangleShape.h>
#include <Physics/Collision/Shape/StaticCompoundShape.h>
#include <Physics/Collision/Shape/MutableCompoundShape.h>
#include <Physics/Collision/Shape/ScaledShape.h>

// body
#include <Physics/Body/BodyCreationSettings.h>
#include <Physics/Body/BodyActivationListener.h>

// #include <Renderer/DebugRenderer.h>

#include "types/transform.h"


inline glm::mat4 fromJolt( const JPH::Mat44& from ) {
	return glm::mat4(
		from(0, 0), from(1, 0), from(2, 0), from(3, 0),
		from(0, 1), from(1, 1), from(2, 1), from(3, 1),
		from(0, 2), from(1, 2), from(2, 2), from(3, 2),
		from(0, 3), from(1, 3), from(2, 3), from(3, 3)
	);
}

inline glm::quat fromJolt( const JPH::Quat& from ) {
	return glm::quat( from.GetW(), from.GetX(), from.GetY(), from.GetZ() );
}

inline glm::vec3 fromJolt( const JPH::Vec3& from ) {
	return glm::vec3( from.GetX(), from.GetY(), from.GetZ() );
}

inline glm::vec3 fromJolt( const JPH::Float3& from ) {
	return glm::vec3( from[0], from[1], from[2] );
}

inline glm::vec2 fromJolt( const JPH::Float2& from ) {
	return glm::vec2( from.x, from.y );  // nice consistency lmao
}

inline glm::vec3 fromJolt( const JPH::ColorArg& from ) {
	return glm::vec3( from.r, from.g, from.b );
}

inline JPH::Quat toJolt( const glm::quat& from ) {
	return JPH::Quat( from.x, from.y, from.z, from.w );
}

inline JPH::Vec3 toJolt( const glm::vec3& from ) {
	return JPH::Vec3( from.x, from.y, from.z );
}

inline JPH::Float3 toJoltFl( const glm::vec3& from ) {
	return JPH::Float3( from.x, from.y, from.z );
}


inline JPH::Quat toJoltRot( const glm::vec3& from )
{
	glm::quat q = from;
	return JPH::Quat( q.x, q.y, q.z, q.w );
}

inline glm::vec3 fromJoltRot( const JPH::Quat& from )
{
	glm::quat q( from.GetW(), from.GetX(), from.GetY(), from.GetZ() );
	return glm::degrees( ToEulerAngles(q) );
}


const char* PhysShapeType2Str( PhysShapeType type );

class PhysicsObject;


struct ContactEvent
{
	struct Contact
	{
		glm::vec3 globalContactPosition;
		glm::vec3 globalContactNormal;
		float contactDistance = 0.f;
		float contactImpulse = 0.f;
	};

	PhysicsObject* first = nullptr;
	PhysicsObject* second = nullptr;

	ContactEvent::Contact contacts[4] = {};
	uint8_t contactCount = 0;
};


struct RayHit
{
	struct Contact
	{
		glm::vec3 globalContactPosition;
		glm::vec3 globalContactNormal;
		float contactDistance = 0.f;
		float contactImpulse = 0.f;
	};

	PhysicsObject* physObj = nullptr;

	ContactEvent::Contact contacts[4] = {};
	uint8_t contactCount = 0;
};


class PhysDebugDraw;


class PhysicsEnvironment: public IPhysicsEnvironment
{
public:
	PhysicsEnvironment();
	~PhysicsEnvironment() override;

	void                            Init() override;
	void                            Shutdown() override;
	void                            Simulate( float sDT ) override;

	// IPhysicsObject*                 CreatePhysicsObject( PhysicsObjectInfo& physInfo ) override;
	// void                            DeletePhysicsObject( IPhysicsObject *body ) override;

	// hmm
	IPhysicsShape*                  CreateShape( const PhysicsShapeInfo& physInfo ) override;
	void                            DestroyShape( IPhysicsShape *body ) override;

	IPhysicsObject*                 CreateObject( IPhysicsShape* spShape, const PhysicsObjectInfo& physInfo ) override;
	void                            DestroyObject( IPhysicsObject *body ) override;

	void                            SetGravity( const glm::vec3& gravity ) override;
	void                            SetGravityY( float gravity ) override; // convenience functions
	void                            SetGravityZ( float gravity ) override;
	glm::vec3                       GetGravity() override;

	// void                            RayTest( const glm::vec3& from, const glm::vec3& to, std::vector< RayHit* >& hits );

	std::vector< PhysicsObject* >   aPhysObjs;

public:
	JPH::ShapeSettings*             LoadModel( const PhysicsShapeInfo& physInfo );

	PhysDebugDraw*                  apDebugDraw;

	JPH::PhysicsSystem*             apPhys;
};


// for debugging
extern IRender* render;


// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).

// TODO: have the game be able to override or entirely handle these
enum : JPH::ObjectLayer
{
	ObjLayer_Stationary = 0,
	ObjLayer_Moving,
	ObjLayer_NoCollide,
	ObjLayer_Count,
};


// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.

extern JPH::BroadPhaseLayer BroadPhase_Stationary;
extern JPH::BroadPhaseLayer BroadPhase_Moving;
extern JPH::BroadPhaseLayer BroadPhase_NoCollide;


// other
extern JPH::ObjectToBroadPhaseLayer gObjectToBroadphase;

