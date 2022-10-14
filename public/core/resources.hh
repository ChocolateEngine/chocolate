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
    size_t size()
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
		std::memcpy( pBuf + sizeof( magic ), &pData, sizeof( T ) );

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
     *    @param  pData     Output structure
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
		if ( *(size_t*)pBuf != magic )
		{
			Log_WarnF( gResourceChannel, "*ResourceManager::Get( Handle ): Invalid handle: %d\n", sHandle );
			return nullptr;
		}

        // Return the data
		return (T*)( pBuf + sizeof( magic ) );
	}

	/*
     *    Get a resource by reference.
     *
     *    @param Handle    The handle to the resource.
     *
     *    @return T&       The resource, nullptr if the handle
     *                     is invalid/ points to a different type.
     */

    // maybe add a fallback parameter?
	T& GetRef( Handle sHandle )
	{
		// Get the magic number from the handle.
		size_t magic = GET_HANDLE_MAGIC( sHandle );

		// Get the index from the handle.
		size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

		// Get the chunk of memory.
		s8*    pBuf  = aPool.GetStart() + index;

		// Check the magic number.
		if ( *(size_t*)pBuf != magic )
		{
            // no way to fallback from this seemingly
			// Log_FatalF( gResourceChannel, "*ResourceManager::GetRef( Handle ): Invalid handle: %d\n", sHandle );
			Log_WarnF( gResourceChannel, "*ResourceManager::GetRef( Handle ): Invalid handle: %d\n", sHandle );
			T data;
			return data;
		}

		T& data = (T)( pBuf + sizeof( magic ) );

		// Return the data
		return data;
	}

	/*
     *    Get a resource.
     *
     *    @param Handle    The handle to the resource.
     * 
     *    @param T*        The resource, nullptr if the handle
     *                     is invalid/ points to a different type.
     */

	bool Get( Handle sHandle, T* pData )
	{
		// Get the magic number from the handle.
		size_t magic = GET_HANDLE_MAGIC( sHandle );

		// Get the index from the handle.
		size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

		// Get the chunk of memory.
		s8*    pBuf  = aPool.GetStart() + index;

		// Check the magic number.
		if ( *(size_t*)pBuf != magic )
		{
			Log_WarnF( gResourceChannel, "*ResourceManager::GetData( Handle ): Invalid handle: %d\n", sHandle );
			return false;
		}

		*pData = *(T*)( pBuf + sizeof( magic ) );

		// Return the data
		return true;
	}

	bool Get( Handle sHandle, T** pData )
	{
		// Get the magic number from the handle.
		size_t magic = GET_HANDLE_MAGIC( sHandle );

		// Get the index from the handle.
		size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

		// Get the chunk of memory.
		s8*    pBuf  = aPool.GetStart() + index;

		// Check the magic number.
		if ( *(size_t*)pBuf != magic )
		{
			Log_WarnF( gResourceChannel, "*ResourceManager::GetData( Handle ): Invalid handle: %d\n", sHandle );
			return false;
		}

		*pData = (T*)( pBuf + sizeof( magic ) );

		// Return the data
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

		/*
		 *    Get the chunk of memory.
		 */
		s8* pBuf = aPool.GetStart() + sIndex * ( sizeof( T ) + sizeof( Handle ) );

		/*
        *   Return the data.
		*/
		// return ( T* )( pBuf + sizeof( magic ) );
		return (T*)( pBuf + sizeof( size_t ) );
	}

	/*
    *    Get a resource by index.
    *
    *    @param size_t    The index of the resource.
    *
    *    @return T *      The resource, nullptr if the handle
    *                     is invalid/ points to a different type.
    */
	Handle GetHandleByIndex( size_t sIndex )
	{
		if ( sIndex >= aSize )
			return InvalidHandle;

		/*
		 *    Get the chunk of memory.
		 */
		s8* pBuf = aPool.GetStart() + sIndex * ( sizeof( T ) + sizeof( Handle ) );

        size_t magic = *(size_t*)pBuf;

		return sIndex | magic << 32;
	}

	/*
     *    Check if this handle is valid.
     *
     *    @param Handle    The handle to the resource.
     *
     *    @return T&       The resource, nullptr if the handle
     *                     is invalid/ points to a different type.
     */

	bool Contains( Handle sHandle )
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
		s8*    pBuf  = aPool.GetStart() + index;

		/*
         *    Check the magic number.
         */
		if ( *(size_t*)pBuf != magic )
		{
			return false;
		}

		return true;
	}

	/*
    * 
    *    Get the number of resources.
    * 
    *    @return size_t    The number of resources.
	*/
	size_t size()
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




template< typename T, typename R >
class ResourceList3
{
	MemPool aPool;
	size_t  aSize;

  public:
	/*
     *    Construct a resource manager.
     */
	ResourceList3() :
		aPool(), aSize()
	{
	}
	/*
     *    Destruct the resource manager.
     */
	~ResourceList3()
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
			return 0;

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
			return 0;

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
			return 0;

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
	}

	/*
     *    Get a resource.
     *
     *    @param Handle    The handle to the resource.
     *
     *    @return T *      The resource, nullptr if the handle
     *                     is invalid/ points to a different type.
     */

	R Get( Handle sHandle )
	{
		// Get the magic number from the handle.
		size_t magic = GET_HANDLE_MAGIC( sHandle );

		// Get the index from the handle.
		size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

		// Get the chunk of memory.
		s8*    pBuf  = aPool.GetStart() + index;

		// Check the magic number.
		if ( *(size_t*)pBuf != magic )
		{
			Log_WarnF( gResourceChannel, "*ResourceManager::Get( Handle ): Invalid handle: %d\n", sHandle );
			return (R)nullptr;
		}

		// Return the data
		return (R)( pBuf + sizeof( magic ) );
	}

#if 0
	/*
     *    Get a resource.
     *
     *    @param Handle    The handle to the resource.
     * 
     *    @param T*        The resource, nullptr if the handle
     *                     is invalid/ points to a different type.
     */

	bool Get( Handle sHandle, R* pData )
	{
		// Get the magic number from the handle.
		size_t magic = GET_HANDLE_MAGIC( sHandle );

		// Get the index from the handle.
		size_t index = GET_HANDLE_INDEX( sHandle ) * ( sizeof( T ) + sizeof( Handle ) );

		// Get the chunk of memory.
		s8*    pBuf  = aPool.GetStart() + index;

		// Check the magic number.
		if ( *(size_t*)pBuf != magic )
		{
			Log_WarnF( gResourceChannel, "*ResourceManager::GetData( Handle ): Invalid handle: %d\n", sHandle );
			return false;
		}

		*pData = *(R*)( pBuf + sizeof( magic ) );

		// Return the data
		return true;
	}
#endif

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

		/*
		 *    Get the chunk of memory.
		 */
		s8* pBuf = aPool.GetStart() + sIndex * ( sizeof( T ) + sizeof( Handle ) );

		/*
        *   Return the data.
		*/
		// return ( T* )( pBuf + sizeof( magic ) );
		return (T*)( pBuf + sizeof( size_t ) );
	}

	/*
    *    Get a resource by index.
    *
    *    @param size_t    The index of the resource.
    *
    *    @return T *      The resource, nullptr if the handle
    *                     is invalid/ points to a different type.
    */
	Handle GetHandleByIndex( size_t sIndex )
	{
		if ( sIndex >= aSize )
			return InvalidHandle;

		// Get the chunk of memory.
		s8*    pBuf  = aPool.GetStart() + sIndex * ( sizeof( T ) + sizeof( Handle ) );

		size_t magic = *(size_t*)pBuf;

		return sIndex | magic << 32;
	}

	/*
     *    Check if this handle is valid.
     *
     *    @param Handle    The handle to the resource.
     *
     *    @return T&       The resource, nullptr if the handle
     *                     is invalid/ points to a different type.
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
		if ( *(size_t*)pBuf != magic )
			return false;

		return true;
	}

	/*
    * 
    *    Get the number of resources.
    * 
    *    @return size_t    The number of resources.
	*/
	size_t size()
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


// TEST
template< typename T >
class ResourceListPtr : public ResourceList3< T, T* >
{
};


// template< typename T >
// class ResourceListRef : public ResourceList3< T, T& >
// {
// };

