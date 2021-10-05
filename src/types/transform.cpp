#include "../../inc/types/transform.h"


glm::mat4 Transform::LocalMatrix() const
{
	glm::mat4 matrix = {};

	matrix = glm::translate(matrix, aPos);
	matrix *= glm::mat4_cast(rotation);
	matrix = glm::scale(matrix, aScale);

	return matrix;
}

glm::mat4 Transform::GlobalMatrix() const
{
	glm::mat4 matrix = {};

	const Transform* transform = this;

	do {
		matrix = transform->LocalMatrix() * matrix;

		//if (transform->parent.valid() && transform->parent.has_component<Transform>())
		//	transform = transform->parent.component<const Transform>().get();
		//else
			transform = nullptr;

	} while (transform);

	return matrix;
}


glm::mat4 Transform::ToViewMatrix(  )
{
	glm::mat4 viewMatrix(1.0f);

	/* Y Rotation (Mouse X) */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[YAW]), glm::vec3(0,1,0) );

	/* X Rotation (Mouse Y) */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[PITCH]), glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]) );

	/* Z Rotation */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[ROLL]), glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]) );

	return glm::translate( viewMatrix, -aPos );
}


// https://stackoverflow.com/questions/17918033/glm-decompose-mat4-into-translation-and-rotation
void DecomposeMatrix( const glm::mat4& m, glm::vec3& pos, glm::quat& rot, glm::vec3& scale )
{
	pos = m[3];

	for(int i = 0; i < 3; i++)
		scale[i] = glm::length(glm::vec3(m[i]));

	const glm::mat3 rotMtx(
		glm::vec3(m[0]) / scale[0],
		glm::vec3(m[1]) / scale[1],
		glm::vec3(m[2]) / scale[2]);

	rot = glm::quat_cast(rotMtx);
}


void DecomposeMatrix( const glm::mat4& m, glm::vec3& pos, glm::quat& rot )
{
	pos = m[3];
	rot = glm::quat_cast(m);
}


void Transform::GlobalDecomposed( glm::vec3* aPos, glm::quat* rotation, glm::vec3* scale ) const
{
	glm::vec3 tempPosition = {};
	glm::quat tempRotation = {};

	if (!aPos)
		aPos = &tempPosition;
	if (!rotation)
		rotation = &tempRotation;

	// ???????????
	// glm::decompose( GlobalMatrix(), *scale, *rotation, *position, glm::vec3(), glm::vec4() );
	if ( scale )
		//DecomposeMatrix( GlobalMatrix(), *position, *rotation, *scale );
		DecomposeMatrix( ToTransformation(*this), *aPos, *rotation, *scale );
	else
		//DecomposeMatrix( GlobalMatrix(), *position, *rotation );
		DecomposeMatrix( ToTransformation(*this), *aPos, *rotation );

	//*rotation = glm::inverse(*rotation); // not sure why, goes crazy without???
}


inline glm::mat4 ToFirstPersonCameraTransformation( const Transform &transform )
{
	// test
	//glm::mat4 viewMatrix = glm::lookAt( transform.aPos, transform.aAng, glm::vec3(0,1,0));
	//return viewMatrix * glm::scale(transform.scale);


	glm::mat4 translation = glm::translate(-transform.aPos);
	// glm::mat4 rotation = glm::toMat4(transform.rotation);

	glm::quat rotFromAng = glm::toQuat( glm::orientate3(transform.aAng) );
	glm::mat4 rotation = glm::toMat4( rotFromAng );

	glm::mat4 scale = glm::scale( transform.aScale );

	return rotation * translation * scale;
}


