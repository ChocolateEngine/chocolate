#pragma once


#include <stdint.h>
#include <stdlib.h>

#include "log.h"


// TODO: make a ChStringView type


template< typename T >
struct ChStringBase
{
	T*       apData = nullptr;
	uint32_t aSize  = 0;

	// Returns true if there are allocated buffers, false if the memory isn't and should not be accessed.
	bool     valid() const { return apData; }

	// Returns the buffer.
	T*&      data() { return apData; }

	// Returns the size of the buffer.
	uint32_t size() const { return aSize; }

	// Returns if the buffer is empty or not.
	bool     empty() const { return aSize == 0; }

	// Returns the topmost buffer.
	T&       top() const { return apData[ aSize - 1 ]; }

	// Resizes the buffer
	void     resize( uint32_t sSize, bool sZero = false )
	{
		if ( apData )
		{
			if ( sSize > 0 )
			{
				void* newData = realloc( apData, sSize * sizeof( T ) );
				if ( newData == nullptr )
					Log_FatalF( "Failed to Resize ChVector: %zd bytes\n", sSize );

				apData = static_cast< T* >( newData );

				if ( sZero && sSize > aSize )
				{
					memset( &apData[ sSize ], 0, aSize - sSize * sizeof( T ) );
				}
			}
			else
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
		}

		aSize = sSize;
	}

	void clear()
	{
		if ( apData )
		{
			free( apData );
			apData = nullptr;
		}

		aSize = 0;
	}

	// Add one element to the buffer and returns it
	T& emplace_back()
	{
		resize( aSize + 1 );
		return apData[ aSize - 1 ];
	}

	// Add one element to the buffer
	void push_back( const T& srData )
	{
		resize( aSize + 1 );
		memcpy( &apData[ aSize - 1 ], &srData, sizeof( T ) );
	}

	void remove( uint32_t sIndex )
	{
		if ( sIndex > aSize )
			Log_FatalF( "Attempted to remove an index out of bounds in string: %zd > %zd\n", sIndex, aSize );

		// shift all memory back one index
		if ( aSize )
		{
			uint32_t sSize = aSize - sIndex;
			memcpy( &apData[ sIndex ], &apData[ sIndex + 1 ], sSize * sizeof( T ) );
		}

		resize( aSize - 1 );
	}

	void operator=( const ChStringBase< T >& srOther )
	{
		memcpy( this, &srOther, sizeof( ChStringBase< T > ) );
	}

	// Index into the string
	T& operator[]( uint32_t sIndex ) const
	{
		if ( sIndex > aSize )
			Log_FatalF( "Attempted to index out of bounds in string: %zd > %zd\n", sIndex, aSize );

		return apData[ sIndex ];
	}

	void operator+=( const T& srOther )
	{
		push_back( srOther );
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

	void init()
	{
		apData = nullptr;
		aSize  = 0;
	}

	explicit ChStringBase() :
		apData( nullptr ), aSize( 0 )
	{
	}

	explicit ChStringBase( uint32_t sSize ) :
		apData( nullptr ), aSize( sSize )
	{
		resize( sSize );
	}

	~ChStringBase()
	{
		if ( apData )
			free( apData );
	}
};


struct ChString : public ChStringBase< char >
{
	void operator=( const char* spOther )
	{
		if ( !spOther )
		{
			apData = nullptr;
			aSize = 0;
			return;
		}

		size_t len = strlen( spOther );

		resize( len, true );
		strcpy( apData, spOther );
		aSize = len;
	}

	void operator=( const std::string& srOther )
	{
		if ( !srOther.empty() )
		{
			apData = nullptr;
			aSize = 0;
			return;
		}

		size_t len = srOther.size();

		resize( len, true );
		strcpy( apData, srOther.data() );
		aSize = len;
	}

	void operator=( std::string_view sOther )
	{
		if ( !sOther.empty() )
		{
			apData = nullptr;
			aSize = 0;
			return;
		}

		size_t len = sOther.size();

		resize( len, true );
		strcpy( apData, sOther.data() );
		aSize = len;
	}

	bool operator==( const std::string& srOther )
	{
		if ( srOther.size() != aSize )
			return false;

		return strncmp( apData, srOther.data(), aSize ) == 0;
	}

	bool operator==( std::string_view sOther )
	{
		if ( sOther.size() != aSize )
			return false;

		return strncmp( apData, sOther.data(), aSize ) == 0;
	}

	bool operator==( const ChString& srOther )
	{
		if ( srOther.aSize != aSize )
			return false;

		return strncmp( apData, srOther.apData, aSize ) == 0;
	}

	bool operator==( const char* spOther )
	{
		return strcmp( apData, spOther ) == 0;
	}

	bool operator!=( const ChString& srOther )
	{
		if ( srOther.aSize == aSize )
			return false;

		return strncmp( apData, srOther.apData, aSize ) != 0;
	}

	bool operator!=( const char* spOther )
	{
		return strcmp( apData, spOther ) != 0;
	}
	
	explicit ChString() : ChStringBase() {}
	explicit ChString( uint32_t sSize ) : ChStringBase( sSize ) {}

	ChString( const char* spBuffer ) : ChStringBase()
	{
		operator=( spBuffer );
	}
};


struct ChWString : public ChStringBase< wchar_t >
{
	void operator=( const wchar_t* spOther )
	{
		if ( !spOther )
		{
			apData = nullptr;
			aSize  = 0;
			return;
		}

		wcscpy( apData, spOther );
		aSize = wcslen( spOther );
	}

	bool operator==( const ChWString& srOther )
	{
		if ( srOther.aSize != aSize )
			return false;

		return wcsncmp( apData, srOther.apData, aSize ) == 0;
	}

	bool operator==( const wchar_t* spOther )
	{
		return wcscmp( apData, spOther ) == 0;
	}

	bool operator!=( const ChWString& srOther )
	{
		if ( srOther.aSize != aSize )
			return false;

		return wcsncmp( apData, srOther.apData, aSize ) == 0;
	}

	bool operator!=( const wchar_t* spOther )
	{
		return wcscmp( apData, spOther ) == 0;
	}
	
	explicit ChWString() : ChStringBase() {}
	explicit ChWString( uint32_t sSize ) : ChStringBase( sSize ) {}

	ChWString( const wchar_t* spBuffer ) : ChStringBase()
	{
		operator=( spBuffer );
	}
};


// Unicode String
#ifdef _WIN32
using ChUString = ChWString;
#elif __unix__
using ChUString = ChString;
#else
#error Undefined ChUString Type
#endif

