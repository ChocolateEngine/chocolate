/*
console.h ( Authored by p0lyh3dron )

Declares the sdfhuosdfhuiosdfhusdfhuisfhu
*/

#pragma once

#include <string>
#include <vector>
#include <functional>

#include "../types/msg.h"


//typedef void (*ConCommandFunc)( int argc, char* argv );
typedef void (*ConCommandFunc)( std::vector< std::string > );


class ConCommand
{
public:

	ConCommand( const std::string& name, ConCommandFunc func )
	{
		Init( name, func );
	}

	void Init( const std::string& name, ConCommandFunc func );

	std::string aName;
	ConCommandFunc aFunc;
};


//#define CON_COMMAND( name ) \
//	void CC_#name( std::vector< std::string > args ); \
//	ConCommand2 name( #name, CC_#name );              \
//	void CC_#name( std::vector< std::string > args )

#define CON_COMMAND( name ) \
	void name( std::vector< std::string > args ); \
	ConCommand name##_cmd( #name, name );              \
	void name( std::vector< std::string > args )


class Console
{
	typedef std::string 			String;
	typedef std::vector< std::string >	StringList;
protected:
	int 				aCmdIndex = -1;
	StringList 			aQueue;
	std::vector< ConCommand* >	aConCommandList;
public:

	/* Register all the ConCommands and ConVars created from static initialization.  */
	void 	RegisterConVars(  );
	/* A.  */
	void 	AddConCommand( ConCommand* cmd );

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


