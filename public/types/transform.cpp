#include "transform.h"


glm::mat4 Transform::ToMatrix( bool useScale ) const
{
	glm::mat4 matrix = glm::translate( aPos );

#if 0
	matrix *= glm::rotate( glm::radians(aAng[YAW]), vec_up );
	matrix *= glm::rotate( glm::radians(aAng[PITCH]), vec_right );
	matrix *= glm::rotate( glm::radians(aAng[ROLL]), vec_forward );

	if ( !useScale )
		return matrix;

	if ( aScale.x == 1.0 && aScale.y == 1.0 && aScale.z == 1.0 )
		return matrix;

	return matrix * glm::scale( aScale );
#else
	// if(!forcedCalculate && !isModelChanged())
	// return model;

	if ( useScale && (aScale.x != 1.0 && aScale.y != 1.0 && aScale.z != 1.0) )
		matrix = glm::scale( matrix, aScale );

	matrix *= glm::eulerAngleYZX(
		glm::radians(aAng.x),
		glm::radians(aAng.y),
		glm::radians(aAng.z)
	);

	return matrix;

	// invModel = glm::inverse( matrix );
	// 
	// transInvModel = glm::transpose(invModel);
	// setChangedModel(false);
	// return model;
#endif
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


