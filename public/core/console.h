/*
console.h ( Authored by p0lyh3dron )

Declares the sdfhuosdfhuiosdfhusdfhuisfhu
*/

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "core/platform.h"


#ifdef _MSC_VER
	// Disable this useless warning of vars in a class needing a DLL Interface when exported
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif

#ifdef __unix__
	#include <float.h>
#endif /* __unix__  */


class ConCommand;

struct ConVarData_t;


enum ECVarFlagChange
{
	ECVarFlagChange_Allow,
	ECVarFlagChange_Deny,
};


// Whenever a convar value changes or concommand wants to be called, we check all flags to see if any have callbacks.
// If the flag has a registered callback function, then we have to check that first before we can change the cvar value, or run the command.
// If it returns true, we can proceed normally.
// If it returns false, don't change the ConVar value or run the ConCommand.
using ConVarFlagChangeFunc = bool( const std::string& sName, const std::vector< std::string >& args, const std::string& fullCommand );

// Function to call for a Console Command
using ConCommandFunc = void( const std::vector< std::string >& args, const std::string& fullCommand );

// Callback Function for when a ConVar Value Changes
using ConVarFunc_Bool = void( bool sPrevValue, bool& newValue );
using ConVarFunc_Int = void( int sPrevValue, int& newValue );
using ConVarFunc_Float = void( float sPrevValue, float& newValue );
using ConVarFunc_String = void( const char* sPrevValue, char*& newValue );
using ConVarFunc_Vec2 = void( const glm::vec2& sPrevValue, glm::vec2& newValue );
using ConVarFunc_Vec3 = void( const glm::vec3& sPrevValue, glm::vec3& newValue );
using ConVarFunc_Vec4 = void( const glm::vec4& sPrevValue, glm::vec4& newValue );

// Callback Function for populating an autocomplete list for a ConCommand
// TODO: Support ConVar dropdowns
using ConCommandDropdownFunc = void(
  const std::vector< std::string >& args,         // arguments currently typed in by the user
  const std::string&                fullCommand,  // the full command string
  std::vector< std::string >&       results       // results to populate the dropdown list with
);

// Callback function to add data to the config.cfg file written from CVARF_ARCHIVE
using FArchive = void( std::string& srOutput );

using ConVarFlag_t = size_t;


struct ConVarFlagData_t
{
	ConVarFlag_t          flag;
	const char*           name;
	size_t                nameLen;
	ConVarFlagChangeFunc* apCallback = nullptr;
};


// maybe change to config option?
enum EConVarType : u8
{
	EConVarType_Invalid,     // Invalid

	EConVarType_Bool,        // Boolean
	EConVarType_Int,         // Integer
	EConVarType_Float,       // Float
	EConVarType_String,      // String Value
	EConVarType_RangeInt,    // Clamped Integer Value
	EConVarType_RangeFloat,  // Clamped Float Value
	EConVarType_Vec2,        // 2 Float Values
	EConVarType_Vec3,        // 3 Float Values
	EConVarType_Vec4,        // 4 Float Values

	// List of Values to Select From
	// Generally static multiple option choice, though maybe we could set it in an init function
	// A "type" or "mode" convar would be perfect for this, like log_dev_level, wireframe mode
	// The returned type for registering it is an integer, for an index into the list
	//EConVarType_SelectionInt,
	//EConVarType_SelectionFloat,  // would there ever be a use case for this?
	//EConVarType_SelectionString,

	EConVarType_Count,
};


struct ConVarData_t
{
	// idk type  aTags;
	ConVarFlag_t aFlags;
	EConVarType  aType;

	union
	{
		struct
		{
			ConVarFunc_Bool* apFunc;
			bool*            apData;
			bool             aDefaultData;
		} aBool;

		struct
		{
			ConVarFunc_Int* apFunc;
			int*            apData;
			int             aDefaultData;
		} aInt;

		struct
		{
			ConVarFunc_Float* apFunc;
			float*            apData;
			float             aDefaultData;
		} aFloat;

		struct
		{
			ConVarFunc_String* apFunc;
			char**             apData;
			const char*        aDefaultData;
		} aString;

		struct
		{
			ConVarFunc_Vec2* apFunc;
			glm::vec2*       apData;
			glm::vec2        aDefaultData;
		} aVec2;

		struct
		{
			ConVarFunc_Vec3* apFunc;
			glm::vec3*       apData;
			glm::vec3        aDefaultData;
		} aVec3;

		struct
		{
			ConVarFunc_Vec4* apFunc;
			glm::vec4*       apData;
			glm::vec4        aDefaultData;
		} aVec4;

		struct
		{
			ConVarFunc_Int* apFunc;
			int*            apData;
			int             aDefaultData;
			int             aMin;
			int             aMax;
		} aRangeInt;

		struct
		{
			ConVarFunc_Float* apFunc;
			float*            apData;
			float             aDefaultData;
			float             aMin;
			float             aMax;
		} aRangeFloat;
	};
};


struct ChConfigOptionDropDown_t
{
	char* apData;
	u32   aLen;
};


class CORE_API ConCommand
{
public:

	ConCommand( const char* name, ConCommandFunc* func, ConVarFlag_t flags = 0 )
	{
		Init( name, nullptr, flags, func, nullptr );
	}

	ConCommand( const char* name, ConCommandFunc* func, const char* desc )
	{
		Init( name, desc, 0, func, nullptr );
	}

	ConCommand( const char* name, ConCommandFunc* func, ConCommandDropdownFunc* dropDownFunc )
	{
		Init( name, nullptr, 0, func, dropDownFunc );
	}

	ConCommand( const char* name, ConCommandFunc* func, ConVarFlag_t flags, ConCommandDropdownFunc* dropDownFunc )
	{
		Init( name, nullptr, flags, func, dropDownFunc );
	}
	
	ConCommand( const char* name, ConCommandFunc* func, ConVarFlag_t flags, const char* desc )
	{
		Init( name, desc, flags, func, nullptr );
	}

	ConCommand( const char* name, ConCommandFunc* func, ConVarFlag_t flags, const char* desc, ConCommandDropdownFunc* dropDownFunc )
	{
		Init( name, desc, flags, func, dropDownFunc );
	}

	void                    Init( const char* name, const char* desc, ConVarFlag_t flags, ConCommandFunc* func, ConCommandDropdownFunc* dropDownFunc );

	std::string             GetPrintMessage();

	ConCommandFunc*         apFunc;
	ConCommandDropdownFunc* apDropDownFunc;
	const char*             aName;
	const char*             aDesc;
	ConVarFlag_t            aFlags;

	const char*  GetName();
	const char*  GetDesc();
	ConVarFlag_t GetFlags();

private:

	// NO COPYING!!
	ConCommand( const ConCommand& );
};


#define CONCMD( name )                                                                                \
	void       name##_func( const std::vector< std::string >& args, const std::string& fullCommand ); \
	ConCommand name##_cmd( #name, name##_func );                                                      \
	void       name##_func( const std::vector< std::string >& args, const std::string& fullCommand )

#define CONCMD_VA( name, ... )                                                                 \
	void       name( const std::vector< std::string >& args, const std::string& fullCommand ); \
	ConCommand name##_cmd( #name, name, __VA_ARGS__ );                                         \
	void       name( const std::vector< std::string >& args, const std::string& fullCommand )

#define CONCMD_DROP( name, func )                                                              \
	void       name( const std::vector< std::string >& args, const std::string& fullCommand ); \
	ConCommand name##_cmd( #name, name, func );                                                \
	void       name( const std::vector< std::string >& args, const std::string& fullCommand )

#define CONCMD_DROP_VA( name, func, ... )                                                      \
	void       name( const std::vector< std::string >& args, const std::string& fullCommand ); \
	ConCommand name##_cmd( #name, name, __VA_ARGS__, func );                                   \
	void       name( const std::vector< std::string >& args, const std::string& fullCommand )

#define CON_COMMAND( name ) CONCMD( name )

#define CON_COMMAND_LAMBDA( name ) \
	ConCommand* name = new ConCommand( #name, [ & ]( const std::vector< std::string >& sArgs )


struct ConVarDescriptor_t
{
	std::string_view aName;
	bool             aIsConVar = false;
	void*            apData    = nullptr;
};


// ----------------------------------------------------------------
// ConVar Functions


// Internal Lists of ConVars and ConCommands
CORE_API ChVector< ConCommand* >&                               Con_GetConCommands();
CORE_API std::vector< std::string_view >&                       Con_GetConVarNames();
CORE_API std::vector< ConVarDescriptor_t >&                     Con_GetConVarList();
CORE_API std::unordered_map< std::string_view, ConVarData_t* >& Con_GetConVarMap();
CORE_API std::unordered_map< std::string_view, ch_string >&     Con_GetConVarDesc();  // ConVar Descriptions


// Register a Console/Config Variable
CORE_API const bool&         Con_RegisterConVar_Bool( const char* spName, bool sDefault, ConVarFlag_t sFlags, const char* spDesc = nullptr, ConVarFunc_Bool* spCallbackFunc = nullptr );
CORE_API const int&          Con_RegisterConVar_Int( const char* spName, int sDefault, ConVarFlag_t sFlags, const char* spDesc = nullptr, ConVarFunc_Int* spCallbackFunc = nullptr );
CORE_API const float&        Con_RegisterConVar_Float( const char* spName, float sDefault, ConVarFlag_t sFlags, const char* spDesc = nullptr, ConVarFunc_Float* spCallbackFunc = nullptr );
CORE_API char*& const        Con_RegisterConVar_String( const char* spName, const char* spDefault, ConVarFlag_t sFlags, const char* spDesc = nullptr, ConVarFunc_String* spCallbackFunc = nullptr );

CORE_API const glm::vec2&    Con_RegisterConVar_Vec2( const char* spName, const glm::vec2& srDefault, ConVarFlag_t sFlags, const char* spDesc = nullptr, ConVarFunc_Vec2* spCallbackFunc = nullptr );
CORE_API const glm::vec3&    Con_RegisterConVar_Vec3( const char* spName, const glm::vec3& srDefault, ConVarFlag_t sFlags, const char* spDesc = nullptr, ConVarFunc_Vec3* spCallbackFunc = nullptr );
CORE_API const glm::vec4&    Con_RegisterConVar_Vec4( const char* spName, const glm::vec4& srDefault, ConVarFlag_t sFlags, const char* spDesc = nullptr, ConVarFunc_Vec4* spCallbackFunc = nullptr );

CORE_API const glm::vec2&    Con_RegisterConVar_Vec2( const char* spName, float sX, float sY, ConVarFlag_t sFlags, const char* spDesc = nullptr, ConVarFunc_Vec2* spCallbackFunc = nullptr );
CORE_API const glm::vec3&    Con_RegisterConVar_Vec3( const char* spName, float sX, float sY, float sZ, ConVarFlag_t sFlags, const char* spDesc = nullptr, ConVarFunc_Vec3* spCallbackFunc = nullptr );
CORE_API const glm::vec4&    Con_RegisterConVar_Vec4( const char* spName, float sX, float sY, float sZ, float sW, ConVarFlag_t sFlags, const char* spDesc = nullptr, ConVarFunc_Vec4* spCallbackFunc = nullptr );

CORE_API const int&          Con_RegisterConVar_RangeInt( const char* spName, int sDefault, int sMin, int sMax, ConVarFlag_t sFlags, const char* spDesc = nullptr, ConVarFunc_Int* spCallbackFunc = nullptr );
CORE_API const float&        Con_RegisterConVar_RangeFloat( const char* spName, float sDefault, float sMin, float sMax, ConVarFlag_t sFlags, const char* spDesc = nullptr, ConVarFunc_Float* spCallbackFunc = nullptr );

// Register ConVar Functions, but without flags (why even have this?)

CORE_API const bool&         Con_RegisterConVar_Bool( const char* spName, bool sDefault, const char* spDesc = nullptr, ConVarFunc_Bool* spCallbackFunc = nullptr );
CORE_API const int&          Con_RegisterConVar_Int( const char* spName, int sDefault, const char* spDesc = nullptr, ConVarFunc_Int* spCallbackFunc = nullptr );
CORE_API const float&        Con_RegisterConVar_Float( const char* spName, float sDefault, const char* spDesc = nullptr, ConVarFunc_Float* spCallbackFunc = nullptr );
CORE_API char*& const        Con_RegisterConVar_String( const char* spName, const char* spDefault, const char* spDesc = nullptr, ConVarFunc_String* spCallbackFunc = nullptr );

CORE_API const glm::vec2&    Con_RegisterConVar_Vec2( const char* spName, const glm::vec2& srDefault, const char* spDesc = nullptr, ConVarFunc_Vec2* spCallbackFunc = nullptr );
CORE_API const glm::vec3&    Con_RegisterConVar_Vec3( const char* spName, const glm::vec3& srDefault, const char* spDesc = nullptr, ConVarFunc_Vec3* spCallbackFunc = nullptr );
CORE_API const glm::vec4&    Con_RegisterConVar_Vec4( const char* spName, const glm::vec4& srDefault, const char* spDesc = nullptr, ConVarFunc_Vec4* spCallbackFunc = nullptr );

CORE_API const glm::vec2&    Con_RegisterConVar_Vec2( const char* spName, float sX, float sY, const char* spDesc = nullptr, ConVarFunc_Vec2* spCallbackFunc = nullptr );
CORE_API const glm::vec3&    Con_RegisterConVar_Vec3( const char* spName, float sX, float sY, float sZ, const char* spDesc = nullptr, ConVarFunc_Vec3* spCallbackFunc = nullptr );
CORE_API const glm::vec4&    Con_RegisterConVar_Vec4( const char* spName, float sX, float sY, float sZ, float sW, const char* spDesc = nullptr, ConVarFunc_Vec4* spCallbackFunc = nullptr );

CORE_API const int&          Con_RegisterConVar_RangeInt( const char* spName, int sDefault, int sMin = INT_MIN, int sMax = INT_MAX, const char* spDesc = nullptr, ConVarFunc_Int* spCallbackFunc = nullptr );
CORE_API const float&        Con_RegisterConVar_RangeFloat( const char* spName, float sDefault, float sMin = FLT_MIN, float sMax = FLT_MAX, const char* spDesc = nullptr, ConVarFunc_Float* spCallbackFunc = nullptr );

// Helper Macros for registering ConVars
// will the va args here be an issue on gcc or clang?

#define CONVAR_BOOL( name, ... )        const bool& name = Con_RegisterConVar_Bool( #name, __VA_ARGS__ )
#define CONVAR_INT( name, ... )         const int& name = Con_RegisterConVar_Int( #name, __VA_ARGS__ )
#define CONVAR_FLOAT( name, ... )       const float& name = Con_RegisterConVar_Float( #name, __VA_ARGS__ )
#define CONVAR_STRING( name, ... )      char*& name = Con_RegisterConVar_String( #name, __VA_ARGS__ )
#define CONVAR_VEC2( name, ... )        const glm::vec2& name = Con_RegisterConVar_Vec2( #name, __VA_ARGS__ )
#define CONVAR_VEC3( name, ... )        const glm::vec3& name = Con_RegisterConVar_Vec3( #name, __VA_ARGS__ )
#define CONVAR_VEC4( name, ... )        const glm::vec4& name = Con_RegisterConVar_Vec4( #name, __VA_ARGS__ )
#define CONVAR_RANGE_INT( name, ... )   const int& name = Con_RegisterConVar_RangeInt( #name, __VA_ARGS__ )
#define CONVAR_RANGE_FLOAT( name, ... ) const float& name = Con_RegisterConVar_RangeFloat( #name, __VA_ARGS__ )

#define CONVAR_BOOL_NAME( var, name, ... )        const bool& var = Con_RegisterConVar_Bool( name, __VA_ARGS__ )
#define CONVAR_INT_NAME( var, name, ... )         const int& var = Con_RegisterConVar_Int( name, __VA_ARGS__ )
#define CONVAR_FLOAT_NAME( var, name, ... )       const float& var = Con_RegisterConVar_Float( name, __VA_ARGS__ )
#define CONVAR_STRING_NAME( var, name, ... )      char*& var = Con_RegisterConVar_String( name, __VA_ARGS__ )
#define CONVAR_VEC2_NAME( var, name, ... )        const glm::vec2& var = Con_RegisterConVar_Vec2( name, __VA_ARGS__ )
#define CONVAR_VEC3_NAME( var, name, ... )        const glm::vec3& var = Con_RegisterConVar_Vec3( name, __VA_ARGS__ )
#define CONVAR_VEC4_NAME( var, name, ... )        const glm::vec4& var = Con_RegisterConVar_Vec4( name, __VA_ARGS__ )
#define CONVAR_RANGE_INT_NAME( var, name, ... )   const int& var = Con_RegisterConVar_RangeInt( name, __VA_ARGS__ )
#define CONVAR_RANGE_FLOAT_NAME( var, name, ... ) const float& var = Con_RegisterConVar_RangeFloat( name, __VA_ARGS__ )


// Extern convars
#define CONVAR_BOOL_EXT( name )        extern const bool& name
#define CONVAR_INT_EXT( name )         extern const int& name
#define CONVAR_FLOAT_EXT( name )       extern const float& name
#define CONVAR_STRING_EXT( name )      extern char*& name
#define CONVAR_VEC2_EXT( name )        extern const glm::vec2& name
#define CONVAR_VEC3_EXT( name )        extern const glm::vec3& name
#define CONVAR_VEC4_EXT( name )        extern const glm::vec4& name
#define CONVAR_RANGE_INT_EXT( name )   extern const int& name
#define CONVAR_RANGE_FLOAT_EXT( name ) extern const float& name

// Helper Macros for registering ConVars with a callback
#define CONVAR_BOOL_CMD( name, defaultVal, flags, desc )                                           \
	void        cvarfunc_##name( bool prevValue, bool& newValue );                                 \
	const bool& name = Con_RegisterConVar_Bool( #name, defaultVal, flags, desc, cvarfunc_##name ); \
	void        cvarfunc_##name( bool prevValue, bool& newValue )

#define CONVAR_INT_CMD( name, defaultVal, flags, desc )                                          \
	void       cvarfunc_##name( int prevValue, int& newValue );                                  \
	const int& name = Con_RegisterConVar_Int( #name, defaultVal, flags, desc, cvarfunc_##name ); \
	void       cvarfunc_##name( int prevValue, int& newValue )

#define CONVAR_FLOAT_CMD( name, defaultVal, flags, desc )                                            \
	void         cvarfunc_##name( float prevValue, float& newValue );                                \
	const float& name = Con_RegisterConVar_Float( #name, defaultVal, flags, desc, cvarfunc_##name ); \
	void         cvarfunc_##name( float prevValue, float& newValue )

#define CONVAR_STRING_CMD( name, defaultVal, flags, desc )                                            \
	void         cvarfunc_##name( const char* prevValue, char*& newValue );                           \
	char*& const name = Con_RegisterConVar_String( #name, defaultVal, flags, desc, cvarfunc_##name ); \
	void         cvarfunc_##name( const char* prevValue, char*& newValue )

#define CONVAR_VEC2_CMD( name, defaultX, defaultY, flags, desc )                                                \
	void             cvarfunc_##name( const glm::vec2& prevValue, glm::vec2& newValue );                        \
	const glm::vec2& name = Con_RegisterConVar_Vec2( #name, defaultX, defaultY, flags, desc, cvarfunc_##name ); \
	void             cvarfunc_##name( const glm::vec2& prevValue, glm::vec2& newValue )

#define CONVAR_VEC3_CMD( name, defaultX, defaultY, defaultZ, flags, desc )                                                \
	void             cvarfunc_##name( const glm::vec3& prevValue, glm::vec3& newValue );                                  \
	const glm::vec3& name = Con_RegisterConVar_Vec3( #name, defaultX, defaultY, defaultZ, flags, desc, cvarfunc_##name ); \
	void             cvarfunc_##name( const glm::vec3& prevValue, glm::vec3& newValue )

#define CONVAR_VEC4_CMD( name, defaultX, defaultY, defaultZ, defaultW, flags, desc )                                      \
	void             cvarfunc_##name( const glm::vec4& prevValue, glm::vec4& newValue );                                  \
	const glm::vec4& name = Con_RegisterConVar_Vec4( #name, defaultX, defaultY, defaultZ, flags, desc, cvarfunc_##name ); \
	void             cvarfunc_##name( const glm::vec4& prevValue, glm::vec4& newValue )

#define CONVAR_RANGE_INT_CMD( name, defaultVal, min, max, flags, desc )                                         \
	void       cvarfunc_##name( int prevValue, int& newValue );                                                 \
	const int& name = Con_RegisterConVar_RangeInt( #name, defaultVal, min, max, flags, desc, cvarfunc_##name ); \
	void       cvarfunc_##name( int prevValue, int& newValue )

#define CONVAR_RANGE_FLOAT_CMD( name, defaultVal, min, max, flags, desc )                                           \
	void         cvarfunc_##name( float prevValue, float& newValue );                                               \
	const float& name = Con_RegisterConVar_RangeFloat( #name, defaultVal, min, max, flags, desc, cvarfunc_##name ); \
	void         cvarfunc_##name( float prevValue, float& newValue )


// Get the data pointer reference of a ConVar
CORE_API bool&         Con_GetConVarData_Bool( const char* spName, bool fallback );
CORE_API int&          Con_GetConVarData_Int( const char* spName, int fallback );    // also works for RangeInt
CORE_API float&        Con_GetConVarData_Float( const char* spName, float fallback );  // also works for RangeFloat
CORE_API char*&        Con_GetConVarData_String( const char* spName, const char* fallback );
CORE_API glm::vec2&    Con_GetConVarData_Vec2( const char* spName, const glm::vec2& fallback );
CORE_API glm::vec3&    Con_GetConVarData_Vec3( const char* spName, const glm::vec3& fallback );
CORE_API glm::vec4&    Con_GetConVarData_Vec4( const char* spName, const glm::vec4& fallback );


// Set the value of a ConVar
// Returns:
// -1 - The ConVar type is incorrect
// 0  - The ConVar doesn't exist
// 1  - The ConVar was set
CORE_API int           Con_SetConVarValue( const char* spName, bool sValue );
CORE_API int           Con_SetConVarValue( const char* spName, int sValue );
CORE_API int           Con_SetConVarValue( const char* spName, float sValue );
CORE_API int           Con_SetConVarValue( const char* spName, const char* spValue );
CORE_API int           Con_SetConVarValue( const char* spName, std::string_view sValue );
CORE_API int           Con_SetConVarValue( const char* spName, const glm::vec2& sValue );
CORE_API int           Con_SetConVarValue( const char* spName, const glm::vec3& sValue );
CORE_API int           Con_SetConVarValue( const char* spName, const glm::vec4& sValue );

CORE_API ch_string     Con_GetConVarDesc( const char* spName );  // Get the description of the ConVar
CORE_API ConVarData_t* Con_GetConVarData( std::string_view sName );  // Get the data of the ConVar

CORE_API const char*   Con_ConVarTypeStr( EConVarType type );    // Get the string of the ConVar Type
CORE_API EConVarType   Con_ConVarType( const char* spType );     // Get the ConVar Type from the string

CORE_API std::string   Con_GetConVarValueStr( ConVarData_t* cvarData );
CORE_API std::string   Con_GetConVarValueStr( const char* name );

// CORE_API std::string   Con_GetConVarFormatted( const char* spName );  // Get a formatted string for the convar name, value, and default value
CORE_API std::string   Con_GetConVarHelp( std::string_view sName );  // Get a formatted string for the help message of this convar

CORE_API u32           Con_GetConVarCount();            // Get the number of ConVars registered
// CORE_API const char*   Con_GetConVarName( u32 index );  // Get the name of the ConVar at the index
// CORE_API ConVarData_t* Con_GetConVarData( u32 index );  // Get the data of the ConVar at the index

CORE_API void          Con_ResetConVar( std::string_view name );  // Reset the ConVar to its default value

CORE_API void          Con_SearchConVars( std::vector< std::string >& results, const char* search, size_t size );
CORE_API void          Con_SearchConVars( std::vector< ConVarDescriptor_t >& results, const char* search, size_t size );

// Search for a ConVar by name, Returns a ConVarDescriptor_t with a nullptr data if not found
CORE_API ConVarDescriptor_t Con_SearchConVarCmd( const char* search, size_t size );

// ----------------------------------------------------------------
// Console Functions

CORE_API void               con_init();
CORE_API void               Con_Shutdown();

CORE_API void               Con_SetConVarRegisterFlags( ConVarFlag_t sFlags );

CORE_API const std::vector< std::string >& Con_GetCommandHistory();

// Checks if the current ConVar is a reference, if so, then return what it's pointing to
// return nullptr if it doesn't point to anything, and return the normal cvar if it's not a ConVarRef
// CORE_API ConVarBase*                         Con_CheckForConVarRef( ConVarBase* cvar );

// Build an auto complete list
CORE_API void                              Con_BuildAutoCompleteList( const std::string& srSearch, std::vector< std::string >& srResults );

// Get all ConVars
// CORE_API const ChVector< ConVarBase* >&      Con_GetConVars();


CORE_API ConCommand*                       Con_GetConCommand( std::string_view name );

CORE_API void                              Con_PrintAllConVars();

// Add a command to the queue
CORE_API void                              Con_QueueCommand( const char* cmd, int len = -1 );
CORE_API void                              Con_QueueCommandSilent( const char* cmd, int len = -1, bool sAddToHistory = true );

// Go through and run every command in the queue
CORE_API void                              Con_Update();

// Find and run a command instantly
CORE_API void                              Con_RunCommand( const char* cmd, int len = -1 );
// CORE_API void                                Con_RunCommandF( const char* command, ... );

// Find and run a parsed command instantly
CORE_API bool                              Con_RunCommandArgs( const std::string& name, const std::vector< std::string >& args );
CORE_API bool                              Con_RunCommandArgs( const std::string& name, const std::vector< std::string >& args, const std::string& fullCommand );

CORE_API void                              Con_ParseCommandLine( std::string_view command, std::string& name, std::vector< std::string >& args, std::string& fullCommand );
CORE_API void                              Con_ParseCommandLineEx( std::string_view command, std::string& name, std::vector< std::string >& args, std::string& fullCommand, size_t& i );

CORE_API ConVarFlag_t                      Con_CreateCvarFlag( const char* name );
CORE_API const char*                       Con_GetCvarFlagName( ConVarFlag_t flag );
CORE_API ConVarFlag_t                      Con_GetCvarFlag( const char* name );
CORE_API size_t                            Con_GetCvarFlagCount();

CORE_API void                              Con_SetCvarFlagCallback( ConVarFlag_t sFlag, ConVarFlagChangeFunc* spCallback );
CORE_API ConVarFlagChangeFunc*             Con_GetCvarFlagCallback( ConVarFlag_t flag );

// Add a callback function to add data to the config.cfg file written from CVARF_ARCHIVE
CORE_API void                              Con_AddArchiveCallback( FArchive* spFunc );

// Write a config of all stuff we want saved
CORE_API void                              Con_Archive( const char* spFile = nullptr );

// Set Default Console Archive File, nullptr to reset to default
CORE_API void                              Con_SetDefaultArchive( const char* spFile, const char* spDefaultFile );

// make sure the convar flag is created when doing static initilization
// CORE_API ConVarFlag_t                        Con_StaticGetCvarFlag( const char* spFile = nullptr );

// New Convar System

// CORE_API ch_handle_t                              Con_RegisterVar( const char* spName, const char* spDesc );


#define CVARF( name )         Con_CreateCvarFlag( "CVARF_" #name )
#define NEW_CVAR_FLAG( name ) ConVarFlag_t name = Con_CreateCvarFlag( #name )
#define EXT_CVAR_FLAG( name ) extern ConVarFlag_t name


CORE_API EXT_CVAR_FLAG( CVARF_NONE );
CORE_API EXT_CVAR_FLAG( CVARF_ARCHIVE );  // save this convar value in a config.cfg file


#ifdef _MSC_VER
	#pragma warning(pop)
#endif

