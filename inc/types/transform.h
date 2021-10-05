#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>


constexpr int PITCH = 0;
constexpr int YAW = 1;
constexpr int ROLL = 2;

constexpr int WORLD_UP = 1;  // y axis for now


struct Transform
{
	glm::mat4 ToMatrix( bool useScale = true ) const;
	glm::mat4 ToViewMatrix(  ) const;

	glm::vec3 aPos = {};
	glm::vec3 aAng = {};
	glm::vec3 aScale = { 1.0f, 1.0f, 1.0f };
};


void DecomposeMatrix( const glm::mat4& m, glm::vec3& pos, glm::quat& rot, glm::vec3& scale );
void DecomposeMatrix( const glm::mat4& m, glm::vec3& pos, glm::quat& rot );


/* !!! USE THIS INSTEAD OF glm::yawPitchRoll() SO IT FITS WITH HOW THE ENGINE WORKS ATM !!! */
inline glm::mat4 PitchYawRoll( const glm::vec3& ang )
{
	return glm::yawPitchRoll( ang[YAW], ang[PITCH], ang[ROLL] );
}

inline glm::mat4 AngToMat4( const glm::vec3& ang )
{
	return glm::yawPitchRoll( ang[YAW], ang[PITCH], ang[ROLL] );
	// return glm::toMat4( glm::toQuat( glm::orientate3(ang) ) );
}

inline glm::quat AngToQuat( const glm::vec3& ang )
{
	return glm::toQuat( glm::yawPitchRoll( ang[YAW], ang[PITCH], ang[ROLL] ) );
	// return glm::toQuat( glm::orientate3(ang) );
}


// does this apply to the viewMatrix only?
inline void GetDirectionVectors( const glm::mat4& mat, glm::vec3& forward, glm::vec3& up, glm::vec3& right )
{
	forward = -glm::normalize( glm::vec3(mat[0][2], mat[1][2], mat[2][2]) );
	up      =  glm::normalize( glm::vec3(mat[0][1], mat[1][1], mat[2][1]) );
	right   =  glm::normalize( glm::vec3(mat[0][0], mat[1][0], mat[2][0]) );
}


inline glm::mat4 ToOrbitalCameraTransformation( const Transform &transform )
{
	return transform.ToMatrix(  );
}

