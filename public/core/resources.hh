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
        LogDev( gResourceChannel, 3, "ResourceManager::Add( T* ): Allocated resource at index %u\n", index );
#endif

        aSize++;

        return index | magic << 32;
    }
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
        LogDev( gResourceChannel, 3, "ResourceManager::Remove( Handle ): Removed resource at index %u\n", index );
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
            LogWarn( gResourceChannel, "*ResourceManager::Get( Handle ): Invalid handle: %d\n", sHandle );
            return nullptr;
        }

#if RESOURCE_DEV
        LogDev( gResourceChannel, 4, "*ResourceManager::Get( Handle ): Retrieved resource at index %u\n", index );
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