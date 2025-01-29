#pragma once

#include <unordered_map>

#include "entity/entity.h"


enum EGameRuleType
{
	EGameRuleType_Invalid,

	EGameRuleType_Bool,
	EGameRuleType_Float,
	EGameRuleType_Double,

	EGameRuleType_Int,          // signed long long
	EGameRuleType_UnsignedInt,  // unsigned long long

	// EGameRuleType_Vec2,  // glm::vec2
	// EGameRuleType_Vec3,  // glm::vec3
	// EGameRuleType_Vec4,  // glm::vec4
	// EGameRuleType_Quat,  // glm::quat

	EGameRuleType_String,
	// EGameRuleType_Vector,  // ChVector of void*
};


// TODO: default values?
// struct GameRule_t
// {
// 	EGameRuleType aType;
// 	std::string   name;
// 	// void*        apValue;
// 
// 	union
// 	{
// 		bool              aBool;
// 		float             aFloat;
// 		double            aDouble;
// 
// 		s64               aInt;
// 		u64               aUnsignedInt;
// 
// 		glm::vec2         aVec2;
// 		glm::vec3         aVec3;
// 		glm::vec4         aVec4;
// 		glm::quat         aQuat;
// 
// 		std::string       aString;
// 		ChVector< void* > aVector;
// 	};
// 
// 	~GameRule_t()
// 	{
// 	}
// };


struct GameRulesDatabase
{
	// Database (THIS WON'T WORK, YOU WILL HAVE NAME CLASHING)
	std::unordered_map< std::string, bool >          aVarBools;
	std::unordered_map< std::string, float >         aVarFloats;
	std::unordered_map< std::string, double >        aVarDoubles;

	std::unordered_map< std::string, s64 >           aVarInts;
	std::unordered_map< std::string, u64 >           aVarUnsignedInts;

	std::unordered_map< std::string, std::string >   aVarStrings;

	// ---------------------------------------------------------------------------------------------
	// Delete Functions

	void                                             DeleteBool( std::string_view sName );
	bool                                             SetBool( std::string_view sName, bool sValue );

	void                                             DeleteFloat( std::string_view sName );
	//bool                                             SetFloat( std::string_view sName, float sValue );

	void                                             DeleteDouble( std::string_view sName );

	void                                             DeleteInt( std::string_view sName );
	void                                             DeleteUnsignedInt( std::string_view sName );

	void                                             DeleteString( std::string_view sName );

	// ---------------------------------------------------------------------------------------------
	// Set Functions

	//inline void SetString( std::string_view sName, std::string_view sValue )
	//{
	//	if ( GameRule_t* var = SetVarInternal( sName, EGameRuleType_String ) )
	//	{
	//		var->aString = sValue.data();
	//	}
	//}

	//inline std::string_view GetString( std::string_view sName )
	//{
	//	if ( GameRule_t* var = GetVarInternal( sName, EGameRuleType_String ) )
	//	{
	//		return var->aString;
	//	}
	//
	//	return "";
	//}

#define CH_SET_GET_VAR( func, type, gameStateType, var )            \
  inline void Set##func( std::string_view sName, type sValue )      \
  {                                                                 \
	if ( GameRule_t* var = SetVarInternal( sName, gameStateType ) ) \
	{                                                               \
	  var->var = sValue;                                            \
	}                                                               \
  }                                                                 \
  inline type Get##func( std::string_view sName )                   \
  {                                                                 \
	if ( GameRule_t* var = GetVarInternal( sName, gameStateType ) ) \
	{                                                               \
	  return var->var;                                              \
	}                                                               \
                                                                    \
	return {};                                                      \
  }

	//CH_SET_GET_VAR( Bool, bool, EGameRuleType_Bool, aBool );
	//CH_SET_GET_VAR( Float, float, EGameRuleType_Float, aFloat );
	//CH_SET_GET_VAR( Double, double, EGameRuleType_Double, aDouble );
	//
	//CH_SET_GET_VAR( Int, s64, EGameRuleType_Int, aInt );
	//CH_SET_GET_VAR( UnsignedInt, u64, EGameRuleType_UnsignedInt, aUnsignedInt );
	//
	//CH_SET_GET_VAR( Vec2, glm::vec2, EGameRuleType_Vec2, aVec2 );
	//CH_SET_GET_VAR( Vec3, glm::vec3, EGameRuleType_Vec3, aVec3 );
	//CH_SET_GET_VAR( Vec4, glm::vec4, EGameRuleType_Vec4, aVec4 );
	//CH_SET_GET_VAR( Quat, glm::quat, EGameRuleType_Quat, aQuat );

	#undef CH_SET_GET_VAR

	// ---------------------------------------------------------------------------------------------
	// Networking
};


GameRulesDatabase* GameRules();

