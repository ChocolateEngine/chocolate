#include "transform.h"


constexpr float g_rot_z = glm::radians( -90.f );

// do base rotation to get Z up
static glm::mat4 gViewMatrixZ = glm::rotate( g_rot_z, vec_forward );

#define VIEWMAT_ANG( axis ) glm::vec3(viewMatrix[0][axis], viewMatrix[1][axis], viewMatrix[2][axis])


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
	if ( useScale && (aScale.x != 1.0 && aScale.y != 1.0 && aScale.z != 1.0) )
		matrix = glm::scale( matrix, aScale );

	matrix *= glm::eulerAngleYZX(
		glm::radians(aAng.x),
		glm::radians(aAng.y),
		glm::radians(aAng.z)
	);

	return matrix;
#endif
}


/* Y Up version of the ViewMatrix */
glm::mat4 Transform::ToViewMatrixY() const
{
	glm::mat4 viewMatrix(1.0f);

	/* Y Rotation - YAW (Mouse X for Y up) */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[YAW]), vec_right );

	/* X Rotation - PITCH (Mouse Y) */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[PITCH]), VIEWMAT_ANG(0) );

	/* Z Rotation - ROLL */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[ROLL]), VIEWMAT_ANG(2) );

	return glm::translate( viewMatrix, -aPos );
}


/* Z Up version of the view matrix */
glm::mat4 Transform::ToViewMatrixZ() const
{
	glm::mat4 viewMatrix = gViewMatrixZ;

	/* Y Rotation - YAW */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[YAW]), VIEWMAT_ANG(1) );

	/* X Rotation - PITCH (Mouse Y) */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[PITCH]), VIEWMAT_ANG(0) );

	/* Z Rotation - ROLL (Mouse X for Z up)*/
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[ROLL]), VIEWMAT_ANG(2) );

	return glm::translate( viewMatrix, -aPos );
}


// =================================================================


glm::mat4 TransformSmall::ToMatrix() const
{
	glm::mat4 matrix = glm::translate( aPos );

	matrix *= glm::rotate( glm::radians(aAng[YAW]), vec_up );
	matrix *= glm::rotate( glm::radians(aAng[PITCH]), vec_right );
	matrix *= glm::rotate( glm::radians(aAng[ROLL]), vec_forward );

	return matrix;
}


/* Y Up version of the ViewMatrix */
glm::mat4 TransformSmall::ToViewMatrixY() const
{
	glm::mat4 viewMatrix(1.0f);

	/* Y Rotation - YAW (Mouse X for Y up) */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[YAW]), vec_right );

	/* X Rotation - PITCH (Mouse Y) */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[PITCH]), VIEWMAT_ANG(0) );

	/* Z Rotation - ROLL */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[ROLL]), VIEWMAT_ANG(2) );

	return glm::translate( viewMatrix, -aPos );
}


/* Z Up version of the view matrix */
glm::mat4 TransformSmall::ToViewMatrixZ() const
{
	glm::mat4 viewMatrix = gViewMatrixZ;

	/* Y Rotation - YAW */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[YAW]), VIEWMAT_ANG(1) );

	/* X Rotation - PITCH (Mouse Y) */
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[PITCH]), VIEWMAT_ANG(0) );

	/* Z Rotation - ROLL (Mouse X for Z up)*/
	viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[ROLL]), VIEWMAT_ANG(2) );

	return glm::translate( viewMatrix, -aPos );
}


// =================================================================


void ToMatrix( glm::mat4& matrix, const glm::vec3& pos, const glm::vec3& ang )
{
	matrix = glm::translate( pos );

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
	// if ( useScale && (aScale.x != 1.0 && aScale.y != 1.0 && aScale.z != 1.0) )
	// 	matrix = glm::scale( matrix, aScale );

	matrix *= glm::eulerAngleYZX(
		glm::radians(ang.x),
		glm::radians(ang.y),
		glm::radians(ang.z)
	);
#endif
}


#undef VIEWMAT_ANG

