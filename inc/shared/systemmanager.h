/*
systemmanager.h ( Authored by Demez )

Declares and defines the SystemManager, 
an interface for getting systems from the engine 
*/
#pragma once

#include <memory>
#include <unordered_map>


//#define GET_SYSTEM( sType ) apSystemManager->Get< sType >( typeid( sType ) )
#define GET_SYSTEM( sType ) apSystemManager->Get< sType >()

// slightly overkill
#define GET_SYSTEM_CHECK( sVar, sType ) \
	sVar = GET_SYSTEM( sType ); // \
	//if ( sVar == nullptr ) \
	//	apCommandManager->Execute( Engine::Commands::EXIT );


class BaseSystem;


class SystemManager
{
	// typedef std::vector< std::any > 	SystemList;
	typedef std::vector< BaseSystem* > 	SystemList;
	typedef std::unordered_map< std::type_index, std::any > 	SystemMap;
public:

	void Add( BaseSystem* sys )
	{
		aSystemList.push_back( sys );
	}

	template< typename T >
	T* Get()
	{
		for ( const auto& sys: aSystemList )
		{
			if (T *result = dynamic_cast<T *>(sys))
			{
				return result;
			}
		}

		return nullptr;
	}
	
	const SystemList& GetSystemList()
	{
		return aSystemList;
	}

private:
	SystemList aSystemList;
	SystemMap aSystemMap;
};


extern SystemManager* systems;

