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

// ConCommand console dropdown list function

using ConVarFlag_t = size_t;


// why do i not need to forward declare the other ones?
class ConVarRef;


class CORE_API ConVarBase
{
public:
	ConVarBase( const std::string& name, ConVarFlag_t flags = 0 );

	ConVarBase( const std::string& name, const std::string& desc );
	ConVarBase( const std::string& name, ConVarFlag_t flags, const std::string& desc );

	virtual std::string		GetPrintMessage(  ) = 0;

	const std::string&      GetName();
	const std::string&      GetDesc();
	ConVarFlag_t            GetFlags();
	ConVarBase*             GetNext();

	friend class ConCommand;
	friend class ConVar;
	friend class ConVarRef;
	friend class Console;

	static ConVarBase*      spConVarBases;
	ConVarBase*             apNext = nullptr;

private:
	std::string             aName;
	std::string             aDesc;
	ConVarFlag_t            aFlags;

	// NO COPYING!!
	ConVarBase( const ConVarBase& );
};


class CORE_API ConCommand : public ConVarBase
{
public:

	ConCommand( const std::string& name, ConCommandFunc func, ConVarFlag_t flags = 0 ):
		ConVarBase( name, flags )
	{
		Init( func, aDropDownFunc );
	}

	ConCommand( const std::string& name, ConCommandFunc func, const std::string& desc ):
		ConVarBase( name, desc )
	{
		Init( func, aDropDownFunc );
	}

	ConCommand( const std::string& name, ConCommandFunc func, ConCommandDropdownFunc dropDownFunc ):
		ConVarBase( name )
	{
		Init( func, dropDownFunc );
	}

	ConCommand( const std::string& name, ConCommandFunc func, ConVarFlag_t flags, ConCommandDropdownFunc dropDownFunc ):
		ConVarBase( name, flags )
	{
		Init( func, dropDownFunc );
	}
	
	ConCommand( const std::string& name, ConCommandFunc func, ConVarFlag_t flags, const std::string& desc ):
		ConVarBase( name, flags, desc )
	{
		Init( func, aDropDownFunc );
	}

	ConCommand( const std::string& name, ConCommandFunc func, ConVarFlag_t flags, const std::string& desc, ConCommandDropdownFunc dropDownFunc ):
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

	ConVar( const std::string& name, const std::string& defaultValue, ConVarFlag_t flags = 0 ):
		ConVarBase( name, flags )
	{
		Init( defaultValue, aFunc );
	}

	ConVar( const std::string& name, const std::string& defaultValue, const std::string& desc ):
		ConVarBase( name, desc )
	{
		Init( defaultValue, aFunc );
	}

	ConVar( const std::string& name, const std::string& defaultValue, ConVarFlag_t flags, const std::string& desc ):
		ConVarBase( name, flags, desc )
	{
		Init( defaultValue, aFunc );
	}

	ConVar( const std::string& name, const std::string& defaultValue, ConVarFlag_t flags, const std::string& desc, ConVarFunc callback ):
		ConVarBase( name, flags, desc )
	{
		Init( defaultValue, callback );
	}


	ConVar( const std::string& name, float defaultValue, ConVarFlag_t flags = 0 ):
		ConVarBase( name, flags )
	{
		Init( defaultValue, aFunc );
	}

	ConVar( const std::string& name, float defaultValue, const std::string& desc ):
		ConVarBase( name, desc )
	{
		Init( defaultValue, aFunc );
	}

	ConVar( const std::string& name, float defaultValue, ConVarFlag_t flags, const std::string& desc ):
		ConVarBase( name, flags, desc )
	{
		Init( defaultValue, aFunc );
	}

	ConVar( const std::string& name, float defaultValue, ConVarFlag_t flags, const std::string& desc, ConVarFunc callback ):
		ConVarBase( name, flags, desc )
	{
		Init( defaultValue, callback );
	}


	ConVar( const std::string& name, const std::string& defaultValue, ConVarFunc callback ):
		ConVarBase( name )
	{
		Init( defaultValue, callback );
	}

	ConVar( const std::string& name, float defaultValue, ConVarFunc callback ):
		ConVarBase( name )
	{
		Init( defaultValue, callback );
	}


	void                Init( const std::string& defaultValue, ConVarFunc func );
	void                Init( float defaultValue, ConVarFunc func );

	std::string         GetPrintMessage(  ) override;

	void                SetValue( const std::string& value );
	void                SetValue( float value );

	void                Reset(  );

	const std::string&  GetValue(  );
	float               GetFloat(  );
	int                 GetInt(  );
	bool                GetBool(  );  // is aValueFloat equal to 1.f?

	// operators !!!!!!

	operator float()                                    { return aValueFloat; }
	//operator double()                                   { return aValueFloat; }
	//operator bool()                                     { return GetBool(); }
	operator std::string()                              { return aValue; }
	operator const char*()                              { return aValue.c_str(); }

	bool    operator==( const bool& other )             { return other == GetBool(); }
	float   operator==( const float& other )            { return other == aValueFloat; }
	double  operator==( const double& other )           { return other == aValueFloat; }
	bool    operator==( const std::string& other )      { return other == aValue; }
	bool    operator==( const char* other )             { return other == aValue; }

	bool    operator!=( const bool& other )             { return other != GetBool(); }
	float   operator!=( const float& other )            { return other != aValueFloat; }
	double  operator!=( const double& other )           { return other != aValueFloat; }
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

private:
	std::string aDefaultValue;
	std::string aValue;
	float       aDefaultValueFloat;
	float       aValueFloat;
	ConVarFunc  aFunc;

	// NO COPYING!!
	ConVar( const ConVar& );
};


class CORE_API ConVarRef : public ConVarBase
{
public:

	ConVarRef( const std::string& name ): ConVarBase( name )
	{
		Init(  );
	}

	void                Init(  );
	void                SetReference( ConVar* ref );
	bool                Valid(  );  // do we have a pointer to another cvar or not?

	std::string         GetPrintMessage(  ) override;

	void                SetValue( const std::string& value );
	void                SetValue( float value );

	const std::string&  GetValue(  );
	float               GetFloat(  );
	bool                GetBool(  );  // is aValueFloat equal to 1.f?

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

#define CONVARREF( name ) \
	ConVarRef name( #name )


//#define CON_COMMAND( name ) \
//	void CC_#name( std::vector< std::string > args ); \
//	ConCommand2 name( #name, CC_#name );              \
//	void CC_#name( std::vector< std::string > args )

#define CONCMD( name ) \
	void name( std::vector< std::string > args ); \
	ConCommand name##_cmd( #name, name );              \
	void name( std::vector< std::string > args )

#define CONCMD_VA( name, ... ) \
	void name( std::vector< std::string > args ); \
	ConCommand name##_cmd( #name, name, __VA_ARGS__ );              \
	void name( std::vector< std::string > args )

#define CONCMD_DROP( name, func ) \
	void name( std::vector< std::string > args ); \
	ConCommand name##_cmd( #name, name, func );              \
	void name( std::vector< std::string > args )

#define CONCMD_DROP_VA( name, func, ... ) \
	void name( std::vector< std::string > args ); \
	ConCommand name##_cmd( #name, name, __VA_ARGS__, func );              \
	void name( std::vector< std::string > args )

#define CON_COMMAND( name ) \
	void name( std::vector< std::string > args ); \
	ConCommand name##_cmd( #name, name );              \
	void name( std::vector< std::string > args )

#define CON_COMMAND_LAMBDA( name ) \
	ConCommand* name = new ConCommand( #name, [ & ]( std::vector< std::string > sArgs )



class CORE_API Console
{
	typedef std::string 			String;
	typedef std::vector< std::string >	StringList;

protected:
	int                                 aCmdIndex = -1;
	StringList                          aQueue;
	StringList                          aCommandHistory;
	std::string                         aTextBuffer;
	StringList                          aAutoCompleteList;

	std::vector< ConCommand* >          aInstantConVars;

	void                                AddToCommandHistory( const std::string& srCmd );
	void                                CheckInstantCommands( const std::string& srCmd );

public:

	/* Register all the ConCommands and ConVars created from static initialization.  */
	void                                RegisterConVars(  );

	const std::vector< std::string >&   GetCommandHistory(  );

	// Checks if the current ConVar is a reference, if so, then return what it's pointing to
	// return nullptr if it doesn't point to anything, and return the normal cvar if it's not a ConVarRef
	ConVarBase*                         CheckForConVarRef( ConVarBase* cvar );

	/* Set and get current user text input.  */
	void                                SetTextBuffer( const std::string& str, bool recalculateList = true );
	const std::string&                  GetTextBuffer(  );

	void                                CalculateAutoCompleteList( const std::string& textBuffer );
	const std::vector< std::string >&   GetAutoCompleteList(  );

	ConVar*                             GetConVar( const std::string& name );
	ConVarBase*                         GetConVarBase( const std::string& name );

	const std::string&                  GetConVarValue( const std::string& name );
	float                               GetConVarFloat( const std::string& name );

	void                                PrintAllConVars(  );

	/* Add a command to the queue  */
	void                                QueueCommand( const std::string& srCmd );
	void                                QueueCommandSilent( const std::string& srCmd, bool sAddToHistory = true );

	/* Go through and run every command inputed into the console last frame  */
	void                                Update(  );

	/* Find and run a command instantly  */
	void                                RunCommand( const std::string& command );
	void                                RunCommandF( const char* command, ... );

	/* Find and run a parsed command instantly  */
	bool                                RunCommand( const std::string &name, const std::vector< std::string > &args );

	void                                ParseCommandLine( const std::string& command, std::string &name, std::vector< std::string > &args );
	void                                ParseCommandLine( const std::string& command, std::string &name, std::vector< std::string > &args, size_t& i );

	ConVarFlag_t                        CreateCvarFlag( const char* name );
	const char*                         GetCvarFlagName( ConVarFlag_t flag );
	ConVarFlag_t                        GetCvarFlag( const char* name );

	/* A.  */
		Console(  );
	/* A.  */
		~Console(  );

friend class ConCommand;
};


CORE_API extern Console* console;

CORE_API Console& GetConsole();  // maybe Core_GetConsole()?


#define NEW_CVAR_FLAG( name ) ConVarFlag_t name = GetConsole().CreateCvarFlag( #name )
#define EXT_CVAR_FLAG( name ) extern ConVarFlag_t name


CORE_API EXT_CVAR_FLAG( CVARF_NONE );
CORE_API EXT_CVAR_FLAG( CVARF_ARCHIVE );  // save this convar value in a config.cfg file
CORE_API EXT_CVAR_FLAG( CVARF_INSTANT );  // instantly call this command (only works with ConCommand)


#ifdef _MSC_VER
	#pragma warning(pop)
#endif

