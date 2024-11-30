/*
 *    mempool.h    --    header file for memory pool
 * 
 *    Authored by Karl "p0lyh3dron" Kreuze on March 20, 2022
 * 
 *    This file is part of the Chik library, a general purpose
 *    library for the Chik engine and her games.
 * 
 *    Included here is a memory pool, which is a very simple
 *    memory allocator. It is not a general purpose allocator,
 *    but it is a very simple one.
 */
#pragma once

#include "core/core.h"

typedef enum
{
	MEMERR_NONE,
	MEMERR_NO_MEMORY,
	MEMERR_OUT_OF_MEMORY,
	MEMERR_INVALID_POOL,
	MEMERR_INVALID_ARG,
	MEMERR_INVALID_SIZE,
	MEMERR_INVALID_POINTER,
} memerror_t;

typedef struct memchunk_s
{
	bool               aUsed;
	s8*                apData;
	s64                aSize;

	struct memchunk_s* apNext;
} memchunk_t;

typedef struct
{
	s8*         apBuf;
	s8*         apEnd;
	s8*         apCur;
	s64         aSize;

	memchunk_t* apFirst;
} mempool_t;

/*
 *    Creates a new memory pool.
 *
 *    @param s64             Size of the memory pool in bytes.
 * 
 *    @return mempool_t *    Pointer to the new memory pool.
 *                           Returns NULL on failure.
 *                           Should be freed with mempool_free().
 */
mempool_t CORE_API* mempool_new( s64 sSize );

/*
 *    Consolidates the memory pool.
 *
 *    @param mempool_t *     Pointer to the memory pool.
 * 
 *    @return memerror_t     Error code.
 */
memerror_t CORE_API mempool_consolidate( mempool_t* spPool );

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
memerror_t CORE_API mempool_realloc( mempool_t* spPool, s64 sSize );

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
memerror_t CORE_API mempool_alloc( mempool_t* spPool, s64 sSize, s8** spBuf );

/*
 *    Frees a memory chunk from the memory pool.
 *
 *    @param mempool_t *     Pointer to the memory pool.
 *    @param s8 *            Pointer to the memory chunk.
 */
void CORE_API       mempool_free( mempool_t* spPool, s8* spChunk );

/*
 *    Destroys a memory pool.
 *
 *    @param mempool_t *     Pointer to the memory pool to destroy.
 * 
 *    @return void
 */
void CORE_API       mempool_destroy( mempool_t* spPool );

/*
 *    Helper function to get the capacity of a memory pool.
 *
 *    @param mempool_t *     Pointer to the memory pool to destroy.
 * 
 *    @return size_t         The capacity of the memory pool
 */
inline size_t       mempool_capacity( mempool_t* spPool )
{
	if ( spPool == nullptr )
		return 0;

	return spPool->apEnd - spPool->apBuf;
}

/*
 *    Helper function to get an error description.
 *
 *    @param memerror_t      The mempool error
 * 
 *    @return const char*    The error description
 */
inline const char* mempool_error2str( memerror_t sError )
{
	switch ( sError )
	{
		default:
			return "Invalid Error Type, you really messed up now";

		case MEMERR_NONE:
			return "None";

		case MEMERR_NO_MEMORY:
			return "No Memory";

		case MEMERR_OUT_OF_MEMORY:
			return "A malloc or realloc call failed and we are out of memory";

		case MEMERR_INVALID_POOL:
			return "Invalid Memory Pool";

		case MEMERR_INVALID_ARG:
			return "Invalid Argument";

		case MEMERR_INVALID_SIZE:
			return "Invalid Size";

		case MEMERR_INVALID_POINTER:
			return "Invalid Pointer";
	};
}
