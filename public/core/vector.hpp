/*
vector.hpp ( Authored by p0lyh3dron )

A vector-like object with no constructor, to provide
functionality where a STL vector may cause segfaults.  
*/
#pragma once

#include <stdint.h>
#include <stdlib.h>

#include "log.h"
#include "asserts.h"


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

	// Returns the size of the buffer in bytes
	uint32_t size_bytes() const { return aSize * sizeof( T ); }

	// Returns the amount of the buffer currently allocated
	uint32_t capacity() const { return aCapacity; }

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
					// Zero out the new data 
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
			// NOTE: this is actually useless, you can use realloc with nullptr
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

	// Pre-allocates memory while maintaining stored size
	void     reserve( uint32_t sSize )
	{
		if ( apData )
		{
			if ( sSize > 0 && sSize > aCapacity )
			{
				void* newData = realloc( apData, sSize * sizeof( T ) );
				if ( newData == nullptr )
					Log_FatalF( "Failed to Resize ChVector: %zd bytes\n", sSize );

				apData = static_cast< T* >( newData );

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
			apData = static_cast< T* >( malloc( sSize * sizeof( T ) ) );

			if ( apData == nullptr )
				Log_FatalF( "Failed to Allocate ChVector: %zd bytes\n", sSize );

			aCapacity = sSize;
		}
	}

	// Reset size, does not free any memory, use free_data() if you want that
	void clear()
	{
		aSize = 0;
	}

	// Free all allocated memory
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
		top() = srData;

		// dst, src, size
		// memcpy( &apData[ aSize - 1 ], &srData, sizeof( T ) );
		// std::copy( &apData[ aSize - 1 ], &srData, sizeof( T ) );
		// std::copy( other.begin(), other.end(), apData );
	}

	void push_back( T&& srData )
	{
		resize( aSize + 1, false );

		// dst, src, size
		// memcpy( &apData[ aSize - 1 ], &srData, sizeof( T ) );
		top() = std::move( srData );
	}

	// Remove an item by index
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

	// Find the index of an item in the vector
	uint32_t index( const T& sItem, uint32_t fallback = UINT32_MAX )
	{
		auto it = std::find( begin(), end(), sItem );
		if ( it != end() )
			return it - begin();

		return fallback;
	}

	// Search for an item and remove it from the vector
	void erase( const T& sItem )
	{
		uint32_t index = this->index( sItem );

		if ( index == UINT32_MAX )
		{
			Log_Error( "Failed to find item in ChVector!\n" );
			return;
		}

		remove( index );
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

	// --------------------------------------------------
	// necessary stuff for objects

	// copying
	void assign( const ChVector& other )
	{
		reserve( other.aSize );
		std::move( other.begin(), other.end(), apData );
	}

	// moving
	void assign( ChVector&& other )
	{
		// swap the values of this one with that one
		std::swap( aSize, other.aSize );
		std::swap( aCapacity, other.aCapacity );
		std::swap( apData, other.apData );
	}

	ChVector& operator=( const ChVector& other )
	{
		if ( valid() )
		{
			aSize = other.aSize;
			consolidate();
		}

		assign( other );
		return *this;
	}

	ChVector& operator=( ChVector&& other )
	{
		if ( valid() )
		{
			aSize = other.aSize;
			consolidate();
		}

		assign( std::move( other ) );
		return *this;
	}

	// copying
	ChVector( const ChVector& other )
	{
		assign( other );
	}

	// moving
	ChVector( ChVector&& other )
	{
		assign( std::move( other ) );
	}
};

