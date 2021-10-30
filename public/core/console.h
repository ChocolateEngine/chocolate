/*
console.h ( Authored by p0lyh3dron )

Declares the sdfhuosdfhuiosdfhusdfhuisfhu
*/

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "types/databuffer.hh"
#include "core/platform.h"


#ifdef _MSC_VER
	// Disable this useless warning of vars in a class needing a DLL Interface when exported
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif


void CORE_API Print( const char* str, ... );


typedef std::function< void( std::vector< std::string > args ) > ConVarFunc;


// why do i not need to forward declare the other ones?
class ConVarRef;


class CORE_API ConVarBase
{
public:
	ConVarBase( const std::string& name );
	virtual std::string		GetPrintMessage(  ) = 0;

	const std::string&      GetName(  );
	ConVarBase*             GetNext(  );

	friend class ConCommand;
	friend class ConVar;
	friend class ConVarRef;
	friend class Console;

private:
	std::string             aName;
	ConVarBase*             apNext = nullptr;

	static ConVarBase*      spConVarBases;
};


class CORE_API ConCommand : public ConVarBase
{
public:

	ConCommand( const std::string& name, ConVarFunc func ):
		ConVarBase( name )
	{
		Init( name, func );
	}

	void Init( const std::string& name, ConVarFunc func );

	std::string GetPrintMessage(  ) override;

	ConVarFunc aFunc;

	friend class Console;
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

	ConVar( const std::string& name, const std::string& defaultValue ): ConVarBase( name )
	{
		Init( name, defaultValue, aFunc );
	}

	ConVar( const std::string& name, const std::string& defaultValue, ConVarFunc callback ): ConVarBase( name )
	{
		Init( name, defaultValue, callback );
	}

	ConVar( const std::string& name, float defaultValue ): ConVarBase( name )
	{
		Init( name, defaultValue, aFunc );
	}

	ConVar( const std::string& name, float defaultValue, ConVarFunc callback ): ConVarBase( name )
	{
		Init( name, defaultValue, callback );
	}

	void                Init( const std::string& name, const std::string& defaultValue, ConVarFunc func );
	void                Init( const std::string& name, float defaultValue, ConVarFunc func );

	std::string         GetPrintMessage(  ) override;

	void                SetValue( const std::string& value );
	void                SetValue( float value );

	void                Reset(  );

	const std::string&  GetValue(  );
	float               GetFloat(  );
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
};


#define CONVAR( name, ... ) \
	ConVar name( #name, __VA_ARGS__ )

#define CONVARREF( name ) \
	ConVarRef name( #name )


//#define CON_COMMAND( name ) \
//	void CC_#name( std::vector< std::string > args ); \
//	ConCommand2 name( #name, CC_#name );              \
//	void CC_#name( std::vector< std::string > args )

#define CON_COMMAND( name ) \
	void name( std::vector< std::string > args ); \
	ConCommand name##_cmd( #name, name );              \
	void name( std::vector< std::string > args )

#define CON_COMMAND_LAMBDA( name ) \
	ConCommand* name = new ConCommand( #name, [ & ]( std::vector< std::string > sArgs )


// will setup later in the future
// maybe for a more advanced console than can let you search and filter out these Msg types
// kind of like source 2 vconsole
// i also want to setup some logging channel system, so i don't need to do "[System] blah" on everything
enum class Msg
{
	Normal,
	Dev,
	Warning,
	Error
};


class CORE_API Console
{
	typedef std::string 			String;
	typedef std::vector< std::string >	StringList;

protected:
	int                                 aCmdIndex = -1;
	StringList                          aQueue;
	std::vector< ConVarBase* >	        aConVarList;
	std::vector< std::string >          aConsoleHistory;
	StringList                          aCommandHistory;
	std::string                         aTextBuffer;
	StringList                          aAutoCompleteList;

	void                                AddToHistory( const std::string& str );

public:

	/* Register all the ConCommands and ConVars created from static initialization.  */
	void 	RegisterConVars(  );
	/* A.  */
	const std::vector<std::string>&     GetConsoleHistory(  );
	std::string                         GetConsoleHistoryStr(  );

	const std::vector< std::string >&   GetCommandHistory(  );

	/* Set and get current user text input.  */
	void                                SetTextBuffer( const std::string& str, bool recalculateList = true );
	const std::string&                  GetTextBuffer(  );

	void                                CalculateAutoCompleteList( const std::string& textBuffer );
	const std::vector< std::string >&   GetAutoCompleteList(  );

	ConVar*                             GetConVar( const std::string& name );
	std::string                         GetConVarValue( const std::string& name );
	float                               GetConVarFloat( const std::string& name );

	void                                PrintAllConVars(  );

	void                                Print( const char* format, ... );
	void                                Print( Msg type, const char* format, ... );

	/* A.  */
	bool 	Empty(  );
	/* A.  */
	void 	Add( const String &srCmd );
	/* A.  */
	void 	DeleteCommand(  );
	/* A.  */
	void 	Update(  );
	/* A.  */
	void 	Clear(  );
	/* A.  */
	void 	MoveCommands(  );
	/* A.  */
	String 	FetchCmd(  );
	/* A.  */
		Console(  );
	/* A.  */
		~Console(  );

friend class ConCommand;
};


CORE_API extern Console* console;


#ifdef _MSC_VER
	#pragma warning(pop)
#endif
