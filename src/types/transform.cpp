#include "../../inc/types/transform.h"


glm::mat4 Transform::ToMatrix( bool useScale ) const
{
	glm::mat4 translation = glm::translate( aPos );
	glm::mat4 rotation = PitchYawRoll( aAng );

	if ( useScale )
		return translation * rotation * glm::scale( aScale );
	else
		return translation * rotation;
}


glm::mat4 Transform::ToViewMatrix(  ) const
{
	glm::mat4 viewMatrix(1.0f);

	/* Y Rotation - YAW (Mouse X) */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[YAW]), glm::vec3(0, 1, 0) );

	/* X Rotation - PITCH (Mouse Y) */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[PITCH]), glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]) );

	/* Z Rotation - ROLL */
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


