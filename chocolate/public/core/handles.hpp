#pragma once

#include "util.h"


// TODO: create better "generation" handles?


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


namespace std
{
template<>
struct hash< handle_test_base_t< u16 > >
{
	size_t operator()( const handle_test_base_t< u16 >& handle ) const
	{
		return ( hash< u16 >()( handle.value ) );
	}
};

template<>
struct hash< handle_test_base_t< u32 > >
{
	size_t operator()( const handle_test_base_t< u32 >& handle ) const
	{
		return ( hash< u32 >()( handle.value ) );
	}
};

template<>
struct hash< handle_test_base_t< u64 > >
{
	size_t operator()( const handle_test_base_t< u64 >& handle ) const
	{
		return ( hash< u64 >()( handle.value ) );
	}
};
}


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

	// #define CH_HANDLE_32( type ) using type = handle_test_base_t< u32 >;
	#define CH_HANDLE_32( type ) using type = u32;

//		namespace std                                             \
//		{                                                         \
//		template<>                                                \
//		struct hash< type >                                       \
//		{                                                         \
//			size_t operator()( type const& handle ) const         \
//			{                                                     \
//				size_t value = ( hash< u32 >()( handle.value ) ); \
//				return value;                                     \
//			}                                                     \
//		}; \
//		}

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


template< typename handle_type, typename data_type, u32 step_size = 4 >
struct ch_handle_array_32_t
{
	u32          arr_count;     // amount of elements in the two arrays
	u32          arr_capacity;  // amount of elements allocated for the two arrays

	data_type*   arr_data;

	// the index of this maps to data array
	handle_type* arr_handles;

	ch_handle_array_32_t() = default;

	ch_handle_array_32_t( u32 count, data_type* data, handle_type* handles ) :
		arr_count( count ),
		arr_data( data ),
		arr_handles( handles )
	{
	}

	~ch_handle_array_32_t()
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
		u32        magic    = ( rand() % 0xFFFFFFFE ) + 1;

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
		for ( u32 i = 0; i < arr_count; i++ )
		{
			if ( arr_handles[ i ] == handle )
			{
				// set memory to zero (is this needed if i might override it from shifting everything back?)
				memset( arr_data[ i ], 0, sizeof( data_type ) );

				// remove the handle
				memset( arr_handles[ i ], 0, sizeof( handle_type ) );

				// shift the data and handle arrays
				for ( u32 j = i; j < arr_count - 1; j++ )
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
		for ( u32 i = 0; i < arr_count; i++ )
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
//		for ( u32 i = 0; i < count; i++ )
//		{
//			// is this a free handle slot?
//			if ( arr_handles[ i ] == 0 )
//			{
//				// shift all the data and handle arrays back one
//				for ( u32 j = i; j < count - 1; j++ )
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


// template< typename data_type, typename handle_type >
// struct ch_handle_array_32_t
// {
// 	u32          count;
// 	data_type*   data;
// 	handle_type* handles;
// };


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


inline u32 util_create_handle_u32()
{
	return ( rand() % 0xFFFFFFFE ) + 1;
}


// ===========================================================================


// we don't use a general u64, since this can be aligned by 4 bytes, and doesn't result in handle mismatches
#define CH_HANDLE_GEN_32( name ) \
	struct name                  \
	{                            \
		u32 index;               \
		u32 generation;          \
	}


// 32-bit handle list with generation support and ref counts
//template< typename HANDLE, typename TYPE, u32 STEP_SIZE = 32 >
//struct ch_handle_list_32
//{
//	//u32   count;
//	u32   capacity;
//	u32*  generation;  // NOTE: maybe we could just store generation here instead of the index as well, it becomes redundant
//	bool* use_list;    // true if handle is in use
//	TYPE* data;
//};


// 32-bit handle list with generation support and ref counts
// TODO: should i have another layer of indirection with an index list so we can defragment the memory and reduce how much memory this takes up?
template< typename HANDLE, typename TYPE, u32 STEP_SIZE = 32 >
struct ch_handle_ref_list_32
{
	//u32   count;
	u32   capacity;
	TYPE* data;
	u32*  generation;
	u16*  ref_count;

	ch_handle_ref_list_32()
	{
	}

	~ch_handle_ref_list_32()
	{
		free( data );
		free( generation );
		free( ref_count );
	}

private:
	bool allocate()
	{
	// 	u32*  new_generation = util_array_extend( generation, capacity, STEP_SIZE );
	// 	TYPE* new_data       = util_array_extend( data, capacity, STEP_SIZE );
	// 	u16*  new_ref        = util_array_extend( ref_count, capacity, STEP_SIZE );
	// 
	// 	if ( !new_generation || !new_data || !new_ref )
	// 	{
	// 		free( new_generation );
	// 		free( new_data );
	// 		free( new_ref );
	// 		return false;
	// 	}
	// 	
	// 	data       = new_data;
	// 	generation = new_generation;
	// 	ref_count  = new_ref;

		if ( util_array_extend( generation, capacity, STEP_SIZE ) )
		{
			::free( generation );
			return false;
		}

		if ( util_array_extend( data, capacity, STEP_SIZE ) )
		{
			::free( data );
			return false;
		}

		if ( util_array_extend( ref_count, capacity, STEP_SIZE ) )
		{
			::free( ref_count );
			return false;
		}

		capacity += STEP_SIZE;

		return true;
	}

	void free_slot( u32 index )
	{
		memset( &data[ index ], 0, sizeof( TYPE ) );
		ref_count[ index ] = 0;
	}

public:
	bool handle_valid( HANDLE s_handle )
	{
		if ( s_handle.index >= capacity )
			return false;

		if ( s_handle.generation == 0 )
			return false;

		if ( s_handle.generation != generation[ s_handle.index ] )
			return false;

		return true;
	}

	bool create( HANDLE& s_handle, TYPE** s_type )
	{
		// Find a free handle
		u32 index = 0;
		for ( ; index < capacity; index++ )
		{
			// ref count must be 0 to be available
			if ( ref_count[ index ] == 0 )
				break;
		}

		if ( index == capacity )
		{
			if ( !allocate() )
			{
				return false;
			}
		}

		ref_count[ index ]++;

		s_handle.index      = index;
		s_handle.generation = ++generation[ index ];
		*s_type             = &data[ index ];

		return true;
	}

	// use an existing handle, potentially useful for loading saves
	// though the generation index would be annoying
	// create_with_handle

	TYPE* get( HANDLE s_handle )
	{
		if ( !handle_valid( s_handle ) )
			return nullptr;

		return &data[ s_handle.index ];
	}

	u16 ref_increment( HANDLE s_handle )
	{
		if ( !handle_valid( s_handle ) )
			return UINT16_MAX;

		return ++ref_count[ s_handle.index ];
	}

	// use this to free the handle
	u16 ref_decrement( HANDLE s_handle )
	{
		if ( !handle_valid( s_handle ) )
			return UINT16_MAX;

		CH_ASSERT( ref_count[ s_handle.index ] == 0 );

		ref_count[ s_handle.index ]--;

		if ( ref_count[ s_handle.index ] == 0 )
		{
			free_slot( s_handle );
			return 0;
		}

		return ref_count[ s_handle.index ];
	}

	u16 ref_get( HANDLE s_handle )
	{
		if ( !handle_valid( s_handle ) )
			return UINT16_MAX;

		return ref_count[ s_handle.index ];
	}
};


// 32-bit handle list with generation support and ref counts
// TODO: should i have another layer of indirection with an index list so we can defragment the memory and reduce how much memory this takes up?
template< typename HANDLE, typename TYPE, u32 STEP_SIZE = 32 >
struct ch_handle_list_32
{
	//u32   count;
	u32   capacity;
	TYPE* data;
	u32*  generation;
	bool* use_list;    // list of entries that are in use

	ch_handle_list_32()
	{
	}

	~ch_handle_list_32()
	{
		::free( data );
		::free( generation );
		::free( use_list );
	}

private:
	bool allocate()
	{
		// u32*  new_generation = ;
		// TYPE* new_data       = util_array_extend( data, capacity, STEP_SIZE );
		// bool* new_use        = util_array_extend( use_list, capacity, STEP_SIZE );

		if ( util_array_extend( generation, capacity, STEP_SIZE ) )
		{
			::free( generation );
			return false;
		}

		if ( util_array_extend( data, capacity, STEP_SIZE ) )
		{
			::free( data );
			return false;
		}

		if ( util_array_extend( use_list, capacity, STEP_SIZE ) )
		{
			::free( use_list );
			return false;
		}

		capacity += STEP_SIZE;

	//	if ( !new_generation || !new_data || !new_use )
	//	{
	//		free( new_generation );
	//		free( new_data );
	//		free( new_use );
	//		return false;
	//	}
	//
	//	data       = new_data;
	//	generation = new_generation;
	//	use_list   = new_use;

		return true;
	}

public:
	bool handle_valid( HANDLE s_handle )
	{
		if ( s_handle.index >= capacity )
			return false;

		if ( s_handle.generation == 0 )
			return false;

		if ( s_handle.generation != generation[ s_handle.index ] )
			return false;

		return true;
	}

	bool create( HANDLE& s_handle, TYPE** s_type )
	{
		// Find a free handle
		u32 index = 0;
		for ( ; index < capacity; index++ )
		{
			// is this handle in use?
			if ( !use_list[ index ] )
				break;
		}

		if ( index == capacity )
		{
			if ( !allocate() )
			{
				return false;
			}
		}

		use_list[ index ] = true;

		s_handle.index      = index;
		s_handle.generation = ++generation[ index ];
		*s_type             = &data[ index ];

		return true;
	}

	void free( u32 index )
	{
		memset( &data[ index ], 0, sizeof( TYPE ) );
		use_list[ index ] = false;
	}

	void free( HANDLE& s_handle )
	{
		if ( !handle_valid( s_handle ) )
			return;

		memset( &data[ s_handle.index ], 0, sizeof( TYPE ) );
		use_list[ s_handle.index ] = false;
	}

	// use an existing handle, potentially useful for loading saves
	// though the generation index would be annoying
	// create_with_handle

	TYPE* get( HANDLE s_handle )
	{
		if ( !handle_valid( s_handle ) )
			return nullptr;

		return &data[ s_handle.index ];
	}
};


// TODO: FINISH THIS
#if 0
// 32 bit handle list with index list for indirection
template< typename HANDLE, typename TYPE, u32 STEP_SIZE = 32 >
struct ch_handle_list_idx_32
{
	struct index_generation_t
	{
		u32 index;
		u32 generation;
	};

	u32   count;  // count in the data array
	u32                 capacity;
	TYPE*               data;
	index_generation_t* indices;  // stores index into the data array
	bool*               use_list;  // list of entries that are in use

	ch_handle_list_32()
	{
	}

	~ch_handle_list_32()
	{
		::free( data );
		::free( generation );
		::free( use_list );
	}

private:
	bool allocate()
	{
		u32*  new_generation = util_array_extend( generation, capacity, STEP_SIZE );
		TYPE* new_data       = util_array_extend( data, capacity, STEP_SIZE );
		bool* new_use        = util_array_extend( use_list, capacity, STEP_SIZE );

		if ( !new_generation || !new_data || !new_use )
		{
			free( new_generation );
			free( new_data );
			free( new_use );
			return false;
		}

		data       = new_data;
		generation = new_generation;
		use_list   = new_use;

		return true;
	}

public:
	bool handle_valid( HANDLE s_handle )
	{
		if ( s_handle.index >= capacity )
			return false;

		if ( s_handle.generation == 0 )
			return false;

		if ( s_handle.generation != generation[ s_handle.index ] )
			return false;

		return true;
	}

	bool create( HANDLE& s_handle, TYPE** s_type )
	{
		// Find a free handle
		u32 index = 0;
		for ( ; index < capacity; index++ )
		{
			// is this handle in use?
			if ( !use_list[ index ] )
				break;
		}

		if ( index == capacity )
		{
			if ( !allocate() )
			{
				return false;
			}
		}

		use_list[ index ] = true;

		s_handle.index      = index;
		s_handle.generation = ++generation[ index ];
		*s_type             = &data[ index ];

		return true;
	}

	void free( u32 index )
	{
		memset( &data[ index ], 0, sizeof( TYPE ) );
		use_list[ index ] = false;
	}

	// use an existing handle, potentially useful for loading saves
	// though the generation index would be annoying
	// create_with_handle

	TYPE* get( HANDLE s_handle )
	{
		if ( !handle_valid( s_handle ) )
			return nullptr;

		return data[ s_handle.index ];
	}
};
#endif


// 32-bit handle list with generation support
// TODO: should i have another layer of indirection with an index list so we can defragment the memory and reduce how much memory this takes up?
template< typename HANDLE, u32 STEP_SIZE = 32 >
struct ch_handle_list_simple_32
{
	//u32   count;
	u32   capacity;
	u32*  generation;
	bool* use_list;    // list of entries that are in use

	ch_handle_list_simple_32()
	{
	}

	~ch_handle_list_simple_32()
	{
		free( generation );
		free( use_list );
	}

private:
	bool allocate()
	{
		u32*  new_generation = util_array_extend( generation, capacity, STEP_SIZE );
		bool* new_use        = util_array_extend( use_list, capacity, STEP_SIZE );

		if ( !new_generation || !new_use )
		{
			free( new_generation );
			free( new_use );
			return false;
		}

		generation = new_generation;
		use_list   = new_use;

		return true;
	}

public:
	bool handle_valid( HANDLE s_handle )
	{
		if ( s_handle.index >= capacity )
			return false;

		if ( s_handle.generation == 0 )
			return false;

		if ( s_handle.generation != generation[ s_handle.index ] )
			return false;

		return true;
	}

	HANDLE create()
	{
		// Find a free handle
		u32 index = 0;
		for ( ; index < capacity; index++ )
		{
			// is this handle in use?
			if ( !use_list[ index ] )
				break;
		}

		if ( index == capacity )
		{
			if ( !allocate() )
			{
				return {};
			}
		}

		use_list[ index ] = true;

		HANDLE handle;
		handle.index      = index;
		handle.generation = ++generation[ index ];

		return handle;
	}

	void free( u32 index )
	{
		use_list[ index ] = false;
	}
};


// TEST - having the above be utility functions, since in some areas, we want a separate array for names/paths


// validate a handle
template< typename HANDLE >
bool handle_list_valid( u32 capacity, u32* generation, HANDLE handle )
{
	if ( handle.index >= capacity )
		return false;

	if ( handle.generation == 0 )
		return false;

	if ( handle.generation != generation[ handle.index ] )
		return false;

	return true;
}

// validate a handle with ref count checking
//template< typename HANDLE >
//bool handle_list_valid_ref( u32 capacity, u32* generation, u32* ref_count, HANDLE handle )
//{
//}


template< typename HANDLE >
bool handle_list_ref_increment( u32 capacity, u32* generation, u16* ref_count, HANDLE handle )
{
	//if ( !handle_list_valid( capacity, generation, handle ) )
	//	return false;
	//
	//if ( handle.index >= capacity )
	//	return false;
	//
	//if ( handle.generation == 0 )
	//	return false;
	//
	//if ( handle.generation != generation[ s_handle.index ] )
	//	return false;
	//
	//return true;

	return false;
}

