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
	sVar = GET_SYSTEM( sType ); \
	if ( sVar == nullptr ) \
		apCommandManager->Execute( Engine::Commands::EXIT );


class BaseSystem;


class SystemManager
{
	// typedef std::vector< std::any > 	SystemList;
	typedef std::vector< BaseSystem* > 	SystemList;
	typedef std::unordered_map< std::type_index, std::any > 	SystemMap;
public:

	/*template< typename T >
	void Add( T* sys )
	{
		aSystemList.push_back( sys );
	}*/

	void Add( BaseSystem* sys )
	{
		aSystemList.push_back( sys );
	}

	/*template< typename T, typename Base >
	void Add( T* sys )
	{
		aSystemMap[Base] = sys;
	}*/

	/*template< typename T >
	T* GetOld( const std::type_info& type )
	{
		for ( const auto& sys: aSystemList )
		{
			if ( sys.type().hash_code() == type.hash_code() )
				return std::any_cast<T*>(sys);
		}

		return nullptr;
	}*/

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

	/*template< typename T, typename Base >
	T* Get( const std::type_info& type )
	{
		return aSystemMap[Base];

		/*for ( auto const& [key, val] : aSystemMap )
		{
			if ( sys.type() == type )
				return std::any_cast<T*>(sys);
		}

		return nullptr;
	}*/

	/* Returns a system list for iterating through. */
	/*std::vector< BaseSystem* > GetSystemList()
	{
		return std::any_cast<std::vector< BaseSystem* >>(aSystemList);
	}*/
	
	SystemList GetSystemList()
	{
		return aSystemList;
	}

private:
	SystemList aSystemList;
	SystemMap aSystemMap;
};
