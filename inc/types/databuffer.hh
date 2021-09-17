/*
databuffer.h ( Authored by p0lyh3dron )

A vector-like object with no constructor, to provide
functionality where a STL vector may cause segfaults.  
*/
#pragma once

#include <stdint.h>

template< class T >
class DataBuffer
{
	T			*apBuffers 	= NULL;
	uint32_t		aBufferCount    = 0;
public:
	/* Returns true if there are allocated buffers, false if the memory isn't and should not be accessed.  */
	bool			IsValid(  ){ return apBuffers; }
	/* Returns the buffer.  */
        T			*&GetBuffer(  ){ return apBuffers; }
	/* Returns the size of the buffer.  */
	uint32_t		GetSize(  ){ return aBufferCount; }
	/* Returns the topmost buffer.  */
	T			&GetTop(  ){ return apBuffers[ aBufferCount - 1 ]; }
	/* Allocates memory for the buffer.  */
	void			Allocate( uint32_t sSize ){ apBuffers = new T[ sSize + 1 ]; aBufferCount = sSize; }
	/* Reallocates the buffer.  */
	void			Reallocate( uint32_t sSize ){ T *pTemp = new T[ sSize + 1 ]; if ( IsValid(  ) ){ for ( uint32_t i = 0; i < aBufferCount && i < sSize; ++i ) pTemp[ i ] = apBuffers[ i ]; delete[  ] apBuffers; } apBuffers = pTemp; aBufferCount = sSize; }
	void			Increment(  ){ Reallocate( aBufferCount + 1 ); }
	/* Frees the buffer when no longer in use.  */
				~DataBuffer(  ){ delete[  ] apBuffers; }
};
