#pragma once

#include "physics/iphysics.h"
#include "physics.h"


class PhysicsShape : public IPhysicsShape
{
public:
	PhysicsShape( const PhysicsShapeInfo& shapeType );
	~PhysicsShape() override;

	PhysicsShapeInfo        aPhysInfo;
	JPH::Ref<JPH::Shape>    aShape;
	JPH::ShapeSettings*     apShapeSettings = nullptr;  // useless?

	friend class PhysicsEnvironment;
	friend class PhysicsObject;
};

