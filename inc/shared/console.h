/*
console.h ( Authored by p0lyh3dron )

Declares the sdfhuosdfhuiosdfhusdfhuisfhu
*/

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "../types/msg.h"
#include "../types/databuffer.hh"


typedef std::function< void( std::vector< std::string > args ) > ConVarFunc;


class ConVarBase
{
public:
	ConVarBase( const std::string& name );
	virtual std::string		GetPrintMessage(  ) = 0;

	const std::string&      GetName(  );
	ConVarBase*             GetNext(  );

	friend class ConCommand;
	friend class ConVar;
	friend class Console;

private:
	std::string             aName;
	ConVarBase*             apNext = nullptr;

	static ConVarBase*      spConVarBases;
};


class ConCommand : public ConVarBase
{
public:

	ConCommand( const std::string& name, ConVarFunc func ):
		ConVarBase( name )
	{
		Init( name, func );
	}

	void Init( const std::string& name, ConVarFunc func );

	std::string GetPrintMessage(  );

	std::string aName;
	ConVarFunc aFunc;

	friend class Console;
};


class ConVar : public ConVarBase
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

	void Init( const std::string& name, const std::string& defaultValue, ConVarFunc func );
	void Init( const std::string& name, float defaultValue, ConVarFunc func );

	std::string GetPrintMessage(  );

	void SetValue( const std::string& value );
	void SetValue( float value );

	void Reset(  );

	const std::string& GetName(  );
	const std::string& GetValue(  );
	float              GetFloat(  );
	bool               GetBool(  );  // is aValueFloat equal to 1.f?

	// operators !!!!!!
	// might not use them out of habit lol

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

	void    operator=( const bool& other )              { SetValue( other ? 1.f : 0.f ); }
	void    operator=( const float& other )             { SetValue( other ); }
	void    operator=( const double& other )            { SetValue( other ); }
	void    operator=( const std::string& other )       { SetValue( other ); }
	void    operator=( const char* other )              { SetValue( other ); }

	void    operator*=( const float& other )            { SetValue( aValueFloat * other ); }
	void    operator*=( const double& other )           { SetValue( aValueFloat * other ); }

	void    operator/=( const float& other )            { SetValue( aValueFloat / other ); }
	void    operator/=( const double& other )           { SetValue( aValueFloat / other ); }

	void    operator+=( const float& other )            { SetValue( aValueFloat + other ); }
	void    operator+=( const double& other )           { SetValue( aValueFloat + other ); }

	void    operator-=( const float& other )            { SetValue( aValueFloat - other ); }
	void    operator-=( const double& other )           { SetValue( aValueFloat - other ); }

	bool    operator<=( const ConVar& other )           { return aValueFloat <= other.aValueFloat; }
	bool    operator>=( const ConVar& other )           { return aValueFloat >= other.aValueFloat; }
	bool    operator==( const ConVar& other )           { return aValueFloat == other.aValueFloat; }

	// useless?
#if 0
	float   operator*( const float& other )             { return aValueFloat * other; }
	double  operator*( const double& other )            { return aValueFloat * other; }

	float   operator/( const float& other )             { return aValueFloat / other; }
	double  operator/( const double& other )            { return aValueFloat / other; }

	float   operator+( const float& other )             { return aValueFloat + other; }
	double  operator+( const double& other )            { return aValueFloat + other; }

	float   operator-( const float& other )             { return aValueFloat - other; }
	double  operator-( const double& other )            { return aValueFloat - other; }

	float   operator*( const ConVar& other )            { return aValueFloat * other.aValueFloat; }
	float   operator/( const ConVar& other )            { return aValueFloat / other.aValueFloat; }
	float   operator+( const ConVar& other )            { return aValueFloat + other.aValueFloat; }
	float   operator-( const ConVar& other )            { return aValueFloat - other.aValueFloat; }
	float   operator<( const ConVar& other )            { return aValueFloat < other.aValueFloat; }
	float   operator>( const ConVar& other )            { return aValueFloat > other.aValueFloat; }
#endif

	friend class Console;

private:
	std::string aName;
	std::string aDefaultValue;
	std::string aValue;
	float       aDefaultValueFloat;
	float       aValueFloat;
	ConVarFunc  aFunc;
};



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


class Console
{
	typedef std::string 			String;
	typedef std::vector< std::string >	StringList;
protected:
	int 				aCmdIndex = -1;
	StringList 			aQueue;
	std::vector< ConVarBase* >	aConVarList;
	std::string         aConsoleHistory;
	StringList 			aCommandHistory;
	std::string         aTextBuffer;
	StringList          aAutoCompleteList;

	void    AddToHistory( const std::string& str );

public:

	/* Register all the ConCommands and ConVars created from static initialization.  */
	void 	RegisterConVars(  );
	/* A.  */
	const std::string&                  GetConsoleHistory(  );

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


