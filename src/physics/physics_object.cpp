#include "physics.h"
#include "physics_object.h"

#include <Physics/Collision/CollideShape.h>

LOG_CHANNEL( Physics );

// ====================================================================================


PhysicsObject::PhysicsObject()
{
}


PhysicsObject::~PhysicsObject()
{
}


glm::vec3 PhysicsObject::GetPos()
{
	// return fromJolt( apEnv->apPhys->GetBodyInterface().GetCenterOfMassPosition( apBody->GetID() ) );
	return fromJolt( apEnv->apPhys->GetBodyInterface().GetPosition( apBody->GetID() ) );
	// return fromJolt( apBody->GetPosition() );
}

glm::vec3 PhysicsObject::GetAng()
{
	return fromJoltRot( apEnv->apPhys->GetBodyInterface().GetRotation( apBody->GetID() ) );
	// return fromJoltRot( apBody->GetRotation() );
}


inline JPH::EActivation GetActivateEnum( bool activate )
{
	return (activate) ? JPH::EActivation::Activate : JPH::EActivation::DontActivate;
}


void PhysicsObject::SetPos( const glm::vec3& pos, bool activate )
{
	apEnv->apPhys->GetBodyInterface().SetPosition( apBody->GetID(), toJolt( pos ), GetActivateEnum( activate ) );
}

void PhysicsObject::SetAng( const glm::vec3& ang, bool activate )
{
	apEnv->apPhys->GetBodyInterface().SetRotation( apBody->GetID(), toJoltRot( glm::radians( ang ) ), GetActivateEnum( activate ) );
}


void PhysicsObject::SetAllowSleeping( bool allow )
{
	apBody->SetAllowSleeping( allow );
}

bool PhysicsObject::GetAllowSleeping()
{
	return apBody->GetAllowSleeping();
}


void PhysicsObject::SetFriction( float val )
{
	apBody->SetFriction( val );
}

float PhysicsObject::GetFriction()
{
	return apBody->GetFriction();
}


void PhysicsObject::SetScale( const glm::vec3& scale )
{
}


void PhysicsObject::Activate( bool active )
{
}


void PhysicsObject::SetAlwaysActive( bool alwaysActive )
{
}


void PhysicsObject::SetCollisionEnabled( bool enable )
{
}


void PhysicsObject::SetContinuousCollisionEnabled( bool enable )
{
}


void PhysicsObject::SetMaxLinearVelocity( float velocity )
{
	auto motion = apBody->GetMotionProperties();
	Assert( motion );
	motion->SetMaxLinearVelocity( velocity );
}

float PhysicsObject::GetMaxLinearVelocity()
{
	auto motion = apBody->GetMotionProperties();
	Assert( motion );
	return motion->GetMaxLinearVelocity();
}


void PhysicsObject::SetMaxAngularVelocity( float velocity )
{
	auto motion = apBody->GetMotionProperties();
	Assert( motion );
	motion->SetMaxAngularVelocity( velocity );
}

float PhysicsObject::GetMaxAngularVelocity()
{
	auto motion = apBody->GetMotionProperties();
	Assert( motion );
	return motion->GetMaxAngularVelocity();
}

void PhysicsObject::SetLinearVelocity( const glm::vec3& velocity )
{
	// auto motion = apBody->GetMotionProperties();
	// motion->SetLinearVelocityClamped( toJolt( velocity ) );
	// motion->SetLinearVelocity( toJolt( velocity ) );
	apEnv->apPhys->GetBodyInterface().SetLinearVelocity( apBody->GetID(), toJolt( velocity ) );
	// apBody->SetLinearVelocity( toJolt( velocity ) );
}

void PhysicsObject::SetAngularVelocity( const glm::vec3& velocity )
{
	// apBody->SetAngularVelocityClamped( toJolt( velocity ) );
	apEnv->apPhys->GetBodyInterface().SetAngularVelocity( apBody->GetID(), toJolt( velocity ) );

	// when can i call this?
	// auto motion = apBody->GetMotionProperties();
	// motion->SetAngularVelocityClamped( toJolt( velocity ) );
}


glm::vec3 PhysicsObject::GetLinearVelocity(  )
{
	return fromJolt( apEnv->apPhys->GetBodyInterface().GetLinearVelocity( apBody->GetID() ) );
	// return fromJolt( apBody->GetLinearVelocity() );
}

glm::vec3 PhysicsObject::GetAngularVelocity(  )
{
	return fromJolt( apBody->GetAngularVelocity() );
}


void PhysicsObject::SetGravityEnabled( bool enabled )
{
	auto motion = apBody->GetMotionProperties();
	motion->SetGravityFactor( enabled );
}

bool PhysicsObject::GetGravityEnabled()
{
	auto motion = apBody->GetMotionProperties();
	return motion->GetGravityFactor();
}


void PhysicsObject::AddForce( const glm::vec3& force )
{
	apBody->AddForce( toJolt( force ) );
}

void PhysicsObject::AddImpulse( const glm::vec3& impulse )
{
	apBody->AddImpulse( toJolt( impulse ) );
}


int PhysicsObject::ContactTest()
{
	return 0;
}


PhysShapeType PhysicsObject::GetShapeType()
{
	return apShape->aPhysInfo.aShapeType;
}


bool PhysicsObject::IsSensor()
{
	return apBody->IsSensor();
}


void PhysicsObject::SetInverseMass( float inverseMass )
{
	apBody->GetMotionProperties()->SetInverseMass( 1.0f / aPhysInfo.aMass );
}


float PhysicsObject::GetInverseMass()
{
	return apBody->GetMotionProperties()->GetInverseMass();
}


// Set the inverse inertia tensor in local space by setting the diagonal and the rotation: \f$I_{body}^{-1} = R \: D \: R^{-1}\f$
void PhysicsObject::SetInverseInertia( const glm::vec3& diagonal, const glm::quat& rot )
{
	apBody->GetMotionProperties()->SetInverseInertia( toJolt( diagonal ), toJolt( rot ) );
}


static inline const JPH::NarrowPhaseQuery &sGetNarrowPhaseQuery( JPH::PhysicsSystem *inSystem, bool inLockBodies )
{
	return inLockBodies ? inSystem->GetNarrowPhaseQuery() : inSystem->GetNarrowPhaseQueryNoLock();
}

// TODO: create some PhysObjFilter class


class PhysCollisionCollectorInternal: public JPH::CollideShapeCollector
{
public:
	// Constructor
	explicit PhysCollisionCollectorInternal( PhysicsObject* spPhysObj, PhysCollisionCollector* spPhysCollector ) : 
		apPhysObj( spPhysObj ),
		apPhysCollector( spPhysCollector )
	{}

	void AddHit( const JPH::CollideShapeResult &inResult ) override
	{
		IPhysicsObject* physObj = nullptr;

		// WTF
		for ( PhysicsObject* physObjIter : apPhysObj->apEnv->aPhysObjs )
		{
			if ( physObjIter->apBody->GetID() != inResult.mBodyID2 )
				continue;
			
			physObj = physObjIter;
			break;
		}

		PhysCollisionResult result{
			.aContactPointOn1 = fromJolt( inResult.mContactPointOn1 ),
			.aContactPointOn2 = fromJolt( inResult.mContactPointOn2 ),
			.aPenetrationAxis = fromJolt( inResult.mPenetrationAxis ),
			.aPenetrationDepth = inResult.mPenetrationDepth,
			.apPhysObj2 = physObj,
			// .apSubShape1 = inResult.mSubShapeID1,
			// .apSubShape2 = inResult.mSubShapeID2,
		};

		apPhysCollector->AddResult( result );
	}

	PhysicsObject*          apPhysObj;
	PhysCollisionCollector* apPhysCollector;
};


void PhysicsObject::CheckCollision( float sMaxSeparationDist, PhysCollisionCollector* spPhysCollector )
{
	CheckCollision( apShape, sMaxSeparationDist, spPhysCollector );
}


// NOTE: COPIED FROM Character.cpp SO IT WOULD PROBABLY NEED CHANGING FOR STANDARD PHYSICS OBJECTS
void PhysicsObject::CheckCollision( IPhysicsShape* spShape, float sMaxSeparationDist, PhysCollisionCollector* spPhysCollector )
{
	PhysCollisionCollectorInternal physCollector( this, spPhysCollector );

	PhysicsShape* shape = (PhysicsShape*)spShape;

	// Create query broadphase layer filter
	JPH::DefaultBroadPhaseLayerFilter broadphase_layer_filter = apEnv->apPhys->GetDefaultBroadPhaseLayerFilter( aLayer );

	// Create query object layer filter
	JPH::DefaultObjectLayerFilter object_layer_filter = apEnv->apPhys->GetDefaultLayerFilter( aLayer );

	// Ignore my own body
	JPH::IgnoreSingleBodyFilter body_filter( apBody->GetID() );

	// Determine position to test
	JPH::Vec3 position;
	JPH::Quat rotation;

	JPH::BodyInterface &bodyInterface = apEnv->apPhys->GetBodyInterface();

	bodyInterface.GetPositionAndRotation( apBody->GetID(), position, rotation );

	JPH::Mat44 query_transform = JPH::Mat44::sRotationTranslation( rotation, position + rotation * shape->aShape->GetCenterOfMass() );

	// Settings for collide shape
	JPH::CollideShapeSettings settings;
	// settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideOnlyWithActive;
	settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideWithAll;
	settings.mActiveEdgeMovementDirection = bodyInterface.GetLinearVelocity( apBody->GetID() );
	settings.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;
	settings.mMaxSeparationDistance = sMaxSeparationDist;

	sGetNarrowPhaseQuery( apEnv->apPhys, true ).CollideShape(
		shape->aShape,
		JPH::Vec3::sReplicate(1.0f),
		query_transform,
		settings,
		physCollector,
		broadphase_layer_filter,
		object_layer_filter,
		body_filter
	);
}


// --------------------------------------------------------------

JPH::Vec3 PhysicsObject::GetPositionJolt()
{
	return apBody->GetPosition();
}

JPH::Quat PhysicsObject::GetRotationJolt()
{
	return apBody->GetRotation();
}


