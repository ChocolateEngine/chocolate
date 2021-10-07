#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>


constexpr int PITCH = 0;  // up / down
constexpr int YAW = 1;    // left / right
constexpr int ROLL = 2;   // fall over on your side and start crying about it https://cdn.discordapp.com/attachments/282679155345195018/895458645763031061/walterdeath.gif


struct Transform
{
	glm::mat4 ToMatrix( bool useScale = true ) const;
	glm::mat4 ToViewMatrixY(  ) const;
	glm::mat4 ToViewMatrixZ(  ) const;

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
}

inline glm::quat AngToQuat( const glm::vec3& ang )
{
	return glm::toQuat( glm::yawPitchRoll( ang[YAW], ang[PITCH], ang[ROLL] ) );
}


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


inline glm::mat4 ToOrbitalCameraTransformation( const Transform &transform )
{
	return transform.ToMatrix(  );
}

