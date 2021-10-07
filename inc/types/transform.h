#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/euler_angles.hpp>


constexpr int PITCH = 0;  // up / down
constexpr int YAW = 1;    // left / right
constexpr int ROLL = 2;   // fall over on your side and start crying about it https://cdn.discordapp.com/attachments/282679155345195018/895458645763031061/walterdeath.gif


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


struct Transform
{
	glm::vec3 aPos = {};
	glm::vec3 aAng = {};
	glm::vec3 aScale = { 1.0f, 1.0f, 1.0f };

	inline glm::mat4 ToMatrix( bool useScale = true ) const
	{
		glm::mat4 translation = glm::translate( aPos );
		glm::mat4 rotation = PitchYawRoll( aAng );

		if ( useScale )
			return translation * rotation * glm::scale( aScale );
		else
			return translation * rotation;
	}

	#define VIEWMAT_ANG( axis ) glm::vec3(viewMatrix[0][axis], viewMatrix[1][axis], viewMatrix[2][axis])
	
	/* Y Up version of the ViewMatrix */
	inline glm::mat4 ToViewMatrixY(  ) const
	{
		glm::mat4 viewMatrix(1.0f);

		/* Y Rotation - YAW (Mouse X for Y up) */
		viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[YAW]), glm::vec3(0, 1, 0) );

		/* X Rotation - PITCH (Mouse Y) */
		viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[PITCH]), VIEWMAT_ANG(0) );

		/* Z Rotation - ROLL */
		viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[ROLL]), VIEWMAT_ANG(2) );

		return glm::translate( viewMatrix, -aPos );
	}

	/* Z Up version of the view matrix */
	inline glm::mat4 ToViewMatrixZ(  ) const
	{
		// do base rotation to get Z up
		glm::mat4 viewMatrix = glm::rotate( glm::radians(-90.f), glm::vec3(1, 0, 0) );

		/* Y Rotation - YAW */
		viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[YAW]), VIEWMAT_ANG(1) );

		/* X Rotation - PITCH (Mouse Y) */
		viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[PITCH]), VIEWMAT_ANG(0) );

		/* Z Rotation - ROLL (Mouse X for Z up)*/
		viewMatrix = glm::rotate( viewMatrix, glm::radians(aAng[ROLL]), VIEWMAT_ANG(2) );

		return glm::translate( viewMatrix, -aPos );
	}

	#undef VIEWMAT_ANG
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

