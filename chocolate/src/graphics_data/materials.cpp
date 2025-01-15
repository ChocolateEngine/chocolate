#include "graphics_data.h"


// allocate a block of memory for var data


struct mat_var_ptr_t
{
	e_mat_var type;
	u16       offset;
};


// what if you just don't use a struct, and "imagine" a struct to avoid padding?
struct mat_var_ptr2_t
{
	u8 count;   // amount of variables
	u16 offset; // offset to the first variable/type in the array
};


// array mat var data
// array for mat var types for each type? no, just have one array for all types, faster to iterate through

// static std::unordered_map< ch_material_h, mat_var_ptr_t > g_mat_vars;


ch_material_h material_load( const char* path )
{
	return 5; // CH_INVALID_HANDLE;
}


ch_material_h material_create()
{
	return 5; // CH_INVALID_HANDLE;
}


// material_get_var_array();