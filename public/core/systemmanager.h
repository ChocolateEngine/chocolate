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


class ISystem;


struct ModuleInterface_t
{
	void*       apInterface;
	const char* apName;
	size_t      aVersion;
};


struct AppModule_t
{
	// void**      apSystem;
	ISystem**   apSystem;
	const char* apModuleName;
	const char* apInterfaceName;
	size_t      apInterfaceVer;
	bool        aRequired = true;
};


enum EModLoadError
{
	EModLoadError_Success,
	EModLoadError_LoadModule,
	EModLoadError_LoadInterface,
	EModLoadError_InitInterface,
};


typedef ModuleInterface_t* ( *cframework_GetInterfacesF )( size_t& srCount );


extern "C"
{
	CORE_API bool          Mod_InitSystems();
	CORE_API void          Mod_Shutdown();

	CORE_API Module        Mod_Load( const char* spPath );
	CORE_API void          Mod_Free( const char* spPath );

	CORE_API bool          Mod_AddSystems( AppModule_t* spModules, size_t sCount );
	CORE_API EModLoadError Mod_LoadSystem( AppModule_t& srModule );
	CORE_API EModLoadError Mod_LoadAndInitSystem( AppModule_t& srModule );

	// CORE_API void   Mod_AddLoadedSystem( Module spModule, ISystem* spSystem );

	CORE_API void*         Mod_GetInterface( const char* spName, size_t sHash );
}


template< typename T >
inline T* Mod_GetInterfaceCast( const char* spName, size_t sVer )
{
	return static_cast< T* >( Mod_GetInterface( spName, sVer ) );
}

