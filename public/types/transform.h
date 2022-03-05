#pragma once

#include <glm/glm.hpp>
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


#define VIEWMAT_ANG( axis ) glm::vec3(viewMatrix[0][axis], viewMatrix[1][axis], viewMatrix[2][axis])

constexpr glm::vec3 vec_up( 0, 0, 1 );
constexpr glm::vec3 vec_right( 0, 1, 0 );
constexpr glm::vec3 vec_forward( 1, 0, 0 );

constexpr float g_rot_z = glm::radians( -90.f );

// do base rotation to get Z up
static glm::mat4 gViewMatrixZ = glm::rotate( g_rot_z, vec_forward );


// only 24 bytes, use this when you don't need scaling
struct TransformSmall
{
	glm::vec3 aPos = {};
	glm::vec3 aAng = {};  // if only i could use float16

	inline glm::mat4 ToMatrix(  ) const
	{
		glm::mat4 matrix = glm::translate( aPos );

		matrix *= glm::rotate( glm::radians(aAng[YAW]), vec_up );
		matrix *= glm::rotate( glm::radians(aAng[PITCH]), vec_right );
		matrix *= glm::rotate( glm::radians(aAng[ROLL]), vec_forward );

		return matrix;
	}

	/* Y Up version of the ViewMatrix */
	inline glm::mat4 ToViewMatrixY(  ) const
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
	inline glm::mat4 ToViewMatrixZ(  ) const
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

};


// takes up 36 bytes !!!!!
struct Transform
{
	glm::vec3 aPos = {};
	glm::vec3 aAng = {};
	glm::vec3 aScale = { 1.0f, 1.0f, 1.0f };

	inline glm::mat4 ToMatrix( bool useScale = true ) const
	{
		glm::mat4 matrix = glm::translate( aPos );

		matrix *= glm::rotate( glm::radians(aAng[YAW]), vec_up );
		matrix *= glm::rotate( glm::radians(aAng[PITCH]), vec_right );
		matrix *= glm::rotate( glm::radians(aAng[ROLL]), vec_forward );

		if ( !useScale )
			return matrix;

		if ( aScale.x == 1.0 && aScale.y == 1.0 && aScale.z == 1.0 )
			return matrix;

		return matrix * glm::scale( aScale );
	}

	/* Y Up version of the ViewMatrix */
	inline glm::mat4 ToViewMatrixY(  ) const
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
	inline glm::mat4 ToViewMatrixZ(  ) const
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
};

#undef VIEWMAT_ANG

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


void DecomposeMatrix( const glm::mat4& m, glm::vec3& pos, glm::quat& rot, glm::vec3& scale );
void DecomposeMatrix( const glm::mat4& m, glm::vec3& pos, glm::quat& rot );


// does this apply to the viewMatrix only?
inline void GetDirectionVectors( const glm::mat4& mat, glm::vec3& forward, glm::vec3& right, glm::vec3& up )
{
	forward = -glm::normalize( glm::vec3(mat[0][2], mat[1][2], mat[2][2]) );
	right   =  glm::normalize( glm::vec3(mat[0][0], mat[1][0], mat[2][0]) );
	up      =  glm::normalize( glm::vec3(mat[0][1], mat[1][1], mat[2][1]) );
}

inline void GetDirectionVectors( const glm::mat4& mat, glm::vec3& forward, glm::vec3& right )
{
	forward = -glm::normalize( glm::vec3(mat[0][2], mat[1][2], mat[2][2]) );
	right   =  glm::normalize( glm::vec3(mat[0][0], mat[1][0], mat[2][0]) );
}

inline void GetDirectionVectors( const glm::mat4& mat, glm::vec3& forward )
{
	forward = -glm::normalize( glm::vec3(mat[0][2], mat[1][2], mat[2][2]) );
}


inline glm::mat4 ToOrbitalCameraTransformation( Transform &transform )
{
	return transform.ToMatrix(  );
}

