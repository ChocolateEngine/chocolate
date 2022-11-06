#pragma once

// #include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>


constexpr int PITCH = 0;  // up / down
constexpr int YAW = 1;    // left / right
constexpr int ROLL = 2;   // fall over on your side and start crying about it https://cdn.discordapp.com/attachments/282679155345195018/895458645763031061/walterdeath.gif


// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
#ifndef M_PI
#define M_PI    3.14159265358979323846264338327950288   /**< pi */
#endif

inline glm::vec3 ToEulerAngles( const glm::quat& q )
{
	glm::vec3 angles;

	// roll (x-axis rotation)
	double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
	double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
	angles[ROLL] = std::atan2( sinr_cosp, cosr_cosp );

	// pitch (y-axis rotation)
	double sinp = 2 * (q.w * q.y - q.z * q.x);
	if ( std::abs( sinp ) >= 1 )
		angles[PITCH] = std::copysign( M_PI / 2, sinp ); // use 90 degrees if out of range
	else
		angles[PITCH] = std::asin( sinp );

	// yaw (z-axis rotation)
	double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
	double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
	angles[YAW] = std::atan2( siny_cosp, cosy_cosp );

	return angles;
}


/* !!! USE THIS INSTEAD OF glm::yawPitchRoll() SO IT FITS WITH HOW THE ENGINE WORKS ATM !!! */
inline glm::mat4 PitchYawRoll( const glm::vec3& ang )
{
	return glm::yawPitchRoll( ang[YAW], ang[PITCH], ang[ROLL] );
}

inline glm::mat4 AngToMat4( const glm::vec3& ang )
{
	return glm::yawPitchRoll( ang[YAW], ang[PITCH], ang[ROLL] );
}

inline glm::quat AngToQuat( const glm::vec3& ang )
{
	return glm::toQuat( glm::yawPitchRoll( ang[YAW], ang[PITCH], ang[ROLL] ) );
}

inline glm::vec3 QuatToAng( const glm::quat& quat )
{
	return ToEulerAngles( quat );
}


constexpr glm::vec3 vec_up( 0, 0, 1 );
constexpr glm::vec3 vec_right( 0, 1, 0 );
constexpr glm::vec3 vec_forward( 1, 0, 0 );


// only 24 bytes, use this when you don't need scaling
struct TransformSmall
{
	glm::vec3 aPos = {};
	glm::vec3 aAng = {};  // if only i could use float16

	glm::mat4 ToMatrix() const;

	/* Y Up version of the ViewMatrix */
	glm::mat4 ToViewMatrixY() const;

	/* Z Up version of the view matrix */
	glm::mat4 ToViewMatrixZ() const;
};


// takes up 36 bytes !!!!!
struct Transform
{
	glm::vec3 aPos = {};
	glm::vec3 aAng = {};
	glm::vec3 aScale = { 1.0f, 1.0f, 1.0f };

	glm::mat4 ToMatrix( bool useScale = true ) const;

	/* Y Up version of the ViewMatrix */
	glm::mat4 ToViewMatrixY() const;

	/* Z Up version of the view matrix */
	glm::mat4 ToViewMatrixZ() const;
};


struct Transform2D
{
	glm::vec2 aPos = {};
	float     aAng = 0.f;
	glm::vec2 aScale = { 1.0f, 1.0f };
};

struct Transform2DSmall
{
	glm::vec2 aPos = {};
	float     aAng = 0.f;
};


void ToMatrix( glm::mat4& matrix, const glm::vec3& pos, const glm::vec3& ang );


inline void Util_GetDirectionVectors( const glm::vec3& srAngles, glm::vec3* spForward, glm::vec3* spRight = nullptr, glm::vec3* spUp = nullptr )
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


// does this apply to the viewMatrix only?
inline void Util_GetViewMatrixZDirection( const glm::mat4& mat, glm::vec3& forward, glm::vec3& right, glm::vec3& up )
{
	forward = -glm::normalize( glm::vec3(mat[0][2], mat[1][2], mat[2][2]) );
	right   =  glm::normalize( glm::vec3(mat[0][0], mat[1][0], mat[2][0]) );
	up      =  glm::normalize( glm::vec3(mat[0][1], mat[1][1], mat[2][1]) );
}

// inline void GetDirectionVectors( const glm::mat4& mat, glm::vec3& forward, glm::vec3& right )
// {
// 	forward = -glm::normalize( glm::vec3(mat[0][2], mat[1][2], mat[2][2]) );
// 	right   =  glm::normalize( glm::vec3(mat[0][0], mat[1][0], mat[2][0]) );
// }
// 
// inline void GetDirectionVectors( const glm::mat4& mat, glm::vec3& forward )
// {
// 	forward = -glm::normalize( glm::vec3(mat[0][2], mat[1][2], mat[2][2]) );
// }

inline glm::vec3 Util_GetMatrixScale( const glm::mat4& mat )
{
	glm::vec3 scale{};

	scale.x = sqrt( mat[ 0 ][ 0 ] * mat[ 0 ][ 0 ] + mat[ 0 ][ 1 ] * mat[ 0 ][ 1 ] + mat[ 0 ][ 2 ] * mat[ 0 ][ 2 ] );
	scale.y = sqrt( mat[ 1 ][ 0 ] * mat[ 1 ][ 0 ] + mat[ 1 ][ 1 ] * mat[ 1 ][ 1 ] + mat[ 1 ][ 2 ] * mat[ 1 ][ 2 ] );
	scale.z = sqrt( mat[ 2 ][ 0 ] * mat[ 2 ][ 0 ] + mat[ 2 ][ 1 ] * mat[ 2 ][ 1 ] + mat[ 2 ][ 2 ] * mat[ 2 ][ 2 ] );

	return scale;
}

inline void Util_GetMatrixDirection( const glm::mat4& mat, glm::vec3* forward, glm::vec3* right = nullptr, glm::vec3* up = nullptr )
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

inline glm::mat4 ToOrbitalCameraTransformation( Transform &transform )
{
	return transform.ToMatrix(  );
}

