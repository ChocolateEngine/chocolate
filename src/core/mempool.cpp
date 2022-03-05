/*
 *    mempool.cpp    --    Memory pool
 *
 *    Authored by Karl "p0lyh3dron" Kreuze on March 5, 2022
 *
 *    This files defines the memory pool.
 */
#include "core/mempool.h"
#include "core/core.h"
/*
 *    Resizes the memory pool.
 *
 *    @param  size_t      The new size of the memory pool.
 *
 *    @return MemError    The result of the operation.
 */
MemError MemPool::Resize( size_t sSize )
{
    s8 *pNew = ( s8* )realloc( apBuf, sSize );
    if( pNew == nullptr ) {
        LogWarn( "Failed to resize memory pool to %d bytes.", sSize );
        return MemPool_OutOfMemory;
    }
    apBuf = pNew;
    apEnd = apBuf + sSize;

    return MemPool_Success;
}
/*
 *    Consolidates regions of fragmented memory.
 */
void MemPool::Consolidate()
{
    if ( apChunks == nullptr )
        return;
    MemChunk *pChunk = apChunks;
    while( pChunk != nullptr ) {
        if( pChunk->aFlags & Mem_Free ) {
            if( pChunk->apNext != nullptr ) {
                if( pChunk->apNext->aFlags & Mem_Free ) {
                    pChunk->aSize += pChunk->apNext->aSize;
                    pChunk->apNext = pChunk->apNext->apNext;
                }
            }
        }
        pChunk = pChunk->apNext;
    }
}
/*
 *    Construct a memory pool.
 *
 *    @param size_t    The size of the memory pool.
 */
MemPool::MemPool( size_t sSize )
{
    apBuf    = ( s8* )calloc( 0, sSize );
    apEnd    = apBuf + sSize;
    apPtr    = apBuf;
    apChunks = nullptr;
}
/*
 *    Destruct the memory pool.
 */
MemPool::~MemPool()
{
    free( apBuf );
}

/*
 *    Allocate a block of memory from the pool.
 *
 *    @param  size_t    The size of the block to allocate.
 *    @return void*     The allocated block.
 */
void *MemPool::Alloc( size_t sSize )
{
    Consolidate();
    /*
     *    Find a free chunk.
     */
    for ( MemChunk *p = apChunks; p != nullptr; p = p->apNext ) {
        if ( p->aFlags & Mem_Free ) {
            /*
             *    Found a free chunk.
             */
            if ( p->aSize >= sSize ) {
                /*
                 *    The chunk is big enough.
                 */
                if ( p->aSize > sSize ) {
                    /*
                     *    The chunk is bigger than the requested size.
                     *    Split the chunk.
                     */
                    MemChunk *pNew = ( MemChunk* )( p->apData + sSize );
                    pNew->aFlags   = Mem_Free;
                    pNew->apNext   = p->apNext;
                    pNew->aSize    = p->aSize - sSize;
                    pNew->apData   = p->apData + sSize;
                    p->apNext      = pNew;
                }
                p->aFlags = Mem_Used;
                return p->apData;
            }
        }
    }
    /*
     *    No free chunk found.
     */
    if( apPtr + sSize > apEnd ) {
        if ( apPtr + sSize > apPtr + aStepSize ) {
            /*
             *    The requested size is bigger than the step size.
             *    Resize the memory pool by the whole new size.
             */
            MemError e = Resize( ( size_t )( apPtr - apEnd ) + sSize + aStepSize );
            if( e != MemPool_Success ) {
                LogWarn( "Failed to resize memory pool to %d bytes.", apPtr + sSize );
                return nullptr;
            }
        } 
        else {
            /*
             *    The requested size is smaller than the step size.
             *    Resize the memory pool by the defined increment
             */
            MemError e = Resize( ( size_t )( apPtr - apEnd ) + aStepSize );
            if( e != MemPool_Success ) {
                LogWarn( "Failed to resize memory pool to %d bytes.", apPtr + aStepSize );
                return nullptr;
            }
        }
    }

    MemChunk *pChunk = ( MemChunk* )malloc( sizeof( MemChunk ) );
    pChunk->aFlags = Mem_Used;
    pChunk->apData = apPtr;
    pChunk->aSize  = sSize;
    pChunk->apNext = nullptr;

    if ( apChunks == nullptr ) {
        apChunks = pChunk;
    } 
    else {
        MemChunk *p = apChunks;
        while( p->apNext != nullptr ) {
            p = p->apNext;
        }
        p->apNext = pChunk;
    }

    void *pRet = apPtr;
    apPtr     += sSize;
    return pRet;
}
/*
 *    Free a block of memory from the pool.
 *
 *    @param  void*     The block to free.
 */
void MemPool::Free( void* spPtr )
{
    for ( MemChunk *p = apChunks; p != nullptr; p = p->apNext ) {
        if( p->apNext == spPtr ) {
            p->apNext = nullptr;
        }
    }
}

/*
 *    Sets the step size of the memory pool for reallocations.
 *
 *    @param  size_t    The new step size.
 */
void MemPool::SetStepSize( size_t sSize )
{
    aStepSize = sSize;
}