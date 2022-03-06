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

using Handle = size_t;

constexpr Handle InvalidHandle = 0;

template < typename T >
class ResourceManager
{
    MemPool aPool;
public:
    /*
     *    Construct a resource manager.
     */
    ResourceManager() : aPool() 
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

        size_t index = ( size_t )pBuf - ( size_t )aPool.GetStart();

        LogDev( 3, "ResourceManager::Add( T* ): Allocated resource at index %u\n", index );

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
        size_t index = sHandle & 0xFFFFFFFF;

        /*
         *    Get the magic number.
         */
        size_t magic = sHandle >> 32;

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

        LogDev( 3, "ResourceManager::Remove( Handle ): Removed resource at index %u\n", index );
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
        size_t magic = sHandle >> 32;

        /*
         *    Get the index from the handle.
         */
        size_t index = sHandle & 0xFFFFFFFF;

        /*
         *    Get the chunk of memory.
         */
        s8 *pBuf = aPool.GetStart() + index;

        /*
         *    Check the magic number.
         */
        if( *( size_t* )pBuf != magic ) {
            LogWarn( "*ResourceManager::Get( Handle ): Invalid handle: %d\n", sHandle );
            return nullptr;
        }

        LogDev( 4, "*ResourceManager::Get( Handle ): Retrieved resource at index %u\n", index );

        /*
         *    Return the data.
         */
        return ( T* )( pBuf + sizeof( magic ) );
    }
};