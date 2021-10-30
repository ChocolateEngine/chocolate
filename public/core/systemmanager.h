/*
systemmanager.h ( Authored by Demez )

Declares and defines the SystemManager, 
an interface for getting systems from the engine 
*/
#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <any>
#include <vector>

#include "core/platform.h"

#ifdef _MSC_VER
	// Disable this useless warning of vars in a class needing a DLL Interface when exported
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif

//#define GET_SYSTEM( sType ) systems->Get< sType >( typeid( sType ) )
#define GET_SYSTEM( sType ) systems->Get< sType >()

// slightly overkill
#define GET_SYSTEM_CHECK( sVar, sType ) \
	sVar = GET_SYSTEM( sType ); // \
	//if ( sVar == nullptr ) \
	//	apCommandManager->Execute( Engine::Commands::EXIT );


class BaseSystem;


class CORE_API SystemManager
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


CORE_API extern SystemManager* systems;


#ifdef _MSC_VER
	#pragma warning(pop)
#endif

