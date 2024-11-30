#pragma once

// #include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>


constexpr int PITCH = 0;  // up / down
constexpr int YAW = 1;    // left / right
constexpr int ROLL = 2;   // fall over on your side and start crying about it https://cdn.discordapp.com/attachments/282679155345195018/895458645763031061/walterdeath.gif


struct AABB
{
	glm::vec3 min;
	glm::vec3 max;
};


struct Ray
{
	glm::vec3 origin;  // Origin Point of Ray
	glm::vec3 dir;     // Direction Vector of the Ray
};


// https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
#ifndef M_PI
#define M_PI    3.14159265358979323846264338327950288   /**< pi */
#endif

glm::vec3        Util_ToEulerAngles( const glm::quat& q );

void             Util_GetDirectionVectors( const glm::vec3& srAngles, glm::vec3* spForward, glm::vec3* spRight = nullptr, glm::vec3* spUp = nullptr );
glm::vec3        Util_VectorToAngles( const glm::vec3& forward );
glm::vec3        Util_VectorToAngles( const glm::vec3& srForward, const glm::vec3& srUp );

glm::vec3        Util_GetMatrixPosition( const glm::mat4& mat );
glm::vec3        Util_GetMatrixScale( const glm::mat4& mat );
glm::quat        Util_GetMatrixRotation( const glm::mat4& mat );
glm::vec3        Util_GetMatrixAngles( const glm::mat4& mat );
void             Util_GetMatrixDirection( const glm::mat4& mat, glm::vec3* forward, glm::vec3* right = nullptr, glm::vec3* up = nullptr );
void             Util_GetMatrixDirectionNoScale( const glm::mat4& mat, glm::vec3* forward, glm::vec3* right = nullptr, glm::vec3* up = nullptr );
void             Util_GetViewMatrixZDirection( const glm::mat4& mat, glm::vec3& forward, glm::vec3& right, glm::vec3& up );

void             Util_ToMatrix( glm::mat4& srMatrix, const glm::vec3& srPos, const glm::vec3& srAng );
void             Util_ToMatrix( glm::mat4& srMatrix, const glm::vec3& srPos, const glm::vec3& srAng, const glm::vec3& srScale );
void             Util_ToMatrix( glm::mat4& srMatrix, const glm::vec3* spPos = nullptr, const glm::vec3* spAng = nullptr, const glm::vec3* spScale = nullptr );
void             Util_ToMatrix( glm::mat4& srMatrix, const glm::vec3* spPos = nullptr, const glm::quat* spRot = nullptr, const glm::vec3* spScale = nullptr );

void             Util_ToViewMatrixY( glm::mat4& srViewMatrix, const glm::quat& srRot );
void             Util_ToViewMatrixY( glm::mat4& srViewMatrix, const glm::vec3& srPos, const glm::quat& srRot );

void             Util_ToViewMatrixY( glm::mat4& srViewMatrix, const glm::vec3& srAng );
void             Util_ToViewMatrixY( glm::mat4& srViewMatrix, const glm::vec3& srPos, const glm::vec3& srAng );

void             Util_ToViewMatrixZ( glm::mat4& srViewMatrix, const glm::vec3& srAng );
void             Util_ToViewMatrixZ( glm::mat4& srViewMatrix, const glm::vec3& srPos, const glm::vec3& srAng );


// Helper Function to rotate quaternions even though it's easy, this is easy to remember and use
glm::quat        Util_BuildRotateQuaternion( glm::vec3 sAxis, float sAngle );
glm::quat        Util_RotateQuaternion( glm::quat sQuat, glm::vec3 sAxis, float sAngle );
glm::quat        Util_RotateQuaternion( glm::quat sQuat, glm::vec3 sAngle );

glm::vec4        Util_BuildPlane( glm::vec3 point, glm::vec3 normal );

float            Util_RayPlaneIntersection( Ray ray, glm::vec4 plane );

bool             Util_RayIntersectsWithAABB( Ray ray, AABB aabb );

// Returns true if the ray interesected with any AABBs
bool             Util_RayInteresectsWithAABBs( glm::vec3& outIntersectionPoint, u32& outIndex, Ray ray, const AABB* aabbList, u32 count );
glm::vec3        Util_GetRayFromScreenSpace( glm::ivec2 mousePos, glm::mat4 projViewMat, glm::vec2 resolution );


inline glm::mat4 Util_ToMatrix( const glm::vec3* spPos = nullptr, const glm::vec3* spAng = nullptr, const glm::vec3* spScale = nullptr )
{
	glm::mat4 matrix;
	Util_ToMatrix( matrix, spPos, spAng, spScale );
	return matrix;
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
	return Util_ToEulerAngles( quat );
}


constexpr glm::vec3 vec_up( 0, 0, 1 );
constexpr glm::vec3 vec_right( 0, 1, 0 );
constexpr glm::vec3 vec_forward( 1, 0, 0 );

constexpr glm::vec3 vec3_pitch( 0, 0, 1 );
constexpr glm::vec3 vec3_yaw( 0, 1, 0 );
constexpr glm::vec3 vec3_roll( 1, 0, 0 );


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

