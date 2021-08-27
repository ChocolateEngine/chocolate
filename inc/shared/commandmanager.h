/*
commandmanager.h ( Authored by p0lyh3dron )

Declares and defines the CommandManager, 
a message-like  interface for instant 
message handling.  
*/
#pragma once

#include <memory>
#include <unordered_map>

class BaseCommand
{
public:
	/* Virtual deconstructor for the function.  */
	virtual 	~BaseCommand(  ){  }
};

template < class... Args >
class Command : public BaseCommand
{
private:
	typedef std::function< void( Args... ) > FuncType;
	FuncType	aFunction;
public:
		Command(  ){  };
  		Command( FuncType sFunction ) : aFunction( sFunction ){  }
	void 	operator(  )( Args... sArgs ){ if( aFunction ) aFunction( sArgs... ); }
};

class CommandManager
{
	typedef BaseCommand 						*BaseCommandPtr;
	typedef std::unordered_map< int, BaseCommandPtr >	        CommandList;
	typedef std::unordered_map< std::type_index, CommandList > 	FMap;
public:
	template < class TEnum, class T >
		void Add( TEnum sName, const T& srCmd ) { aFmap[ typeid( TEnum ) ][ ( int )sName ] = BaseCommandPtr( new T( srCmd ) ); }
	template < class T, class... Args >	
		void Execute( T sName, Args... sArgs )
		{
			typedef Command< Args... > 	CommandType;
			CommandType *c = reinterpret_cast< CommandType* >( aFmap[ typeid( T ) ][ ( int )sName ] );
			if ( c )
				( *c )( sArgs... );
		}
private:
	FMap aFmap;
};
