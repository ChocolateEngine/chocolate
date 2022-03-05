/*
 *    resources.h    --    Resource management
 *
 *    Authroed by Karl "p0lyh3dron" Kreuze on March 5, 2022
 *
 *    This files declares the resource manager, along with
 *    all the types related to handles and resource management.
 */
#pragma once

#include "mempool.h"

using Handle = size_t;

template < typename T >
class ResourceManager
{
    MemPool aPool;
public:
    /*
     *    Construct a resource manager.
     *
     *    @param size_t    Count of resources to be allocated.
     */
    ResourceManager( size_t sCount = 256 * sizeof( T ) ) : aPool( sCount * sizeof( T ) )
    {

    }
    /*
     *    Destruct the resource manager.
     */
    ~ResourceManager()
    {
        
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
        size_t magic = rand();

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
        *( size_t* )pBuf = magic;
        pBuf            += sizeof( magic );
        *( T* )pBuf      = *pData;

        size_t index = ( size_t )pBuf - ( size_t )aPool.GetStart() - sizeof( magic );

        return index | magic << 32;
    }
    /*
     *    Remove a resource.
     *
     *    @param Handle    The handle to the resource.
     */
    void   Remove( Handle sHandle )
    {

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
            LogWarn( "Invalid handle: %d\n", sHandle );
            return nullptr;
        }

        /*
         *    Return the data.
         */
        return ( T* )( pBuf + sizeof( magic ) );
    }
};