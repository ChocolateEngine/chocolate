#pragma once

// Game Implementation of Physics, mainly for helper stuff and loading physics

#include "physics/iphysics.h"


enum EPhysTransformMode : u8
{
	// Don't do anything with the transform component
	EPhysTransformMode_None,

	// Override the transform component's values with what we got from the physics object
	EPhysTransformMode_Update,

	// Have the physics object use the values from the transform component
	EPhysTransformMode_Inherit,
};


IPhysicsEnvironment*      GetPhysEnv();

// TODO: when physics materials are implemented, split up models by their physics material
void                      Phys_GetModelVerts( Handle sModel, PhysDataConvex_t& srData );
void                      Phys_GetModelTris( Handle sModel, std::vector< PhysTriangle_t >& srTris );
void                      Phys_GetModelInd( Handle sModel, PhysDataConcave_t& srData );

void                      Phys_Init();
void                      Phys_Shutdown();

void                      Phys_CreateEnv();
void                      Phys_DestroyEnv();

// Simulate This Physics Environment
void                      Phys_Simulate( IPhysicsEnvironment* spPhysEnv, float sFrameTime );
void                      Phys_SetMaxVelocities( IPhysicsObject* spPhysObj );


extern Ch_IPhysics*       ch_physics;

