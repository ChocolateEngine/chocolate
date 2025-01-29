/*
 *    mempool.c    --    source file for memory pool
 * 
 *    Authored by Karl "p0lyh3dron" Kreuze on March 20, 2022
 * 
 *    This file is part of the Chik library, a general purpose
 *    library for the Chik engine and her games.
 * 
 *    This file defines the functions used for allocating and freeing
 *    memory.
 */

#include "core/mempool.h"

/*
 *    Creates a new memory pool.
 *
 *    @param s64             Size of the memory pool in bytes.
 * 
 *    @return mempool_t *    Pointer to the new memory pool.
 *                           Returns NULL on failure.
 *                           Should be freed with mempool_free().
 */
mempool_t* mempool_new( s64 sSize )
{
	if ( sSize <= 0 )
	{
		Log_Error( "Invalid memory pool size.\n" );
		return 0;
	}

	mempool_t* pMempool = static_cast< mempool_t* >( malloc( sizeof( mempool_t ) ) );

	if ( pMempool == 0 )
	{
		Log_Error( "Could not allocate memory for memory pool.\n" );
		return 0;
	}

	pMempool->apFirst = 0;
	pMempool->aSize   = sSize;
	pMempool->apBuf   = static_cast< s8* >( malloc( sSize ) );

	if ( pMempool->apBuf == 0 )
	{
		Log_Error( "Could not allocate memory for memory pool buffer.\n" );
		free( pMempool );
		return 0;
	}

	// Zero out the new data
	memset( pMempool->apBuf, 0, sSize );

	pMempool->apCur = pMempool->apBuf;
	pMempool->apEnd = pMempool->apBuf + sSize;
	return pMempool;
}

/*
 *    Consolidates the memory pool.
 *
 *    @param mempool_t *     Pointer to the memory pool.
 * 
 *    @return memerror_t     Error code.
 */
memerror_t mempool_consolidate( mempool_t* spPool )
{
	if ( spPool == 0 )
	{
		Log_Error( "Invalid memory pool.\n" );
		return MEMERR_INVALID_ARG;
	}

	if ( spPool->apFirst == 0 )
	{
		Log_Dev( 2, "Memory pool is free.\n" );
		return MEMERR_NONE;
	}

	for ( memchunk_t* pChunk = spPool->apFirst; pChunk != 0; pChunk = pChunk->apNext )
	{
		/*
         *    If the current and next chunk are adjacent, merge them.
         */
		if ( !pChunk->aUsed && pChunk->apNext != 0 && !pChunk->apNext->aUsed )
		{
			memchunk_t* pNext = pChunk->apNext;
			pChunk->aSize += pNext->aSize;
			pChunk->apNext = pNext->apNext;

			free( pNext );
		}
	}

	return MEMERR_NONE;
}

#if 0
void mempool_verify( mempool_t* spPool )
{
	if ( !spPool )
		return;

	size_t i = 0;
	for ( memchunk_t* pChunk = spPool->apFirst; pChunk != 0; pChunk = pChunk->apNext, i++ )
	{
		s8* buf = spPool->apBuf + ( pChunk->aSize * i );
		if ( buf != pChunk->apData )
		{
			pChunk->apData = buf;
			Log_Warn( "realloc happened\n" );
		}
	}
}
#endif

/*
 *    Reallocates a memory pool.
 *
 *    @param mempool_t *     Pointer to the memory pool.
 *    @param s64             Size of the memory pool in bytes.
 * 
 *    @return memerror_t     Error code.
 *                           MEMERR_NONE on success.
 *                           MEMERR_INVALID_ARG if the memory pool is NULL.
 *                           MEMERR_INVALID_SIZE if the size is <= 0.
 *                           MEMERR_NO_MEMORY if the memory pool could not be allocated.
 */
memerror_t mempool_realloc( mempool_t* spPool, s64 sSize )
{
	if ( spPool == 0 )
	{
		Log_Error( "Invalid memory pool.\n" );
		return MEMERR_INVALID_ARG;
	}

	if ( sSize <= 0 )
	{
		Log_Error( "Invalid memory pool size.\n" );
		return MEMERR_INVALID_SIZE;
	}

	s8* pBuf = static_cast< s8* >( realloc( spPool->apBuf, sSize ) );

	if ( pBuf == 0 )
	{
		Log_Error( "Could not reallocate memory for memory pool buffer.\n" );
		return MEMERR_NO_MEMORY;
	}

	spPool->apCur = pBuf + ( spPool->apCur - spPool->apBuf );
	spPool->apBuf = pBuf;
	spPool->apEnd = spPool->apBuf + sSize;
	spPool->aSize = sSize;

	// Zero out the new data
	memset( spPool->apCur, 0, sSize - spPool->aSize );

	// Update Pointers to data in all Chunks
	size_t i      = 0;
	for ( memchunk_t* pChunk = spPool->apFirst; pChunk != 0; pChunk = pChunk->apNext )
		pChunk->apData = spPool->apBuf + ( pChunk->aSize * i++ );

	return MEMERR_NONE;
}

/*
 *    Allocates a new memory chunk from the memory pool.
 *
 *    @param mempool_t *     Pointer to the memory pool.
 *    @param s64             Size of the memory chunk in bytes.
 * 
 *    @return s8 *           Pointer to the new memory chunk.
 *                           Returns NULL on failure.
 *                           Should be freed with mempool_free().
 *                           The memory chunk is not initialized.
 */
memerror_t mempool_alloc( mempool_t* spPool, s64 sSize, s8** spBuf )
{
	if ( spPool == 0 )
	{
		Log_Error( "Invalid memory pool.\n" );
		return MEMERR_INVALID_POOL;
	}

	if ( sSize <= 0 )
	{
		Log_Error( "Invalid memory chunk size.\n" );
		return MEMERR_INVALID_SIZE;
	}

	/*
     *    Check for free chunks.
     */
	for ( memchunk_t* pChunk = spPool->apFirst; pChunk != 0; pChunk = pChunk->apNext )
	{
		if ( !pChunk->aUsed && pChunk->aSize >= sSize )
		{
			/*
             *    Split the chunk if it's too big.
             */
			if ( pChunk->aSize > sSize )
			{
				memchunk_t* pNew = (memchunk_t*)calloc( 0, sizeof( memchunk_t ) );
				pNew->aUsed      = true;
				pNew->aSize      = pChunk->aSize - sSize;
				pNew->apNext     = pChunk->apNext;
				pChunk->apNext   = pNew;
			}
			pChunk->aUsed  = true;
			pChunk->aSize  = sSize;

			// Zero out the memory
			memset( pChunk->apData, 0, pChunk->aSize );

			*spBuf         = pChunk->apData;
			return MEMERR_NONE;
		}
	}

	/*
     *    No free chunks, check if there's enough space.
     */
	if ( spPool->apCur + sSize > spPool->apEnd )
		return MEMERR_NO_MEMORY;

	/*
     *    Allocate a new chunk.
     */
	memchunk_t* pChunk = (memchunk_t*)malloc( sizeof( memchunk_t ) );

	if ( !pChunk )
		return MEMERR_OUT_OF_MEMORY;

	s8* pBuf       = spPool->apCur;
	pChunk->aUsed  = true;
	pChunk->aSize  = sSize;
	pChunk->apData = spPool->apCur;
	pChunk->apNext = 0;
	spPool->apCur += sSize;

	/*
     *    Add the chunk to the list.
     */
	if ( spPool->apFirst == 0 )
	{
		spPool->apFirst = pChunk;
	}
	else
	{
		memchunk_t* pLast = spPool->apFirst;
		while ( pLast->apNext != 0 )
		{
			pLast = pLast->apNext;
		}
		pLast->apNext = pChunk;
	}

	*spBuf = pBuf;
	return MEMERR_NONE;
}

/*
 *    Frees a memory chunk from the memory pool.
 *
 *    @param mempool_t *     Pointer to the memory pool.
 *    @param s8 *            Pointer to the memory chunk.
 */
void mempool_free( mempool_t* spPool, s8* spChunk )
{
	if ( spPool == 0 )
	{
		Log_Error( "Invalid memory pool.\n" );
		return;
	}

	if ( spChunk == 0 )
	{
		Log_Error( "Invalid memory chunk.\n" );
		return;
	}

	/*
     *    Find the chunk.
     */
	memchunk_t* pChunk = spPool->apFirst;
	while ( pChunk != 0 )
	{
		if ( pChunk->apData == spChunk )
		{
			break;
		}
		pChunk = pChunk->apNext;
	}

	// Should not be nullptr?
	CH_ASSERT( pChunk );

	if ( !pChunk || pChunk->apData != spChunk )
	{
		Log_Error( "Could not find memory chunk.\n" );
		return;
	}

	/*
     *    Free the chunk.
     */
	pChunk->aUsed = false;

	// Set it's memory to Zero
	memset( pChunk->apData, 0, pChunk->aSize );
}

/*
 *    Destroys a memory pool.
 *
 *    @param mempool_t *     Pointer to the memory pool to destroy.
 * 
 *    @return void
 */
void mempool_destroy( mempool_t* spPool )
{
	if ( spPool == 0 )
	{
		Log_Error( "Invalid memory pool.\n" );
		return;
	}

	if ( spPool->apBuf != 0 )
	{
		free( spPool->apBuf );
	}

	memchunk_t* pChunk = spPool->apFirst;
	while ( pChunk != 0 )
	{
		memchunk_t* pNext = pChunk->apNext;
		free( pChunk );
		pChunk = pNext;
	}

	free( spPool );
}