#include "core/console.h"


LOG_CHANNEL( Console );

extern ConVarFlag_t                    gConVarRegisterFlags;
extern std::vector< ConVarFlagData_t > gConVarFlags;


constexpr size_t                       one = 1;

// ================================================================================
// ConVar2 System


// return false if the ConVar already exists
// return true if the ConVar was created
bool Con_RegisterConVar_Base( ConVarData_t** conVarDataIn, const char* spName, const char* spDesc, ConVarFlag_t sFlags, EConVarType sType )
{
	if ( !spName )
	{
		Log_Error( gLC_Console, "ConVar Name is NULL\n" );
		return false;
	}

	auto it = Con_GetConVarMap().find( spName );

	if ( it != Con_GetConVarMap().end() )
	{
		ConVarData_t* conVarData = it->second;

		if ( conVarData->aType != sType )
		{
			Log_ErrorF( gLC_Console, "ConVar Type Mismatch: %s - %s VS %s\n", spName, Con_ConVarTypeStr( conVarData->aType ), Con_ConVarTypeStr( sType ) );
			return false;
		}
		else
		{
			if ( conVarDataIn )
				*conVarDataIn = conVarData;

			return false;
		}
	}

	// Make sure this is a valid type
	if ( sType >= EConVarType_Count )
	{
		Log_ErrorF( gLC_Console, "Invalid ConVar Type: %s\n", spName );
		return false;
	}

	ConVarData_t* conVarData     = new ConVarData_t;
	conVarData->aFlags           = sFlags;
	conVarData->aType            = sType;

	Con_GetConVarMap()[ spName ] = conVarData;

	if ( spDesc )
	{
		Con_GetConVarDesc()[ spName ] = ch_str_copy( spDesc );
	}
	else
	{
		Con_GetConVarDesc()[ spName ] = {};
	}

	Con_GetConVarNames().push_back( spName );
	Con_GetConVarList().emplace_back( spName, true, conVarData );

	// Make sure to add any base convar register flags
	conVarData->aFlags |= gConVarRegisterFlags;

	if ( conVarDataIn )
		*conVarDataIn = conVarData;

	return true;
}


// ------------------------------------------------------------------------------


const bool& Con_RegisterConVar_Bool( const char* spName, bool sDefault, ConVarFlag_t sFlags, const char* spDesc, ConVarFunc_Bool* spCallbackFunc )
{
	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_Bool );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aBool.apData;

		return sDefault;
	}

	cvarData->aBool.apFunc       = spCallbackFunc;
	cvarData->aBool.aDefaultData = sDefault;

	cvarData->aBool.apData       = ch_malloc< bool >( 1 );
	memcpy( cvarData->aBool.apData, &sDefault, sizeof( bool ) );

	return *cvarData->aBool.apData;
}


const int& Con_RegisterConVar_Int( const char* spName, int sDefault, ConVarFlag_t sFlags, const char* spDesc, ConVarFunc_Int* spCallbackFunc )
{
	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_Int );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aInt.apData;

		return sDefault;
	}

	cvarData->aInt.apFunc       = spCallbackFunc;
	cvarData->aInt.aDefaultData = sDefault;

	cvarData->aInt.apData       = ch_malloc< int >( 1 );
	memcpy( cvarData->aInt.apData, &sDefault, sizeof( int ) );

	return *cvarData->aInt.apData;
}


const float& Con_RegisterConVar_Float( const char* spName, float sDefault, ConVarFlag_t sFlags, const char* spDesc, ConVarFunc_Float* spCallbackFunc )
{
	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_Float );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aFloat.apData;

		return sDefault;
	}

	cvarData->aFloat.apFunc       = spCallbackFunc;
	cvarData->aFloat.aDefaultData = sDefault;

	cvarData->aFloat.apData       = ch_malloc< float >( 1 );
	memcpy( cvarData->aFloat.apData, &sDefault, sizeof( float ) );

	return *cvarData->aFloat.apData;
}


char*& const Con_RegisterConVar_String( const char* spName, const char* spDefault, ConVarFlag_t sFlags, const char* spDesc, ConVarFunc_String* spCallbackFunc )
{
	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_String );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aString.apData;

		char* temp = ch_str_copy( spDefault ).data;
		return temp;
	}

	cvarData->aString.apFunc       = spCallbackFunc;
	cvarData->aString.aDefaultData = ch_str_copy( spDefault ).data;

	char* value                    = ch_str_copy( spDefault ).data;

	cvarData->aString.apData       = ch_malloc< char* >( 1 );
	memcpy( cvarData->aString.apData, &value, sizeof( char* ) );

	return *cvarData->aString.apData;
}


const glm::vec2& Con_RegisterConVar_Vec2( const char* spName, const glm::vec2& srDefault, ConVarFlag_t sFlags, const char* spDesc, ConVarFunc_Vec2* spCallbackFunc )
{
	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_Vec2 );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aVec2.apData;

		// MEMORY LEAK
		glm::vec2* temp = ch_malloc< glm::vec2 >( 1 );
		memcpy( temp, &srDefault, sizeof( glm::vec2 ) );
		return *temp;
	}

	cvarData->aVec2.apFunc       = spCallbackFunc;
	cvarData->aVec2.aDefaultData = srDefault;

	cvarData->aVec2.apData       = ch_malloc< glm::vec2 >( 1 );
	memcpy( cvarData->aVec2.apData, &srDefault, sizeof( glm::vec2 ) );

	return *cvarData->aVec2.apData;
}


const glm::vec3& Con_RegisterConVar_Vec3( const char* spName, const glm::vec3& srDefault, ConVarFlag_t sFlags, const char* spDesc, ConVarFunc_Vec3* spCallbackFunc )
{
	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_Vec3 );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aVec3.apData;

		// MEMORY LEAK
		glm::vec3* temp = ch_malloc< glm::vec3 >( 1 );
		memcpy( temp, &srDefault, sizeof( glm::vec3 ) );
		return *temp;
	}

	cvarData->aVec3.apFunc       = spCallbackFunc;
	cvarData->aVec3.aDefaultData = srDefault;

	cvarData->aVec3.apData       = ch_malloc< glm::vec3 >( 1 );
	memcpy( cvarData->aVec3.apData, &srDefault, sizeof( glm::vec3 ) );

	return *cvarData->aVec3.apData;
}


const glm::vec4& Con_RegisterConVar_Vec4( const char* spName, const glm::vec4& srDefault, ConVarFlag_t sFlags, const char* spDesc, ConVarFunc_Vec4* spCallbackFunc )
{
	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_Vec4 );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aVec4.apData;

		// MEMORY LEAK
		glm::vec4* temp = ch_malloc< glm::vec4 >( 1 );
		memcpy( temp, &srDefault, sizeof( glm::vec4 ) );
		return *temp;
	}

	cvarData->aVec4.apFunc       = spCallbackFunc;
	cvarData->aVec4.aDefaultData = srDefault;

	cvarData->aVec4.apData       = ch_malloc< glm::vec4 >( 1 );
	memcpy( cvarData->aVec4.apData, &srDefault, sizeof( glm::vec4 ) );

	return *cvarData->aVec4.apData;
}


const glm::vec2& Con_RegisterConVar_Vec2( const char* spName, float sX, float sY, ConVarFlag_t sFlags, const char* spDesc, ConVarFunc_Vec2* spCallbackFunc )
{
	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_Vec2 );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aVec2.apData;

		// MEMORY LEAK
		glm::vec2* temp = ch_malloc< glm::vec2 >( 1 );
		temp->x = sX;
		temp->y = sY;
		return *temp;
	}

	cvarData->aVec2.apFunc       = spCallbackFunc;
	cvarData->aVec2.aDefaultData = { sX, sY };

	cvarData->aVec2.apData       = ch_malloc< glm::vec2 >( 1 );
	cvarData->aVec2.apData->x    = sX;
	cvarData->aVec2.apData->y    = sY;

	return *cvarData->aVec2.apData;
}


const glm::vec3& Con_RegisterConVar_Vec3( const char* spName, float sX, float sY, float sZ, ConVarFlag_t sFlags, const char* spDesc, ConVarFunc_Vec3* spCallbackFunc )
{
	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_Vec3 );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aVec3.apData;

		// MEMORY LEAK
		glm::vec3* temp = ch_malloc< glm::vec3 >( 1 );
		temp->x = sX;
		temp->y = sY;
		temp->z = sZ;
		return *temp;
	}

	cvarData->aVec3.apFunc       = spCallbackFunc;
	cvarData->aVec3.aDefaultData = { sX, sY, sZ };

	cvarData->aVec3.apData       = ch_malloc< glm::vec3 >( 1 );
	cvarData->aVec3.apData->x    = sX;
	cvarData->aVec3.apData->y    = sY;
	cvarData->aVec3.apData->z    = sZ;

	return *cvarData->aVec3.apData;
}


const glm::vec4& Con_RegisterConVar_Vec4( const char* spName, float sX, float sY, float sZ, float sW, ConVarFlag_t sFlags, const char* spDesc, ConVarFunc_Vec4* spCallbackFunc )
{
	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_Vec4 );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aVec4.apData;

		// MEMORY LEAK
		glm::vec4* temp = ch_malloc< glm::vec4 >( 1 );
		temp->x = sX;
		temp->y = sY;
		temp->z = sZ;
		temp->w = sW;
		return *temp;
	}

	cvarData->aVec4.apFunc       = spCallbackFunc;
	cvarData->aVec4.aDefaultData = { sX, sY, sZ, sW };

	cvarData->aVec4.apData       = ch_malloc< glm::vec4 >( 1 );
	cvarData->aVec4.apData->x    = sX;
	cvarData->aVec4.apData->y    = sY;
	cvarData->aVec4.apData->z    = sZ;
	cvarData->aVec4.apData->w    = sW;

	return *cvarData->aVec4.apData;
}


const int& Con_RegisterConVar_RangeInt( const char* spName, int sDefault, int sMin, int sMax, ConVarFlag_t sFlags, const char* spDesc, ConVarFunc_Int* spCallbackFunc )
{
	// Make sure the default value is within the range
	sDefault               = std::clamp( sDefault, sMin, sMax );

	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_RangeInt );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aRangeInt.apData;

		return sDefault;
	}

	cvarData->aRangeInt.apFunc       = spCallbackFunc;
	cvarData->aRangeInt.aDefaultData = sDefault;
	cvarData->aRangeInt.aMin         = sMin;
	cvarData->aRangeInt.aMax         = sMax;

	cvarData->aRangeInt.apData       = ch_malloc< int >( 1 );
	memcpy( cvarData->aRangeInt.apData, &sDefault, sizeof( int ) );

	return *cvarData->aRangeInt.apData;
}


const float& Con_RegisterConVar_RangeFloat( const char* spName, float sDefault, float sMin, float sMax, ConVarFlag_t sFlags, const char* spDesc, ConVarFunc_Float* spCallbackFunc )
{
	// Make sure the default value is within the range
	sDefault               = std::clamp( sDefault, sMin, sMax );

	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_RangeFloat );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aRangeFloat.apData;

		return sDefault;
	}

	cvarData->aRangeFloat.apFunc       = spCallbackFunc;
	cvarData->aRangeFloat.aDefaultData = sDefault;
	cvarData->aRangeFloat.aMin         = sMin;
	cvarData->aRangeFloat.aMax         = sMax;

	cvarData->aRangeFloat.apData       = ch_malloc< float >( 1 );
	memcpy( cvarData->aRangeFloat.apData, &sDefault, sizeof( float ) );

	return *cvarData->aRangeFloat.apData;
}


// ------------------------------------------------------------------------------


const bool& Con_RegisterConVar_Bool( const char* spName, bool sDefault, const char* spDesc, ConVarFunc_Bool* spCallbackFunc )
{
	return Con_RegisterConVar_Bool( spName, sDefault, 0, spDesc, spCallbackFunc );
}


const int& Con_RegisterConVar_Int( const char* spName, int sDefault, const char* spDesc, ConVarFunc_Int* spCallbackFunc )
{
	return Con_RegisterConVar_Int( spName, sDefault, 0, spDesc, spCallbackFunc );
}


const float& Con_RegisterConVar_Float( const char* spName, float sDefault, const char* spDesc, ConVarFunc_Float* spCallbackFunc )
{
	return Con_RegisterConVar_Float( spName, sDefault, 0, spDesc, spCallbackFunc );
}


char*& const Con_RegisterConVar_String( const char* spName, const char* spDefault, const char* spDesc, ConVarFunc_String* spCallbackFunc )
{
	return Con_RegisterConVar_String( spName, spDefault, 0, spDesc, spCallbackFunc );
}


const glm::vec2& Con_RegisterConVar_Vec2( const char* spName, const glm::vec2& srDefault, const char* spDesc, ConVarFunc_Vec2* spCallbackFunc )
{
	return Con_RegisterConVar_Vec2( spName, srDefault, 0, spDesc, spCallbackFunc );
}


const glm::vec3& Con_RegisterConVar_Vec3( const char* spName, const glm::vec3& srDefault, const char* spDesc, ConVarFunc_Vec3* spCallbackFunc )
{
	return Con_RegisterConVar_Vec3( spName, srDefault, 0, spDesc, spCallbackFunc );
}


const glm::vec4& Con_RegisterConVar_Vec4( const char* spName, const glm::vec4& srDefault, const char* spDesc, ConVarFunc_Vec4* spCallbackFunc )
{
	return Con_RegisterConVar_Vec4( spName, srDefault, 0, spDesc, spCallbackFunc );
}


const glm::vec2& Con_RegisterConVar_Vec2( const char* spName, float sX, float sY, const char* spDesc, ConVarFunc_Vec2* spCallbackFunc )
{
	return Con_RegisterConVar_Vec2( spName, sX, sY, 0, spDesc, spCallbackFunc );
}


const glm::vec3& Con_RegisterConVar_Vec3( const char* spName, float sX, float sY, float sZ, const char* spDesc, ConVarFunc_Vec3* spCallbackFunc )
{
	return Con_RegisterConVar_Vec3( spName, sX, sY, sZ, 0, spDesc, spCallbackFunc );
}


const glm::vec4& Con_RegisterConVar_Vec4( const char* spName, float sX, float sY, float sZ, float sW, const char* spDesc, ConVarFunc_Vec4* spCallbackFunc )
{
	return Con_RegisterConVar_Vec4( spName, sX, sY, sZ, sW, 0, spDesc, spCallbackFunc );
}


const int& Con_RegisterConVar_RangeInt( const char* spName, int sDefault, int sMin, int sMax, const char* spDesc, ConVarFunc_Int* spCallbackFunc )
{
	return Con_RegisterConVar_RangeInt( spName, sDefault, sMin, sMax, 0, spDesc, spCallbackFunc );
}


const float& Con_RegisterConVar_RangeFloat( const char* spName, float sDefault, float sMin, float sMax, const char* spDesc, ConVarFunc_Float* spCallbackFunc )
{
	return Con_RegisterConVar_RangeFloat( spName, sDefault, sMin, sMax, 0, spDesc, spCallbackFunc );
}


// ------------------------------------------------------------------------------


ch_string Con_GetConVarDesc( const char* spName )
{
	auto it = Con_GetConVarDesc().find( spName );

	if ( it != Con_GetConVarDesc().end() )
		return it->second;

	Log_ErrorF( gLC_Console, "ConVar Description not found: %s\n", spName );
	return {};
}


// ConVarData_t* Con_GetConVarData( const char* spName )
// {
// 	auto it = Con_GetConVarMap().find( spName );
//
// 	if ( it != Con_GetConVarMap().end() )
// 		return it->second;
//
// 	Log_ErrorF( gLC_Console, "ConVar Data not found: %s\n", spName );
// 	return nullptr;
// }


ConVarData_t* Con_GetConVarData( std::string_view sName )
{
	auto it = Con_GetConVarMap().find( sName );

	if ( it != Con_GetConVarMap().end() )
		return it->second;

	return nullptr;
}


const char* gConVarTypeStr[] = {
	"Invalid",
	"Bool",
	"Integer",
	"Float",
	"String",
	"RangeInteger",
	"RangeFloat",
	"Vec2",
	"Vec3",
	"Vec4",
};


static_assert( CH_ARR_SIZE( gConVarTypeStr ) == EConVarType_Count );


const char* Con_ConVarTypeStr( EConVarType sType )
{
	if ( sType < 0 || sType >= EConVarType_Count )
	{
		Log_ErrorF( gLC_Console, "Invalid ConVar Type: %d\n", sType );
		return nullptr;
	}

	return gConVarTypeStr[ sType ];
}


EConVarType Con_ConVarType( const char* spType )
{
	for ( size_t i = 0; i < EConVarType_Count; i++ )
	{
		if ( ch_strcasecmp( spType, gConVarTypeStr[ i ] ) == 0 )
			return (EConVarType)i;
	}

	Log_ErrorF( gLC_Console, "Invalid ConVar Type: %s\n", spType );
	return EConVarType_Invalid;
}


std::string Con_GetConVarValueStr( ConVarData_t* cvarData )
{
	switch ( cvarData->aType )
	{
		case EConVarType_Bool:
			return *cvarData->aBool.apData ? "true" : "false";

		case EConVarType_Int:
			return vstring( "%d", *cvarData->aInt.apData );

		case EConVarType_Float:
			return vstring( "%.6f", *cvarData->aFloat.apData );

		case EConVarType_String:
			return vstring( "\"%s\"", *cvarData->aString.apData );

		case EConVarType_Vec2:
			return vstring( "%.6f %.6f", cvarData->aVec2.apData->x, cvarData->aVec2.apData->y );

		case EConVarType_Vec3:
			return vstring( "%.6f %.6f %.6f", cvarData->aVec3.apData->x, cvarData->aVec3.apData->y, cvarData->aVec3.apData->z );

		case EConVarType_Vec4:
			return vstring( "%.6f %.6f %.6f %.6f", cvarData->aVec4.apData->x, cvarData->aVec4.apData->y, cvarData->aVec4.apData->z, cvarData->aVec4.apData->w );

		case EConVarType_RangeInt:
			return vstring( "%d", *cvarData->aRangeInt.apData );

		case EConVarType_RangeFloat:
			return vstring( "%.6f", *cvarData->aRangeFloat.apData );

		default:
			return "Invalid ConVar Type";
	}
}

static std::string gStrEmpty;

std::string Con_GetConVarValueStr( const char* name )
{
	ConVarData_t* cvarData = Con_GetConVarData( name );

	if ( !cvarData )
		return gStrEmpty;

	return Con_GetConVarValueStr( cvarData );
}


std::string Con_GetConVarHelp( std::string_view spName )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
		return vstring( "ConVar not found: %s\n", spName.data() );

	std::string msg = vstring( "%s%s %s", ANSI_CLR_DEFAULT, spName.data(), Con_GetConVarValueStr( cvarData ).data() );

	// Check if the current value differs from the default value
	switch ( cvarData->aType )
	{
		case EConVarType_Bool:
			
			if ( *cvarData->aBool.apData != cvarData->aBool.aDefaultData )
				msg += vstring( " " ANSI_CLR_YELLOW "(%s default)", cvarData->aBool.aDefaultData ? "true" : "false" );
			break;

		case EConVarType_Int:
			if ( *cvarData->aInt.apData != cvarData->aInt.aDefaultData )
				msg += vstring( " " ANSI_CLR_YELLOW "(%d default)", cvarData->aInt.aDefaultData );
			break;

		case EConVarType_Float:
			if ( *cvarData->aFloat.apData != cvarData->aFloat.aDefaultData )
				msg += vstring( " " ANSI_CLR_YELLOW "(%.6f default)", cvarData->aFloat.aDefaultData );
			break;

		case EConVarType_String:
			if ( ch_strcasecmp( *cvarData->aString.apData, cvarData->aString.aDefaultData ) != 0 )
				msg += vstring( " " ANSI_CLR_YELLOW "(\"%s\" default)", cvarData->aString.aDefaultData );
			break;

		case EConVarType_Vec2:
			if ( *cvarData->aVec2.apData != cvarData->aVec2.aDefaultData )
				msg += vstring( " " ANSI_CLR_YELLOW "(%.6f %.6f default)", cvarData->aVec2.aDefaultData.x, cvarData->aVec2.aDefaultData.y );
			break;

		case EConVarType_Vec3:
			if ( *cvarData->aVec3.apData != cvarData->aVec3.aDefaultData )
				msg += vstring( " " ANSI_CLR_YELLOW "(%.6f %.6f %.6f default)", cvarData->aVec3.aDefaultData.x, cvarData->aVec3.aDefaultData.y, cvarData->aVec3.aDefaultData.z );
			break;

		case EConVarType_Vec4:
			if ( *cvarData->aVec4.apData != cvarData->aVec4.aDefaultData )
				msg += vstring( " " ANSI_CLR_YELLOW "(%.6f %.6f %.6f %.6f default)", cvarData->aVec4.aDefaultData.x, cvarData->aVec4.aDefaultData.y, cvarData->aVec4.aDefaultData.z, cvarData->aVec4.aDefaultData.w );
			break;

		case EConVarType_RangeInt:
		{
			if ( *cvarData->aRangeInt.apData != cvarData->aRangeInt.aDefaultData )
				msg += vstring( " " ANSI_CLR_YELLOW "(%d default)", cvarData->aRangeInt.aDefaultData );

			// put in data about the range this convar can be, also check if it's INT_MIN and INT_MAX, don't print that
			std::string minStr;

			if ( cvarData->aRangeInt.aMin == INT_MIN )
				minStr = "-inf";
			else
				minStr = vstring( "%d", cvarData->aRangeInt.aMin );

			std::string maxStr;

			if ( cvarData->aRangeInt.aMax == INT_MAX )
				maxStr = "inf";
			else
				maxStr = vstring( "%d", cvarData->aRangeInt.aMax );

			msg += vstring( " Range (%s - %s)", minStr.c_str(), maxStr.c_str() );
			break;
		}

		case EConVarType_RangeFloat:
		{
			if ( *cvarData->aRangeFloat.apData != cvarData->aRangeFloat.aDefaultData )
				msg += vstring( " " ANSI_CLR_YELLOW "(%.6f default)", cvarData->aRangeFloat.aDefaultData );

			// put in data about the range this convar can be, also check if it's FLT_MIN and FLT_MAX, don't print that
			std::string minStr;

			if ( cvarData->aRangeFloat.aMin == FLT_MIN )
				minStr = "-inf";
			else
				minStr = vstring( "%.6f", cvarData->aRangeFloat.aMin );

			std::string maxStr;

			if ( cvarData->aRangeFloat.aMax == FLT_MAX )
				maxStr = "inf";
			else
				maxStr = vstring( "%.6f", cvarData->aRangeFloat.aMax );

			msg += vstring( " Range (%s - %s)", minStr.c_str(), maxStr.c_str() );
			break;
		}
	}

	std::string type = Con_ConVarTypeStr( cvarData->aType );
	msg += ANSI_CLR_CYAN " - " + type;

	if ( cvarData->aFlags )
	{
		msg += "\n\t" ANSI_CLR_DARK_GREEN;
		// TODO: this could be better probably
		// 3:30 am and very lazy programming here
		for ( size_t i = 0, j = 0; i < gConVarFlags.size(); i++ )
		{
			if ( ( one << i ) > cvarData->aFlags )
				break;

			if ( !( cvarData->aFlags & ( one << i ) ) )
				continue;

			if ( j != 0 )
				msg += " | ";

			msg += Con_GetCvarFlagName( ( one << i ) );
			j++;
		}
	}

	// Get the Console Variable Description
	const char* desc = Con_GetConVarDesc( spName.data() ).data;

	if ( desc )
	{
		msg += vstring( "\n\t" ANSI_CLR_CYAN "%s" ANSI_CLR_DEFAULT "\n\n", desc );
		return msg;
	}

	return msg + ANSI_CLR_DEFAULT "\n";
}


void Con_ResetConVar( std::string_view name )
{
	ConVarData_t* cvarData = Con_GetConVarData( name );

	if ( !cvarData )
	{
		Log_ErrorF( gLC_Console, "ConVar not found: %s\n", name.data() );
		return;
	}

	switch ( cvarData->aType )
	{
		case EConVarType_Bool:
			*cvarData->aBool.apData = cvarData->aBool.aDefaultData;
			break;

		case EConVarType_Int:
			*cvarData->aInt.apData = cvarData->aInt.aDefaultData;
			break;

		case EConVarType_Float:
			*cvarData->aFloat.apData = cvarData->aFloat.aDefaultData;
			break;

		case EConVarType_String:
			*cvarData->aString.apData = ch_str_realloc( *cvarData->aString.apData, cvarData->aString.aDefaultData ).data;
			break;

		case EConVarType_Vec2:
			*cvarData->aVec2.apData = cvarData->aVec2.aDefaultData;
			break;

		case EConVarType_Vec3:
			*cvarData->aVec3.apData = cvarData->aVec3.aDefaultData;
			break;

		case EConVarType_Vec4:
			*cvarData->aVec4.apData = cvarData->aVec4.aDefaultData;
			break;

		case EConVarType_RangeInt:
			*cvarData->aRangeInt.apData = cvarData->aRangeInt.aDefaultData;
			break;

		case EConVarType_RangeFloat:
			*cvarData->aRangeFloat.apData = cvarData->aRangeFloat.aDefaultData;
			break;

		default:
			Log_ErrorF( gLC_Console, "Invalid ConVar Type: %s\n", name.data() );
			break;
	}
}


// ------------------------------------------------------------------------------


int Con_SetConVarValueInternal_Bool( ConVarData_t* cvarData, const char* spName, bool sValue )
{
	if ( cvarData->aType != EConVarType_Bool )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"Bool\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	bool  oldValue = *cvarData->aBool.apData;
	bool* value    = cvarData->aBool.apData;
	*value         = sValue;

	if ( cvarData->aBool.apFunc )
		cvarData->aBool.apFunc( oldValue, sValue );

	return 1;
}


int Con_SetConVarValueInternal_Int( ConVarData_t* cvarData, const char* spName, int sValue )
{
	if ( cvarData->aType != EConVarType_Int && cvarData->aType != EConVarType_RangeInt )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"Int\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	if ( cvarData->aType == EConVarType_RangeInt )
	{
		if ( sValue < cvarData->aRangeInt.aMin || sValue > cvarData->aRangeInt.aMax )
		{
			Log_ErrorF( gLC_Console, "ConVar Value out of Range for %s, got %d, must be within %d - %d\n", spName, sValue, cvarData->aRangeInt.aMin, cvarData->aRangeInt.aMax );
			sValue = std::clamp( sValue, cvarData->aRangeInt.aMin, cvarData->aRangeInt.aMax );
		}

		int  oldValue = *cvarData->aRangeInt.apData;
		int* value    = cvarData->aRangeInt.apData;
		*value        = sValue;

		if ( cvarData->aRangeInt.apFunc )
			cvarData->aRangeInt.apFunc( oldValue, sValue );
	}
	else
	{
		int  oldValue = *cvarData->aInt.apData;
		int* value    = cvarData->aInt.apData;
		*value        = sValue;

		if ( cvarData->aInt.apFunc )
			cvarData->aInt.apFunc( oldValue, sValue );
	}

	return 1;
}


int Con_SetConVarValueInternal_Float( ConVarData_t* cvarData, const char* spName, float sValue )
{
	// check if the convar type is float or ranged
	if ( cvarData->aType != EConVarType_Float && cvarData->aType != EConVarType_RangeFloat )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"Float\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	if ( cvarData->aType == EConVarType_RangeFloat )
	{
		if ( sValue < cvarData->aRangeFloat.aMin || sValue > cvarData->aRangeFloat.aMax )
		{
			Log_ErrorF( gLC_Console, "ConVar Value out of Range for %s, got %.6f, must be within %.6f - %.6f\n", spName, sValue, cvarData->aRangeFloat.aMin, cvarData->aRangeFloat.aMax );
			sValue = std::clamp( sValue, cvarData->aRangeFloat.aMin, cvarData->aRangeFloat.aMax );
		}

		float  oldValue = *cvarData->aRangeFloat.apData;
		float* value    = cvarData->aRangeFloat.apData;
		*value          = sValue;

		if ( cvarData->aRangeFloat.apFunc )
			cvarData->aRangeFloat.apFunc( oldValue, sValue );
	}
	else
	{
		float  oldValue = *cvarData->aFloat.apData;
		float* value = cvarData->aFloat.apData;
		*value       = sValue;

		if ( cvarData->aFloat.apFunc )
			cvarData->aFloat.apFunc( oldValue, sValue );
	}

	return 1;
}


int Con_SetConVarValueInternal_String( ConVarData_t* cvarData, const char* spName, char* spValue )
{
	if ( cvarData->aType != EConVarType_String )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"String\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	char** value    = cvarData->aString.apData;
	char*  oldValue = nullptr;

	// If there's a callback function, back up the old value
	if ( cvarData->aString.apFunc )
		oldValue = ch_str_copy( *value ).data;

	char*  newValue = ch_str_realloc( *value, spValue ).data;

	if ( newValue == nullptr )
	{
		Log_ErrorF( gLC_Console, "Failed to set ConVar Value for %s\n", spName );
		return 0;
	}

	*value = newValue;

	if ( cvarData->aString.apFunc )
	{
		cvarData->aString.apFunc( oldValue, newValue );
		ch_str_free( oldValue );
	}

	return 1;
}


int Con_SetConVarValueInternal_String( ConVarData_t* cvarData, const char* spName, std::string_view sValue )
{
	if ( cvarData->aType != EConVarType_String )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"String\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	char** value    = cvarData->aString.apData;
	char*  oldValue = nullptr;

	// If there's a callback function, back up the old value
	if ( cvarData->aString.apFunc )
		oldValue = ch_str_copy( *value ).data;

	char* newValue = ch_str_realloc( *value, sValue.data(), sValue.size() ).data;

	if ( newValue == nullptr )
	{
		Log_ErrorF( gLC_Console, "Failed to set ConVar Value for %s\n", spName );
		return 0;
	}

	*value = newValue;

	if ( cvarData->aString.apFunc )
	{
		cvarData->aString.apFunc( oldValue, newValue );
		ch_str_free( oldValue );
	}

	return 1;
}


int Con_SetConVarValueInternal_Vec( ConVarData_t* cvarData, const char* spName, const glm::vec2& srValue )
{
	if ( cvarData->aType != EConVarType_Vec2 )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"Vec2\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	glm::vec2  oldValue = *cvarData->aVec2.apData;
	glm::vec2* value    = cvarData->aVec2.apData;
	value->x            = srValue.x;
	value->y            = srValue.y;

	if ( cvarData->aVec2.apFunc )
		cvarData->aVec2.apFunc( oldValue, *value );

	return 1;
}


int Con_SetConVarValueInternal_Vec( ConVarData_t* cvarData, const char* spName, const glm::vec3& srValue )
{
	if ( cvarData->aType != EConVarType_Vec3 )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"Vec3\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	glm::vec3  oldValue = *cvarData->aVec3.apData;
	glm::vec3* value    = cvarData->aVec3.apData;
	value->x            = srValue.x;
	value->y            = srValue.y;
	value->z            = srValue.z;

	if ( cvarData->aVec3.apFunc )
		cvarData->aVec3.apFunc( oldValue, *value );

	return 1;
}


int Con_SetConVarValueInternal_Vec( ConVarData_t* cvarData, const char* spName, const glm::vec4& srValue )
{
	if ( cvarData->aType != EConVarType_Vec4 )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"Vec4\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	glm::vec4  oldValue = *cvarData->aVec4.apData;
	glm::vec4* value    = cvarData->aVec4.apData;
	value->x            = srValue.x;
	value->y            = srValue.y;
	value->z            = srValue.z;
	value->w            = srValue.w;

	if ( cvarData->aVec4.apFunc )
		cvarData->aVec4.apFunc( oldValue, *value );

	return 1;
}


// ------------------------------------------------------------------------------


// Get the data pointer of a ConVar
bool& Con_GetConVarData_Bool( const char* spName, bool fallback )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
	{
		// MEMORY LEAK
		// TODO: Store these in an array somewhere and free them when the program ends
		bool* temp = ch_malloc< bool >( 1 );
		memcpy( temp, &fallback, sizeof( bool ) );
		return *temp;
	}

	if ( cvarData->aType != EConVarType_Bool )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"%s\", expected \"Bool\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );

		bool* temp = ch_malloc< bool >( 1 );
		memcpy( temp, &fallback, sizeof( bool ) );
		return *temp;
	}

	return *cvarData->aBool.apData;
}


int& Con_GetConVarData_Int( const char* spName, int fallback )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
	{
		int* temp = ch_malloc< int >( 1 );
		memcpy( temp, &fallback, sizeof( int ) );
		return *temp;
	}

	if ( cvarData->aType == EConVarType_Int )
	{
		return *cvarData->aInt.apData;
	}

	if ( cvarData->aType == EConVarType_RangeInt )
	{
		return *cvarData->aRangeInt.apData;
	}

	Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"%s\", expected \"Int\" or \"RangeInt\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );

	int* temp = ch_malloc< int >( 1 );
	memcpy( temp, &fallback, sizeof( int ) );
	return *temp;
}


float& Con_GetConVarData_Float( const char* spName, float fallback )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
	{
		float* temp = ch_malloc< float >( 1 );
		memcpy( temp, &fallback, sizeof( float ) );
		return *temp;
	}

	if ( cvarData->aType == EConVarType_Float )
	{
		return *cvarData->aFloat.apData;
	}

	if ( cvarData->aType == EConVarType_RangeFloat )
	{
		return *cvarData->aRangeFloat.apData;
	}

	Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"%s\", expected \"Float\" or \"RangeFloat\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );

	float* temp = ch_malloc< float >( 1 );
	memcpy( temp, &fallback, sizeof( float ) );
	return *temp;
}


char*& Con_GetConVarData_String( const char* spName, const char* fallback )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
	{
		char* temp = ch_str_copy( fallback ).data;
		return temp;
	}

	if ( cvarData->aType != EConVarType_String )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"%s\", expected \"String\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );

		char* temp = ch_str_copy( fallback ).data;
		return temp;
	}

	return *cvarData->aString.apData;
}


glm::vec2& Con_GetConVarData_Vec2( const char* spName, const glm::vec2& fallback )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
	{
		glm::vec2* temp = ch_malloc< glm::vec2 >( 1 );
		memcpy( temp, &fallback, sizeof( glm::vec2 ) );
		return *temp;
	}

	if ( cvarData->aType != EConVarType_Vec2 )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"%s\", expected \"Vec2\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );

		glm::vec2* temp = ch_malloc< glm::vec2 >( 1 );
		memcpy( temp, &fallback, sizeof( glm::vec2 ) );
		return *temp;
	}

	return *cvarData->aVec2.apData;
}


glm::vec3& Con_GetConVarData_Vec3( const char* spName, const glm::vec3& fallback )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
	{
		glm::vec3* temp = ch_malloc< glm::vec3 >( 1 );
		memcpy( temp, &fallback, sizeof( glm::vec3 ) );
		return *temp;
	}

	if ( cvarData->aType != EConVarType_Vec3 )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"%s\", expected \"Vec3\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );

		glm::vec3* temp = ch_malloc< glm::vec3 >( 1 );
		memcpy( temp, &fallback, sizeof( glm::vec3 ) );
		return *temp;
	}

	return *cvarData->aVec3.apData;
}


glm::vec4& Con_GetConVarData_Vec4( const char* spName, const glm::vec4& fallback )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
	{
		glm::vec4* temp = ch_malloc< glm::vec4 >( 1 );
		memcpy( temp, &fallback, sizeof( glm::vec4 ) );
		return *temp;
	}

	if ( cvarData->aType != EConVarType_Vec4 )
	{
		Log_ErrorF( gLC_Console, "ConVar Type Mismatch for %s, got \"%s\", expected \"Vec4\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );

		glm::vec4* temp = ch_malloc< glm::vec4 >( 1 );
		memcpy( temp, &fallback, sizeof( glm::vec4 ) );
		return *temp;
	}

	return *cvarData->aVec4.apData;
}


// ------------------------------------------------------------------------------


int Con_SetConVarValue( const char* spName, bool sValue )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
		return -1;

	bool prevValue = *cvarData->aBool.apData;

	int  ret       = Con_SetConVarValueInternal_Bool( cvarData, spName, sValue );

	return ret;
}


int Con_SetConVarValue( const char* spName, int sValue )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
		return -1;

	int prevValue = *cvarData->aInt.apData;

	int ret       = Con_SetConVarValueInternal_Int( cvarData, spName, sValue );

	return ret;
}


int Con_SetConVarValue( const char* spName, float sValue )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
		return -1;

	return Con_SetConVarValueInternal_Float( cvarData, spName, sValue );
}


int Con_SetConVarValue( const char* spName, const char* spValue )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
		return -1;

	return Con_SetConVarValueInternal_String( cvarData, spName, spValue );
}


int Con_SetConVarValue( const char* spName, std::string_view sValue )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
		return -1;

	return Con_SetConVarValueInternal_String( cvarData, spName, sValue );
}


int Con_SetConVarValue( const char* spName, const glm::vec2& srValue )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
		return -1;

	return Con_SetConVarValueInternal_Vec( cvarData, spName, srValue );
}


int Con_SetConVarValue( const char* spName, const glm::vec3& srValue )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
		return -1;

	return Con_SetConVarValueInternal_Vec( cvarData, spName, srValue );
}


int Con_SetConVarValue( const char* spName, const glm::vec4& srValue )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
		return -1;

	return Con_SetConVarValueInternal_Vec( cvarData, spName, srValue );
}


// ------------------------------------------------------------------------------


template< typename VEC >
bool ParseVector( const char* spName, const std::vector< std::string >& args, VEC& srVector )
{
	if ( args.size() < srVector.length() )
	{
		Log_ErrorF( gLC_Console, "ConVar \"%s\", Type of \"Vec2\" requires 2 arguments\n", spName );
		return false;
	}

	for ( size_t i = 0; i < args.size(); i++ )
	{
		if ( !ch_to_float( args[ i ].data(), srVector[ i ] ) )
		{
			Log_ErrorF( gLC_Console, "ConVar \"%s\", Invalid argument %d \"%s\" for Vector type, expected a number\n", spName, i, args[ i ].data() );
			return false;
		}
	}

	return true;
}


// Alias List of options for booleans because it's funny
const std::string_view gConVarBoolList_True[] = {
	"true",
	"t",
	"yes",
	"yeah",
	"yup",
	"y",
	"enabled",
	"on",
	"uh-huh",
	"sure",
	"i guess",
	"probably",
	"eeffoc",
};


const std::string_view gConVarBoolList_False[] = {
	"false",
	"f",
	"no",
	"nah",
	"nope",
	"n",
	"disabled",
	"off",
	"nuh-uh",
	"fuck",
	"fuck you",
	"fuck off",
	"shit fuck",
	"plsno",
	"dont",
};


const std::string_view gConVarBoolList_CoinFlip[] = {
	"maybe",
	"idk",
	"?",
	"rand",
	"random",
	"coinflip",
	"coin",
	"flip",  // this would indicate flipping the current boolean value, so maybe remove this
	"rng",
	"roll",
	"dice",
	"!rtd",
	"!rtv",
	"rtd",
	"rtv",
	"username",
	"dQw4w9WgXcQ",
	// "https://www.youtube.com/watch?v=dQw4w9WgXcQ",
};


// returns true if it found a match
bool CheckBoolConVarInput( std::string_view arg, const std::string_view* aliasList, size_t aliasListSize )
{
	for ( u32 i = 0; i < aliasListSize; i++ )
	{
		if ( arg.size() != aliasList[ i ].size() )
			continue;

		if ( ch_strncasecmp( arg.data(), aliasList[ i ].data(), arg.size() ) == 0 )
			return true;
	}

	return false;
}


bool Con_ProcessConVar( ConVarData_t* cvar, const char* name, const std::vector< std::string >& args, std::string_view fullCommand )
{
	if ( args.empty() )
	{
		const std::string& help = Con_GetConVarHelp( name );
		Log_Msg( gLC_Console, help.data() );
		return true;
	}

	switch ( cvar->aType )
	{
		case EConVarType_Bool:
		{
			// check 1 and 0 first for a fast-path
			if ( args[ 0 ].size() == 1 )
			{
				if ( ch_strncasecmp( args[ 0 ].data(), "1", 1 ) == 0 )
				{
					Con_SetConVarValueInternal_Bool( cvar, name, true );
					break;
				}
				else if ( ch_strncasecmp( args[ 0 ].data(), "0", 1 ) == 0 )
				{
					Con_SetConVarValueInternal_Bool( cvar, name, false );
					break;
				}
			}

			if ( CheckBoolConVarInput( fullCommand, gConVarBoolList_True, ARR_SIZE( gConVarBoolList_True ) ) )
			{
				Con_SetConVarValueInternal_Bool( cvar, name, true );
				break;
			}

			if ( CheckBoolConVarInput( fullCommand, gConVarBoolList_False, ARR_SIZE( gConVarBoolList_False ) ) )
			{
				Con_SetConVarValueInternal_Bool( cvar, name, false );
				break;
			}

			if ( CheckBoolConVarInput( fullCommand, gConVarBoolList_CoinFlip, ARR_SIZE( gConVarBoolList_CoinFlip ) ) )
			{
				Con_SetConVarValueInternal_Bool( cvar, name, rand() % 2 == 0 );
				break;
			}

			// Check to see if it's a number
			long value = 0;
			if ( !ch_to_long( args[ 0 ].data(), value ) )
			{
				// this might be useless, i think we only need to check for float actually
				float valueFl = 0.f;
				if ( !ch_to_float( args[ 0 ].data(), valueFl ) )
				{
					log_t group = Log_GroupBeginEx( gLC_Console, ELogType_Error );

					Log_GroupF( group, "ConVar \"%s\", Invalid argument \"%s\" for Bool type, expected a number, or any of these options:\n", name, args[ 0 ].data() );

					Log_Group( group, "True:\n    " );
					for ( u32 i = 0; i < ARR_SIZE( gConVarBoolList_True ); i++ )
						Log_GroupF( group, "%s ", gConVarBoolList_True[ i ].data() );

					Log_Group( group, "\nFalse:\n    " );
					for ( u32 i = 0; i < ARR_SIZE( gConVarBoolList_False ); i++ )
						Log_GroupF( group, "%s ", gConVarBoolList_False[ i ].data() );

					Log_Group( group, "\nCoinFlip:\n    " );
					for ( u32 i = 0; i < ARR_SIZE( gConVarBoolList_CoinFlip ); i++ )
						Log_GroupF( group, "%s ", gConVarBoolList_CoinFlip[ i ].data() );

					Log_GroupEnd( group );
					break;
				}
				else
				{
					Con_SetConVarValueInternal_Bool( cvar, name, valueFl > 0 );
				}
			}
			else
			{
				Con_SetConVarValueInternal_Bool( cvar, name, value > 0 );
			}

			break;
		}
		case EConVarType_Int:
		case EConVarType_RangeInt:
		{
			long value = 0;
			if ( !ch_to_long( args[ 0 ].data(), value ) )
			{
				Log_ErrorF( gLC_Console, "ConVar \"%s\", Invalid argument \"%s\" for Integer type, expected a number\n", name, args[ 0 ].data() );
				break;
			}

			Con_SetConVarValueInternal_Int( cvar, name, (int)value );
			break;
		}
		case EConVarType_Float:
		case EConVarType_RangeFloat:
		{
			float value = 0.f;
			if ( !ch_to_float( args[ 0 ].data(), value ) )
			{
				Log_ErrorF( gLC_Console, "ConVar \"%s\", Invalid argument \"%s\" for Float type, expected a number\n", name, args[ 0 ].data() );
				break;
			}

			Con_SetConVarValueInternal_Float( cvar, name, value );
			break;
		}
		case EConVarType_String:
		{
			Con_SetConVarValueInternal_String( cvar, name, fullCommand );
			break;
		}
		case EConVarType_Vec2:
		{
			glm::vec2 value;
			if ( ParseVector( name, args, value ) )
			{
				Con_SetConVarValueInternal_Vec( cvar, name, value );
			}

			break;
		}
		case EConVarType_Vec3:
		{
			glm::vec3 value;
			if ( ParseVector( name, args, value ) )
			{
				Con_SetConVarValueInternal_Vec( cvar, name, value );
			}

			break;
		}
		case EConVarType_Vec4:
		{
			glm::vec4 value;
			if ( ParseVector( name, args, value ) )
			{
				Con_SetConVarValueInternal_Vec( cvar, name, value );
			}

			break;
		}
		default:
			break;
	}

	return true;
}

