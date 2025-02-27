#include "transform.h"


// do base rotation to get Z up
static glm::mat4 gViewMatrixZ = glm::rotate( glm::radians( -90.f ), vec_forward );

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


// from vkquake
glm::vec3 Util_VectorToAngles( const glm::vec3& forward )
{
	glm::vec3 angles;

	if ( forward.x == 0.f && forward.y == 0.f )
	{
		// either vertically up or down
		angles[ PITCH ] = ( forward.z > 0 ) ? -90 : 90;
		angles[ YAW ]   = 0;
		angles[ ROLL ]  = 0;
	}
	else
	{
		angles[ PITCH ] = -atan2( forward.z, sqrt( glm::dot( forward, forward ) ) );
		angles[ YAW ]   = atan2( forward.y, forward.x );
		angles[ ROLL ]  = 0;

		angles          = glm::degrees( angles );
	}

	return angles;
}


glm::vec3 Util_VectorToAngles( const glm::vec3& forward, const glm::vec3& up )
{
	glm::vec3 angles;

	if ( forward.x == 0 && forward.y == 0 )
	{
		// either vertically up or down
		if ( forward[ 2 ] > 0 )
		{
			angles[ PITCH ] = -90;
			angles[ YAW ]   = glm::degrees( ::atan2( -up[ 1 ], -up[ 0 ] ) );
		}
		else
		{
			angles[ PITCH ] = 90;
			angles[ YAW ]   = glm::degrees( atan2( up[ 1 ], up[ 0 ] ) );
		}
		angles[ ROLL ] = 0;
	}
	else
	{
		angles[ PITCH ] = -atan2( forward.z, sqrt( glm::dot( forward, forward ) ) );
		angles[ YAW ]   = atan2( forward.y, forward.x );

		float     cp = cos( angles[ PITCH ] ), sp = sin( angles[ PITCH ] );
		float     cy = cos( angles[ YAW ] ), sy = sin( angles[ YAW ] );
		glm::vec3 tleft, tup;
		tleft.x        = -sy;
		tleft.y        = cy;
		tleft.z        = 0;
		tup.x          = sp * cy;
		tup.y          = sp * sy;
		tup.z          = cp;
		angles[ ROLL ] = -atan2( glm::dot( up, tleft ), glm::dot( up, tup ) );

		angles         = glm::degrees( angles );
	}

	return angles;
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


glm::quat Util_GetMatrixRotation( const glm::mat4& mat )
{
	glm::vec3 scale = {};
	for ( int i = 0; i < 3; i++ )
		scale[ i ] = glm::length( glm::vec3( mat[ i ] ) );

	const glm::mat3 rotMtx(
		glm::vec3( mat[ 0 ] ) / scale[ 0 ],
		glm::vec3( mat[ 1 ] ) / scale[ 1 ],
		glm::vec3( mat[ 2 ] ) / scale[ 2 ] );

	return glm::quat_cast( rotMtx );
}


glm::vec3 Util_GetMatrixAngles( const glm::mat4& mat )
{
	glm::vec3 scale = {};
	for ( int i = 0; i < 3; i++ )
		scale[ i ] = glm::length( glm::vec3( mat[ i ] ) );

	const glm::mat3 rotMtx(
		glm::vec3( mat[ 0 ] ) / scale[ 0 ],
		glm::vec3( mat[ 1 ] ) / scale[ 1 ],
		glm::vec3( mat[ 2 ] ) / scale[ 2 ] );

	return Util_ToEulerAngles( glm::quat_cast( rotMtx ) );
}


void Util_GetMatrixDirection( const glm::mat4& mat, glm::vec3* forward, glm::vec3* right, glm::vec3* up )
{
	glm::vec3 scale = Util_GetMatrixScale( mat );

	glm::quat quat  = mat / glm::scale( scale );

	if ( forward )
		*forward = ( quat * vec_forward );

	if ( right )
		*right = ( quat * vec_right );

	if ( up )
		*up = ( quat * vec_up );
}


void Util_GetMatrixDirectionNoScale( const glm::mat4& mat, glm::vec3* forward, glm::vec3* right, glm::vec3* up )
{
	glm::quat quat = mat;

	if ( forward )
		*forward = ( quat * vec_forward );

	if ( right )
		*right = ( quat * vec_right );

	if ( up )
		*up = ( quat * vec_up );
}


void Util_GetViewMatrixZDirection( const glm::mat4& mat, glm::vec3& forward, glm::vec3& right, glm::vec3& up )
{
	forward = -glm::normalize( glm::vec3( mat[ 0 ][ 2 ], mat[ 1 ][ 2 ], mat[ 2 ][ 2 ] ) );
	right   = glm::normalize( glm::vec3( mat[ 0 ][ 0 ], mat[ 1 ][ 0 ], mat[ 2 ][ 0 ] ) );
	up      = glm::normalize( glm::vec3( mat[ 0 ][ 1 ], mat[ 1 ][ 1 ], mat[ 2 ][ 1 ] ) );
}


void Util_ToMatrix( glm::mat4& srMatrix, const glm::vec3* spPos, const glm::quat* spRot, const glm::vec3* spScale )
{
	if ( spPos )
		srMatrix = glm::translate( *spPos );
	else
		srMatrix = glm::identity< glm::mat4 >();

	if ( spScale )
		srMatrix = glm::scale( srMatrix, *spScale );

	if ( spRot )
		srMatrix *= glm::toMat4( *spRot );
}


void Util_ToMatrix( glm::mat4& srMatrix, const glm::vec3* spPos, const glm::vec3* spAng, const glm::vec3* spScale )
{
	if ( spPos )
		srMatrix = glm::translate( *spPos );
	else
		srMatrix = glm::identity< glm::mat4 >();

	if ( spScale )
		srMatrix = glm::scale( srMatrix, *spScale );
	
	// if ( spAng )
	// 	srMatrix *= glm::eulerAngleYZX(
	// 	  glm::radians( spAng->x ),
	// 	  glm::radians( spAng->y ),
	// 	  glm::radians( spAng->z ) );

	if ( spAng )
		srMatrix *= glm::eulerAngleZYX(
			glm::radians( spAng->z ),
			glm::radians( spAng->y ),
			glm::radians( spAng->x ) );
}


void Util_ToMatrix( glm::mat4& srMatrix, const glm::vec3& srPos, const glm::vec3& srAng )
{
	Util_ToMatrix( srMatrix, &srPos, &srAng );
}


void Util_ToMatrix( glm::mat4& srMatrix, const glm::vec3& srPos, const glm::vec3& srAng, const glm::vec3& srScale )
{
	Util_ToMatrix( srMatrix, &srPos, &srAng, &srScale );
}


void Util_ToViewMatrixY( glm::mat4& srViewMatrix, const glm::quat& srRot )
{
	srViewMatrix = glm::toMat4( srRot );
}


void Util_ToViewMatrixY( glm::mat4& srViewMatrix, const glm::vec3& srPos, const glm::quat& srRot )
{
	Util_ToViewMatrixY( srViewMatrix, srRot );
	srViewMatrix = glm::translate( srViewMatrix, -srPos );
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
	srViewMatrix = glm::rotate( srViewMatrix, glm::radians( srAng[ PITCH ] ), VIEWMAT_ANG( PITCH ) );

	/* Z Rotation - ROLL */
	srViewMatrix = glm::rotate( srViewMatrix, glm::radians( srAng[ ROLL ] ), VIEWMAT_ANG( ROLL ) );
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
	srViewMatrix = glm::rotate( srViewMatrix, glm::radians( srAng[ YAW ] ), VIEWMAT_ANG( YAW ) );

	/* X Rotation - PITCH (Mouse Y) */
	srViewMatrix = glm::rotate( srViewMatrix, glm::radians( srAng[ PITCH ] ), VIEWMAT_ANG( PITCH ) );

	/* Z Rotation - ROLL */
	srViewMatrix = glm::rotate( srViewMatrix, glm::radians( srAng[ ROLL ] ), VIEWMAT_ANG( ROLL ) );
}


void Util_ToViewMatrixZ( glm::mat4& srViewMatrix, const glm::vec3& srPos, const glm::vec3& srAng )
{
	Util_ToViewMatrixZ( srViewMatrix, srAng );
	srViewMatrix = glm::translate( srViewMatrix, -srPos );
}


void Util_ToViewMatrixZ( glm::mat4& srViewMatrix, const glm::quat& srRot )
{
	srViewMatrix = glm::toMat4( srRot );
}


void Util_ToViewMatrixZ( glm::mat4& srViewMatrix, const glm::vec3& srPos, const glm::quat& srRot )
{
	srViewMatrix = gViewMatrixZ;
	Util_ToViewMatrixZ( srViewMatrix, srRot );
	srViewMatrix = glm::translate( srViewMatrix, -srPos );
}


glm::quat Util_BuildRotateQuaternion( glm::vec3 sAxis, float sAngle )
{
	float     angleRad = glm::radians( sAngle );
	glm::vec3 axisNorm = glm::normalize( sAxis );

	float     w        = glm::cos( angleRad / 2 );
	float     v        = glm::sin( angleRad / 2 );
	glm::vec3 qv       = axisNorm * v;

	glm::quat quaternion( w, qv );

	return quaternion;
}


glm::quat Util_RotateQuaternion( glm::quat sQuat, glm::vec3 sAxis, float sAngle )
{
	return sQuat * Util_BuildRotateQuaternion( sAxis, sAngle );
}


glm::quat Util_RotateQuaternion( glm::quat sQuat, glm::vec3 sAngle )
{
	glm::quat out = sQuat;

	out *= Util_RotateQuaternion( out, vec_forward, sAngle.x );
	out *= Util_RotateQuaternion( out, vec_right, sAngle.y );
	out *= Util_RotateQuaternion( out, vec_up, sAngle.z );

	return out;
}


glm::vec4 Util_BuildPlane( glm::vec3 point, glm::vec3 normal )
{
	glm::vec4 res;
	normal = glm::normalize( normal );
	res.w  = glm::dot( normal, point );
	res.x  = normal.x;
	res.y  = normal.y;
	res.z  = normal.z;
	return res;
}


float Util_RayPlaneIntersection( Ray ray, glm::vec4 plane )
{
	glm::vec3 plane3 = { plane.x, plane.y, plane.z };
	float numer      = glm::dot( plane3, ray.origin ) - plane.w;
	float denom      = glm::dot( plane3, ray.dir );

	// Check if the ray direction is nearly parallel to the plane (denominator is close to zero)
	// normal is orthogonal to vector, cant intersect
	if ( fabsf( denom ) < FLT_EPSILON ) 
	{
		// Ray is parallel to plane, no intersection
		return -1.0f;
	}

	return -( numer / denom );
}


// https://tavianator.com/2011/ray_box.html
// https://tavianator.com/2015/ray_box_nan.html
// https://tavianator.com/2022/ray_box_boundary.html
bool Util_RayIntersectsWithAABB( Ray ray, AABB aabb )
{
	double tMin = -INFINITY, tMax = INFINITY;

	for ( int i = 0; i < 3; i++ )
	{
		// Compute the intersection points of the ray with the planes of the AABB along the current axis
		double t1 = ( aabb.min[ i ] - ray.origin[ i ] ) / ray.dir[ i ];
		double t2 = ( aabb.max[ i ] - ray.origin[ i ] ) / ray.dir[ i ];

		// Keep track of the furthest entry point and closest exit point of the ray
		tMin      = glm::max( tMin, glm::min( t1, t2 ) );
		tMax      = glm::min( tMax, glm::max( t1, t2 ) );
	}

	return tMax > glm::max( tMin, 0.0 );
}


bool Util_RayInteresectsWithAABBs( glm::vec3& outIntersectionPoint, u32& outIndex, Ray ray, const AABB* aabbList, u32 count )
{
	double tMin        = -INFINITY;  // Initialize tMin to negative infinity
	double tMax        = INFINITY;   // Initialize tMax to positive infinity

	bool   intersected = false;

	for ( size_t i = 0; i < count; ++i )
	{
		const AABB& aabb      = aabbList[ i ];

		double      localTMin = -INFINITY;
		double      localTMax = INFINITY;

		for ( int j = 0; j < 3; j++ )
		{
			// Compute the intersection points of the ray with the planes of the AABB along the current axis
			double t1 = ( aabb.min[ j ] - ray.origin[ j ] ) / ray.dir[ j ];
			double t2 = ( aabb.max[ j ] - ray.origin[ j ] ) / ray.dir[ j ];

			// Keep track of the furthest entry point and closest exit point of the ray
			localTMin = glm::max( localTMin, glm::min( t1, t2 ) );
			localTMax = glm::min( localTMax, glm::max( t1, t2 ) );
		}

		// Check if the closest exit point of the ray is beyond the furthest entry point for this AABB
		if ( localTMax > fmax( localTMin, 0.0 ) )
		{
			// Is this AABB is the first intersection or closer than the previously found intersection?
			if ( !intersected || localTMin < tMin )
			{
				// Update the intersection point and AABB index
				outIntersectionPoint.x = ray.origin.x + localTMin * ray.dir.x;
				outIntersectionPoint.y = ray.origin.y + localTMin * ray.dir.y;
				outIntersectionPoint.z = ray.origin.z + localTMin * ray.dir.z;
				outIndex               = i;
				intersected            = true;
				tMin                   = localTMin;  // Update tMin for the closest intersection
			}
		}
	}

	return intersected;
}


// https://stackoverflow.com/a/56348846
glm::vec3 Util_GetRayFromScreenSpace( glm::ivec2 mousePos, glm::mat4 projViewMat, glm::vec2 resolution )
{
	glm::mat4 invProjView = glm::inverse( projViewMat );

	float     halfWidth   = resolution.x / 2.f;
	float     halfHeight  = resolution.y / 2.f;

	// Calculate the screen space coordinates of the near and far planes based on mouse position
	glm::vec4 near( ( mousePos.x - halfWidth ) / halfWidth, -1 * ( mousePos.y - halfHeight ) / halfHeight, -1, 1.0 );
	glm::vec4 far( ( mousePos.x - halfWidth ) / halfWidth, -1 * ( mousePos.y - halfHeight ) / halfHeight, 1, 1.0 );

	// Transform these coordinates into world space
	glm::vec4 nearResult = invProjView * near;
	glm::vec4 farResult  = invProjView * far;

	// DISARRANGE DISORDER DISRUPT DISTRUB DEPART DISORGANIZE JUMBLE BREAK UPSET UNSETTLE the result
	nearResult /= nearResult.w;
	farResult /= farResult.w;

	// Return final ray direction, normalizing it so it's a unit vector (vector with a length/magnitude of 1)
	return glm::normalize( farResult - nearResult );
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

