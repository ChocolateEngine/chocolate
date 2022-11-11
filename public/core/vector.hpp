/*
vector.hpp ( Authored by p0lyh3dron )

A vector-like object with no constructor, to provide
functionality where a STL vector may cause segfaults.  
*/
#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "log.h"


template< typename T >
struct ChVector
{
	T*       apData    = nullptr;
	uint32_t aSize     = 0;
	uint32_t aCapacity = 0;

	// Returns true if there are allocated buffers, false if the memory isn't and should not be accessed.
	bool     valid() const { return apData; }

	// Returns the buffer.
	T*&      data() { return apData; }

	// Returns the size of the buffer.
	uint32_t size() const { return aSize; }

	// Returns the amount of the buffer currently allocated
	uint32_t capacity() const { return aCapacity; }

	// Returns the size of the buffer in bytes
	uint32_t size_bytes() const { return aSize * sizeof( T ); }

	// Returns if the buffer is empty or not.
	bool     empty() const { return aSize == 0; }

	// Returns the topmost buffer.
	T&       top() const { return apData[ aSize - 1 ]; }

	// Resizes the buffer
	void     resize( uint32_t sSize, bool sZero = false )
	{
		if ( apData )
		{
			if ( sSize > 0 && sSize > aCapacity )
			{
				void* newData = realloc( apData, sSize * sizeof( T ) );
				if ( newData == nullptr )
					Log_FatalF( "Failed to Resize ChVector: %zd bytes\n", sSize );

				apData = static_cast< T* >( newData );

				if ( sZero && sSize > aCapacity )
				{
					memset( &apData[ sSize - 1 ], 0, ( sSize - aCapacity ) * sizeof( T ) );
				}

				aCapacity = sSize;
			}
			else if ( sSize == 0 )
			{
				free( apData );
				apData = nullptr;
			}
		}
		else if ( sSize > 0 )
		{
			if ( sZero )
				apData = static_cast< T* >( calloc( sSize, sizeof( T ) ) );
			else
				apData = static_cast< T* >( malloc( sSize * sizeof( T ) ) );

			if ( apData == nullptr )
				Log_FatalF( "Failed to Allocate ChVector: %zd bytes\n", sSize );

			aCapacity = sSize;
		}

		aSize = sSize;
	}

	void clear()
	{
		aSize = 0;
	}

	void free_data()
	{
		if ( apData )
		{
			free( apData );
			apData = nullptr;
		}

		aSize = 0;
		aCapacity = 0;
	}
	
	// Free's extra memory allocated (aCapacity)
	void consolidate()
	{
		if ( aSize == aCapacity || !apData )
			return;

		if ( aSize == 0 )
		{
			free( apData );
			apData = nullptr;
		}
		else
		{
			void* newData = realloc( apData, aSize * sizeof( T ) );
			if ( newData == nullptr )
				Log_FatalF( "Failed to Resize ChVector: %zd bytes\n", apData );

			apData = static_cast< T* >( newData );
		}

		aCapacity = aSize;
	}

	// Add one element to the buffer and returns it
	T& emplace_back( bool sZero = false )
	{
		resize( aSize + 1, sZero );
		return apData[ aSize - 1 ];
	}

	// Add one element to the buffer
	void push_back( const T& srData )
	{
		resize( aSize + 1, false );
		memcpy( &apData[ aSize - 1 ], &srData, sizeof( T ) );

		// Assert( _heapchk() == _HEAPOK );
	}

	void remove( uint32_t sIndex )
	{
		if ( sIndex > aSize )
			Log_FatalF( "Attempted to remove an index out of bounds in buffer: %zd > %zd\n", sIndex, aSize );

		// shift all memory back one index
		if ( aSize )
		{
			uint32_t sSize = aSize - sIndex;
			memcpy( &apData[ sIndex ], &apData[ sIndex + 1 ], sSize * sizeof( T ) );
		}

		// resize( aSize - 1 );
		aSize--;
	}

	// Index into the buffer
	T& operator[]( uint32_t sIndex ) const
	{
		if ( sIndex > aSize )
			Log_FatalF( "Attempted to index out of bounds in buffer: %zd > %zd\n", sIndex, aSize );

		return apData[ sIndex ];
	}

	// Support for range based for loops
	T* begin() const
	{
		return apData;
	}

	T* end() const
	{
		return &apData[ aSize ];
	}

	ChVector() :
		apData( nullptr ), aSize( 0 )
	{
	}

	ChVector( uint32_t sSize ) :
		apData( nullptr ), aSize( 0 )
	{
		resize( sSize );
	}

	~ChVector()
	{
		free_data();
	}
};

