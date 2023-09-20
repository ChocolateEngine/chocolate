#pragma once

#include "core/resource.h"

#include <cstring>
#include <unordered_map>

#include "core/filesystem.h"
#include "core/log.h"
#include "core/mempool.h"
#include "core/platform.h"

// -----------------------------------------------------------------------
// Resource Manager
//
// The purpose of this is for hotloading support
// The Update function will check all added resources to see if any on disk have changed in anyway
// If any have, and the type is supplied with a callback function,
// then the callback is run to tell whatever type it is to reload this file
// -----------------------------------------------------------------------

LOG_REGISTER_CHANNEL2( ResourceSystem, LogColor::DarkYellow );


struct ResourceData_t
{
	// path
	std::string aPath{};

	// basic file info
	off_t       aSize{};
	time_t      aCreateTime{};
	time_t      aModTime{};

	u32         aRefCount = 0;

	// maybe use this if stat changes between files
	// sometype aHash;
};


// temp list of resource types that will be used intiially
// 
// - Models
// - Materials
// - Textures
// - Sounds
// 
// 
// other resources that won't use this hot reloading resource system
// 
// - Maps/Scenes
// - Shaders
// - 
//



// std::unordered_map< std::string, ResourceData_t > gResources;
std::unordered_map< ChHandle_t, ResourceList< ResourceData_t > > gResources;
ResourceList< ResourceType_t >                                   gResourceTypes;


// ----------------------------------------------------------------
// Public Functions


// TODO: when this is added, have what this function does be a Job, and queue files to reload
// this function will instead update those files
void Resource_Update()
{
	// For Each Resource Type, Check all resources to see if they need updating
	for ( auto& [ typeHandle, resourceList ] : gResources )
	{
		ResourceType_t* type = gResourceTypes.Get( typeHandle );
		if ( type == nullptr )
		{
			Log_Error( gLC_ResourceSystem, "Invalid Resource Type\n" );
			continue;
		}

		if ( type->aPaused )
			continue;

		for ( u32 i = 0; i < resourceList.aHandles.size(); )
		{
			ChHandle_t resourceHandle = resourceList.aHandles[ i ];
			ResourceData_t* data = nullptr;

			if ( !resourceList.Get( resourceHandle, &data ) )
			{
				resourceList.aHandles.remove( i );
				Log_Error( gLC_ResourceSystem, "Invalid Resource found when updating!\n" );
				continue;
			}

			i++;

			struct stat fileStat
			{
			};
			int ret = FileSys_Stat( data->aPath.c_str(), &fileStat );

			if ( ret != 0 )
			{
				Log_ErrorF( gLC_ResourceSystem, "Remove from file list: \"%s\"\n", data->aPath.c_str() );
				continue;
			}

			// Compare certain values of stat

			// Check Size
			if ( data->aSize != fileStat.st_size )
			{
				Log_DevF( gLC_ResourceSystem, 2, "File Size Changed: \"%s\"\n", data->aPath.c_str() );

				if ( type->apFuncReload )
					type->apFuncReload( resourceHandle, data->aPath );
			}

			// Check file modification time
			else if ( data->aModTime != fileStat.st_mtime )
			{
				Log_DevF( gLC_ResourceSystem, 2, "File Modification Time Changed: \"%s\"\n", data->aPath.c_str() );

				if ( type->apFuncReload )
					type->apFuncReload( resourceHandle, data->aPath );
			}

			// Check file creation time
			else if ( data->aCreateTime != fileStat.st_ctime )
			{
				Log_DevF( gLC_ResourceSystem, 2, "File Creation Time Changed: \"%s\"\n", data->aPath.c_str() );

				if ( type->apFuncReload )
					type->apFuncReload( resourceHandle, data->aPath );
			}

			data->aSize       = fileStat.st_size;
			data->aModTime    = fileStat.st_mtime;
			data->aCreateTime = fileStat.st_ctime;
		}
	}
}


void Resource_Shutdown()
{
}


ChHandle_t Resource_RegisterType( const ResourceType_t& srType )
{
	return gResourceTypes.Add( srType );
}


#if 0
bool Resource_Add( ChHandle_t shType, ChHandle_t shResource, const std::string& srPath )
{
	auto it = gResources.find( shResource );

	if ( it != gResources.end() )
	{
		Log_WarnF( gLC_ResourceSystem, "Resource already added: \"%s\"\n", srPath.c_str() );
		return false;
	}

	// make sure type is valid
	ResourceType_t* type = gResourceTypes.Get( shType );
	if ( type == nullptr )
	{
		Log_ErrorF( gLC_ResourceSystem, "Unknown Resource Type for adding file: \"%s\"\n", srPath.c_str() );
		return false;
	}

	struct stat fileStat
	{
	};
	int ret = FileSys_Stat( srPath.c_str(), &fileStat );

	if ( ret != 0 )
	{
		Log_ErrorF( gLC_ResourceSystem, "Failed to call stat on file: \"%s\"\n", srPath.c_str() );
		return false;
	}

	ResourceData_t& data = gResources[ shType ];
	data.aPath           = srPath;
	data.aType           = shType;

	data.aSize           = fileStat.st_size;
	data.aModTime        = fileStat.st_mtime;
	data.aCreateTime     = fileStat.st_ctime;

	return true;
}


void Resource_Remove( ChHandle_t shResource )
{
	auto it = gResources.find( shResource );

	if ( it != gResources.end() )
	{
		Log_Warn( gLC_ResourceSystem, "Trying to remove resource does not exist\n" );
		return;
	}

	gResources.erase( shResource );
}
#endif


// Load's this resource from disk
bool Resource_Load( ChHandle_t shType, ChHandle_t& shResource, const fs::path& srPath )
{
	return false;
}


// Create's a resource from memory
bool Resource_Create( ChHandle_t shType, ChHandle_t& shResource, const fs::path& srInternalPath, void* spData )
{
	return false;
}


// Add's an already created resource externally
bool Resource_Add( ChHandle_t shType, ChHandle_t& shResource, const fs::path& srPath )
{
	return false;
}


// Queue's a Resource for Deletion
void Resource_Free( ChHandle_t shType, ChHandle_t shResource )
{
}


// Pause or Resume Updating of this Resource Type
void Resource_SetTypePaused( ChHandle_t sType, bool sPaused )
{
}


// locks a resource currently in use, so we don't try to update it in the background if it changed on disk
// instead, we can queue that resource reload, and wait for the resource to be unlocked, and then do that reload
// you can also lock a resource multiple times
// returns a handle to a lock, this will be
ChHandle_t Resource_Lock( ChHandle_t sType, ChHandle_t sResource )
{
	return CH_INVALID_HANDLE;
}


void Resource_Unlock( ChHandle_t sLock )
{
}


void Resource_IncrementRef( ChHandle_t sType, ChHandle_t sResource )
{
	auto it = gResources.find( sType );

	if ( it != gResources.end() )
	{
		Log_Warn( gLC_ResourceSystem, "Type of resource count increment resource ref that does not exist\n" );
		return;
	}

	// it->second.aRefCount++;
}


void Resource_DecrementRef( ChHandle_t sType, ChHandle_t sResource )
{
	auto it = gResources.find( sResource );

	if ( it != gResources.end() )
	{
		Log_Warn( gLC_ResourceSystem, "Trying to decrement resource ref does not exist\n" );
		return;
	}

	// it->second.aRefCount--;
	// 
	// if ( it->second.aRefCount == 0 )
	// 	Resource_Free( it->second.aType, sResource );
}

