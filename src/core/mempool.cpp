/*
 *    mempool.cpp    --    Memory pool
 *
 *    Authored by Karl "p0lyh3dron" Kreuze on March 5, 2022
 *
 *    This files defines the memory pool.
 */
#include "core/mempool.h"
#include "core/profiler.h"

#include "core/core.h"

LOG_REGISTER_CHANNEL( MemPool, LogColor::DarkCyan );

#define LOCK() auto m = AutoMutex( apMutex )

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
                    LogDev( gMemPoolChannel, 2, "Consolidated memory chunk at %p with new size %d\n", pChunk, pChunk->aSize );
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

    apMutex = SDL_CreateMutex();
    if ( apMutex == nullptr ) {
        LogError( gMemPoolChannel, "Failed to create mutex: %s\n", SDL_GetError() );
    }
}
/*
 *    Construct a memory pool.
 */
MemPool::MemPool() {
    apBuf    = nullptr;
    apEnd    = nullptr;
    apPtr    = nullptr;
    apChunks = nullptr;

    apMutex = SDL_CreateMutex();
    if ( apMutex == nullptr ) {
        LogError( gMemPoolChannel, "Failed to create mutex: %s\n", SDL_GetError() );
    }
}
/*
 *    Destruct the memory pool.
 */
MemPool::~MemPool()
{
    if ( apMutex != nullptr ) {
        SDL_DestroyMutex( apMutex );
    }
    free( apBuf );
}

/*
 *    Resizes the memory pool.
 *
 *    @param  size_t      The new size of the memory pool.
 *
 *    @return MemError    The result of the operation.
 */
MemError MemPool::Resize( size_t sSize )
{
    if ( apBuf == nullptr )
    {
        apBuf = ( s8* )malloc( sSize );
        if ( apBuf == nullptr ) {
            return MemPool_OutOfMemory;
        }
        
        apEnd = apBuf + sSize;
        apPtr = apBuf;
        aSize = sSize;
    }

    s8 *pNew = ( s8* )realloc( apBuf, sSize );
    if( pNew == nullptr ) {
        LogWarn( gMemPoolChannel, "Failed to resize memory pool to %d bytes.\n", sSize );
        return MemPool_OutOfMemory;
    }
    apPtr = pNew + ( apPtr - apBuf );
    apBuf = pNew;
    apEnd = apBuf + sSize;
    aSize = sSize;

    LogDev( gMemPoolChannel, 2, "Resized memory pool to %d bytes.\n", sSize );

    return MemPool_Success;
}
/*
 *    Allocate a block of memory from the pool.
 *
 *    @param  size_t    The size of the block to allocate.
 *    @return void*     The allocated block.
 */
void *MemPool::Alloc( size_t sSize )
{
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

                    LogDev( gMemPoolChannel, 3, "Split chunk at %p with new size %d\n", pNew, pNew->aSize );
                }
                LogDev( gMemPoolChannel, 4, "Allocated %d bytes in previously freed region at %p\n", sSize, p->apData );
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
                LogWarn( gMemPoolChannel, "Failed to resize memory pool to %d bytes.\n", apPtr + sSize );
                return nullptr;
            }
        } 
        else {
            /*
             *    The requested size is smaller than the step size.
             *    Resize the memory pool by the defined increment
             */
            MemError e = Resize( aSize + aStepSize );
            if( e != MemPool_Success ) {
                LogWarn( gMemPoolChannel, "Failed to resize memory pool to %d bytes.\n", apPtr + aStepSize );
                return nullptr;
            }
        }
        /*
         *    Consolidate free memory.
         */
        Consolidate();
    }

    MemChunk *pChunk = ( MemChunk* )malloc( sizeof( MemChunk ) );
    pChunk->aFlags = Mem_Used;
    pChunk->apData = apPtr;
    pChunk->aSize  = sSize;
    pChunk->apNext = nullptr;

#ifdef TRACY_ENABLE
    // TracyAlloc( pChunk, sSize );
#endif

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

    LogDev( gMemPoolChannel, 4, "Allocated %d bytes at %p.\n", sSize, apPtr );

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
        if( p->apData == spPtr ) {
            p->aFlags = Mem_Free;

#ifdef TRACY_ENABLE
            // TracyFree( spPtr );
#endif
            return;
        }
    }
    LogWarn( gMemPoolChannel, "Failed to find memory at %p.\n", spPtr );
}

/*
 *    Clear all blocks of memory from the pool.
 */
void MemPool::Clear()
{
    for ( MemChunk *p = apChunks; p != nullptr; p = p->apNext ) {
        p->aFlags = Mem_Free;
    }
    LogDev( gMemPoolChannel, 3, "Freed all memory from pool\n" );
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