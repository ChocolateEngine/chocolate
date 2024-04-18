#include "core/filesystem.h"
#include "core/console.h"
#include "core/profiler.h"
#include "core/platform.h"
#include "core/log.h"
#include "core/build_number.h"
#include "util.h"

#include <cstring>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <mutex>

LOG_REGISTER_CHANNEL( Console, LogColor::Gray );


struct ConVarFlagData_t
{
	ConVarFlag_t           flag;
	const char*            name;
	size_t                 nameLen;
	ConVarFlagChangeFunc*  apCallback = nullptr;
	ConVarFlagChangeFunc2* apCallback2 = nullptr;
};


std::vector< std::string >                     gQueue;
std::vector< std::string >                     gCommandHistory;
std::vector< ConCommand* >                     gInstantConVars;
std::vector< ConVarFlagData_t >                gConVarFlags;
std::vector< ConVarData* >                     gConVarData;
ConVarFlag_t                                   gConVarRegisterFlags;

std::vector< FArchive* >                       gArchiveCallbacks;

constexpr const char*                          CON_ARCHIVE_FILE    = "cfg/config.cfg";
constexpr const char*                          CON_ARCHIVE_DEFAULT = "cfg/config_default.cfg";
std::string                                    gConArchiveFile     = CON_ARCHIVE_FILE;
std::string                                    gConArchiveDefault  = CON_ARCHIVE_DEFAULT;

// std::unordered_map< ConVarBase*, std::string > gConVarLowercaseNames;


DLL_EXPORT ConVarFlag_t CVARF_NONE = 0;

DLL_EXPORT NEW_CVAR_FLAG( CVARF_ARCHIVE );
DLL_EXPORT NEW_CVAR_FLAG( CVARF_INSTANT );


// ================================================================================


// convars to register after static initalization
// this is a function to ensure this variable gets initialized first
static ChVector< ConVarBase* >& Con_GetConVars()
{
	static ChVector< ConVarBase* > convars;
	return convars;
}


inline bool Con_IsConVarRef( ConVarBase* cvar )
{
	return typeid( *cvar ) == typeid( ConVarRef );
}


static void Con_RegisterConVar1( ConVarBase* spCvar )
{
	Con_GetConVars().push_back( spCvar );

	if ( gConVarRegisterFlags )
	{
		spCvar->aFlags |= gConVarRegisterFlags;
	}
}


void Con_SetConVarRegisterFlags( ConVarFlag_t sFlags )
{
	gConVarRegisterFlags = sFlags;
}


ConVarBase::ConVarBase( const char* name, ConVarFlag_t flags ) :
	aName( name ), aDesc( nullptr ), aFlags( flags )
{
}

ConVarBase::ConVarBase( const char* name, const char* desc ) :
	aName( name ), aDesc( desc ), aFlags()
{
}

ConVarBase::ConVarBase( const char* name, ConVarFlag_t flags, const char* desc ) :
	aName( name ), aDesc( desc ), aFlags( flags )
{
}

const char* ConVarBase::GetName()
{
	return aName;
}

const char* ConVarBase::GetDesc()
{
	return aDesc;
}

ConVarFlag_t ConVarBase::GetFlags()
{
	return aFlags;
}


// ================================================================================


void ConCommand::Init( ConCommandFunc* func, ConCommandDropdownFunc* dropDownFunc )
{
	apFunc           = func;
	apDropDownFunc   = dropDownFunc;

	ConVarBase* cvar = Con_GetConVarBase( aName );

	if ( cvar )
	{
		Log_WarnF( "Multiple ConCommands with the same name! \"%s\"", aName );
		return;
	}

	Con_RegisterConVar1( this );
}

std::string ConCommand::GetPrintMessage(  )
{
	if ( !aDesc )
		return vstring( UNIX_CLR_DEFAULT "%s\n", aName );

	return vstring( UNIX_CLR_DEFAULT "%s\n\t" UNIX_CLR_CYAN "%s" UNIX_CLR_DEFAULT "\n", aName, aDesc );
}

// ================================================================================


ConVarData::~ConVarData()
{
	if ( apDefaultValue )
		Util_FreeString( apDefaultValue );

	if ( apValue )
		Util_FreeString( apValue );
}


ConVar::~ConVar()
{
}


void ConVar::Init( std::string_view defaultValue, ConVarFunc* callback )
{
	ConVar* cvar = Con_GetConVar( aName );

	if ( cvar )
	{
		apData = cvar->apData;

		if ( apData->apDefaultValue != defaultValue )
			Log_WarnF( "Differing Default Values between Convar \"%s\" (A: %s, B: %s)", aName, apData->apDefaultValue, defaultValue.data() );

		if ( gConVarRegisterFlags )
		{
			cvar->aFlags |= gConVarRegisterFlags;
			aFlags |= gConVarRegisterFlags;
		}

		return;
	}

	if ( !apData )
	{
		apData = Con_CreateConVarData();

		Con_RegisterConVar1( this );
	}

	apData->aDefaultValueLen   = defaultValue.size();
	apData->apDefaultValue     = Util_AllocString( defaultValue.data(), defaultValue.size() );

	apData->aDefaultValueFloat = ToDouble( defaultValue.data(), 0.f );
	apData->apFunc             = callback;

	SetValue( defaultValue );
}


void ConVar::Init( float defaultValue, ConVarFunc* callback )
{
	ConVar* cvar = Con_GetConVar( aName );

	if ( cvar )
	{
		apData = cvar->apData;

		if ( apData->aDefaultValueFloat != defaultValue )
			Log_WarnF( "Differing Default Values between Convar \"%s\" (A: %.6f, B: %.6f)", aName, apData->aDefaultValueFloat, defaultValue );

		if ( gConVarRegisterFlags )
		{
			cvar->aFlags |= gConVarRegisterFlags;
			aFlags |= gConVarRegisterFlags;
		}

		return;
	}

	if ( !apData )
	{
		apData = Con_CreateConVarData();

		Con_RegisterConVar1( this );
	}

	std::string defaultValueStr = ToString( defaultValue );
	apData->aDefaultValueLen    = defaultValueStr.size();
	apData->apDefaultValue      = Util_AllocString( defaultValueStr.data(), defaultValueStr.size() );

	apData->aDefaultValueFloat                         = defaultValue;
	apData->apFunc                                     = callback;

	SetValue( defaultValueStr );
}

// 1 as a size_t
// 1Ui64
constexpr size_t one = 1;

std::string ConVar::GetPrintMessage()
{
	// TODO: make a shared util function for some of this, smh
	std::string msg = vstring( "%s%s %s", UNIX_CLR_DEFAULT, aName, GetValue().data() );

	msg += vstring( " " UNIX_CLR_YELLOW "(%s default)", apData->apDefaultValue );

	if ( aFlags )
	{
		msg += "\n\t" UNIX_CLR_DARK_GREEN;
		// TODO: this could be better probably
		// 3:30 am and very lazy programming here
		for ( size_t i = 0, j = 0; i < gConVarFlags.size(); i++ )
		{
			if ( ( one << i ) > aFlags )
				break;

			if ( !( aFlags & ( one << i ) ) )
				continue;

			if ( j != 0 )
				msg += " | ";

			msg += Con_GetCvarFlagName( ( one << i ) );
			j++;
		}
	}

	if ( aDesc )
		return msg + "\n\t" UNIX_CLR_CYAN + aDesc + UNIX_CLR_DEFAULT "\n\n";

	return msg + UNIX_CLR_DEFAULT "\n";
}


void ConVar::SetValue( std::string_view value )
{
	apData->apValue   = Util_ReallocString( apData->apValue, value.data(), value.size() );
	apData->aValueLen = value.size();

	float newValue = apData->aValueFloat;
	if ( !ToFloat( apData->apValue, newValue ) )
		return;

	apData->aValueFloat = newValue;
}


void ConVar::SetValue( float value )
{
	//if ( apValue )
	//	free( apValue );

	apData->aValueFloat     = value;
	std::string valueStdStr = ToString( value );

	apData->apValue         = Util_ReallocString( apData->apValue, valueStdStr.data(), valueStdStr.size() );
	apData->aValueLen       = valueStdStr.size();
}


void ConVar::Reset()
{
	Init( apData->apDefaultValue, apData->apFunc );
}


const char* ConVar::GetChar() const
{
	return apData->apValue;
}


u32 ConVar::GetValueLen() const
{
	return apData->aValueLen;
}


std::string_view ConVar::GetValue() const
{
	return std::string_view( apData->apValue, apData->aValueLen );
}


float ConVar::GetFloat() const
{
	return apData->aValueFloat;
}


int ConVar::GetInt() const
{
	return (int)apData->aValueFloat;
}


bool ConVar::GetBool() const
{
	return apData->aValueFloat >= 1.f;
}


// amazing, have to put these here or i get some ConVarRef undefined error
// also not const due to the function call
bool    ConVar::operator<=( ConVarRef& other )              { return apData->aValueFloat <= other.GetFloat(); }
bool    ConVar::operator>=( ConVarRef& other )              { return apData->aValueFloat >= other.GetFloat(); }
bool    ConVar::operator==( ConVarRef& other )              { return apData->aValueFloat == other.GetFloat(); }
bool    ConVar::operator!=( ConVarRef& other )              { return apData->aValueFloat != other.GetFloat(); }

float   ConVar::operator*( ConVarRef& other )               { return apData->aValueFloat * other.GetFloat(); }
float   ConVar::operator/( ConVarRef& other )               { return apData->aValueFloat / other.GetFloat(); }
float   ConVar::operator+( ConVarRef& other )               { return apData->aValueFloat + other.GetFloat(); }
float   ConVar::operator-( ConVarRef& other )               { return apData->aValueFloat - other.GetFloat(); }


// ================================================================================


void ConVarRef::Init()
{
	// if this is created later, just search for the convar
	SetReference( Con_GetConVar( aName ) );

	Con_RegisterConVar1( this );
}

void ConVarRef::SetReference( ConVar* ref )
{
	apRef = ref;
	aValid = apRef != nullptr;
}

// should never be called on a ConVarRef really
std::string ConVarRef::GetPrintMessage(  )
{
	if ( apRef )
		return apRef->GetPrintMessage();

	return vstring( "Invalid ConVarRef: %s\n", aName );
}

void ConVarRef::SetValue( const std::string& value )
{
	if ( apRef ) apRef->SetValue( value );
}

void ConVarRef::SetValue( float value )
{
	if ( apRef ) apRef->SetValue( value );
}

std::string_view ConVarRef::GetValue(  )
{
	static std::string empty = "";
	return apRef ? apRef->apData->apValue : empty;
}

float ConVarRef::GetFloat(  )
{
	return apRef ? apRef->apData->aValueFloat : 0.f;
}

bool ConVarRef::GetBool(  )
{
	return apRef ? apRef->GetBool(  ) : false;
}


// uhhh
//bool operator<=( ConVar& left, ConVarRef& other )              { return left.GetFloat() <= other.GetFloat(); }
//bool operator>=( ConVar& left, ConVarRef& other )              { return left.GetFloat() >= other.GetFloat(); }
//bool operator==( ConVar& left, ConVarRef& other )              { return left.GetFloat() == other.GetFloat(); }


// ================================================================================


// int& con_search_behavior = Con_RegisterConVar_RangeInt( "con_search_behavior", "0 - must start with this string, 1 - must contain this string", 0, 0, 1 );


CONVAR( con_search_behavior, 1, "0 - must start with this string, 1 - must contain this string" );


static bool ConVarNameCheck( const char* name, const char* search, size_t size, bool* spStartsWith )
{
	// Must start with string
	bool startsWith = ( ch_strncasecmp( name, search, size ) == 0 );

	if ( startsWith )
	{
		if ( spStartsWith )
			*spStartsWith = true;

		return true;
	}

	if ( con_search_behavior == 0.f )
		return false;

	// See if it contains the string
	if ( strcasestr( name, search ) )
		return true;

	return false;
}


static void SearchConVars( std::vector< std::string >& results, const char* search, size_t size )
{
	std::vector< std::string > resultsStartWith;  // results that the convar name starts with the search
	std::vector< std::string > resultsContain;    // results that contain the string somewhere in in a convar name

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( Con_IsConVarRef( cvar ) )
			continue;

		bool startsWith = false;
		if ( !ConVarNameCheck( cvar->aName, search, size, &startsWith ) )
			continue;

		if ( startsWith )
			resultsStartWith.push_back( cvar->GetName() );
		else
			resultsContain.push_back( cvar->GetName() );
	}

	if ( resultsStartWith.size() )
	{
		results.insert( results.end(), resultsStartWith.begin(), resultsStartWith.end() );
	}

	if ( resultsContain.size() )
	{
		results.insert( results.end(), resultsContain.begin(), resultsContain.end() );
	}
}


static void SearchConVars( std::vector< ConVarBase* >& results, const char* search, size_t size )
{
	std::vector< ConVarBase* > resultsStartWith;  // results that the convar name starts with the search
	std::vector< ConVarBase* > resultsContain;    // results that contain the string somewhere in in a convar name

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( Con_IsConVarRef( cvar ) )
			continue;

		bool startsWith = false;
		if ( !ConVarNameCheck( cvar->aName, search, size, &startsWith ) )
			continue;

		if ( startsWith )
			resultsStartWith.push_back( cvar );
		else
			resultsContain.push_back( cvar );
	}

	if ( resultsStartWith.size() )
	{
		results.insert( results.end(), resultsStartWith.begin(), resultsStartWith.end() );
	}

	if ( resultsContain.size() )
	{
		results.insert( results.end(), resultsContain.begin(), resultsContain.end() );
	}
}


// remove duplicate user inputs from the history
CONVAR( con_remove_dup_input_history, 1 );


// ================================================================================
// ConVar2 System


static auto& Con_GetConVar2Map()
{
	static std::unordered_map< std::string_view, ConVarData_t* > convars;
	return convars;
}


static auto& Con_GetConVar2Desc()
{
	static std::unordered_map< std::string_view, const char* > convars;
	return convars;
}


// return false if the ConVar already exists
// return true if the ConVar was created
bool Con_RegisterConVar_Base( ConVarData_t** conVarDataIn, const char* spName, const char* spDesc, ConVarFlag_t sFlags, EConVarType sType )
{
	auto it = Con_GetConVar2Map().find( spName );

	if ( it != Con_GetConVar2Map().end() )
	{
		ConVarData_t* conVarData = it->second;

		if ( conVarData->aType != sType )
		{
			Log_ErrorF( gConsoleChannel, "ConVar Type Mismatch: %s - %s VS %s\n", spName, Con_ConVarTypeStr( conVarData->aType ), Con_ConVarTypeStr( sType ) );
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
		Log_ErrorF( gConsoleChannel, "Invalid ConVar Type: %s\n", spName );
		return false;
	}

	ConVarData_t* conVarData       = new ConVarData_t;
	conVarData->aFlags             = sFlags;
	conVarData->aType              = sType;

	Con_GetConVar2Map()[ spName ]  = conVarData;
	Con_GetConVar2Desc()[ spName ] = spDesc;

	if ( conVarDataIn )
		*conVarDataIn = conVarData;

	return true;
}


// ------------------------------------------------------------------------------


bool& Con_RegisterConVar_Bool( const char* spName, const char* spDesc, bool sDefault, ConVarFlag_t sFlags, ConVarFunc_Bool* spCallbackFunc )
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


int& Con_RegisterConVar_Int( const char* spName, const char* spDesc, int sDefault, ConVarFlag_t sFlags, ConVarFunc_Int* spCallbackFunc )
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


float& Con_RegisterConVar_Float( const char* spName, const char* spDesc, float sDefault, ConVarFlag_t sFlags, ConVarFunc_Float* spCallbackFunc )
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


char*& Con_RegisterConVar_String( const char* spName, const char* spDesc, const char* spDefault, ConVarFlag_t sFlags, ConVarFunc_String* spCallbackFunc )
{
	ConVarData_t* cvarData = nullptr;
	bool          ret      = Con_RegisterConVar_Base( &cvarData, spName, spDesc, sFlags, EConVarType_String );

	if ( !ret )
	{
		if ( cvarData )
			return *cvarData->aString.apData;

		char* temp = Util_AllocString( spDefault );
		return temp;
	}

	cvarData->aString.apFunc       = spCallbackFunc;
	cvarData->aString.aDefaultData = Util_AllocString( spDefault );

	char* value                    = Util_AllocString( spDefault );

	cvarData->aString.apData       = ch_malloc< char* >( 1 );
	memcpy( cvarData->aString.apData, &value, sizeof( char* ) );

	return *cvarData->aString.apData;
}


glm::vec2& Con_RegisterConVar_Vec2( const char* spName, const char* spDesc, const glm::vec2& srDefault, ConVarFlag_t sFlags, ConVarFunc_Vec2* spCallbackFunc )
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


glm::vec3& Con_RegisterConVar_Vec3( const char* spName, const char* spDesc, const glm::vec3& srDefault, ConVarFlag_t sFlags, ConVarFunc_Vec3* spCallbackFunc )
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


glm::vec4& Con_RegisterConVar_Vec4( const char* spName, const char* spDesc, const glm::vec4& srDefault, ConVarFlag_t sFlags, ConVarFunc_Vec4* spCallbackFunc )
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


int& Con_RegisterConVar_RangeInt( const char* spName, const char* spDesc, int sDefault, int sMin, int sMax, ConVarFlag_t sFlags, ConVarFunc_Int* spCallbackFunc )
{
	// Make sure the default value is within the range
	sDefault = std::clamp( sDefault, sMin, sMax );

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


float& Con_RegisterConVar_RangeFloat( const char* spName, const char* spDesc, float sDefault, float sMin, float sMax, ConVarFlag_t sFlags, ConVarFunc_Float* spCallbackFunc )
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
	cvarData->aRangeFloat.aMin = sMin;
	cvarData->aRangeFloat.aMax         = sMax;

	cvarData->aRangeFloat.apData       = ch_malloc< float >( 1 );
	memcpy( cvarData->aRangeFloat.apData, &sDefault, sizeof( float ) );

	return *cvarData->aRangeFloat.apData;
}


// ------------------------------------------------------------------------------


const char* Con_GetConVarDesc( const char* spName )
{
	auto it = Con_GetConVar2Desc().find( spName );

	if ( it != Con_GetConVar2Desc().end() )
		return it->second;

	Log_ErrorF( gConsoleChannel, "ConVar Description not found: %s\n", spName );
	return nullptr;
}


ConVarData_t* Con_GetConVarData( const char* spName )
{
	auto it = Con_GetConVar2Map().find( spName );

	if ( it != Con_GetConVar2Map().end() )
		return it->second;

	Log_ErrorF( gConsoleChannel, "ConVar Data not found: %s\n", spName );
	return nullptr;
}


ConVarData_t* Con_GetConVarData( std::string_view sName )
{
	auto it = Con_GetConVar2Map().find( sName );

	if ( it != Con_GetConVar2Map().end() )
		return it->second;

	Log_ErrorF( gConsoleChannel, "ConVar Data not found: %s\n", sName.data() );
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
		Log_ErrorF( gConsoleChannel, "Invalid ConVar Type: %d\n", sType );
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

	Log_ErrorF( gConsoleChannel, "Invalid ConVar Type: %s\n", spType );
	return EConVarType_Invalid;
}


std::string Con_GetConVarValueStr( ConVarData_t* cvarData )
{
	switch ( cvarData->aType )
	{
		case EConVarType_Bool:
			return cvarData->aBool.apData ? "true" : "false";

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


std::string Con_GetConVarValueStr( const char* name )
{
	ConVarData_t* cvarData = Con_GetConVarData( name );

	if ( !cvarData )
		return vstring( "ConVar not found: %s\n", name );

	return Con_GetConVarValueStr( cvarData );
}


std::string Con_GetConVarHelp( const char* spName )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
		return vstring( "ConVar not found: %s\n", spName );

	std::string msg = vstring( "%s%s %s", UNIX_CLR_DEFAULT, spName, Con_GetConVarValueStr( cvarData ).data() );

	// Check if the current value differs from the default value
	switch ( cvarData->aType )
	{
		case EConVarType_Bool:
			if ( *cvarData->aBool.apData != cvarData->aBool.aDefaultData )
				msg += vstring( " " UNIX_CLR_YELLOW "(%s default)", cvarData->aBool.aDefaultData ? "true" : "false" );
			break;

		case EConVarType_Int:
			if ( *cvarData->aInt.apData != cvarData->aInt.aDefaultData )
				msg += vstring( " " UNIX_CLR_YELLOW "(%d default)", cvarData->aInt.aDefaultData );
			break;

		case EConVarType_Float:
			if ( *cvarData->aFloat.apData != cvarData->aFloat.aDefaultData )
				msg += vstring( " " UNIX_CLR_YELLOW "(%.6f default)", cvarData->aFloat.aDefaultData );
			break;

		case EConVarType_String:
			if ( ch_strcasecmp( *cvarData->aString.apData, cvarData->aString.aDefaultData ) != 0 )
				msg += vstring( " " UNIX_CLR_YELLOW "(\"%s\" default)", cvarData->aString.aDefaultData );
			break;

		case EConVarType_Vec2:
			if ( *cvarData->aVec2.apData != cvarData->aVec2.aDefaultData )
				msg += vstring( " " UNIX_CLR_YELLOW "(%.6f %.6f default)", cvarData->aVec2.aDefaultData.x, cvarData->aVec2.aDefaultData.y );
			break;

		case EConVarType_Vec3:
			if ( *cvarData->aVec3.apData != cvarData->aVec3.aDefaultData )
				msg += vstring( " " UNIX_CLR_YELLOW "(%.6f %.6f %.6f default)", cvarData->aVec3.aDefaultData.x, cvarData->aVec3.aDefaultData.y, cvarData->aVec3.aDefaultData.z );
			break;

		case EConVarType_Vec4:
			if ( *cvarData->aVec4.apData != cvarData->aVec4.aDefaultData )
				msg += vstring( " " UNIX_CLR_YELLOW "(%.6f %.6f %.6f %.6f default)", cvarData->aVec4.aDefaultData.x, cvarData->aVec4.aDefaultData.y, cvarData->aVec4.aDefaultData.z, cvarData->aVec4.aDefaultData.w );
			break;

		case EConVarType_RangeInt:
		{
			if ( *cvarData->aRangeInt.apData != cvarData->aRangeInt.aDefaultData )
				msg += vstring( " " UNIX_CLR_YELLOW "(%d default)", cvarData->aRangeInt.aDefaultData );

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
				msg += vstring( " " UNIX_CLR_YELLOW "(%.6f default)", cvarData->aRangeFloat.aDefaultData );

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

	if ( cvarData->aFlags )
	{
		msg += "\n\t" UNIX_CLR_DARK_GREEN;
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
	const char* desc = Con_GetConVarDesc( spName );

	if ( desc )
	{
		msg += vstring( "\n\t" UNIX_CLR_CYAN "%s" UNIX_CLR_DEFAULT "\n\n", desc );
		return msg;
	}

	return msg + UNIX_CLR_DEFAULT "\n";
}


// ------------------------------------------------------------------------------


int Con_SetConVarValueInternal_Bool( ConVarData_t* cvarData, const char* spName, bool sValue )
{
	if ( cvarData->aType != EConVarType_Bool )
	{
		Log_ErrorF( gConsoleChannel, "ConVar Type Mismatch for %s, got \"Bool\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}
	
	bool* value = cvarData->aBool.apData;
	*value = sValue;
	return 1;
}


int Con_SetConVarValueInternal_Int( ConVarData_t* cvarData, const char* spName, int sValue )
{
	if ( cvarData->aType != EConVarType_Int || cvarData->aType != EConVarType_RangeInt )
	{
		Log_ErrorF( gConsoleChannel, "ConVar Type Mismatch for %s, got \"Int\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	if ( cvarData->aType == EConVarType_RangeInt )
	{
		if ( sValue < cvarData->aRangeInt.aMin || sValue > cvarData->aRangeInt.aMax )
		{
			Log_ErrorF( gConsoleChannel, "ConVar Value out of Range for %s, got %d, must be within %d - %d\n", spName, sValue, cvarData->aRangeInt.aMin, cvarData->aRangeInt.aMax );
			sValue = std::clamp( sValue, cvarData->aRangeInt.aMin, cvarData->aRangeInt.aMax );
		}
	}

	int* value = cvarData->aInt.apData;
	*value     = sValue;
	return 1;
}


int Con_SetConVarValueInternal_Float( ConVarData_t* cvarData, const char* spName, float sValue )
{
	// check if the convar type is float or ranged
	if ( cvarData->aType != EConVarType_Float || cvarData->aType != EConVarType_RangeFloat )
	{
		Log_ErrorF( gConsoleChannel, "ConVar Type Mismatch for %s, got \"Float\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	if ( cvarData->aType == EConVarType_RangeFloat )
	{
		if ( sValue < cvarData->aRangeFloat.aMin || sValue > cvarData->aRangeFloat.aMax )
		{
			Log_ErrorF( gConsoleChannel, "ConVar Value out of Range for %s, got %.6f, must be within %.6f - %.6f\n", spName, sValue, cvarData->aRangeFloat.aMin, cvarData->aRangeFloat.aMax );
			sValue = std::clamp( sValue, cvarData->aRangeFloat.aMin, cvarData->aRangeFloat.aMax );
		}
	}

	float* value = cvarData->aFloat.apData;
	*value       = sValue;
	return 1;
}


int Con_SetConVarValueInternal_String( ConVarData_t* cvarData, const char* spName, char* spValue )
{
	if ( cvarData->aType != EConVarType_String )
	{
		Log_ErrorF( gConsoleChannel, "ConVar Type Mismatch for %s, got \"String\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	char** value = cvarData->aString.apData;
	char*  newValue = Util_ReallocString( *value, spValue );

	if ( newValue == nullptr )
	{
		Log_ErrorF( gConsoleChannel, "Failed to set ConVar Value for %s\n", spName );
		return 0;
	}

	*value = newValue;
	return 1;
}


int Con_SetConVarValueInternal_String( ConVarData_t* cvarData, const char* spName, std::string_view sValue )
{
	if ( cvarData->aType != EConVarType_String )
	{
		Log_ErrorF( gConsoleChannel, "ConVar Type Mismatch for %s, got \"String\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	char** value = cvarData->aString.apData;
	char* newValue = Util_ReallocString( *value, sValue.data(), sValue.size() );

	if ( newValue == nullptr )
	{
		Log_ErrorF( gConsoleChannel, "Failed to set ConVar Value for %s\n", spName );
		return 0;
	}

	*value = newValue;
	return 1;
}


int Con_SetConVarValueInternal_Vec( ConVarData_t* cvarData, const char* spName, const glm::vec2& srValue )
{
	if ( cvarData->aType != EConVarType_Vec2 )
	{
		Log_ErrorF( gConsoleChannel, "ConVar Type Mismatch for %s, got \"Vec2\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	glm::vec2* value = cvarData->aVec2.apData;
	value->x         = srValue.x;
	value->y         = srValue.y;
	return 1;
}


int Con_SetConVarValueInternal_Vec( ConVarData_t* cvarData, const char* spName, const glm::vec3& srValue )
{
	if ( cvarData->aType != EConVarType_Vec3 )
	{
		Log_ErrorF( gConsoleChannel, "ConVar Type Mismatch for %s, got \"Vec3\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	glm::vec3* value = cvarData->aVec3.apData;
	value->x         = srValue.x;
	value->y         = srValue.y;
	value->z         = srValue.z;
	return 1;
}


int Con_SetConVarValueInternal_Vec( ConVarData_t* cvarData, const char* spName, const glm::vec4& srValue )
{
	if ( cvarData->aType != EConVarType_Vec4 )
	{
		Log_ErrorF( gConsoleChannel, "ConVar Type Mismatch for %s, got \"Vec4\", expected \"%s\"\n", spName, Con_ConVarTypeStr( cvarData->aType ) );
		return 0;
	}

	glm::vec4* value = cvarData->aVec4.apData;
	value->x         = srValue.x;
	value->y         = srValue.y;
	value->z         = srValue.z;
	value->w         = srValue.w;
	return 1;
}


// ------------------------------------------------------------------------------


int Con_SetConVarValue( const char* spName, bool sValue )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
		return -1;

	bool prevValue = *cvarData->aBool.apData;

	int ret = Con_SetConVarValueInternal_Bool( cvarData, spName, sValue );

	if ( ret == 1 )
	{
		if ( cvarData->aBool.apFunc )
			cvarData->aBool.apFunc( prevValue, *cvarData->aBool.apData );
	}

	return ret;
}


int Con_SetConVarValue( const char* spName, int sValue )
{
	ConVarData_t* cvarData = Con_GetConVarData( spName );

	if ( !cvarData )
		return -1;

	int prevValue = *cvarData->aInt.apData;

	int ret = Con_SetConVarValueInternal_Int( cvarData, spName, sValue );

	if ( ret == 1 )
	{
		if ( cvarData->aInt.apFunc )
			cvarData->aInt.apFunc( prevValue, *cvarData->aInt.apData );
	}

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


#if 0
bool& Con_RegisterConVarRef_Bool( const char* spName, bool sFallback )
{
}


int& Con_RegisterConVarRef_Int( const char* spName, int sFallback )
{
}


float& Con_RegisterConVarRef_Float( const char* spName, float sFallback )
{
}


const char*& Con_RegisterConVarRef_Str( const char* spName, const char* spFallback )
{
}


float& Con_RegisterConVarRef_Range( const char* spName, float sFallback )
{
}
#endif


// ================================================================================


void Con_AddToCommandHistory( const std::string &srCmd )
{
	if ( srCmd.empty() )
		return;

	if ( con_remove_dup_input_history )
	{
		vec_remove_if( gCommandHistory, srCmd );
		gCommandHistory.push_back( srCmd );
	}
	else
	{
		if ( gCommandHistory.empty() || gCommandHistory.back() != srCmd )
			gCommandHistory.push_back( srCmd );
	}
}


void Con_CheckInstantCommands( const std::string &srCmd )
{
	for ( size_t i = 0; i < gInstantConVars.size(); i++ )
	{
		if ( gInstantConVars[i]->GetName() == srCmd )
			gInstantConVars[i]->apFunc( {} );
	}
}


void Con_QueueCommand( const std::string &srCmd )
{
	Con_CheckInstantCommands( srCmd );

	gQueue.push_back( srCmd );

	Con_AddToCommandHistory( srCmd );

	Log_Ex( gConsoleChannel, LogType::Input, srCmd.c_str() );
}


void Con_QueueCommandSilent( const std::string &srCmd, bool sAddToHistory )
{
	if ( srCmd.empty() )
		return;

	Con_CheckInstantCommands( srCmd );

	gQueue.push_back( srCmd );

	if ( sAddToHistory )
		Con_AddToCommandHistory( srCmd );
}


// TODO: rethink this function
void Con_RegisterConVars()
{
	PROF_SCOPE();

	// static bool registered = false;

	//if ( registered )
	//	return;

	std::vector< ConVarRef* > cvarRefList;

	gInstantConVars.clear();

	for ( ConVarBase* current : Con_GetConVars() )
	{
		if ( typeid(*current) == typeid(ConVarRef) )
		{
			ConVarRef* cvarRef = static_cast<ConVarRef*>(current);

			if ( cvarRef->apRef == nullptr )
				cvarRefList.push_back( cvarRef );
		}

		if ( current->GetFlags() & CVARF_INSTANT && typeid(*current) == typeid(ConCommand) )
			gInstantConVars.push_back( static_cast<ConCommand*>(current) );
	}

	// Now link all cvar references
	for ( ConVarRef* cvarRef: cvarRefList )
		cvarRef->SetReference( Con_GetConVar( cvarRef->GetName() ) );

	// registered = true;
}


const std::vector< std::string >& Con_GetCommandHistory()
{
	return gCommandHistory;
}


uint32_t Con_GetConVarCount()
{
	return Con_GetConVars().size();
}


ConVarBase* Con_GetConVar( uint32_t sIndex )
{
	if ( sIndex >= Con_GetConVars().size() )
		return nullptr;

	return Con_GetConVars()[ sIndex ];
}


ConVar* Con_GetConVar( std::string_view name )
{
	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( typeid( *cvar ) != typeid( ConVar ) )
			continue;

		if ( cvar->GetName() == name )
			return static_cast< ConVar* >( cvar );
	}

	return nullptr;
}


ConVarBase* Con_GetConVarBase( std::string_view name )
{
	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( cvar->GetName() == name )
			return cvar;
	}

	return nullptr;
}


ConVarData* Con_CreateConVarData()
{
	ConVarData* cvarData = new ConVarData;
	gConVarData.push_back( cvarData );
	return cvarData;
}


void Con_FreeConVarData( ConVarData* spData )
{
	if ( !spData )
		return;

	vec_remove( gConVarData, spData );
	delete spData;
}


static std::string g_strEmpty = "";


std::string_view Con_GetConVarValue( std::string_view name )
{
	if ( ConVar* convar = Con_GetConVar( name ) )
		return convar->GetValue();
	
	return g_strEmpty;
}


float Con_GetConVarFloat( std::string_view name )
{
	if ( ConVar* convar = Con_GetConVar( name ) )
		return convar->GetFloat();
	
	return 0.f;
}


void Con_PrintAllConVars()
{
	std::vector< std::string > ConVarMsgs;
	std::vector< std::string > ConCommandMsgs;

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( typeid(*cvar) == typeid(ConVar)  )
			ConVarMsgs.push_back( cvar->GetPrintMessage() );

		else if ( typeid(*cvar) == typeid(ConCommand) )
			ConCommandMsgs.push_back( cvar->GetPrintMessage() );
	}

	LogGroup group = Log_GroupBegin( gConsoleChannel );
	Log_Group( group, "\nConVars:\n--------------------------------------\n" );
	for ( const auto& msg : ConVarMsgs )
		Log_Group( group, msg.c_str() );

	Log_Group( group, "--------------------------------------\nConCommands:\n--------------------------------------\n" );
	for ( const auto& msg : ConCommandMsgs )
		Log_Group( group, msg.c_str() );

	Log_Group( group, "--------------------------------------\n" );
	Log_GroupEnd( group );
}


// ConVarBase* Con_CheckForConVarRef( ConVarBase* cvar )
// {
// 	if ( typeid(*cvar) == typeid(ConVarRef) )
// 	{
// 		ConVarRef* cvarRef = static_cast<ConVarRef*>(cvar);
// 
// 		if ( cvarRef->apRef == nullptr )
// 		{
// 			Log_WarnF( gConsoleChannel, "Found unlinked cvar ref: %s\n", cvarRef->GetName() );
// 			return nullptr;
// 		}
// 
// 		return cvarRef->apRef;
// 	}
// 
// 	return cvar;
// }


void Con_BuildAutoCompleteList( const std::string& srSearch, std::vector< std::string >& srResults )
{
	PROF_SCOPE();

	if ( srSearch.empty() )
		return;

	std::string commandName;
	std::string fullCommand;
	std::vector< std::string > args;
	Con_ParseCommandLine( srSearch, commandName, args, fullCommand );

	str_lower( commandName );

#if 0
	std::vector< std::string > resultsStartWith;  // results that the convar name starts with the search
	std::vector< std::string > resultsContain;    // results that contain the string somewhere in in a convar name

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( Con_IsConVarRef( cvar ) )
			continue;

		bool startsWithString = false;

		// this SHOULD be fine, if the convar doesn't exist in here, something is very wrong
		if ( !ConVarNameCheck( cvar->aName, commandName.c_str(), commandName.size(), &startsWithString ) )
			continue;

		size_t cvarNameLen = strlen( cvar->aName );
		
		// if ( args.empty() )
		if ( cvarNameLen >= srSearch.size() && commandName.size() >= srSearch.size() )
		{
			if ( startsWithString )
				resultsStartWith.push_back( cvar->aName );
			else
				resultsContain.push_back( cvar->aName );

			continue;
		}

		// make sure this actually matches
		if ( cvarNameLen != commandName.size() || !ConVarNameCheck( cvar->aName, commandName.c_str(), cvarNameLen, nullptr ) )
			continue;

		// is this a concommand with a drop down function?
		if ( typeid(*cvar) == typeid(ConCommand) )
		{
			auto cmd = static_cast<ConCommand*>(cvar);

			if ( cmd->apDropDownFunc )
			{
				std::vector< std::string > dropDownArgs;
				cmd->apDropDownFunc( args, dropDownArgs );

				for ( auto dropArg: dropDownArgs )
				{
					resultsStartWith.push_back( vstring( "%s %s", cvar->aName, dropArg.c_str() ) );
				}

				break;
			}
		}

		resultsStartWith.push_back( cvar->aName );
		break;
	}

	if ( resultsStartWith.size() )
	{
		srResults.insert( srResults.end(), resultsStartWith.begin(), resultsStartWith.end() );
	}

	if ( resultsContain.size() )
	{
		srResults.insert( srResults.end(), resultsContain.begin(), resultsContain.end() );
	}

#else

	std::vector< ConVarBase* > results;
	SearchConVars( results, commandName.data(), commandName.size() );

	if ( results.empty() )
		return;

	for ( ConVarBase* cvar : results )
	{
		size_t cvarNameLen = strlen( cvar->aName );
		
		// if ( args.empty() )
		if ( cvarNameLen >= srSearch.size() && commandName.size() >= srSearch.size() )
		{
			srResults.push_back( cvar->aName );
			continue;
		}

		// make sure this actually matches
		if ( cvarNameLen != commandName.size() || !ConVarNameCheck( cvar->aName, commandName.c_str(), cvarNameLen, nullptr ) )
			continue;

		// is this a concommand with a drop down function?
		if ( typeid(*cvar) == typeid(ConCommand) )
		{
			auto cmd = static_cast<ConCommand*>(cvar);

			if ( cmd->apDropDownFunc )
			{
				std::vector< std::string > dropDownArgs;
				cmd->apDropDownFunc( args, dropDownArgs );

				for ( auto dropArg: dropDownArgs )
				{
					srResults.push_back( vstring( "%s %s", cvar->aName, dropArg.c_str() ) );
				}

				break;
			}
		}

		srResults.push_back( cvar->aName );
		break;
	}

#endif
}


void Con_Update()
{
	PROF_SCOPE();

	static bool init = false;

	if ( !init )
	{
		// TODO: rethink this stupid thing
		// also call again if convar count changes
		Con_RegisterConVars();
		init = true;
	}

	// commands could queue new commands to be run next update, so don't try to run those now
	size_t queueSize = gQueue.size();
	for ( size_t i = 0; i < queueSize; i++ )
	{
		Con_RunCommand( gQueue[ 0 ] );
		vec_remove_index( gQueue, 0 );
	}

	// clear the queue, but make sure to not remove any new commands added to the queue from commands just ran
	// for ( size_t i = 0; i < queueSize; i++ )
	// {
	// 	vec_remove_index( gQueue, 0 );
	// }
}


void Con_RunCommand( std::string_view command )
{
	PROF_SCOPE();

	std::string commandName;
	std::string fullCommand;
	std::vector< std::string > args;

	for ( size_t i = 0; i < command.size(); i++ )
	{
		commandName.clear();
		args.clear();

		Con_ParseCommandLineEx( command, commandName, args, fullCommand, i );
		str_lower( commandName );

		Con_RunCommandArgs( commandName, args, fullCommand );
	}
}


bool Con_RunCommandBase( ConVarBase* cvar, const std::vector< std::string >& args )
{
	bool commandCalled = false;

	// Check Flag Callbacks
	if ( cvar->aFlags )
	{
		for ( auto& data : gConVarFlags )
		{
			if ( !( cvar->aFlags & data.flag ) )
				continue;

			// Can we execute these arguments on the ConVar/ConCommand?
			if ( data.apCallback && !data.apCallback( cvar, args ) )
			{
				return true;  // HACK: RETURN TRUE IN THIS CASE
			}
		}
	}

	if ( typeid( *cvar ) == typeid( ConVar ) )
	{
		ConVar* convar = static_cast< ConVar* >( cvar );
		commandCalled  = true;

		if ( !args.empty() )
		{
			if ( convar->apData->apFunc )
			{
				std::string prevString = convar->GetValue().data();
				float       prevFloat  = convar->apData->aValueFloat;

				convar->SetValue( args[ 0 ] );
				convar->apData->apFunc( prevString.data(), prevFloat, args );
			}
			else
			{
				convar->SetValue( args[ 0 ] );
			}
		}
		else
		{
			Log_Msg( gConsoleChannel, convar->GetPrintMessage().c_str() );
		}
	}
	else if ( typeid( *cvar ) == typeid( ConCommand ) )
	{
		ConCommand* cmd = static_cast< ConCommand* >( cvar );
		commandCalled   = true;
		cmd->apFunc( args );
	}

	return commandCalled;
}


template< typename VEC >
bool ParseVector( const char* spName, const std::vector< std::string >& args, VEC& srVector )
{
	if ( args.size() < srVector.length() )
	{
		Log_ErrorF( gConsoleChannel, "ConVar \"%s\", Type of \"Vec2\" requires 2 arguments\n", spName );
		return false;
	}

	for ( size_t i = 0; i < args.size(); i++ )
	{
		if ( !ToFloat( args[ i ].data(), srVector[ i ] ) )
		{
			Log_ErrorF( gConsoleChannel, "ConVar \"%s\", Invalid argument %d \"%s\" for Vector type, expected a number\n", spName, i, args[ i ].data() );
			return false;
		}
	}

	return true;
}


bool Con_RunCommandBaseV2( ConVarData_t* cvar, const char* name, const std::vector< std::string >& args, std::string_view fullCommand )
{
	bool commandCalled = false;

	// Check Flag Callbacks
	if ( cvar->aFlags )
	{
		for ( auto& data : gConVarFlags )
		{
			if ( !( cvar->aFlags & data.flag ) )
				continue;

			// Can we execute these arguments on the ConVar/ConCommand?
			if ( data.apCallback2 && !data.apCallback2( cvar, args ) )
			{
				return true;  // HACK: RETURN TRUE IN THIS CASE
			}
		}
	}

	commandCalled  = true;

	if ( args.size() )
	{
		switch ( cvar->aType )
		{
			case EConVarType_Bool:
			{
				// check for true, false, t, f, yes, no, yeah, nah, y, or n
				if ( args[ 0 ].size() == 4 && ch_strncasecmp( args[ 0 ].data(), "true", 4 ) == 0 )
				{
					Con_SetConVarValueInternal_Bool( cvar, name, true );
				}
				else if ( args[ 0 ].size() == 5 && ch_strncasecmp( args[ 0 ].data(), "false", 5 ) == 0 )
				{
					Con_SetConVarValueInternal_Bool( cvar, name, false );
				}
				else if ( args[ 0 ].size() == 3 && ch_strncasecmp( args[ 0 ].data(), "yes", 3 ) == 0 )
				{
					Con_SetConVarValueInternal_Bool( cvar, name, true );
				}
				else if ( args[ 0 ].size() == 2 && ch_strncasecmp( args[ 0 ].data(), "no", 2 ) == 0 )
				{
					Con_SetConVarValueInternal_Bool( cvar, name, false );
				}
				else if ( args[ 0 ].size() == 4 && ch_strncasecmp( args[ 0 ].data(), "yeah", 4 ) == 0 )
				{
					Con_SetConVarValueInternal_Bool( cvar, name, true );
				}
				else if ( args[ 0 ].size() == 3 && ch_strncasecmp( args[ 0 ].data(), "nah", 3 ) == 0 )
				{
					Con_SetConVarValueInternal_Bool( cvar, name, false );
				}
				else if ( args[ 0 ].size() == 1 && ch_strncasecmp( args[ 0 ].data(), "t", 1 ) == 0 )
				{
					Con_SetConVarValueInternal_Bool( cvar, name, true );
				}
				else if ( args[ 0 ].size() == 1 && ch_strncasecmp( args[ 0 ].data(), "f", 1 ) == 0 )
				{
					Con_SetConVarValueInternal_Bool( cvar, name, false );
				}
				else if ( args[ 0 ].size() == 1 && ch_strncasecmp( args[ 0 ].data(), "y", 1 ) == 0 )
				{
					Con_SetConVarValueInternal_Bool( cvar, name, true );
				}
				else if ( args[ 0 ].size() == 1 && ch_strncasecmp( args[ 0 ].data(), "n", 1 ) == 0 )
				{
					Con_SetConVarValueInternal_Bool( cvar, name, false );
				}
				else
				{
					long value = 0;
					if ( !ToLong3( args[ 0 ].data(), value ) )
					{
						Log_ErrorF( gConsoleChannel, "ConVar \"%s\", Invalid argument \"%s\" for Bool type, expected a number, true, false, yes, no, yeah, nah, t, f, y, or n\n", name, args[ 0 ].data() );
						break;
					}

					Con_SetConVarValueInternal_Bool( cvar, name, value > 0 );
				}
				break;
			}
			case EConVarType_Int:
			case EConVarType_RangeInt:
			{
				long value = 0;
				if ( !ToLong3( args[ 0 ].data(), value ) )
				{
					Log_ErrorF( gConsoleChannel, "ConVar \"%s\", Invalid argument \"%s\" for Integer type, expected a number\n", name, args[ 0 ].data() );
					break;
				}

				Con_SetConVarValueInternal_Int( cvar, name, (int)value );
				break;
			}
			case EConVarType_Float:
			case EConVarType_RangeFloat:
			{
				float value = 0.f;
				if ( !ToFloat( args[ 0 ].data(), value ) )
				{
					Log_ErrorF( gConsoleChannel, "ConVar \"%s\", Invalid argument \"%s\" for Float type, expected a number\n", name, args[ 0 ].data() );
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
	}
	else
	{
		const std::string& help = Con_GetConVarHelp( name );
		Log_Msg( gConsoleChannel, help.data() );
	}

	return commandCalled;
}


bool Con_RunCommandArgs( const std::string& name, const std::vector< std::string >& args, std::string_view fullCommand )
{
	PROF_SCOPE();

	bool commandCalled = false;

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		size_t cvarNameLen = strlen( cvar->aName );

		if ( name.size() != cvarNameLen )
			continue;

		if ( ch_strncasecmp( cvar->aName, name.c_str(), cvarNameLen ) != 0 )
			continue;

		if ( Con_IsConVarRef( cvar ) )
			continue;

		commandCalled = Con_RunCommandBase( cvar, args );
		break;
	}

	// ConVar2 System - TODO: STORE THE CVAR NAMES AS LOWER CASE AND HASH SEARCH THEM
	for ( auto& [ cvarName, cvarData ] : Con_GetConVar2Map() )
	{
		if ( name.size() != cvarName.size() )
			continue;

		if ( ch_strncasecmp( cvarName.data(), name.c_str(), cvarName.size() ) != 0 )
			continue;

		commandCalled = Con_RunCommandBaseV2( cvarData, name.data(), args, fullCommand );
		break;
	}

	// command wasn't used?
	if ( !commandCalled )
		Log_WarnF( gConsoleChannel, "Command \"%s\" is undefined\n", name.c_str() );

	return commandCalled;
}


bool Con_RunCommandArgs( const std::string& name, const std::vector< std::string >& args )
{
	// Join args together as one string with spaces
	std::string fullCommand;

	for ( size_t i = 0; i < args.size(); i++ )
	{
		fullCommand += args[ i ];

		if ( i + 1 < args.size() )
			fullCommand += " ";
	}

	return Con_RunCommandArgs( name, args, fullCommand );
}


void Con_ParseCommandLine( std::string_view command, std::string& name, std::vector< std::string >& args, std::string& fullCommand )
{
	size_t i = 0;
	Con_ParseCommandLineEx( command, name, args, fullCommand, i );
}


void Con_ParseCommandLineEx( std::string_view command, std::string& name, std::vector< std::string >& args, std::string& fullCommand, size_t& i )
{
	PROF_SCOPE();

	std::string curArg;

	for ( ; i < command.size(); i++ )
	{
		if ( name.size() )
		{
			if ( command[ i ] == ' ' || command[ i ] == ';' )
				break;

			name += command[i];
		}
		else if ( command[ i ] == ' ' || command[ i ] == ';' )
		{
			continue;
		}
		else
		{
			name += command[i];
		}
	}

	i++;

	for ( ; i < command.size(); i++ )
	{
		char ch = command[i];

#ifdef _WIN32
		if ( ch == '\r' )
			continue;
#endif

		fullCommand += ch;

		if ( ch == '\n' || ch == ';' )
			break;

		// if a space, add
		if ( ch == ' ' )
		{
			if ( curArg.size() )
				args.push_back( curArg );

			curArg = "";
		}

		// are we entering a quote?
		else if ( ch == '"' || ch == '\'' )
		{
			char q = ch;

			for ( i++; i < command.size(); i++ )
			{
				if ( command[i] == q )
					break;
				else
					curArg += command[i];
			}

			if ( curArg.size() )
				args.push_back( curArg );

			curArg = "";
		}
		else
		{
			curArg += ch;
		}
	}

	if ( curArg.size() )
		args.push_back( curArg );
}


ConVarFlag_t Con_CreateCvarFlag( const char* name )
{
	// make sure this exists first
	if ( ConVarFlag_t flag = Con_GetCvarFlag( name ) )
	{
		return flag;
	}

	ConVarFlag_t newBitShift = (1 << gConVarFlags.size());
	size_t len = strlen( name );
	gConVarFlags.emplace_back( newBitShift, name, len );
	return newBitShift;
}

const char* Con_GetCvarFlagName( ConVarFlag_t flag )
{
	for ( auto& data : gConVarFlags )
	{
		if ( data.flag == flag )
			return data.name;
	}

	return nullptr;
}

ConVarFlag_t Con_GetCvarFlag( const char* name )
{
	size_t len = strlen( name );

	for ( auto& data : gConVarFlags )
	{
		if ( data.nameLen != len )
			continue;

		if ( strncmp( data.name, name, len ) == 0 )
			return data.flag;
	}

	return CVARF_NONE;
}

size_t Con_GetCvarFlagCount()
{
	return gConVarFlags.size();
}


void Con_SetCvarFlagCallback( ConVarFlag_t sFlag, ConVarFlagChangeFunc* spCallback )
{
	for ( auto& data : gConVarFlags )
	{
		if ( data.flag == sFlag )
			data.apCallback = spCallback;
	}
}


ConVarFlagChangeFunc* Con_GetCvarFlagCallback( ConVarFlag_t sFlag )
{
	for ( auto& data : gConVarFlags )
	{
		if ( data.flag == sFlag )
			return data.apCallback;
	}

	return {};
}


void Con_Shutdown()
{
	gArchiveCallbacks.clear();

	// for ( ConVarBase* current : Con_GetConVars() )
	// {
	// 	if ( typeid( *current ) == typeid( ConVar ) )
	// 	{
	// 		// mark data as nullptr
	// 		ConVar* cvar = static_cast< ConVar* >( current );
	// 		cvar->apData = nullptr;
	// 	}
	// }

	// free convar data
	for ( u32 i = 0; i < gConVarData.size(); i++ )
	{
		delete gConVarData[ i ];
	}

	gConVarData.clear();
}


// Add a callback function to add data to the config.cfg file
void Con_AddArchiveCallback( FArchive* sFunc )
{
	gArchiveCallbacks.push_back( sFunc );
}


void Con_Archive( const char* spFile )
{
	std::string output =
	  "// -----------------------------------------------------\n"
	  "// Auto Generated File by Chocolate Engine\n\n";

	// Write all ConVars marked with CVARF_ARCHIVE to this file
	for ( ConVarBase* command : Con_GetConVars() )
	{
		if ( typeid( *command ) != typeid( ConVar ) )
			continue;

		ConVar* cvar = static_cast< ConVar* >( command );

		if ( cvar->aFlags & CVARF_ARCHIVE )
			output += vstring( "%s %s\n", cvar->aName, cvar->GetValue().data() );
	}

	// Run all callback functons on this
	for ( auto& callback : gArchiveCallbacks )
	{
		output += "\n// -----------------------------------------------------\n";
		callback( output );
	}

	std::string filename;
	if ( spFile )
	{
		filename = spFile;
		if ( FileSys_GetFileExt( filename ) != "cfg" )
		{
			filename += ".cfg";
		}
		filename.insert( 0, "cfg" PATH_SEP_STR );
	}
	else
	{
		filename = gConArchiveFile;
	}

	// TODO: have a better way to check for a parent path
	fs::path filenamePath = filename.c_str();
	
	fs::path parentPath   = filenamePath.parent_path();

	if ( !parentPath.empty() )
	{
		FileSys_CreateDirectory( parentPath.string() );
	}

	// Write the data
	FILE* fp = fopen( filename.c_str(), "wb" );

	if ( fp == nullptr )
	{
		Log_ErrorF( gConsoleChannel, "Failed to open file handle: \"%s\"\n", filename.c_str() );
		return;
	}

	fwrite( output.c_str(), sizeof( char ), output.size(), fp );
	fclose( fp );

	Log_DevF( gConsoleChannel, 1, "Wrote Config to File: \"%s\"\n", filename.c_str() );
}


// Set Default Console Archive File
void Con_SetDefaultArchive( const char* spFile, const char* spDefaultFile )
{
	if ( !spFile )
	{
		gConArchiveFile = CON_ARCHIVE_FILE;
	}
	else
	{
		gConArchiveFile = spFile;

		if ( !gConArchiveFile.starts_with( "cfg/" )
#ifdef _WIN32
		     && !gConArchiveFile.starts_with( "cfg\\" )
#endif
		)
			gConArchiveFile = "cfg" PATH_SEP_STR + gConArchiveFile;
	}

	if ( !spDefaultFile )
	{
		gConArchiveDefault = CON_ARCHIVE_DEFAULT;
	}
	else
	{
		gConArchiveDefault = spDefaultFile;

		if ( !gConArchiveDefault.starts_with( "cfg/" )
#ifdef _WIN32
		     && !gConArchiveDefault.starts_with( "cfg\\" )
#endif
		)
			gConArchiveDefault = "cfg" PATH_SEP_STR + gConArchiveDefault;
	}


}


// ================================================================================
// Console ConCommands


void exec_dropdown(
	const std::vector< std::string >& args,  // arguments currently typed in by the user
	std::vector< std::string >& results )      // results to populate the dropdown list with
{
	for ( const auto& file : FileSys_ScanDir( "cfg", ReadDir_AllPaths | ReadDir_Recursive ) )
	{
		if ( file.ends_with( ".." ) )
			continue;

		std::string execName = FileSys_GetFileName( file );

		if ( args.size() && !execName.starts_with( args[0] ) )
			continue;

		results.push_back( execName );
	}
}


CONCMD_DROP_VA( exec, exec_dropdown, 0, "Execute a script full of console commands" )
{
	if ( args.size() == 0 )
	{
		Log_Msg( gConsoleChannel, "No Path Specified for exec!\n" );
		return;
	}

	std::string path;

	if ( args[ 0 ].starts_with( "cfg/" )
#ifdef _WIN32
	     || args[ 0 ].starts_with( "cfg\\" )
#endif
	|| FileSys_IsAbsolute( args[ 0 ].c_str() ) )
	{
		path = args[ 0 ];
	}
	else
	{
		path = "cfg" CH_PATH_SEP_STR + args[ 0 ];
	}

	if ( !FileSys_IsFile( path ) )
	{
		if ( !path.ends_with( ".cfg" ) )
			path += ".cfg";
	}

	if ( !FileSys_IsFile( path ) )
	{
		Log_WarnF( gConsoleChannel, "File does not exist: \"%s\"\n", path.c_str() );
		return;
	}

	std::ifstream fileStream = std::ifstream(path, std::ios::in | std::ios::binary | std::ios::ate);

	if ( !fileStream.is_open() )
	{
		Log_ErrorF( gConsoleChannel, "Failed to open file for exec: \"%s\"\n", path.c_str() );
		return;
	}

	int fileLen = fileStream.tellg();
	fileStream.seekg(0, fileStream.beg);

	char* buf = new char[fileLen];
	fileStream.read(buf, fileLen);
	fileStream.close();

	int ch = 0;
	bool inComment = false;
	std::string line;

	// parse it
	while ( ch < fileLen )
	{
#ifdef _WIN32
		if ( buf[ch] == '\r' )
		{
			ch++;
			continue;
		}
		else
#endif
		if ( buf[ch] == '\n' || buf[ch] == ';' )
		{
			if ( line != "" )
			{
				if ( line == "exec " + args[0] )
					Log_Warn( gConsoleChannel, "cfg file trying to exec itself and cause infinite recursion\n" );
				else
					Con_RunCommand( line );

				line = "";
			}

			inComment = false;
		}
		// check for a comment
		else if ( buf[ch] == '/' && ch + 1 < fileLen && buf[ch + 1] == '/' )
		{
			inComment = true;
			ch++;
		}
		else if ( !inComment )
		{
			line += buf[ch];
		}

		ch++;
	}

	if ( line != "" )
		Con_RunCommand( line );

	delete[] buf;
}


CONCMD_VA( echo, "Print a string to the console" )
{
	// std::string msg = "";
	auto p = args.begin();
	std::string msg = *p++;

	if (p == args.end() - 1)
		msg += " ";

	// for (auto p = args.begin(); p != args.end(); p)
	for (; p != args.end(); ++p)
	{
		msg += *p;

		if (p == args.end() - 1)
			msg += " ";
	}

	msg += "\n";

	Log_Msg( gConsoleChannel, msg.c_str() );
}


void help_dropdown(
	const std::vector< std::string >& args,  // arguments currently typed in by the user
	std::vector< std::string >& results )      // results to populate the dropdown list with
{
	std::string name = args.empty() ? "" : str_lower2(args[0]);
	SearchConVars( results, name.data(), name.size() );
}


CONCMD_DROP_VA( help, help_dropdown, 0, "If no args specified, Print all Registered ConVars, otherwise, print information about this convar" )
{
	if ( args.empty() )
	{
		Con_PrintAllConVars();
		Args_PrintRegistered();

		Log_Msg( gConsoleChannel, "--------------------------------------\n" );
		return;
	}

	ConVarBase* cvar = Con_GetConVarBase( args[0] );

	if ( cvar )
	{
		Log_Msg( gConsoleChannel, cvar->GetPrintMessage().c_str() );
	}
	else
	{
		for ( u32 i = 0; i < Args_GetRegisteredCount(); i++ )
		{
			const Arg_t* arg = Args_GetRegisteredData( i );

			for ( u32 n = 0; n < arg->aNames.size(); n++ )
			{
				if ( arg->aNames[ n ] == args[ 0 ] )
				{
					Log_Msg( gConsoleChannel, Args_GetRegisteredPrint( arg ).c_str() );
					return;
				}
			}
		}

		Log_WarnF( gConsoleChannel, "Convar not found: %s\n", args[0].c_str() );
	}
}


void FindStr( bool andSearch, ConVarBase* cvar, const std::vector< std::string >& args, std::vector< std::string >& results )
{
	for ( auto arg : args )
	{
		if ( strstr( cvar->GetName(), arg.c_str() ) )
		{
			results.push_back( cvar->GetPrintMessage() );

			if ( !andSearch )
				return;
		}
	}
}


static void FindStrArg( bool andSearch, const Arg_t* spArg, const std::vector< std::string >& args, std::vector< std::string >& results )
{
	if ( !spArg )
		return;

	for ( auto arg : args )
	{
		for ( u32 n = 0; n < spArg->aNames.size(); n++ )
		{
			if ( strstr( spArg->aNames[ n ], arg.c_str() ) )
			{
				results.push_back( Args_GetRegisteredPrint( spArg ) );

				if ( !andSearch )
					return;
			}
		}
	}
}


void CmdFind( bool andSearch, const std::vector< std::string >& args )
{
	std::vector< std::string > resultsCvar;
	std::vector< std::string > resultsCCmd;
	std::vector< std::string > resultsArgs;

	for ( ConVarBase* cvar : Con_GetConVars() )
	{
		if ( IS_TYPE( *cvar, ConVar ) )
		{
			FindStr( andSearch, cvar, args, resultsCvar );
		}
		else if ( IS_TYPE( *cvar, ConCommand ) )
		{
			FindStr( andSearch, cvar, args, resultsCCmd );
		}
	}

	for ( u32 i = 0; i < Args_GetRegisteredCount(); i++ )
	{
		FindStrArg( andSearch, Args_GetRegisteredData( i ), args, resultsArgs );
	}

	Log_MsgF( gConsoleChannel, "Search Results: %zu\n", resultsCvar.size() + resultsCCmd.size() );

	Log_MsgF( gConsoleChannel,
		"\nConVars: %zu"
		"\n--------------------------------------\n", resultsCvar.size() );

	for ( const auto& msg : resultsCvar )
		Log_Msg( gConsoleChannel, msg.c_str() );

	Log_MsgF( gConsoleChannel,
		"--------------------------------------\n"
		"\nConCommands: %zu"
		"\n--------------------------------------\n", resultsCCmd.size() );

	for ( const auto& msg : resultsCCmd )
		Log_Msg( gConsoleChannel, msg.c_str() );

	Log_MsgF( gConsoleChannel,
		"--------------------------------------\n"
		"\nArguments: %zu"
		"\n--------------------------------------\n", resultsArgs.size() );

	for ( const auto& msg : resultsArgs )
		Log_Msg( gConsoleChannel, msg.c_str() );

	Log_Msg( gConsoleChannel, "--------------------------------------\n" );
}


// IDEA: later when you add in ConVar flags and descriptions, make a findex cmd that you can choose specific parts to search
// like only desc, or only name, or both, and probably the rest of the args for flag searching
// and if search string is empty but flags isn't, just list all for those flags
CONCMD_VA( find, "Search if cvar name contains any of the search arguments" )
{
	if ( args.empty() )
	{
		Log_MsgF( gConsoleChannel, "%s\n", find_cmd.GetDesc() );
		return;
	}

	CmdFind( false, args );
}


CONCMD_VA( findand, "Search if cvar name contains all of the search arguments" )
{
	if ( args.empty() )
	{
		Log_MsgF( gConsoleChannel, "%s\n", findand_cmd.GetDesc() );
		return;
	}

	CmdFind( true, args );
}


void reset_cvar_dropdown(
	const std::vector< std::string >& args,  // arguments currently typed in by the user
	std::vector< std::string >& results )      // results to populate the dropdown list with
{
	std::string name = args.empty() ? "" : str_lower2(args[0]);

	std::vector< ConVarBase* > searchResults;
	SearchConVars( searchResults, name.data(), name.size() );

	for ( ConVarBase* cvar : searchResults )
	{
		if ( IS_TYPE( *cvar, ConVar ) )
			results.push_back( cvar->GetName() );
	}
}


CONCMD_DROP_VA( cvar_reset, reset_cvar_dropdown, 0, "reset a convar back to it's default value" )
{
	if ( args.empty() )
	{
		Log_Msg( gConsoleChannel, "No ConVar specified to reset!\n" );
		return;
	}

	ConVar* cvar = Con_GetConVar( args[0] );

	if ( cvar )
	{
		cvar->Reset();
	}
	else
	{
		Log_WarnF( gConsoleChannel, "Convar not found: %s\n", args[ 0 ].c_str() );
	}
}


CONCMD_DROP_VA( cvar_toggle, reset_cvar_dropdown, 0, "toggle a convar between two values" )
{
	if ( args.empty() )
	{
		Log_Msg( gConsoleChannel, "No ConVar specified to reset!\n" );
		return;
	}

	const char* value0 = nullptr;
	const char* value1 = nullptr;

	if ( args.size() >= 3 )
	{
		value0 = args[ 1 ].c_str();
		value1 = args[ 2 ].c_str();
	}
	else if ( args.size() == 2 )
	{
		value0 = args[ 1 ].c_str();
		value1 = "0";  // Default Other Type to toggle between
	}
	else
	{
		// Default Types to toggle between
		value0 = "0";
		value1 = "1";
	}

	ConVar* cvar = Con_GetConVar( args[0] );

	if ( !cvar )
	{
		Log_WarnF( gConsoleChannel, "Convar not found: %s\n", args[ 0 ].c_str() );
		return;
	}
	
	if ( cvar->GetValue() == value1 )
		cvar->SetValue( value0 );
	else
		cvar->SetValue( value1 );
}


// same as in source engine lol
CONCMD_VA( host_writeconfig, "Write a config (can optionally specify a path) containing all ConVars marked with archive, and extra data provided by callback functions" )
{
	if ( args.size() )
		Con_Archive( args[ 0 ].c_str() );
	else
		Con_Archive();
}


CONCMD_VA( con_cvar_stack_size, "Print the stack usage of all convars" )
{
	size_t      size    = 0;

	Log_DevF( gConsoleChannel, 1, "sizeof( ConvarBase ): %zu\n", sizeof( ConVarBase ) );
	Log_DevF( gConsoleChannel, 1, "sizeof( Convar ):     %zu\n", sizeof( ConVar ) );
	Log_DevF( gConsoleChannel, 1, "sizeof( ConCommand ): %zu\n", sizeof( ConCommand ) );
	Log_DevF( gConsoleChannel, 1, "sizeof( ConVarRef ):  %zu\n", sizeof( ConVarRef ) );

	for ( ConVarBase* current : Con_GetConVars() )
	{
		if ( typeid( *current ) == typeid( ConVar ) )
			size += sizeof( ConVar );

		else if ( typeid( *current ) == typeid( ConCommand ) )
			size += sizeof( ConCommand );

		else if ( typeid( *current ) == typeid( ConVarRef ) )
			size += sizeof( ConVarRef );

		else
			size += sizeof( *current );
	}

	Log_DevF( gConsoleChannel, 1, "Convar Stack Size: %zu bytes\n", size );
}


// use if you need to quit the engine right now
CONCMD_VA( _abort, CVARF_INSTANT, "funny" )
{
	abort();
}


CONCMD_VA( build_number, "Print Build Number" )
{
	Log_MsgF( "Chocolate Engine Build Number: %zu\n", Core_GetBuildNumber() );
}


CONCMD_VA( cat, "Print a File" )
{
	Log_Msg( "\"cat\" command not implemented\n" );
}


#ifdef _WIN32
CONCMD( heapcheck )
{
	int result = _heapchk();

	switch ( result )
	{
		case _HEAPEMPTY:
			Log_Msg( "Heap Empty\n" );
			break;

		case _HEAPOK:
			Log_Msg( "Heap OK\n" );
			break;

		case _HEAPBADBEGIN:
			Log_Msg( "Heap Bad Begin\n" );
			break;

		case _HEAPBADNODE:
			Log_Msg( "Heap Bad Node\n" );
			break;

		case _HEAPEND:
			Log_Msg( "Heap End\n" );
			break;

		case _HEAPBADPTR:
			Log_Msg( "Heap Bad Pointer\n" );
			break;
	}
}
#endif

