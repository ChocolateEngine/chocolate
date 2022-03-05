/*
 *    mempool.h    --    Memory pool
 *
 *    Authored by Karl "p0lyh3dron" Kreuze on March 5, 2022
 *
 *    This files declares the memory pool.
 */
#pragma once

#include <stdio.h>

#include "../util.h"

using s8 = char;

enum MemError {
    MemPool_Success       = 0,
    MemPool_OutOfMemory = 1,
};

enum MemFlag {
    Mem_Free = 1 << 0,
    Mem_Used = 1 << 1,
};

struct MemChunk {
    size_t    aFlags;
    s8       *apData;
    size_t    aSize;

    MemChunk *apNext;
};

class MemPool
{
    s8       *apBuf;
    s8       *apEnd;
    s8       *apPtr;
    
    MemChunk *apChunks;

    size_t    aSize;

    size_t    aStepSize = 2048;

    /*
     *    Resizes the memory pool.
     *
     *    @param  size_t      The new size of the memory pool.
     *
     *    @return MemError    The result of the operation.
     */
    MemError Resize( size_t sSize );
    /*
     *    Consolidates regions of fragmented memory.
     */
    void Consolidate();
public:
    /*
     *    Construct a memory pool.
     *
     *    @param size_t    The size of the memory pool.
     */
     MemPool( size_t sSize );
    /*
     *    Destruct the memory pool.
     */
    ~MemPool();

    /*
     *    Allocate a block of memory from the pool.
     *
     *    @param  size_t    The size of the block to allocate.
     *    @return void*     The allocated block.
     */
    void *Alloc( size_t sSize );
    /*
     *    Free a block of memory from the pool.
     *
     *    @param  void*     The block to free.
     */
    void  Free( void* spPtr );
    /*
     *    Sets the step size of the memory pool for reallocations.
     *
     *    @param  size_t    The new step size.
     */
    void  SetStepSize( size_t sSize );
};