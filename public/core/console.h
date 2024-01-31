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


class ConVarBase;
class ConCommand;
class ConVar;
class ConVarRef;


enum ECVarFlagChange
{
	ECVarFlagChange_Allow,
	ECVarFlagChange_Deny,
};


// Whenever a convar value changes or concommand wants to be called, we check all flags to see if any have callbacks.
// If the flag has a registered callback function, then we have to check that first before we can change the cvar value, or run the command.
// If it returns true, we can proceed normally.
// If it returns false, don't change the ConVar value or run the ConCommand.
using ConVarFlagChangeFunc = bool( ConVarBase* spBase, const std::vector< std::string >& args );

// Function to call for a Console Command
using ConCommandFunc = void( const std::vector< std::string >& args );

// Callback Function for when a ConVar Value Changes
using ConVarFunc = void( const std::string& prevString, float prevFloat, const std::vector< std::string >& args );

// Callback Function for populating an autocomplete list for a ConCommand
// TODO: Support ConVar dropdowns
using ConCommandDropdownFunc = void(
  const std::vector< std::string >& args,    // arguments currently typed in by the user
  std::vector< std::string >&       results  // results to populate the dropdown list with
  );

// Callback function to add data to the config.cfg file written from CVARF_ARCHIVE
using FArchive = void( std::string& srOutput );

// ConCommand console dropdown list function

using ConVarFlag_t = size_t;


enum EChConfigOption
{
	EChConfigOption_None,

	EChConfigOption_Bool,      // Boolean
	EChConfigOption_Int,       // Integer
	EChConfigOption_Float,     // Float
	EChConfigOption_String,    // String Value
	EChConfigOption_DropDown,  // Drop-Down of String Values

	// not a good type
	EChConfigOption_Slider,    // Float Slider
};


enum EConVar2Type
{
	EConVar2Type_None,

	EConVar2Type_Bool,      // Boolean
	EConVar2Type_Int,       // Integer
	EConVar2Type_Float,     // Float
	EConVar2Type_String,    // String Value
	EConVar2Type_Ranged,    // Clamped Float Value
	EConVar2Type_Vec2,      // 2 Float Values
	EConVar2Type_Vec3,      // 3 Float Values
	EConVar2Type_Vec4,      // 4 Float Values
	EConVar2Type_DropDown,  // Drop-Down of String Values
};


class CORE_API ChConfigOption
{
  public:

	  EChConfigOption aType;
};


// name is stored elsewhere
struct ConVarBase2Data_t
{
	const char*  apDesc;
};


struct ConVar2Data_t
{
	ConVarFlag_t aFlags;
	ConVarFunc*  apFunc;
	EConVar2Type aType;
	void*        apData;
	void*        apDefaultData;
};


using ConVarFlagChangeFunc2 = bool( ConVar2Data_t* spBase, const std::vector< std::string >& args );


// data not used very often, this is only used for ui for the drop down function
struct ConCommand2LocalData_t
{
	ConCommandDropdownFunc* apDropDownFunc;
};


// data used in the console update function
struct ConCommand2Data_t
{
	ConVarFlag_t    aFlags;
	ConCommandFunc* apFunc;
};


struct ChConfigOptionDropDown_t
{
	char* apData;
	u32   aLen;
};


// what if instead of returning a reference, it takes in an existing variable to use and copies that for the default value?
// this way it can easily stay on the stack, unless these variables are allocated on the stack already for speed


CORE_API bool&              Con_RegisterConVar_Bool( const char* spName, const char* spDesc, bool sDefault, ConVarFlag_t sFlags = 0, ConVarFunc* spCallbackFunc = nullptr );
CORE_API int&               Con_RegisterConVar_Int( const char* spName, const char* spDesc, int sDefault, ConVarFlag_t sFlags = 0, ConVarFunc* spCallbackFunc = nullptr );
CORE_API float&             Con_RegisterConVar_Float( const char* spName, const char* spDesc, float sDefault, ConVarFlag_t sFlags = 0, ConVarFunc* spCallbackFunc = nullptr );
CORE_API const char*&       Con_RegisterConVar_Str( const char* spName, const char* spDesc, const char* spDefault, ConVarFlag_t sFlags = 0, ConVarFunc* spCallbackFunc = nullptr );
CORE_API glm::vec2&         Con_RegisterConVar_Vec2( const char* spName, const char* spDesc, const glm::vec2& srDefault, ConVarFlag_t sFlags = 0, ConVarFunc* spCallbackFunc = nullptr );
CORE_API glm::vec3&         Con_RegisterConVar_Vec3( const char* spName, const char* spDesc, const glm::vec3& srDefault, ConVarFlag_t sFlags = 0, ConVarFunc* spCallbackFunc = nullptr );
CORE_API glm::vec4&         Con_RegisterConVar_Vec4( const char* spName, const char* spDesc, const glm::vec4& srDefault, ConVarFlag_t sFlags = 0, ConVarFunc* spCallbackFunc = nullptr );
CORE_API float&             Con_RegisterConVar_Range( const char* spName, const char* spDesc, float sDefault, float sMin = FLT_MIN, float sMax = FLT_MAX, ConVarFlag_t sFlags = 0, ConVarFunc* spCallbackFunc = nullptr );

CORE_API bool&              Con_RegisterConVarRef_Bool( const char* spName, bool sFallback );
CORE_API int&               Con_RegisterConVarRef_Int( const char* spName, int sFallback );
CORE_API float&             Con_RegisterConVarRef_Float( const char* spName, float sFallback );
CORE_API const char*&       Con_RegisterConVarRef_Str( const char* spName, const char* spFallback );
CORE_API float&             Con_RegisterConVarRef_Range( const char* spName, float sFallback );

CORE_API void               Con_SetConVarValue( const char* spName, bool sValue );
CORE_API void               Con_SetConVarValue( const char* spName, int sValue );
CORE_API void               Con_SetConVarValue( const char* spName, float sValue );
CORE_API void               Con_SetConVarValue( const char* spName, const char* spValue );

CORE_API ConVarBase2Data_t* Con_GetConVarBase2Data( const char* spName );
CORE_API ConVar2Data_t*     Con_GetConVar2Data( const char* spName );
CORE_API ConCommand2Data_t* Con_GetConCommand2Data( const char* spName );


class CORE_API ConVarBase
{
public:
	ConVarBase( const char* name, ConVarFlag_t flags = 0 );
	ConVarBase( const char* name, const char* desc );
	ConVarBase( const char* name, ConVarFlag_t flags, const char* desc );

	virtual std::string		GetPrintMessage() = 0;

	const char*             GetName();
	const char*             GetDesc();
	ConVarFlag_t            GetFlags();

	friend class ConCommand;
	friend class ConVar;
	friend class ConVarRef;
	friend class Console;

	const char*             aName;
	const char*             aDesc;
	ConVarFlag_t            aFlags;

private:
	// NO COPYING!!
	ConVarBase( const ConVarBase& );
};


// TODO: make a command argument tokenizer, so you can get individial arguments, or the raw command string
class CORE_API ConCommand : public ConVarBase
{
public:

	ConCommand( const char* name, ConCommandFunc* func, ConVarFlag_t flags = 0 ):
		ConVarBase( name, flags )
	{
		Init( func, nullptr );
	}

	ConCommand( const char* name, ConCommandFunc* func, const char* desc ) :
		ConVarBase( name, desc )
	{
		Init( func, nullptr );
	}

	ConCommand( const char* name, ConCommandFunc* func, ConCommandDropdownFunc* dropDownFunc ):
		ConVarBase( name )
	{
		Init( func, dropDownFunc );
	}

	ConCommand( const char* name, ConCommandFunc* func, ConVarFlag_t flags, ConCommandDropdownFunc* dropDownFunc ):
		ConVarBase( name, flags )
	{
		Init( func, dropDownFunc );
	}
	
	ConCommand( const char* name, ConCommandFunc* func, ConVarFlag_t flags, const char* desc ) :
		ConVarBase( name, flags, desc )
	{
		Init( func, nullptr );
	}

	ConCommand( const char* name, ConCommandFunc* func, ConVarFlag_t flags, const char* desc, ConCommandDropdownFunc* dropDownFunc ) :
		ConVarBase( name, flags, desc )
	{
		Init( func, dropDownFunc );
	}

	void Init( ConCommandFunc* func, ConCommandDropdownFunc* dropDownFunc );

	std::string GetPrintMessage(  ) override;

	ConCommandFunc*         apFunc;
	ConCommandDropdownFunc* apDropDownFunc;

	friend class Console;

private:

	// NO COPYING!!
	ConCommand( const ConCommand& );
};


struct ConVarData
{
	ConVarData()
	{
	}

	~ConVarData();

	u32         aDefaultValueLen;
	u32         aValueLen;

	char*       apDefaultValue = nullptr;
	char*       apValue        = nullptr;

	float       aDefaultValueFloat;
	float       aValueFloat;
	ConVarFunc* apFunc;

   private:
	// NO COPYING!!
	ConVarData( const ConVarData& );
};


#define CVAR_OP_EQ_SELF( otherType, leftValue, otherValue ) \
	void    operator*=( otherType& other )    { return SetValue( leftValue * otherValue ); } \
	void    operator/=( otherType& other )    { return SetValue( leftValue / otherValue ); } \
	void    operator+=( otherType& other )    { return SetValue( leftValue + otherValue ); } \
	void    operator-=( otherType& other )    { return SetValue( leftValue - otherValue ); }

#define CVAR_OP( outType, otherType, leftValue, otherValue ) \
	bool    operator<=( otherType& other )    { return leftValue <= otherValue; } \
	bool    operator>=( otherType& other )    { return leftValue >= otherValue; } \
	bool    operator!=( otherType& other )    { return leftValue != otherValue; } \
	bool    operator==( otherType& other )    { return leftValue == otherValue; } \
                                                                                         \
	outType operator*( otherType& other )     { return leftValue * otherValue;  } \
	outType operator/( otherType& other )     { return leftValue / otherValue;  } \
	outType operator+( otherType& other )     { return leftValue + otherValue;  } \
	outType operator-( otherType& other )     { return leftValue - otherValue;  }


// TODO: allow convars to be able to be linked
class CORE_API ConVar : public ConVarBase
{
public:

	ConVar( const char* name, std::string_view defaultValue, ConVarFlag_t flags = 0 ) :
		ConVarBase( name, flags )
	{
		Init( defaultValue, nullptr );
	}

	ConVar( const char* name, std::string_view defaultValue, const char* desc ) :
		ConVarBase( name, desc )
	{
		Init( defaultValue, nullptr );
	}

	ConVar( const char* name, std::string_view defaultValue, ConVarFlag_t flags, const char* desc ) :
		ConVarBase( name, flags, desc )
	{
		Init( defaultValue, nullptr );
	}

	ConVar( const char* name, std::string_view defaultValue, ConVarFlag_t flags, const char* desc, ConVarFunc callback ) :
		ConVarBase( name, flags, desc )
	{
		Init( defaultValue, callback );
	}


	ConVar( const char* name, float defaultValue, ConVarFlag_t flags = 0 ):
		ConVarBase( name, flags )
	{
		Init( defaultValue, nullptr );
	}

	ConVar( const char* name, float defaultValue, const char* desc ) :
		ConVarBase( name, desc )
	{
		Init( defaultValue, nullptr );
	}

	ConVar( const char* name, float defaultValue, ConVarFlag_t flags, const char* desc ) :
		ConVarBase( name, flags, desc )
	{
		Init( defaultValue, nullptr );
	}

	ConVar( const char* name, float defaultValue, ConVarFlag_t flags, const char* desc, ConVarFunc* callback ) :
		ConVarBase( name, flags, desc )
	{
		Init( defaultValue, callback );
	}


	ConVar( const char* name, std::string_view defaultValue, ConVarFunc* callback ) :
		ConVarBase( name )
	{
		Init( defaultValue, callback );
	}

	ConVar( const char* name, float defaultValue, ConVarFunc* callback ):
		ConVarBase( name )
	{
		Init( defaultValue, callback );
	}

	~ConVar();
	
	void               Init( std::string_view defaultValue, ConVarFunc* func );
	void               Init( float defaultValue, ConVarFunc* func );

	std::string        GetPrintMessage() override;

	void               SetValue( std::string_view value );
	void               SetValue( float value );

	void               Reset();

	const char*        GetChar() const;
	u32                GetValueLen() const;
	std::string_view   GetValue() const;
	float              GetFloat() const;
	int                GetInt() const;
	bool               GetBool() const;  // is aValueFloat equal to 1.f?

	// operators !!!!!!

	operator const float&() { return apData->aValueFloat; }
	//operator double()                                   { return aValueFloat; }
	//operator bool()                                     { return GetBool(); }
	// operator const std::string&()                       { return aValue.data(); }
	operator std::string_view()                         { return apData->apValue; }
	operator const char*()                              { return apData->apValue; }

	bool    operator==( const bool& other )             { return other == GetBool(); }
	bool    operator==( const float& other )            { return other == apData->aValueFloat; }
	bool    operator==( const double& other )           { return other == apData->aValueFloat; }
	bool    operator==( const std::string& other )      { return ch_strcmplen( other, apData->aValueLen, apData->apValue ); }
	bool    operator==( const char* other )             { return ch_strcmplen( other, apData->aValueLen, apData->apValue ); }

	bool    operator!=( const bool& other )             { return other != GetBool(); }
	bool    operator!=( const float& other )            { return other != apData->aValueFloat; }
	bool    operator!=( const double& other )           { return other != apData->aValueFloat; }
	bool    operator!=( const std::string& other )      { return !ch_strcmplen( other, apData->aValueLen, apData->apValue ); }
	bool    operator!=( const char* other )             { return !ch_strcmplen( other, apData->aValueLen, apData->apValue ); }

	void    operator=( const bool& other )              { SetValue( other ? 1.f : 0.f ); }
	void    operator=( const float& other )             { SetValue( other ); }
//	void    operator=( const double& other )            { SetValue( other ); }
	void    operator=( const std::string& other )       { SetValue( other ); }
	void    operator=( const char* other )              { SetValue( other ); }

	CVAR_OP_EQ_SELF(    const float,    GetFloat(), other );
	CVAR_OP_EQ_SELF(    const double,   GetFloat(), other );

	CVAR_OP_EQ_SELF(    const ConVar,   GetFloat(), other.GetFloat() );
	CVAR_OP( float,     const ConVar,   GetFloat(), other.GetFloat() );

	bool    operator<=( ConVarRef& other );
	bool    operator>=( ConVarRef& other );
	bool    operator==( ConVarRef& other );
	bool    operator!=( ConVarRef& other );

	float   operator*( ConVarRef& other );
	float   operator/( ConVarRef& other );
	float   operator+( ConVarRef& other );
	float   operator-( ConVarRef& other );

	friend class Console;
	friend class ConVarRef;

	ConVarData* apData;

private:
	// NO COPYING!!
	ConVar( const ConVar& );
};


class CORE_API ConVarRef : public ConVarBase
{
public:

	ConVarRef( const char* name ): ConVarBase( name )
	{
		Init(  );
	}

	void                Init();
	void                SetReference( ConVar* ref );
	bool                Valid();  // do we have a pointer to another cvar or not?

	std::string         GetPrintMessage(  ) override;

	void                SetValue( const std::string& value );
	void                SetValue( float value );

	std::string_view    GetValue();
	float               GetFloat();
	bool                GetBool();  // is aValueFloat equal to 1.f?

	bool aValid = false;
	ConVar* apRef = nullptr;

	// less operators !!!!!!

	operator float()                                    { return GetFloat(); }
	//operator double()                                   { return GetFloat(); }
	//operator bool()                                     { return GetBool(); }
	operator std::string_view()                         { return GetValue(); }
	operator const char*()                              { return GetValue().data(); }

	bool    operator==( const bool& other )             { return other == GetBool(); }
	float   operator==( const float& other )            { return other == GetFloat(); }
	double  operator==( const double& other )           { return other == GetFloat(); }
	bool    operator==( const std::string& other )      { return other == GetValue(); }
	bool    operator==( const char* other )             { return other == GetValue(); }

	bool    operator!=( const bool& other )             { return other != GetBool(); }
	float   operator!=( const float& other )            { return other != GetFloat(); }
	double  operator!=( const double& other )           { return other != GetFloat(); }
	bool    operator!=( const std::string& other )      { return other != GetValue(); }
	bool    operator!=( const char* other )             { return other != GetValue(); }

	void    operator=( const bool& other )              { SetValue( other ? 1.f : 0.f ); }
	void    operator=( const float& other )             { SetValue( other ); }
	void    operator=( const double& other )            { SetValue( other ); }
	void    operator=( const std::string& other )       { SetValue( other ); }
	void    operator=( const char* other )              { SetValue( other ); }

	CVAR_OP_EQ_SELF(    const float,    GetFloat(), other );
	CVAR_OP_EQ_SELF(    const double,   GetFloat(), other );

	CVAR_OP_EQ_SELF(    const ConVar,   GetFloat(), other.GetFloat() );
	CVAR_OP( float,     const ConVar,   GetFloat(), other.GetFloat() );

	CVAR_OP_EQ_SELF(    ConVarRef,      GetFloat(), other.GetFloat() );
	CVAR_OP( float,     ConVarRef,      GetFloat(), other.GetFloat() );

	friend class Console;

private:

	// NO COPYING!!
	ConVarRef( const ConVarRef& );
};


#define CONVAR( name, ... ) \
	ConVar name( #name, __VA_ARGS__ )

#define CONVAR_CMD( name, value ) \
	void cvarfunc_##name( const std::string& prevString, float prevFloat, const std::vector< std::string >& args ); \
	ConVar name( #name, value, cvarfunc_##name ); \
	void cvarfunc_##name( const std::string& prevString, float prevFloat, const std::vector< std::string >& args )

#define CONVAR_CMD_EX( name, ... ) \
	void cvarfunc_##name( const std::string& prevString, float prevFloat, const std::vector< std::string >& args ); \
	ConVar name( #name, __VA_ARGS__, cvarfunc_##name ); \
	void cvarfunc_##name( const std::string& prevString, float prevFloat, const std::vector< std::string >& args )

#define CONVARREF( name ) \
	ConVarRef name( #name )


//#define CON_COMMAND( name ) \
//	void CC_#name( std::vector< std::string > args ); \
//	ConCommand2 name( #name, CC_#name );              \
//	void CC_#name( std::vector< std::string > args )

#define CONCMD( name ) \
	void       name##_func( const std::vector< std::string >& args ); \
	ConCommand name##_cmd( #name, name##_func );              \
	void       name##_func( const std::vector< std::string >& args )

#define CONCMD_VA( name, ... ) \
	void name( const std::vector< std::string >& args ); \
	ConCommand name##_cmd( #name, name, __VA_ARGS__ );              \
	void       name( const std::vector< std::string >& args )

#define CONCMD_DROP( name, func ) \
	void       name( const std::vector< std::string >& args ); \
	ConCommand name##_cmd( #name, name, func );              \
	void       name( const std::vector< std::string >& args )

#define CONCMD_DROP_VA( name, func, ... ) \
	void       name( const std::vector< std::string >& args ); \
	ConCommand name##_cmd( #name, name, __VA_ARGS__, func );              \
	void       name( const std::vector< std::string >& args )

#define CON_COMMAND( name ) CONCMD( name )

#define CON_COMMAND_LAMBDA( name ) \
	ConCommand* name = new ConCommand( #name, [ & ]( const std::vector< std::string >& sArgs )


// ----------------------------------------------------------------
// Console Functions

CORE_API void  Con_SetConVarRegisterFlags( ConVarFlag_t sFlags );

// Register all the ConCommands and ConVars created from static initialization
CORE_API void  Con_RegisterConVars();

CORE_API const std::vector< std::string >& Con_GetCommandHistory();

// Checks if the current ConVar is a reference, if so, then return what it's pointing to
// return nullptr if it doesn't point to anything, and return the normal cvar if it's not a ConVarRef
// CORE_API ConVarBase*                         Con_CheckForConVarRef( ConVarBase* cvar );

// Build an auto complete list
CORE_API void                              Con_BuildAutoCompleteList( const std::string& srSearch, std::vector< std::string >& srResults );

// Get all ConVars
// CORE_API const ChVector< ConVarBase* >&      Con_GetConVars();

CORE_API uint32_t                          Con_GetConVarCount();
CORE_API ConVarBase*                       Con_GetConVar( uint32_t sIndex );

CORE_API ConVar*                           Con_GetConVar( std::string_view name );
CORE_API ConVarBase*                       Con_GetConVarBase( std::string_view name );

CORE_API ConVarData*                       Con_CreateConVarData();
CORE_API void                              Con_FreeConVarData( ConVarData* spData );

CORE_API std::string_view      Con_GetConVarValue( std::string_view name );
CORE_API float                 Con_GetConVarFloat( std::string_view name );

CORE_API void                  Con_PrintAllConVars();

// Add a command to the queue
CORE_API void                  Con_QueueCommand( const std::string& srCmd );
CORE_API void                  Con_QueueCommandSilent( const std::string& srCmd, bool sAddToHistory = true );

// Go through and run every command in the queue
CORE_API void                  Con_Update();

// Find and run a command instantly
CORE_API void                  Con_RunCommand( std::string_view command );
// CORE_API void                                Con_RunCommandF( const char* command, ... );

// Find and run a parsed command instantly
CORE_API bool                  Con_RunCommandArgs( const std::string& name, const std::vector< std::string >& args );

CORE_API void                  Con_ParseCommandLine( std::string_view command, std::string& name, std::vector< std::string >& args );
CORE_API void                  Con_ParseCommandLineEx( std::string_view command, std::string& name, std::vector< std::string >& args, size_t& i );

CORE_API ConVarFlag_t          Con_CreateCvarFlag( const char* name );
CORE_API const char*           Con_GetCvarFlagName( ConVarFlag_t flag );
CORE_API ConVarFlag_t          Con_GetCvarFlag( const char* name );
CORE_API size_t                Con_GetCvarFlagCount();

CORE_API void                  Con_SetCvarFlagCallback( ConVarFlag_t sFlag, ConVarFlagChangeFunc* spCallback );
CORE_API ConVarFlagChangeFunc* Con_GetCvarFlagCallback( ConVarFlag_t flag );

// Add a callback function to add data to the config.cfg file written from CVARF_ARCHIVE
CORE_API void                  Con_AddArchiveCallback( FArchive* spFunc );

// Write a config of all stuff we want saved
CORE_API void                  Con_Archive( const char* spFile = nullptr );

// Set Default Console Archive File, nullptr to reset to default
CORE_API void                  Con_SetDefaultArchive( const char* spFile, const char* spDefaultFile );

// make sure the convar flag is created when doing static initilization
// CORE_API ConVarFlag_t                        Con_StaticGetCvarFlag( const char* spFile = nullptr );

// New Convar System

// CORE_API Handle                              Con_RegisterVar( const char* spName, const char* spDesc );


#define CVARF( name )         Con_CreateCvarFlag( "CVARF_" #name )
#define NEW_CVAR_FLAG( name ) ConVarFlag_t name = Con_CreateCvarFlag( #name )
#define EXT_CVAR_FLAG( name ) extern ConVarFlag_t name


CORE_API EXT_CVAR_FLAG( CVARF_NONE );
CORE_API EXT_CVAR_FLAG( CVARF_ARCHIVE );  // save this convar value in a config.cfg file
CORE_API EXT_CVAR_FLAG( CVARF_INSTANT );  // instantly call this command (only works with ConCommand)


#ifdef _MSC_VER
	#pragma warning(pop)
#endif

