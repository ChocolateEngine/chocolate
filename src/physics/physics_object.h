#pragma once

#include "physics/iphysics.h"
#include "physics.h"
#include "physics_shape.h"


// abstraction for physics objects, or bullet collision shapes
class PhysicsObject: public IPhysicsObject
{
public:
	PhysicsObject();
	~PhysicsObject() override;

	//bool                  Valid();

	glm::vec3             GetPos() override;
	glm::vec3             GetAng() override;

	void                  SetPos( const glm::vec3& pos, bool activate = true ) override;
	void                  SetAng( const glm::vec3& ang, bool activate = true ) override;

	// Transform&            GetWorldTransform() override;
	// void                  SetWorldTransform( const Transform& transform ) override;

	void                  Activate( bool active ) override;
	void                  SetAlwaysActive( bool alwaysActive ) override;
	void                  SetCollisionEnabled( bool enable ) override;
	void                  SetContinuousCollisionEnabled( bool enable ) override;

	void                  SetScale( const glm::vec3& scale ) override;

	void                  SetMaxLinearVelocity( float velocity ) override;
	float                 GetMaxLinearVelocity() override;

	void                  SetMaxAngularVelocity( float velocity ) override;
	float                 GetMaxAngularVelocity() override;

	void                  SetLinearVelocity( const glm::vec3& velocity ) override;
	void                  SetAngularVelocity( const glm::vec3& velocity ) override;
	glm::vec3             GetLinearVelocity() override;
	glm::vec3             GetAngularVelocity() override;

	void                  SetAllowSleeping( bool allow ) override;
	bool                  GetAllowSleeping() override;

	void                  SetFriction( float val ) override;
	float                 GetFriction() override;

	// void                  SetGravity( const glm::vec3& gravity ) override;
	// void                  SetGravity( float gravity ) override;  // convenience function
	
	void                  SetGravityEnabled( bool enabled ) override;
	bool                  GetGravityEnabled() override;

	void                  AddForce( const glm::vec3& force ) override;
	void                  AddImpulse( const glm::vec3& impulse ) override;

	int                   ContactTest() override;

	PhysShapeType         GetShapeType() override;

	// Check if this is a sensor. A sensor will receive collision callbacks, but will not cause any collision responses and can be used as a trigger volume.
	bool                  IsSensor() override;

	// ===============================================================
	// Part of Motion Properties

	void                  SetInverseMass( float inverseMass ) override;
	float                 GetInverseMass() override;

	// Set the inverse inertia tensor in local space by setting the diagonal and the rotation: \f$I_{body}^{-1} = R \: D \: R^{-1}\f$
	void                  SetInverseInertia( const glm::vec3& diagonal, const glm::quat& rot ) override;

	void                  CheckCollision( float sMaxSeparationDist, PhysCollisionCollector* spPhysCollector ) override;
	void                  CheckCollision( IPhysicsShape* spShape, float sMaxSeparationDist, PhysCollisionCollector* spPhysCollector ) override;

	// ===============================================================
	// Internal Functions

	JPH::Vec3             GetPositionJolt();
	JPH::Quat             GetRotationJolt();

	JPH::Body*            apBody = nullptr;
	PhysicsShape*         apShape = nullptr;

	// The layer the body is in
	JPH::ObjectLayer      aLayer;

	// um
	PhysicsEnvironment*   apEnv = nullptr;

	PhysicsObjectInfo     aPhysInfo;

	friend class PhysicsEnvironment;
};


