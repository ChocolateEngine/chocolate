#pragma once

#include "util.h"


// we don't use a general u64, since this can be aligned by 4 bytes, and doesn't result in handle mismatches
#define CH_HANDLE_GEN_32( name )                              \
	struct name                                               \
	{                                                         \
		u32  index;                                           \
		u32  generation;                                      \
                                                              \
		bool operator!()                                      \
		{                                                     \
			return generation == 0;                           \
		}                                                     \
	};                                                        \
                                                              \
	namespace std                                             \
	{                                                         \
	template<>                                                \
	struct hash< name >                                       \
	{                                                         \
		size_t operator()( name const& handle ) const         \
		{                                                     \
			size_t value = ( hash< u32 >()( handle.index ) ); \
			value ^= ( hash< u32 >()( handle.generation ) );  \
			return value;                                     \
		}                                                     \
	};                                                        \
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

	// same as ref_decrement, but doesn't memset 0 the memory yet
	// use ref_decrement_finish after you free data in it
	u16 ref_decrement_delay( HANDLE s_handle )
	{
		if ( !handle_valid( s_handle ) )
			return UINT16_MAX;

		CH_ASSERT( ref_count[ s_handle.index ] != 0 );

		return --ref_count[ s_handle.index ];
	}

	// free it if the ref count is 0, useful if you need to free other stuff in it
	void ref_decrement_finish( HANDLE s_handle )
	{
		if ( !handle_valid( s_handle ) )
			return;

		if ( ref_count[ s_handle.index ] == 0 )
			free_slot( s_handle.index );
	}

	u16 ref_decrement( HANDLE s_handle )
	{
		if ( !handle_valid( s_handle ) )
			return UINT16_MAX;

		CH_ASSERT( ref_count[ s_handle.index ] != 0 );

		ref_count[ s_handle.index ]--;

		if ( ref_count[ s_handle.index ] == 0 )
		{
			free_slot( s_handle.index );
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


template< typename HANDLE, u32 STEP_SIZE = 32 >
struct ch_handle_list_ref_simple_32
{
	//u32   count;
	u32   capacity;
	u32*  generation;
	u16*  ref_count;

	ch_handle_list_ref_simple_32()
	{
	}

	~ch_handle_list_ref_simple_32()
	{
		free( generation );
		free( ref_count );
	}

   private:
	bool allocate()
	{
		if ( util_array_extend( generation, capacity, STEP_SIZE ) )
		{
			::free( generation );
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

		HANDLE handle;
		handle.index      = index;
		handle.generation = ++generation[ index ];

		return handle;
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

		CH_ASSERT( ref_count[ s_handle.index ] != 0 );

		ref_count[ s_handle.index ]--;

		if ( ref_count[ s_handle.index ] == 0 )
			return 0;

		return ref_count[ s_handle.index ];
	}

	u16 ref_get( HANDLE s_handle )
	{
		if ( !handle_valid( s_handle ) )
			return UINT16_MAX;

		return ref_count[ s_handle.index ];
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

