#include "transform.h"


constexpr float g_rot_z = glm::radians( -90.f );

// do base rotation to get Z up
static glm::mat4 gViewMatrixZ = glm::rotate( g_rot_z, vec_forward );

#define VIEWMAT_ANG( axis ) glm::vec3( srViewMatrix[ 0 ][ axis ], srViewMatrix[ 1 ][ axis ], srViewMatrix[ 2 ][ axis ] )

// =================================================================


glm::vec3 Util_ToEulerAngles( const glm::quat& q )
{
	glm::vec3 angles;

	// roll (x-axis rotation)
	double    sinr_cosp = 2 * ( q.w * q.x + q.y * q.z );
	double    cosr_cosp = 1 - 2 * ( q.x * q.x + q.y * q.y );
	angles[ ROLL ]      = std::atan2( sinr_cosp, cosr_cosp );

	// pitch (y-axis rotation)
	double sinp         = 2 * ( q.w * q.y - q.z * q.x );
	if ( std::abs( sinp ) >= 1 )
		angles[ PITCH ] = std::copysign( M_PI / 2, sinp );  // use 90 degrees if out of range
	else
		angles[ PITCH ] = std::asin( sinp );

	// yaw (z-axis rotation)
	double siny_cosp = 2 * ( q.w * q.z + q.x * q.y );
	double cosy_cosp = 1 - 2 * ( q.y * q.y + q.z * q.z );
	angles[ YAW ]    = std::atan2( siny_cosp, cosy_cosp );

	return angles;
}


void Util_GetDirectionVectors( const glm::vec3& srAngles, glm::vec3* spForward, glm::vec3* spRight, glm::vec3* spUp )
{
	glm::vec3 rad = glm::radians( srAngles );

	float     sy  = sin( rad[ YAW ] );
	float     cy  = cos( rad[ YAW ] );

	float     sp  = sin( rad[ PITCH ] );
	float     cp  = cos( rad[ PITCH ] );

	float     sr  = sin( rad[ ROLL ] );
	float     cr  = cos( rad[ ROLL ] );

	if ( spForward )
	{
		spForward->x = cp * cy;
		spForward->y = cp * sy;
		spForward->z = -sp;
	}

	if ( spRight )
	{
		spRight->x = ( -1 * sr * sp * cy + -1 * cr * -sy );
		spRight->y = ( -1 * sr * sp * sy + -1 * cr * cy );
		spRight->z = -1 * sr * cp;
	}

	if ( spUp )
	{
		spUp->x = ( cr * sp * cy + -sr * -sy );
		spUp->y = ( cr * sp * sy + -sr * cy );
		spUp->z = cr * cp;
	}
}


glm::vec3 Util_GetMatrixPosition( const glm::mat4& mat )
{
	return glm::vec3( mat[ 3 ][ 0 ], mat[ 3 ][ 1 ], mat[ 3 ][ 2 ] );
}


glm::vec3 Util_GetMatrixScale( const glm::mat4& mat )
{
	glm::vec3 scale{};

	scale.x = glm::dot( mat[ 0 ], mat[ 0 ] );
	scale.y = glm::dot( mat[ 1 ], mat[ 1 ] );
	scale.z = glm::dot( mat[ 2 ], mat[ 2 ] );

	return glm::sqrt( scale );
}


void Util_GetMatrixDirection( const glm::mat4& mat, glm::vec3* forward, glm::vec3* right, glm::vec3* up )
{
	glm::vec3 scale = Util_GetMatrixScale( mat );

	glm::quat quat  = mat / glm::scale( scale );

	if ( forward )
		*forward = ( quat * glm::vec3( 1, 0, 0 ) );

	if ( right )
		*right = ( quat * glm::vec3( 0, 1, 0 ) );

	if ( up )
		*up = ( quat * glm::vec3( 0, 0, 1 ) );
}


void Util_GetViewMatrixZDirection( const glm::mat4& mat, glm::vec3& forward, glm::vec3& right, glm::vec3& up )
{
	forward = -glm::normalize( glm::vec3( mat[ 0 ][ 2 ], mat[ 1 ][ 2 ], mat[ 2 ][ 2 ] ) );
	right   = glm::normalize( glm::vec3( mat[ 0 ][ 0 ], mat[ 1 ][ 0 ], mat[ 2 ][ 0 ] ) );
	up      = glm::normalize( glm::vec3( mat[ 0 ][ 1 ], mat[ 1 ][ 1 ], mat[ 2 ][ 1 ] ) );
}


void Util_ToMatrix( glm::mat4& srMatrix, const glm::vec3& srPos, const glm::vec3& srAng )
{
	srMatrix = glm::translate( srPos );

	srMatrix *= glm::eulerAngleYZX(
	  glm::radians( srAng.x ),
	  glm::radians( srAng.y ),
	  glm::radians( srAng.z ) );
}


void Util_ToMatrix( glm::mat4& srMatrix, const glm::vec3& srPos, const glm::vec3& srAng, const glm::vec3& srScale )
{
	srMatrix = glm::translate( srPos );

	srMatrix = glm::scale( srMatrix, srScale );

	srMatrix *= glm::eulerAngleYZX(
	  glm::radians( srAng.x ),
	  glm::radians( srAng.y ),
	  glm::radians( srAng.z ) );
}


void Util_ToMatrix( glm::mat4& srMatrix, glm::vec3* spPos, glm::vec3* spAng, glm::vec3* spScale )
{
	if ( spPos )
		srMatrix = glm::translate( *spPos );
	else
		srMatrix = glm::identity< glm::mat4 >();

	if ( spScale )
		srMatrix = glm::scale( srMatrix, *spScale );

	if ( spAng )
		srMatrix *= glm::eulerAngleYZX(
		  glm::radians( spAng->x ),
		  glm::radians( spAng->y ),
		  glm::radians( spAng->z ) );
}


void Util_ToViewMatrixY( glm::mat4& srViewMatrix, const glm::vec3& srAng )
{
#if 0
	viewMatrix = glm::eulerAngleYZX(
		glm::radians(ang[YAW]),
		glm::radians(ang[ROLL]),
		glm::radians(ang[PITCH])
	);
#else
	/* Y Rotation - YAW (Mouse X for Y up) */
	srViewMatrix = glm::rotate( glm::radians( srAng[ YAW ] ), vec_right );

	/* X Rotation - PITCH (Mouse Y) */
	srViewMatrix = glm::rotate( srViewMatrix, glm::radians( srAng[ PITCH ] ), VIEWMAT_ANG( 0 ) );

	/* Z Rotation - ROLL */
	srViewMatrix = glm::rotate( srViewMatrix, glm::radians( srAng[ ROLL ] ), VIEWMAT_ANG( 2 ) );
#endif
}


void Util_ToViewMatrixY( glm::mat4& srViewMatrix, const glm::vec3& srPos, const glm::vec3& srAng )
{
	Util_ToViewMatrixY( srViewMatrix, srAng );
	srViewMatrix = glm::translate( srViewMatrix, -srPos );
}


void Util_ToViewMatrixZ( glm::mat4& srViewMatrix, const glm::vec3& srAng )
{
	srViewMatrix = gViewMatrixZ;

	/* Y Rotation - YAW (Mouse X for Y up) */
	srViewMatrix = glm::rotate( srViewMatrix, glm::radians( srAng[ YAW ] ), VIEWMAT_ANG( 1 ) );

	/* X Rotation - PITCH (Mouse Y) */
	srViewMatrix = glm::rotate( srViewMatrix, glm::radians( srAng[ PITCH ] ), VIEWMAT_ANG( 0 ) );

	/* Z Rotation - ROLL */
	srViewMatrix = glm::rotate( srViewMatrix, glm::radians( srAng[ ROLL ] ), VIEWMAT_ANG( 2 ) );
}


void Util_ToViewMatrixZ( glm::mat4& srViewMatrix, const glm::vec3& srPos, const glm::vec3& srAng )
{
	Util_ToViewMatrixZ( srViewMatrix, srAng );
	srViewMatrix = glm::translate( srViewMatrix, -srPos );
}


// =================================================================


glm::mat4 Transform::ToMatrix( bool useScale ) const
{
	glm::mat4 matrix;

	if ( useScale && ( aScale.x != 1.0 && aScale.y != 1.0 && aScale.z != 1.0 ) )
		Util_ToMatrix( matrix, aPos, aAng, aScale );
	else
		Util_ToMatrix( matrix, aPos, aAng );

	return matrix;
}


/* Y Up version of the ViewMatrix */
glm::mat4 Transform::ToViewMatrixY() const
{
	glm::mat4 viewMatrix(1.0f);
	Util_ToViewMatrixY( viewMatrix, aPos, aAng );
	return viewMatrix;
}


/* Z Up version of the view matrix */
glm::mat4 Transform::ToViewMatrixZ() const
{
	glm::mat4 viewMatrix = gViewMatrixZ;
	Util_ToViewMatrixZ( viewMatrix, aPos, aAng );
	return viewMatrix;
}


// =================================================================


glm::mat4 TransformSmall::ToMatrix() const
{
	glm::mat4 matrix;
	Util_ToMatrix( matrix, aPos, aAng );
	return matrix;
}


/* Y Up version of the ViewMatrix */
glm::mat4 TransformSmall::ToViewMatrixY() const
{
	glm::mat4 viewMatrix( 1.0f );
	Util_ToViewMatrixY( viewMatrix, aPos, aAng );
	return viewMatrix;
}


/* Z Up version of the view matrix */
glm::mat4 TransformSmall::ToViewMatrixZ() const
{
	glm::mat4 viewMatrix = gViewMatrixZ;
	Util_ToViewMatrixZ( viewMatrix, aPos, aAng );
	return viewMatrix;
}


#undef VIEWMAT_ANG

