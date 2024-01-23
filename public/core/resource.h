#pragma once

#include <cstring>
#include <filesystem>
#include <functional>

#include "asserts.h"
#include "core/filesystem.h"
#include "log.h"
#include "mempool.h"
#include "platform.h"
#include "util.h"
#include "vector.hpp"

namespace fs = std::filesystem;

// -----------------------------------------------------------------------
// Resource Manager (maybe change to Asset Manager?)
//
// The purpose of this is for hotloading support
// The Update function will check all added resources to see if any on disk have changed in anyway
// If any have, and the type is supplied with a callback function,
// then the callback is run to tell whatever type it is to reload this file
// -----------------------------------------------------------------------

static LOG_REGISTER_CHANNEL( Resource, LogColor::DarkYellow );

#define CH_GET_HANDLE_INDEX( sHandle ) ( sHandle & 0xFFFFFFFF )
#define CH_GET_HANDLE_MAGIC( sHandle ) ( sHandle >> 32 )

// backwards compatibility
#define GET_HANDLE_INDEX( sHandle )    CH_GET_HANDLE_INDEX( sHandle )
#define GET_HANDLE_MAGIC( sHandle )    CH_GET_HANDLE_MAGIC( sHandle )

// change Handle to this?
using ChResource_t                     = size_t;

using ChHandle_t                       = size_t;
using Handle                           = ChHandle_t;  // backwards compatibility

constexpr ChHandle_t CH_INVALID_HANDLE = 0;
constexpr ChHandle_t InvalidHandle     = CH_INVALID_HANDLE;  // backwards compatibility

using ResourceFunc_Load                = bool( ChHandle_t& item, const fs::path& srPath, void* spUserData );
using ResourceFunc_Reload              = bool( ChHandle_t& item, const fs::path& srPath );
using ResourceFunc_Create              = bool( ChHandle_t& item, const fs::path& srInternalPath, void* spData, void* spUserData );
using ResourceFunc_Free                = void( ChHandle_t item );
using ResourceFunc_OnLoadFinish        = void( ChHandle_t item );


struct ResourceType_t
{
	const char*                apName = nullptr;
	ResourceFunc_Load*         apFuncLoad;
	ResourceFunc_Reload*       apFuncReload;
	ResourceFunc_Create*       apFuncCreate;
	ResourceFunc_Free*         apFuncFree;
	ResourceFunc_OnLoadFinish* apFuncFinish;  // Called when this asset is finished loading in the background

	bool                       aPaused = false;
};


CORE_API void       Resource_Update();

// Free all still loaded resources
CORE_API void       Resource_Shutdown();

CORE_API ChHandle_t Resource_RegisterType( const ResourceType_t& srType );

// CORE_API ChHandle_t Resource_RegisterType( const char* spName, ResourceFunc_Load* apFuncLoad, ResourceFunc_Create* apFuncCreate, ResourceFunc_Free* apFuncFree );

// old idea's
// CORE_API bool       Resource_Add( ChHandle_t shType, ChHandle_t shResource, const std::string& srPath );
// CORE_API void       Resource_Remove( ChHandle_t shResource );

// Load's this resource from disk
CORE_API bool       Resource_Load( ChHandle_t shType, ChHandle_t& shResource, const fs::path& srPath );

// Create's a resource from memory
CORE_API bool       Resource_Create( ChHandle_t shType, ChHandle_t& shResource, const fs::path& srInternalPath, void* spData );

// Add's an already created resource externally
CORE_API bool       Resource_Add( ChHandle_t shType, ChHandle_t& shResource, const fs::path& srPath );

// Queue's a Resource for Deletion
CORE_API void       Resource_Free( ChHandle_t shType, ChHandle_t shResource );

// Pause or Resume Updating of this Resource Type
CORE_API void       Resource_SetTypePaused( ChHandle_t sType, bool sPaused );


// locks a resource currently in use, so we don't try to update it in the background if it changed on disk
// instead, we can queue that resource reload, and wait for the resource to be unlocked, and then do that reload
// you can also lock a resource multiple times
// returns a handle to a lock, this will be
CORE_API ChHandle_t Resource_Lock( ChHandle_t sType, ChHandle_t sResource );
CORE_API void       Resource_Unlock( ChHandle_t sLock );

CORE_API void       Resource_IncrementRef( ChHandle_t sType, ChHandle_t sResource );
CORE_API void       Resource_DecrementRef( ChHandle_t sType, ChHandle_t sResource );


struct ResourceAutoLock_t
{
	ChHandle_t aLock;

	ResourceAutoLock_t( ChHandle_t sType, ChHandle_t sResource )
	{
		aLock = Resource_Lock( sType, sResource );
	}

	~ResourceAutoLock_t()
	{
		Resource_Unlock( aLock );
	}
};


// -----------------------------------------------------------------------
// Original Resource List System
// -----------------------------------------------------------------------


// TODO: IDEA TO ALLOW FOR CONSOLIDATING MEMORY
// what if the index stored in the handle is actually an index into a different memory pool
// this different memory pool will store the real index for where our data is in the main memory pool
// this can allow us to consolidate the main memory pool and free a good chunk of memory
// the memory used in the alternate memory pool should be very small, so it won't matter that we can't consolidate that
//
// however, lookups of data would be slower, since we have to load the index memory pool to get the index
// and then load the main memory pool and return our data from that
//


template< typename T >
struct ResourceList
{
	mempool_t*             apPool;
	u32                    aSize;
	u32                    aStepSize;

	// TODO: this could just be `ChHandle_t* apHandles` and `u32 aHandlesAllocated`
	ChVector< ChHandle_t > aHandles;

	/*
     *    Construct a resource manager.
     */
	ResourceList() :
		aSize(), aStepSize( 8 )
	{
		apPool = mempool_new( ( sizeof( T ) + sizeof( unsigned int ) ) * 8 );
	}

	ResourceList( size_t sSize, size_t sStepSize ) :
		aSize( sSize ), aStepSize( sStepSize )
	{
		apPool = mempool_new( ( sizeof( T ) + sizeof( unsigned int ) ) * sSize );
	}

	/*
     *    Destruct the resource manager.
     */
	~ResourceList()
	{
	}

	// TEMP DEBUGGING
	bool ValidateMemPool()
	{
		memchunk_t* pChunk = apPool->apFirst;
		for ( pChunk = apPool->apFirst; pChunk != 0; pChunk = pChunk->apNext )
		{
			if ( *(unsigned int*)pChunk->apData != 69420 )
			{
				Log_Warn( "mempool broken ???\n" );
				return false;
			}
		}

		return true;
	}

	/*
	 * @brief      Consolidate the memory pool (PROBABLY BROKEN DUE TO HANDLES STORING AN INDEX INTO THE BUFFER !!!)
	 * @tparam T   Resource Type
	 */
	void Consolidate()
	{
		mempool_consolidate( apPool );
	}

	/*
	 * @brief      Allocate a Chunk of Memory
	 * @tparam T   Resource Type
	 * @return s8* The Buffer of Memory Allocated 
	 */
	s8* Allocate( unsigned int sMagic )
	{
		s8*        pBuf = nullptr;
		memerror_t err  = MEMERR_NONE;

		// Allocate a chunk of memory.
		err             = mempool_alloc( apPool, sizeof( unsigned int ) + sizeof( T ), &pBuf );

		if ( err == MEMERR_NO_MEMORY )
		{
			err = mempool_realloc( apPool, apPool->aSize + ( ( sizeof( T ) + sizeof( unsigned int ) ) * aStepSize ) );

			if ( err != MEMERR_NONE )
			{
				Log_ErrorF( "MemPool Error while Creating a Resource: %s\n", mempool_error2str( err ) );
				return nullptr;
			}

			err = mempool_alloc( apPool, sizeof( unsigned int ) + sizeof( T ), &pBuf );
			if ( err != MEMERR_NONE )
			{
				Log_ErrorF( "Failed to Allocate Resource: %s\n", mempool_error2str( err ) );
				return nullptr;
			}
		}
		else if ( err != MEMERR_NONE )
		{
			return nullptr;
		}

		// Write the magic number to the chunk
		std::memcpy( pBuf, &sMagic, sizeof( sMagic ) );

		return pBuf;
	}

	/*
     *    Expands the memory pool used internally if the size is greater than the current size
     */
	void EnsureSize( s64 sSize )
	{
		s64 size = sSize * ( sizeof( T ) + sizeof( unsigned int ) );

		if ( size > apPool->aSize )
			mempool_realloc( apPool, size );
	}

	/*
     *    Allocate a resource.
     *
     *    @return Handle    The handle to the resource.
     */
	ChHandle_t Add( const T& pData )
	{
		// Generate a handle magic number.
		unsigned int magic = ( rand() % 0xFFFFFFFE ) + 1;

		// Allocate a chunk of memory.
		s8*          pBuf  = Allocate( magic );

		if ( pBuf == nullptr )
			return InvalidHandle;

		// Write the data to the chunk
		// followed by the data.
		std::memcpy( pBuf + sizeof( unsigned int ), &pData, sizeof( T ) );

		unsigned int index = ( (size_t)pBuf - (size_t)apPool->apBuf ) / ( sizeof( T ) + sizeof( magic ) );

#if RESOURCE_DEBUG
		Log_DevF( gResourceChannel, 3, "ResourceManager::Add( T* ): Allocated resource at index %u\n", index );
#endif

		aSize++;

		ChHandle_t& handle = aHandles.emplace_back();
		handle             = index | (int64_t)magic << 32;
		return handle;
	}

	/*
     *    Create a new resource.
     *
     *    @param  pData     Output structure
     *    @return Handle    The handle to the resource.
     */
	ChHandle_t Create( T* pData, bool sZero = true )
	{
		// Generate a handle magic number.
		unsigned int magic = ( rand() % 0xFFFFFFFE ) + 1;

		// Allocate a chunk of memory.
		s8*          pBuf  = Allocate( magic, sZero );

		if ( pBuf == nullptr )
			return InvalidHandle;

		// Re-assign the output pointer to a pointer to the data
		*pData             = *(T*)( pBuf + sizeof( u32 ) );

		u32 index           = ( (size_t)pBuf - (size_t)apPool->apBuf ) / ( sizeof( T ) + sizeof( magic ) );

		// Set the remaining memory to zero if wanted
		if ( sZero )
		{
			memset( pBuf + sizeof( u32 ), 0, sizeof( T ) );
		}

#if RESOURCE_DEBUG
		Log_DevF( gResourceChannel, 3, "ResourceManager::Add( T* ): Allocated resource at index %u\n", index );
#endif

		aSize++;

		ChHandle_t& handle = aHandles.emplace_back();
		handle             = index | (int64_t)magic << 32;
		return handle;
	}

	/*
     *    Create a new resource.
     *
     *    @param  pData     Output structure for a pointer
     *    @return Handle    The handle to the resource.
     */
	ChHandle_t Create( T** pData, bool sZero = true )
	{
		// Generate a handle magic number.
		unsigned int magic = ( rand() % 0xFFFFFFFE ) + 1;

		// Allocate a chunk of memory.
		s8*          pBuf  = Allocate( magic );

		if ( pBuf == nullptr )
			return InvalidHandle;

		// Re-assign the output pointer to a pointer to the data
		*pData             = (T*)( pBuf + sizeof( unsigned int ) );

		unsigned int index = ( (size_t)pBuf - (size_t)apPool->apBuf ) / ( sizeof( T ) + sizeof( magic ) );

#if RESOURCE_DEBUG
		Log_DevF( gResourceChannel, 3, "ResourceManager::Add( T* ): Allocated resource at index %u\n", index );
#endif

		// Set the remaining memory to zero if wanted
		if ( sZero )
		{
			memset( pBuf + sizeof( unsigned int ), 0, sizeof( T ) );
		}

		aSize++;

		ChHandle_t& handle = aHandles.emplace_back();
		handle             = index | (int64_t)magic << 32;
		return handle;
	}

	/*
	 *    Gets Magic Number and Index from handle, returns true/false if succeeded
	 */
	bool GetMagicAndIndex( ChHandle_t sHandle, u32& srMagic, u32& srIndex )
	{
		// Check if handle is valid
		if ( sHandle == InvalidHandle )
			return false;

		// Get the magic number from the handle.
		srMagic = CH_GET_HANDLE_MAGIC( sHandle );

		// Get the index from the handle.
		srIndex = CH_GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( u32 ) );

		if ( srIndex > mempool_capacity( apPool ) )
		{
			Log_WarnF( gResourceChannel, "GetMagicAndIndex(): Index larger than mempool capacity: %zd > %zd\n", srIndex, mempool_capacity( apPool ) );
			return false;
		}

		return true;
	}

	/*
	 *    Gets Data from handle, returns true/false if succeeded
	 */
	s8* GetHandleData( ChHandle_t sHandle )
	{
		// Check if handle is valid
		unsigned int magic, index;
		if ( !GetMagicAndIndex( sHandle, magic, index ) )
			return nullptr;

		// Get the chunk of memory.
		s8* spBuf = apPool->apBuf + index;

		// Check if the buffer is nullptr
		if ( spBuf == nullptr )
		{
			Log_WarnF( gResourceChannel, "GetHandleData(): Invalid index - Buffer is nullptr: %zd\n", index );
			return nullptr;
		}

		// Verify the magic numbers at the start of the buffer match
		if ( *(unsigned int*)spBuf != magic )
		{
			Log_Warn( gResourceChannel, "GetHandleData(): Invalid magic number\n" );
			return nullptr;
		}

		return spBuf;
	}

	/*
     *    Update a resource.
     *
     *    @param  sHandle   Handle to Data
     *    @param  pData     New Data
	 * 
     *    @return bool      Whether the update was sucessful or not.
     */
	bool Update( ChHandle_t sHandle, const T& pData )
	{
		// Get handle data and check if the handle is valid
		s8* pBuf = nullptr;
		if ( !( pBuf = GetHandleData( sHandle ) ) )
			return false;

		// Write the new data to memory
		std::memcpy( pBuf + sizeof( u32 ), &pData, sizeof( T ) );

		return true;
	}

	/*
     *    Remove a resource.
     *
     *    @param Handle    The handle to the resource.
     */
	void Remove( ChHandle_t sHandle )
	{
		// Get handle data and check if the handle is valid
		s8*          pBuf = nullptr;
		// if ( !GetHandleData( sHandle, &pBuf ) )
		// 	return;

		// Check if handle is valid
		unsigned int magic, index;
		if ( !GetMagicAndIndex( sHandle, magic, index ) )
			return;

		// Get the chunk of memory.
		pBuf = apPool->apBuf + index;

		// Check if the buffer is nullptr
		if ( pBuf == nullptr )
		{
			Log_WarnF( gResourceChannel, CH_FUNC_NAME_CLASS ": Invalid index - Buffer is nullptr: %zd\n", index );
			return;
		}

		// Verify the magic numbers at the start of the buffer match
		if ( *(unsigned int*)pBuf != magic )
		{
			Log_Warn( gResourceChannel, CH_FUNC_NAME_CLASS ": Invalid magic number\n" );
			return;
		}

		// Free the chunk of memory.
		mempool_free( apPool, pBuf );

		aHandles.erase( sHandle );

		aSize--;

#if RESOURCE_DEV
		Log_DevF( gResourceChannel, 3, "ResourceManager::Remove( Handle ): Removed resource at index %u\n", index );
#endif
	}

	/*
     *    Get a resource.
     *
     *    @param Handle    The handle to the resource.
     *
     *    @return T *      The resource, nullptr if the handle
     *                     is invalid/ points to a different type.
     */

	T* Get( ChHandle_t sHandle )
	{
		// Get handle data and check if the handle is valid
		s8* pBuf = nullptr;
		if ( !( pBuf = GetHandleData( sHandle ) ) )
			return nullptr;

		// Return the data
		return (T*)( pBuf + sizeof( unsigned int ) );
	}

	/*
     *    Get a resource.
     *
     *    @param Handle    The handle to the resource.
     * 
     *    @param T*        The resource, nullptr if the handle
     *                     is invalid/ points to a different type.
     * 
     *    @return bool     Returns whether the Handle was valid the resource data was found or not.
     */

	bool Get( ChHandle_t sHandle, T* pData )
	{
		// Get handle data and check if the handle is valid
		s8* pBuf = nullptr;
		if ( !( pBuf = GetHandleData( sHandle ) ) )
			return false;

		// Set the data on the output parameter
		*pData = *(T*)( pBuf + sizeof( unsigned int ) );

		return true;
	}

	/*
     *    Get a resource.
     *
     *    @param Handle    The handle to the resource.
     * 
     *    @param T**       The resource, nullptr if the handle
     *                     is invalid/ points to a different type.
     * 
     *    @return bool     Returns whether the Handle was valid the resource data was found or not.
     */

	bool Get( ChHandle_t sHandle, T** pData )
	{
		// Get handle data and check if the handle is valid
		s8* pBuf = nullptr;
		if ( !( pBuf = GetHandleData( sHandle ) ) )
			return false;

		// Set the data on the output parameter
		*pData = (T*)( pBuf + sizeof( unsigned int ) );

		return true;
	}

	/*
     *    Get a resource by index.
     *
     *    @param size_t    The index of the resource.
     * 
     *    @param T*        The resource, nullptr if the handle
     *                     is invalid/ points to a different type.
     * 
     *    @return bool     Returns whether the Handle was valid the resource data was found or not.
     */
	bool GetByIndex( size_t sIndex, T* pData )
	{
		if ( sIndex >= aSize )
			return false;

		// Get the chunk of memory.
		s8* pBuf = apPool->apBuf + sIndex * ( sizeof( T ) + sizeof( unsigned int ) );

		// Set the data on the output parameter
		*pData   = *(T*)( pBuf + sizeof( unsigned int ) );

		return true;
	}

	/*
     *    Get a resource by index.
     *
     *    @param size_t    The index of the resource.
     * 
     *    @param T**       The resource, nullptr if the handle
     *                     is invalid/ points to a different type.
     * 
     *    @return bool     Returns whether the Handle was valid the resource data was found or not.
     */
	bool GetByIndex( size_t sIndex, T** pData )
	{
		if ( sIndex >= aSize )
			return false;

		// Get the chunk of memory.
		s8* pBuf = apPool->apBuf + sIndex * ( sizeof( T ) + sizeof( unsigned int ) );

		// Set the data on the output parameter
		*pData   = (T*)( pBuf + sizeof( unsigned int ) );

		return true;
	}

	/*
    *    Get a resource by index.
    *
    *    @param size_t    The index of the resource.
    *
    *    @return Handle   The handle to the resource, InvalidHandle if index is out of bounds
    */
	// ChHandle_t GetHandleByIndex( size_t sIndex )
	// {
	// 	if ( sIndex >= aSize )
	// 		return InvalidHandle;
	// 
	// 	// Get the chunk of memory.
	// 	s8*          pBuf  = apPool->apBuf + sIndex * ( sizeof( T ) + sizeof( unsigned int ) );
	// 
	// 	unsigned int magic = *(unsigned int*)pBuf;
	// 
	// 	return sIndex | (int64_t)magic << 32;
	// }

	/*
     *    Check if this handle is valid.
     *
     *    @param Handle    The handle to the resource.
     *
     *    @return bool     Whether the handle is valid or not.
     */

	bool Contains( Handle sHandle )
	{
		// Get handle data and check if the handle is valid
		return GetHandleData( sHandle );
	}

	/*
    * 
    *    Get a Random Handle
    * 
    *    @return Handle    The Handle picked at random
	*/
	ChHandle_t Random( u32 sMin, u32 sMax ) const
	{
		CH_ASSERT( sMin <= aHandles.size() );
		CH_ASSERT( sMax <= aHandles.size() );
		CH_ASSERT( sMin < sMax );

		u32 value = RandomU32( sMin, sMax );
		return aHandles[ value ];
	}

	/*
    * 
    *    Get the number of resources.
    * 
    *    @return u32    The number of resources.
	*/
	u32 size() const
	{
		return aSize;
	}

	/*
    * 
    *    Is this resource list empty?
    * 
    *    @return bool
	*/
	bool empty() const
	{
		return aSize == 0;
	}

	/*
    * 
    *    Clear the Resource List
	*/
	void clear()
	{
		aHandles.clear();
		apPool->aSize = 0;
		aSize         = 0;
	}
	
	u32 GetHandleCount()
	{
		return aHandles.size();
	}

	ChHandle_t GetHandleByIndex( u32 sIndex )
	{
		if ( aHandles.size() <= sIndex )
			return CH_INVALID_HANDLE;

		return aHandles[ sIndex ];
	}
};
