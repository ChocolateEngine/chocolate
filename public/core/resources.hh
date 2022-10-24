/*
 *    resources.h    --    Resource management
 *
 *    Authroed by Karl "p0lyh3dron" Kreuze on March 5, 2022
 *
 *    This files declares the resource manager, along with
 *    all the types related to handles and resource management.
 */
#pragma once

#include <cstring>

#include "platform.h"
#include "mempool.h"

static LOG_REGISTER_CHANNEL( Resource, LogColor::DarkYellow );

#define GET_HANDLE_INDEX( sHandle ) ( sHandle & 0xFFFFFFFF )
#define GET_HANDLE_MAGIC( sHandle ) ( sHandle >> 32 )

using Handle = size_t;

constexpr Handle InvalidHandle = 0;

// TODO: REMOVE ME - OLDER VERSION OF ResourceList
template < typename T >
class ResourceManager
{
    MemPool aPool;
    size_t  aSize;
public:
    /*
     *    Construct a resource manager.
     */
    ResourceManager() : aPool(), aSize()
    {

    }
    /*
     *    Destruct the resource manager.
     */
    ~ResourceManager()
    {
        
    }

    /*
     *    Allocate memory.
     *
     *    @param size_t    The size of the memory to be allocated.
     */
    void   Allocate( size_t sSize )
    {
        aPool.Resize( sSize * sizeof( T ) + sSize * sizeof( Handle ) );
    }
    /*
     *    Allocate a resource.
     *
     *    @return Handle    The handle to the resource.
     */
    Handle Add( T *pData )
    {
        /*
         *    Generate a handle magic number.
         */
        size_t magic = ( rand() % 0xFFFFFFFE ) + 1;

        /*
         *    Allocate a chunk of memory.
         */
        s8 *pBuf = ( s8* )aPool.Alloc( sizeof( magic ) + sizeof( T ) );
        if( pBuf == nullptr )
            return 0;
        
        /*
         *   Write the magic number to the chunk
         *   followed by the data.
         */
        std::memcpy( pBuf, &magic, sizeof( magic ) );
        std::memcpy( pBuf + sizeof( magic ), pData, sizeof( T ) );

        size_t index = ( ( size_t )pBuf - ( size_t )aPool.GetStart() ) / ( sizeof( T ) + sizeof( magic ) );

#if RESOURCE_DEBUG
        Log_DevF( gResourceChannel, 3, "ResourceManager::Add( T* ): Allocated resource at index %u\n", index );
#endif

        aSize++;

        return index | magic << 32;
	}
#if 1
	/*
     *    Create a new resource.
     *
     *    @return Handle    The handle to the resource.
     */
	Handle Create( T* pData )
	{
		/*
         *    Generate a handle magic number.
         */
		size_t magic = ( rand() % 0xFFFFFFFE ) + 1;

		/*
         *    Allocate a chunk of memory.
         */
		s8*    pBuf  = (s8*)aPool.Alloc( sizeof( magic ) + sizeof( T ) );
		if ( pBuf == nullptr )
			return 0;

		/*
         *   Write the magic number to the chunk
         *   followed by the data.
         */
		std::memcpy( pBuf, &magic, sizeof( magic ) );
		// std::memcpy( pBuf + sizeof( magic ), pData, sizeof( T ) );

        *pData       = *pBuf;

		size_t index = ( (size_t)pBuf - (size_t)aPool.GetStart() ) / ( sizeof( T ) + sizeof( magic ) );

#if RESOURCE_DEBUG
		Log_DevF( gResourceChannel, 3, "ResourceManager::Add( T* ): Allocated resource at index %u\n", index );
#endif

		aSize++;

		return index | magic << 32;
	}
#endif
    /*
     *    Remove a resource.
     *
     *    @param Handle    The handle to the resource.
     */
    void   Remove( Handle sHandle )
    {
        /*
         *    Get the index of the resource.
         */
        size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

        /*
         *    Get the magic number.
         */
        size_t magic = GET_HANDLE_MAGIC( sHandle );

        /*
         *    Get the chunk of memory.
         */
        s8 *pBuf = ( s8* )aPool.GetStart() + index;

        /*
         *    Check the magic number.
         */
        if( std::memcmp( pBuf, &magic, sizeof( magic ) ) != 0 )
            return;

        /*
         *    Free the chunk of memory.
         */
        aPool.Free( pBuf );

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
    T     *Get( Handle sHandle )
    {
        /*
         *    Get the magic number from the handle.
         */
        size_t magic = GET_HANDLE_MAGIC( sHandle );

        /*
         *    Get the index from the handle.
         */
        size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

        /*
         *    Get the chunk of memory.
         */
        s8 *pBuf = aPool.GetStart() + index;

        /*
         *    Check the magic number.
         */
        if( *( size_t* )pBuf != magic ) {
            Log_WarnF( gResourceChannel, "*ResourceManager::Get( Handle ): Invalid handle: %d\n", sHandle );
            return nullptr;
        }

#if RESOURCE_DEV
        Log_DevF( gResourceChannel, 4, "*ResourceManager::Get( Handle ): Retrieved resource at index %u\n", index );
#endif

        /*
         *    Return the data.
         */
        return ( T* )( pBuf + sizeof( magic ) );
    }
    /*
    *    Get a resource by index.
    *
    *    @param size_t    The index of the resource.
    *
    *    @return T *      The resource, nullptr if the handle
    *                     is invalid/ points to a different type.
    */	
	T     *GetByIndex( size_t sIndex )
    {
		if ( sIndex >= aSize )
			return nullptr;

		/*
		 *    Get the chunk of memory.
		 */
		s8 *pBuf = aPool.GetStart() + sIndex * ( sizeof( T ) + sizeof( Handle ) );

		/*
        *   Return the data.
		*/
		// return ( T* )( pBuf + sizeof( magic ) );
		return ( T* )( pBuf + sizeof( size_t ) );
	}
    /*
    * 
    *    Get the number of resources.
    * 
    *    @return size_t    The number of resources.
	*/
    size_t size() const
    {
		return aSize;
    }
};




template< typename T >
class ResourceList
{
	MemPool aPool;
	size_t  aSize;

public:
	/*
     *    Construct a resource manager.
     */
	ResourceList() :
		aPool(), aSize()
	{
	}
	/*
     *    Destruct the resource manager.
     */
	~ResourceList()
	{
	}

	/*
     *    Allocate memory.
     *
     *    @param size_t    The size of the memory to be allocated.
     */
	void Allocate( size_t sSize )
	{
		aPool.Resize( sSize * sizeof( T ) + sSize * sizeof( Handle ) );
	}

	/*
     *    Allocate a resource.
     *
     *    @return Handle    The handle to the resource.
     */
	Handle Add( const T& pData )
	{
		// Generate a handle magic number.
		size_t magic = ( rand() % 0xFFFFFFFE ) + 1;

		// Allocate a chunk of memory.
		s8*    pBuf  = (s8*)aPool.Alloc( sizeof( magic ) + sizeof( T ) );
		if ( pBuf == nullptr )
			return InvalidHandle;

        // Write the magic number to the chunk
        // followed by the data.
		std::memcpy( pBuf, &magic, sizeof( magic ) );
		std::memcpy( pBuf + sizeof( magic ), &pData, sizeof( T ) );

		size_t index = ( (size_t)pBuf - (size_t)aPool.GetStart() ) / ( sizeof( T ) + sizeof( magic ) );

#if RESOURCE_DEBUG
		Log_DevF( gResourceChannel, 3, "ResourceManager::Add( T* ): Allocated resource at index %u\n", index );
#endif

		aSize++;

		return index | magic << 32;
	}

	/*
     *    Update a resource.
     *
     *    @param  sHandle   Handle to Data
     *    @param  pData     New Data
	 * 
     *    @return bool      Whether the update was sucessful or not.
     */
	bool Update( Handle sHandle, const T& pData )
	{
		// Check if handle is valid
		if ( sHandle == InvalidHandle )
			return false;

		// Get the magic number from the handle.
		size_t magic = GET_HANDLE_MAGIC( sHandle );

		// Get the index from the handle.
		size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

		// Get the chunk of memory.
		s8*    pBuf  = aPool.GetStart() + index;

		// Check the magic number.
		if ( pBuf == nullptr || *(size_t*)pBuf != magic )
		{
			Log_WarnF( gResourceChannel, "*ResourceManager::Get( Handle ): Invalid handle: %d\n", sHandle );
			return false;
		}

		// Write the new data to memory
		std::memcpy( pBuf + sizeof( magic ), &pData, sizeof( T ) );
		return true;
	}

	/*
     *    Create a new resource.
     *
     *    @param  pData     Output structure
     *    @return Handle    The handle to the resource.
     */
	Handle Create( T* pData )
	{
		// Generate a handle magic number.
		size_t magic = ( rand() % 0xFFFFFFFE ) + 1;

		// Allocate a chunk of memory.
		s8*    pBuf  = (s8*)aPool.Alloc( sizeof( magic ) + sizeof( T ) );
		if ( pBuf == nullptr )
			return InvalidHandle;

        // Write the magic number to the chunk
        // followed by the data.
		std::memcpy( pBuf, &magic, sizeof( magic ) );
		// std::memcpy( pBuf + sizeof( magic ), pData, sizeof( T ) );

		*pData       = *(T*)( pBuf + sizeof( magic ) );

		size_t index = ( (size_t)pBuf - (size_t)aPool.GetStart() ) / ( sizeof( T ) + sizeof( magic ) );

  #if RESOURCE_DEBUG
		Log_DevF( gResourceChannel, 3, "ResourceManager::Add( T* ): Allocated resource at index %u\n", index );
  #endif

		aSize++;

		return index | magic << 32;
	}

	/*
     *    Create a new resource.
     *
     *    @param  pData     Output structure for a pointer
     *    @return Handle    The handle to the resource.
     */
	Handle Create( T** pData )
	{
		// Generate a handle magic number.
		size_t magic = ( rand() % 0xFFFFFFFE ) + 1;

		// Allocate a chunk of memory.
		s8*    pBuf  = (s8*)aPool.Alloc( sizeof( magic ) + sizeof( T ) );
		if ( pBuf == nullptr )
			return InvalidHandle;

        // Write the magic number to the chunk
        // followed by the data.
		std::memcpy( pBuf, &magic, sizeof( magic ) );
		// std::memcpy( pBuf + sizeof( magic ), pData, sizeof( T ) );

		*pData       = (T*)( pBuf + sizeof( magic ) );

		size_t index = ( (size_t)pBuf - (size_t)aPool.GetStart() ) / ( sizeof( T ) + sizeof( magic ) );

  #if RESOURCE_DEBUG
		Log_DevF( gResourceChannel, 3, "ResourceManager::Add( T* ): Allocated resource at index %u\n", index );
  #endif

		aSize++;

		return index | magic << 32;
	}

	/*
     *    Remove a resource.
     *
     *    @param Handle    The handle to the resource.
     */
	void Remove( Handle sHandle )
	{
		// Check if handle is valid
		if ( sHandle == InvalidHandle )
			return;

		// Get the index of the resource.
		size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

		// Get the magic number.
		size_t magic = GET_HANDLE_MAGIC( sHandle );

		// Get the chunk of memory.
		s8*    pBuf  = (s8*)aPool.GetStart() + index;

		// Check the magic number.
		if ( std::memcmp( pBuf, &magic, sizeof( magic ) ) != 0 )
			return;

		// Free the chunk of memory.
		aPool.Free( pBuf );

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

	T* Get( Handle sHandle )
	{
		// Check if handle is valid
		if ( sHandle == InvalidHandle )
			return nullptr;

		// Get the magic number from the handle.
		size_t magic = GET_HANDLE_MAGIC( sHandle );

		// Get the index from the handle.
		size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

		// Get the chunk of memory.
		s8*    pBuf  = aPool.GetStart() + index;

		// Check the magic number.
		if ( pBuf == nullptr || *(size_t*)pBuf != magic )
		{
			Log_WarnF( gResourceChannel, "*ResourceManager::Get( Handle ): Invalid handle: %zd\n", sHandle );
			return nullptr;
		}

        // Return the data
		return (T*)( pBuf + sizeof( magic ) );
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

	bool Get( Handle sHandle, T* pData )
	{
		// Check if handle is valid
		if ( sHandle == InvalidHandle )
			return false;

		// Get the magic number from the handle.
		size_t magic = GET_HANDLE_MAGIC( sHandle );

		// Get the index from the handle.
		size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

		// Get the chunk of memory.
		s8*    pBuf  = aPool.GetStart() + index;

		// Check the magic number.
		if ( pBuf == nullptr || *(size_t*)pBuf != magic )
		{
			Log_WarnF( gResourceChannel, "*ResourceManager::GetData( Handle ): Invalid handle: %zd\n", sHandle );
			return false;
		}

		// Set the data on the output parameter
		*pData = *(T*)( pBuf + sizeof( magic ) );

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

	bool Get( Handle sHandle, T** pData )
	{
		// Check if handle is valid
		if ( sHandle == InvalidHandle )
			return false;

		// Get the magic number from the handle.
		size_t magic = GET_HANDLE_MAGIC( sHandle );

		// Get the index from the handle.
		size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

		// Get the chunk of memory.
		s8*    pBuf  = aPool.GetStart() + index;

		// Check the magic number.
		if ( pBuf == nullptr || *(size_t*)pBuf != magic )
		{
			Log_WarnF( gResourceChannel, "*ResourceManager::GetData( Handle ): Invalid handle: %zd\n", sHandle );
			return false;
		}

		// Set the data on the output parameter
		*pData = (T*)( pBuf + sizeof( magic ) );

		return true;
	}

	/*
    *    Get a resource by index.
    *
    *    @param size_t    The index of the resource.
    *
    *    @return T *      The resource, nullptr if the handle
    *                     is invalid/ points to a different type.
    */
	T* GetByIndex( size_t sIndex )
	{
		if ( sIndex >= aSize )
			return nullptr;

		// Get the chunk of memory.
		s8* pBuf = aPool.GetStart() + sIndex * ( sizeof( T ) + sizeof( Handle ) );

		// Return the data.
		// return ( T* )( pBuf + sizeof( magic ) );
		return (T*)( pBuf + sizeof( size_t ) );
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
		s8* pBuf = aPool.GetStart() + sIndex * ( sizeof( T ) + sizeof( Handle ) );

		// Set the data on the output parameter
		*pData   = *(T*)( pBuf + sizeof( size_t ) );

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
		s8* pBuf = aPool.GetStart() + sIndex * ( sizeof( T ) + sizeof( Handle ) );

		// Set the data on the output parameter
		*pData   = (T*)( pBuf + sizeof( size_t ) );

		return true;
	}

	/*
    *    Get a resource by index.
    *
    *    @param size_t    The index of the resource.
    *
    *    @return Handle   The handle to the resource, InvalidHandle if index is out of bounds
    */
	Handle GetHandleByIndex( size_t sIndex )
	{
		if ( sIndex >= aSize )
			return InvalidHandle;

		// Get the chunk of memory.
		s8* pBuf = aPool.GetStart() + sIndex * ( sizeof( T ) + sizeof( Handle ) );

        size_t magic = *(size_t*)pBuf;

		return sIndex | magic << 32;
	}

	/*
     *    Check if this handle is valid.
     *
     *    @param Handle    The handle to the resource.
     *
     *    @return bool     Whether the handle is valid or not.
     */

	bool Contains( Handle sHandle )
	{
		// Get the magic number from the handle.
		size_t magic = GET_HANDLE_MAGIC( sHandle );

		// Get the index from the handle.
		size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

		// Get the chunk of memory.
		s8*    pBuf  = aPool.GetStart() + index;

		// Check the magic number.
		if ( pBuf == nullptr || *(size_t*)pBuf != magic )
			return false;

		return true;
	}

	/*
    * 
    *    Get the number of resources.
    * 
    *    @return size_t    The number of resources.
	*/
	size_t size() const
	{
		return aSize;
	}

	/*
    * 
    *    Clear the Resource List
	*/
	void clear()
	{
		aPool.Clear();
		aSize = 0;
	}
};

