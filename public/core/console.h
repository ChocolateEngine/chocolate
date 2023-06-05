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


typedef std::function< void( const std::vector< std::string >& args ) > ConCommandFunc;
typedef std::function< void( const std::string& prevString, float prevFloat, const std::vector< std::string >& args ) > ConVarFunc;

typedef std::function< void(
	const std::vector< std::string >& args,  // arguments currently typed in by the user
	std::vector< std::string >& results      // results to populate the dropdown list with
) > ConCommandDropdownFunc;

using FArchive = void( std::string& srOutput );

// ConCommand console dropdown list function

using ConVarFlag_t = size_t;


// why do i not need to forward declare the other ones?
class ConVarRef;


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


class CORE_API ChConfigOption
{
  public:

	  EChConfigOption aType;
};


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


class CORE_API ConCommand : public ConVarBase
{
public:

	ConCommand( const char* name, ConCommandFunc func, ConVarFlag_t flags = 0 ):
		ConVarBase( name, flags )
	{
		Init( func, aDropDownFunc );
	}

	ConCommand( const char* name, ConCommandFunc func, const char* desc ) :
		ConVarBase( name, desc )
	{
		Init( func, aDropDownFunc );
	}

	ConCommand( const char* name, ConCommandFunc func, ConCommandDropdownFunc dropDownFunc ):
		ConVarBase( name )
	{
		Init( func, dropDownFunc );
	}

	ConCommand( const char* name, ConCommandFunc func, ConVarFlag_t flags, ConCommandDropdownFunc dropDownFunc ):
		ConVarBase( name, flags )
	{
		Init( func, dropDownFunc );
	}
	
	ConCommand( const char* name, ConCommandFunc func, ConVarFlag_t flags, const char* desc ) :
		ConVarBase( name, flags, desc )
	{
		Init( func, aDropDownFunc );
	}

	ConCommand( const char* name, ConCommandFunc func, ConVarFlag_t flags, const char* desc, ConCommandDropdownFunc dropDownFunc ) :
		ConVarBase( name, flags, desc )
	{
		Init( func, dropDownFunc );
	}

	void Init( ConCommandFunc func, ConCommandDropdownFunc dropDownFunc );

	std::string GetPrintMessage(  ) override;

	ConCommandFunc aFunc;
	ConCommandDropdownFunc aDropDownFunc;

	friend class Console;

private:

	// NO COPYING!!
	ConCommand( const ConCommand& );
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
		Init( defaultValue, aFunc );
	}

	ConVar( const char* name, std::string_view defaultValue, const char* desc ) :
		ConVarBase( name, desc )
	{
		Init( defaultValue, aFunc );
	}

	ConVar( const char* name, std::string_view defaultValue, ConVarFlag_t flags, const char* desc ) :
		ConVarBase( name, flags, desc )
	{
		Init( defaultValue, aFunc );
	}

	ConVar( const char* name, std::string_view defaultValue, ConVarFlag_t flags, const char* desc, ConVarFunc callback ) :
		ConVarBase( name, flags, desc )
	{
		Init( defaultValue, callback );
	}


	ConVar( const char* name, float defaultValue, ConVarFlag_t flags = 0 ):
		ConVarBase( name, flags )
	{
		Init( defaultValue, aFunc );
	}

	ConVar( const char* name, float defaultValue, const char* desc ) :
		ConVarBase( name, desc )
	{
		Init( defaultValue, aFunc );
	}

	ConVar( const char* name, float defaultValue, ConVarFlag_t flags, const char* desc ) :
		ConVarBase( name, flags, desc )
	{
		Init( defaultValue, aFunc );
	}

	ConVar( const char* name, float defaultValue, ConVarFlag_t flags, const char* desc, ConVarFunc callback ) :
		ConVarBase( name, flags, desc )
	{
		Init( defaultValue, callback );
	}


	ConVar( const char* name, std::string_view defaultValue, ConVarFunc callback ) :
		ConVarBase( name )
	{
		Init( defaultValue, callback );
	}

	ConVar( const char* name, float defaultValue, ConVarFunc callback ):
		ConVarBase( name )
	{
		Init( defaultValue, callback );
	}

	
	void               Init( std::string_view defaultValue, ConVarFunc func );
	void               Init( float defaultValue, ConVarFunc func );

	std::string        GetPrintMessage() override;

	void               SetValue( std::string_view value );
	void               SetValue( float value );

	void               Reset();

	std::string_view   GetValue();
	float              GetFloat();
	int                GetInt();
	bool               GetBool();  // is aValueFloat equal to 1.f?

	// operators !!!!!!

	operator const float&()                             { return aValueFloat; }
	//operator double()                                   { return aValueFloat; }
	//operator bool()                                     { return GetBool(); }
	operator const std::string&()                       { return aValue; }
	operator const char*()                              { return aValue.c_str(); }

	bool    operator==( const bool& other )             { return other == GetBool(); }
	bool    operator==( const float& other )            { return other == aValueFloat; }
	bool    operator==( const double& other )           { return other == aValueFloat; }
	bool    operator==( const std::string& other )      { return other == aValue; }
	bool    operator==( const char* other )             { return other == aValue; }

	bool    operator!=( const bool& other )             { return other != GetBool(); }
	bool    operator!=( const float& other )            { return other != aValueFloat; }
	bool    operator!=( const double& other )           { return other != aValueFloat; }
	bool    operator!=( const std::string& other )      { return other != aValue; }
	bool    operator!=( const char* other )             { return other != aValue; }

	void    operator=( const bool& other )              { SetValue( other ? 1.f : 0.f ); }
	void    operator=( const float& other )             { SetValue( other ); }
//	void    operator=( const double& other )            { SetValue( other ); }
	void    operator=( const std::string& other )       { SetValue( other ); }
	void    operator=( const char* other )              { SetValue( other ); }

	CVAR_OP_EQ_SELF(    const float,    aValueFloat, other );
	CVAR_OP_EQ_SELF(    const double,   aValueFloat, other );

	CVAR_OP_EQ_SELF(    const ConVar,   GetFloat(), other.aValueFloat );
	CVAR_OP( float,     const ConVar,   GetFloat(), other.aValueFloat );

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

	std::string aDefaultValue;
	std::string aValue;
	float       aDefaultValueFloat;
	float       aValueFloat;
	ConVarFunc  aFunc;

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

	const std::string&  GetValue();
	float               GetFloat();
	bool                GetBool();  // is aValueFloat equal to 1.f?

	bool aValid = false;
	ConVar* apRef = nullptr;

	// less operators !!!!!!

	operator float()                                    { return GetFloat(); }
	//operator double()                                   { return GetFloat(); }
	//operator bool()                                     { return GetBool(); }
	operator std::string()                              { return GetValue(); }
	operator const char*()                              { return GetValue().c_str(); }

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

	CVAR_OP_EQ_SELF(    const ConVar,   GetFloat(), other.aValueFloat );
	CVAR_OP( float,     const ConVar,   GetFloat(), other.aValueFloat );

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

#define CON_COMMAND( name ) \
	void       name( const std::vector< std::string >& args ); \
	ConCommand name##_cmd( #name, name );              \
	void       name( const std::vector< std::string >& args )

#define CON_COMMAND_LAMBDA( name ) \
	ConCommand* name = new ConCommand( #name, [ & ]( const std::vector< std::string >& sArgs )


// ----------------------------------------------------------------
// Console Functions


// Register all the ConCommands and ConVars created from static initialization
CORE_API void                                Con_RegisterConVars();

CORE_API const std::vector< std::string >&   Con_GetCommandHistory();

// Checks if the current ConVar is a reference, if so, then return what it's pointing to
// return nullptr if it doesn't point to anything, and return the normal cvar if it's not a ConVarRef
// CORE_API ConVarBase*                         Con_CheckForConVarRef( ConVarBase* cvar );

// Build an auto complete list
CORE_API void                                Con_BuildAutoCompleteList( const std::string& srSearch, std::vector< std::string >& srResults );

// Get all ConVars
// CORE_API const ChVector< ConVarBase* >&      Con_GetConVars();

CORE_API uint32_t                            Con_GetConVarCount();
CORE_API ConVarBase*                         Con_GetConVar( uint32_t sIndex );

CORE_API ConVar*                             Con_GetConVar( std::string_view name );
CORE_API ConVarBase*                         Con_GetConVarBase( std::string_view name );

CORE_API const std::string&                  Con_GetConVarValue( std::string_view name );
CORE_API float                               Con_GetConVarFloat( std::string_view name );

CORE_API void                                Con_PrintAllConVars();

// Add a command to the queue
CORE_API void                                Con_QueueCommand( const std::string& srCmd );
CORE_API void                                Con_QueueCommandSilent( const std::string& srCmd, bool sAddToHistory = true );

// Go through and run every command in the queue
CORE_API void                                Con_Update();

// Find and run a command instantly
CORE_API void                                Con_RunCommand( std::string_view command );
// CORE_API void                                Con_RunCommandF( const char* command, ... );

// Find and run a parsed command instantly
CORE_API bool                                Con_RunCommandArgs( const std::string &name, const std::vector< std::string > &args );

CORE_API void                                Con_ParseCommandLine( std::string_view command, std::string& name, std::vector< std::string >& args );
CORE_API void                                Con_ParseCommandLineEx( std::string_view command, std::string& name, std::vector< std::string >& args, size_t& i );

CORE_API ConVarFlag_t                        Con_CreateCvarFlag( const char* name );
CORE_API const char*                         Con_GetCvarFlagName( ConVarFlag_t flag );
CORE_API ConVarFlag_t                        Con_GetCvarFlag( const char* name );
CORE_API size_t                              Con_GetCvarFlagCount();

// Add a callback function to add data to the config.cfg file
CORE_API void                                Con_AddArchiveCallback( FArchive* spFunc );

// Write a config of all stuff we want saved
CORE_API void                                Con_Archive( const char* spFile = nullptr );

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

