#include "types/transform.h"


glm::mat4 Transform::LocalMatrix() const
{
	glm::mat4 matrix = {};

	matrix = glm::translate(matrix, position);
	matrix *= glm::mat4_cast(rotation);
	matrix = glm::scale(matrix, scale);

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


void Transform::GlobalDecomposed( glm::vec3* position, glm::quat* rotation, glm::vec3* scale ) const
{
	glm::vec3 tempPosition = {};
	glm::quat tempRotation = {};

	if (!position)
		position = &tempPosition;
	if (!rotation)
		rotation = &tempRotation;

	// ???????????
	// glm::decompose( GlobalMatrix(), *scale, *rotation, *position, glm::vec3(), glm::vec4() );
	if ( scale )
		//DecomposeMatrix( GlobalMatrix(), *position, *rotation, *scale );
		DecomposeMatrix( ToTransformation(*this), *position, *rotation, *scale );
	else
		//DecomposeMatrix( GlobalMatrix(), *position, *rotation );
		DecomposeMatrix( ToTransformation(*this), *position, *rotation );

	//*rotation = glm::inverse(*rotation); // not sure why, goes crazy without???
}


