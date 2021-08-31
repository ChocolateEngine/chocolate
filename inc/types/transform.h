#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>


struct Transform
{
	glm::mat4 LocalMatrix() const;
	glm::mat4 GlobalMatrix() const;
	void GlobalDecomposed( glm::vec3* position, glm::quat* rotation = nullptr, glm::vec3* scale = nullptr ) const;

	glm::vec3 position = {};
	glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
	glm::quat rotation = {};
};


inline glm::mat4 ToTransformation(const Transform &transform)
{
	glm::mat4 translation = glm::translate(transform.position);
	glm::mat4 rotation = glm::toMat4(transform.rotation);
	glm::mat4 scale = glm::scale(transform.scale);

	return translation * rotation * scale;
}


inline glm::mat4 ToFirstPersonCameraTransformation(const Transform &transform)
{
	glm::mat4 translation = glm::translate(-transform.position);
	glm::mat4 rotation = glm::toMat4(transform.rotation);
	glm::mat4 scale = glm::scale(transform.scale);

	return rotation * translation * scale;
}


inline glm::mat4 ToOrbitalCameraTransformation(const Transform &transform)
{
	return ToTransformation(transform);
}


inline glm::mat4 ToTransformationNoScale(const Transform &transform)
{
	glm::mat4 translation = glm::translate(-transform.position);
	glm::mat4 rotation = glm::toMat4(transform.rotation);

	return rotation * translation;
}

