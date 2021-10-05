#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>


constexpr int PITCH = 0;
constexpr int YAW = 1;
constexpr int ROLL = 2;

constexpr int WORLD_UP = 1;  // y axis for now


struct Transform
{
	glm::mat4 LocalMatrix() const;
	glm::mat4 GlobalMatrix() const;
	void GlobalDecomposed( glm::vec3* position, glm::quat* rotation = nullptr, glm::vec3* scale = nullptr ) const;

	glm::mat4 ToViewMatrix(  );

	glm::vec3 aPos = {};
	glm::vec3 aScale = { 1.0f, 1.0f, 1.0f };
	glm::vec3 aAng = {};

	glm::quat rotation = {};  // TODO: fully remove this
};


void DecomposeMatrix( const glm::mat4& m, glm::vec3& pos, glm::quat& rot, glm::vec3& scale );
void DecomposeMatrix( const glm::mat4& m, glm::vec3& pos, glm::quat& rot );


inline glm::mat4 ToTransformation( const Transform &transform )
{
	glm::mat4 translation = glm::translate(transform.aPos);
	glm::mat4 rotation = glm::toMat4(transform.rotation);
	glm::mat4 scale = glm::scale(transform.aScale);

	return translation * rotation * scale;
}


inline glm::mat4 ToOrbitalCameraTransformation( const Transform &transform )
{
	return ToTransformation(transform);
}


inline glm::mat4 ToTransformationNoScale( const Transform &transform )
{
	glm::mat4 translation = glm::translate(-transform.aPos);
	glm::mat4 rotation = glm::toMat4(transform.rotation);

	return rotation * translation;
}

