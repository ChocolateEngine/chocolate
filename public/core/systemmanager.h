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

//#define GET_SYSTEM( sType ) systems->Get< sType >( typeid( sType ) )
#define GET_SYSTEM( sType ) systems->Get< sType >()

// slightly overkill
#define GET_SYSTEM_ASSERT( sVar, sType ) \
	sVar = GET_SYSTEM( sType ); \
	Assert( sVar != nullptr );

#define GET_SYSTEM_CHECK GET_SYSTEM_ASSERT


class BaseSystem;


struct ModuleInterface_t
{
	void*       apInterface;
	const char* apName;
	size_t      aHash;
};


struct AppModules_t
{
	void**      apSystem;
	const char* apModuleName;
	const char* apInterfaceName;
	size_t      apInterfaceHash;
};


typedef ModuleInterface_t* ( *cframework_GetInterfacesF )( size_t& srCount );


extern "C"
{
	CORE_API bool  Mod_InitSystems();
	CORE_API void  Mod_Shutdown();

	CORE_API bool  Mod_Load( const char* spPath );
	CORE_API bool  Mod_AddSystems( AppModules_t* spModules, size_t sCount );

	CORE_API void* Mod_GetInterface( const char* spName, size_t sHash );
}

