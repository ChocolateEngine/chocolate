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
};


class ConVar : public ConVarBase
{
public:

	ConVar( const std::string& name, const std::string& defaultValue ):
		ConVarBase( name )
	{
		Init( name, defaultValue, aFunc );
	}

	ConVar( const std::string& name, const std::string& defaultValue, ConVarFunc callback ):
		ConVarBase( name )
	{
		Init( name, defaultValue, callback );
	}

	void Init( const std::string& name, const std::string& defaultValue, ConVarFunc func );

	std::string GetPrintMessage(  );

	void SetValue( const std::string& value );

	const std::string& GetValue(  );
	float              GetFloat(  );

	std::string aName;
	std::string aDefaultValue;
	std::string aValue;
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

	void    CalculateAutoCompleteList(  );

public:

	/* Register all the ConCommands and ConVars created from static initialization.  */
	void 	RegisterConVars(  );
	/* A.  */
	const std::string&    GetConsoleHistory(  );

	StringList            GetCommandHistory(  );

	/* Set and get current user text input.  */
	void                  SetTextBuffer( const std::string& str );
	const std::string&    GetTextBuffer(  );

	const std::vector< std::string >& GetAutoCompleteList(  );

	void    PrintAllConVars(  );

	void    Print( const char* str, ... );

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


