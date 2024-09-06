#pragma once

#include "util.h"


struct handle_test_16_t
{
	u16 value;

	handle_test_16_t() = default;
	handle_test_16_t( u16 value ) :
		value( value ) {}

	operator u16() const
	{
		return value;
	}

	bool operator==( const handle_test_16_t& other ) const
	{
		return value == other.value;
	}

	bool operator!=( const handle_test_16_t& other ) const
	{
		return value != other.value;
	}

	bool operator<( const handle_test_16_t& other ) const
	{
		return value < other.value;
	}

	bool operator>( const handle_test_16_t& other ) const
	{
		return value > other.value;
	}

	bool operator<=( const handle_test_16_t& other ) const
	{
		return value <= other.value;
	}

	bool operator>=( const handle_test_16_t& other ) const
	{
		return value >= other.value;
	}

	handle_test_16_t& operator++()
	{
		++value;
		return *this;
	}

	handle_test_16_t operator++( int )
	{
		handle_test_16_t temp = *this;
		++value;
		return temp;
	}

	handle_test_16_t& operator--()
	{
		--value;
		return *this;
	}
};


template< typename data_type >
struct handle_test_base_t
{
	data_type value;

	handle_test_base_t() = default;
	handle_test_base_t( data_type value ) :
		value( value ) {}

	operator data_type() const
	{
		return value;
	}

	bool operator==( const handle_test_base_t& other ) const
	{
		return value == other.value;
	}

	bool operator!=( const handle_test_base_t& other ) const
	{
		return value != other.value;
	}

	bool operator<( const handle_test_base_t& other ) const
	{
		return value < other.value;
	}

	bool operator>( const handle_test_base_t& other ) const
	{
		return value > other.value;
	}

	bool operator<=( const handle_test_base_t& other ) const
	{
		return value <= other.value;
	}

	bool operator>=( const handle_test_base_t& other ) const
	{
		return value >= other.value;
	}

	handle_test_base_t& operator++()
	{
		++value;
		return *this;
	}

	handle_test_base_t operator++( int )
	{
		handle_test_base_t temp = *this;
		++value;
		return temp;
	}

	handle_test_base_t& operator--()
	{
		--value;
		return *this;
	}
};


#if _DEBUG

#if 0
#define CH_HANDLE_DEF_BASE( struct_name, data_type ) struct struct_name                                           \
{															 \
	data_type value;												 \
															 \
	struct_name() = default;							 \
	struct_name( type value ) :							 \
		value( value ) {}									 \
															 \
	operator type() const									 \
	{														 \
		return value;										 \
	}														 \
															 \
	bool operator==( const struct_name& other ) const	 \
	{														 \
		return value == other.value;						 \
	}														 \
															 \
	bool operator!=( const struct_name& other ) const	 \
	{														 \
		return value != other.value;						 \
	}														 \
															 \
	bool operator<( const struct_name& other ) const	 \
	{														 \
		return value < other.value;							 \
	}														 \
															 \
	bool operator>( const struct_name& other ) const	 \
	{														 \
		return value > other.value;							 \
	}														 \
															 \
	bool operator<=( const struct_name& other ) const	 \
	{														 \
		return value <= other.value;						 \
	}														 \
															 \
	bool operator>=( const struct_name& other ) const	 \
	{														 \
		return value >= other.value;						 \
	}														 \
															 \
	struct_name& operator++()							 \
	{														 \
		++value;											 \
		return *this;
	} \
	 \
	struct_name operator++( int ) \
	{ \
		struct_name temp = *this; \
		++value; \
		return temp; \
	} \
	 \
	struct_name& operator--() \
	{ \
		--value; \
		return *this; \
	} \
}
#endif

#define CH_HANDLE_16( type ) using type = handle_test_base_t< u16 >
#define CH_HANDLE_32( type ) using type = handle_test_base_t< u32 >
#define CH_HANDLE_64( type ) using type = handle_test_base_t< u64 >

#else

#define CH_HANDLE_16( type ) using type = u16
#define CH_HANDLE_32( type ) using type = u32
#define CH_HANDLE_64( type ) using type = u64

#endif


template< typename handle_type, typename data_type, u16 step_size = 4 >
struct ch_handle_array_16_t
{
	u16          arr_count;     // amount of elements in the two arrays
	u16          arr_capacity;  // amount of elements allocated for the two arrays

	data_type*   arr_data;

	// the index of this maps to data array
	handle_type* arr_handles;

	ch_handle_array_16_t() = default;

	ch_handle_array_16_t( u16 count, data_type* data, handle_type* handles ) :
		arr_count( count ),
		arr_data( data ),
		arr_handles( handles )
	{
	}

	~ch_handle_array_16_t()
	{
		if ( arr_data )
			ch_free( arr_data );

		if ( arr_handles )
			ch_free( arr_handles );
	}

private:
	bool allocate_data_and_handle()
	{
		if ( arr_count == arr_capacity )
		{
			// allocate more memory for the data array
			data_type* new_data = ch_realloc( arr_data, arr_capacity + step_size );

			if ( !new_data )
				return false;

			arr_data = new_data;

			// allocate more memory for the handle array
			handle_type* new_handles = ch_realloc( arr_handles, arr_capacity + step_size );

			if ( !new_handles )
				return false;

			arr_handles = new_handles;

			arr_capacity += step_size;
		}
		
		return true;
	}

public:
	handle_type create( data_type** data_ptr )
	{
		// check if we need to allocate more memory
		if ( !allocate_data_and_handle() )
			return {};

		// generate a handle magic number.
		u16        magic    = ( rand() % 0xFFFE ) + 1;

		data_type* new_data = arr_data[ arr_count ];

		// re-assign the output pointer to a pointer to the data
		*data_ptr           = (data_type*)( arr_data[ arr_count ] );

		// set the remaining memory to zero
		memset( arr_data[ arr_count ], 0, sizeof( data_type ) );

		// create the handle
		arr_handles[ arr_count ] = magic;

		handle_type handle   = arr_handles[ arr_count ];

		arr_count++;
		return handle;
	}

	void free( handle_type handle )
	{
		for ( u16 i = 0; i < arr_count; i++ )
		{
			if ( arr_handles[ i ] == handle )
			{
				// set memory to zero (is this needed if i might override it from shifting everything back?)
				memset( arr_data[ i ], 0, sizeof( data_type ) );

				// remove the handle
				memset( arr_handles[ i ], 0, sizeof( handle_type ) );

				// shift the data and handle arrays
				for ( u16 j = i; j < arr_count - 1; j++ )
				{
					arr_data[ j ]    = arr_data[ j + 1 ];
					arr_handles[ j ] = arr_handles[ j + 1 ];
				}

				arr_count--;
				break;
			}
		}
	}

	data_type* get( handle_type handle )
	{
		// this SUCKS
		for ( u16 i = 0; i < arr_count; i++ )
		{
			if ( arr_handles[ i ] == handle )
				return arr_data[ i ];
		}

		return nullptr;
	}

	// free auto consolidates
//	void consolidate()
//	{
//		// shift the data and handle arrays
//		for ( u16 i = 0; i < count; i++ )
//		{
//			// is this a free handle slot?
//			if ( arr_handles[ i ] == 0 )
//			{
//				// shift all the data and handle arrays back one
//				for ( u16 j = i; j < count - 1; j++ )
//				{
//					arr_data[ j ]    = arr_data[ j + 1 ];
//					arr_handles[ j ] = arr_handles[ j + 1 ];
//				}
//
//				count--;
//			}
//		}
//	}

	void consolidate()
	{
		if ( arr_capacity > arr_count )
		{
			// allocate less memory for the data array
			data_type* new_data = ch_realloc( arr_data, arr_count );

			if ( new_data )
				arr_data = new_data;

			// allocate less memory for the handle array
			handle_type* new_handles = ch_realloc( arr_handles, arr_count );

			if ( new_handles )
				arr_handles = new_handles;

			arr_capacity = arr_count;
		}
	}

};


template< typename data_type, typename handle_type >
struct ch_handle_array_32_t
{
	u32          count;
	data_type*   data;
	handle_type* handles;
};


template< typename data_type, typename handle_type >
struct ch_handle_array_64_t
{
	u64          count;
	data_type*   data;
	handle_type* handles;
};


#define CH_HANDLE_ARRAY_16( struct_name, data_type, handle_type, step_size ) \
	CH_HANDLE_16( handle_type ); \
	struct struct_name : ch_handle_array_16_t< data_type, handle_type, step_size >

