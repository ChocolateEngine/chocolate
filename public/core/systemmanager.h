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
	ISystem**   apSystem;  // TODO: try ISystem*&
	const char* apModuleName;
	const char* apInterfaceName;
	u16         apInterfaceVer;
	bool        aRequired = true;
};


enum EModLoadError
{
	EModLoadError_Success,        // No errors :D
	EModLoadError_LoadModule,     // An Error Occurred when Loading the Module/DLL 
	EModLoadError_LoadInterface,  // An Error Occurred when Getting the Interface Function from the Module
	EModLoadError_InitInterface,  // An Error Occurred when Initializing the Interface
};


typedef ModuleInterface_t* ( *fn_ch_get_interfaces_t )( u8& srCount );


extern "C"
{
	CORE_API bool          Mod_InitSystems();
	CORE_API void          Mod_Shutdown();

	CORE_API Module        Mod_Load( const char* spPath );
	CORE_API void          Mod_Free( const char* spPath );

	CORE_API bool          Mod_AddSystems( AppModule_t* spModules, size_t sCount );
	CORE_API EModLoadError Mod_LoadSystem( AppModule_t& srModule );
	CORE_API bool          Mod_InitSystem( AppModule_t& srModule );
	CORE_API EModLoadError Mod_LoadAndInitSystem( AppModule_t& srModule );

	// CORE_API void   Mod_AddLoadedSystem( Module spModule, ISystem* spSystem );

	CORE_API void*         Mod_GetSystem( const char* spName, size_t sHash );
}


template< typename T >
inline T* Mod_GetSystemCast( const char* spName, size_t sVer )
{
	return static_cast< T* >( Mod_GetSystem( spName, sVer ) );
}


#define CH_GET_SYSTEM( var, type, name, ver )     \
	var = Mod_GetSystemCast< type >( name, ver ); \
	if ( var == nullptr )                            \
	{                                                \
		Log_Error( "Failed to load " name "\n" );    \
		return false;                                \
	}

