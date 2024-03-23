#include "physics.h"
#include "physics_debug.h"
#include "physics_object.h"


glm::vec3 PhysVirtualCharacter::GetLinearVelocity()
{
	return fromJolt( character->GetLinearVelocity() );
}


// Set the linear velocity of the character (m / s)
void PhysVirtualCharacter::SetLinearVelocity( const glm::vec3& linearVelocity )
{
	character->SetLinearVelocity( toJolt( linearVelocity ) );
}


// Get the position of the character
glm::vec3 PhysVirtualCharacter::GetPosition()
{
	return fromJolt( character->GetPosition() );
}


// Set the position of the character
void PhysVirtualCharacter::SetPosition( const glm::vec3& position )
{
	character->SetPosition( toJolt( position ) );
}


// Get the rotation of the character
glm::quat PhysVirtualCharacter::GetRotation()
{
	return fromJolt( character->GetRotation() );
}


// Set the rotation of the character
void PhysVirtualCharacter::SetRotation( const glm::quat& rotation )
{
	character->SetRotation( toJolt( rotation ) );
}


// Enable or Disable Collision
void PhysVirtualCharacter::EnableCollision( bool enabled )
{
	disableCollision = !enabled;
}


EPhysGroundState PhysVirtualCharacter::GetGroundState()
{
	JPH::CharacterBase::EGroundState groundState = character->GetGroundState();

	switch ( groundState )
	{
		case JPH::CharacterBase::EGroundState::OnGround:
			return EPhysGroundState_OnGround;

		case JPH::CharacterBase::EGroundState::OnSteepGround:
			return EPhysGroundState_OnSteepGround;

		case JPH::CharacterBase::EGroundState::NotSupported:
			return EPhysGroundState_NotSupported;

		case JPH::CharacterBase::EGroundState::InAir:
			return EPhysGroundState_InAir;
	}
}


glm::vec3 PhysVirtualCharacter::GetShapeOffset()
{
	return fromJolt( character->GetShapeOffset() );
}


void PhysVirtualCharacter::SetShapeOffset( const glm::vec3& offset )
{
	character->SetShapeOffset( toJolt( offset ) );
}


glm::mat4 PhysVirtualCharacter::GetCenterOfMassTransform()
{
	return fromJolt( character->GetCenterOfMassTransform() );
}


void PhysVirtualCharacter::SetAllowDebugDraw( bool sAllow )
{
	debugDraw = sAllow;
}


bool PhysVirtualCharacter::GetAllowDebugDraw()
{
	return debugDraw;
}

