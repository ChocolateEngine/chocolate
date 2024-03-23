#pragma once

#include "physics/iphysics.h"
#include "physics.h"


class PhysicsShape : public IPhysicsShape
{
public:
	PhysicsShape( PhysShapeType shapeType );
	~PhysicsShape() override;

	// PhysicsShapeInfo        aPhysInfo;
	PhysShapeType           aType;
	JPH::Ref<JPH::Shape>    aShape;
	JPH::ShapeSettings*     apShapeSettings = nullptr;  // useless?
	u32                     aRefCount       = 1;

	friend class PhysicsEnvironment;
	friend class PhysicsObject;
};

