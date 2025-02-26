#pragma once

//
// Handle's all Graphics Data, such as Textures, Materials, and Models
// Does not upload anything to the GPU, that is the job of the Renderer
// This is just a data manager, useful for servers
//


#include "core/core.h"

#include "core/handles.hpp"


// ------------------------------------------------------------------------------------


enum class GraphicsFmt
{
	INVALID,

	// -------------------------
	// 8 bit depth

	R8_SRGB,
	R8_SINT,
	R8_UINT,

	RG88_SRGB,
	RG88_SINT,
	RG88_UINT,

	RGB888_SRGB,
	RGB888_SINT,
	RGB888_UINT,

	RGBA8888_SRGB,
	RGBA8888_SNORM,
	RGBA8888_UNORM,

	// -------------------------

	BGRA8888_SRGB,
	BGRA8888_SNORM,
	BGRA8888_UNORM,

	// -------------------------
	// 16 bit depth

	R16_SFLOAT,
	R16_SINT,
	R16_UINT,

	RG1616_SFLOAT,
	RG1616_SINT,
	RG1616_UINT,

	RGB161616_SFLOAT,
	RGB161616_SINT,
	RGB161616_UINT,

	RGBA16161616_SFLOAT,
	RGBA16161616_SINT,
	RGBA16161616_UINT,

	// -------------------------
	// 32 bit depth

	R32_SFLOAT,
	R32_SINT,
	R32_UINT,

	RG3232_SFLOAT,
	RG3232_SINT,
	RG3232_UINT,

	RGB323232_SFLOAT,
	RGB323232_SINT,
	RGB323232_UINT,

	RGBA32323232_SFLOAT,
	RGBA32323232_SINT,
	RGBA32323232_UINT,

	// -------------------------
	// GPU Compressed Formats

	BC1_RGB_UNORM_BLOCK,
	BC1_RGB_SRGB_BLOCK,
	BC1_RGBA_UNORM_BLOCK,
	BC1_RGBA_SRGB_BLOCK,

	BC2_UNORM_BLOCK,
	BC2_SRGB_BLOCK,

	BC3_UNORM_BLOCK,
	BC3_SRGB_BLOCK,

	BC4_UNORM_BLOCK,
	BC4_SNORM_BLOCK,

	BC5_UNORM_BLOCK,
	BC5_SNORM_BLOCK,

	BC6H_UFLOAT_BLOCK,
	BC6H_SFLOAT_BLOCK,

	BC7_UNORM_BLOCK,
	BC7_SRGB_BLOCK,

	// -------------------------
	// Other Formats

	D16_UNORM,
	D32_SFLOAT,
	D32_SFLOAT_S8_UINT,
};


constexpr glm::vec4 gFrustumFaceData[ 8u ] = {
	// Near Face
	{ 1, 1, -1, 1.f },
	{ -1, 1, -1, 1.f },
	{ 1, -1, -1, 1.f },
	{ -1, -1, -1, 1.f },

	// Far Face
	{ 1, 1, 1, 1.f },
	{ -1, 1, 1, 1.f },
	{ 1, -1, 1, 1.f },
	{ -1, -1, 1, 1.f },
};


enum e_frustum
{
	e_frustum_top,
	e_frustum_bottom,
	e_frustum_right,
	e_frustum_left,
	e_frustum_near,
	e_frustum_far,
	e_frustum_count,
};


using e_mat_var = u8;
enum : e_mat_var
{
	e_mat_var_invalid,
	e_mat_var_string,
	e_mat_var_float,
	e_mat_var_int,
	e_mat_var_vec2,
	e_mat_var_vec3,
	e_mat_var_vec4,
	e_mat_var_count,
};


inline const char* g_mat_var_str[] = {
	"Invalid",
	"String",
	"Float",
	"Int",
	"Vec2",
	"Vec3",
	"Vec4",
};


static_assert( ARR_SIZE( g_mat_var_str ) == e_mat_var_count );


enum e_vertex_attribute : u8
{
	e_vertex_attribute_position,    // vec3
	e_vertex_attribute_normal,      // vec3
	e_vertex_attribute_tex_coord,   // vec2
	e_vertex_attribute_color,       // vec4
	e_vertex_attribute_tangent,     // vec3
	e_vertex_attribute_bi_tangent,  // vec3

	// e_vertex_attribute_bone_index,        // uvec4
	// e_vertex_attribute_bone_weight,       // vec4

	e_vertex_attribute_count
};


using f_vertex_format = u16;
enum : f_vertex_format
{
	f_vertex_format_none       = 0,
	f_vertex_format_position   = ( 1 << e_vertex_attribute_position ),
	f_vertex_format_normal     = ( 1 << e_vertex_attribute_normal ),
	f_vertex_format_tex_coord  = ( 1 << e_vertex_attribute_tex_coord ),
	f_vertex_format_color      = ( 1 << e_vertex_attribute_color ),
	f_vertex_format_tangent    = ( 1 << e_vertex_attribute_tangent ),
	f_vertex_format_bi_tangent = ( 1 << e_vertex_attribute_bi_tangent ),

	f_vertex_format_all        = ( 1 << e_vertex_attribute_count ) - 1,
};


// ------------------------------------------------------------------------------------


//struct ch_model_h
//{
//	u32 index;
//	u32 generation;
//};


CH_HANDLE_GEN_32( ch_model_h );
CH_HANDLE_GEN_32( ch_material_h );


// ------------------------------------------------------------------------------------


struct material_create_info_t
{
};


struct frustum_t
{
	glm::vec4 planes[ e_frustum_count ];
	glm::vec3 points[ 8 ];  // 4 Positions for the near plane corners, last 4 are the far plane corner positions

	// https://iquilezles.org/articles/frustumcorrect/
	bool      is_box_visible( const glm::vec3& sMin, const glm::vec3& sMax ) const
	{
		PROF_SCOPE();

		// Check Box Outside/Inside of Frustum
		for ( int i = 0; i < e_frustum_count; i++ )
		{
			if ( ( glm::dot( planes[ i ], glm::vec4( sMin.x, sMin.y, sMin.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMax.x, sMin.y, sMin.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMin.x, sMax.y, sMin.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMax.x, sMax.y, sMin.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMin.x, sMin.y, sMax.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMax.x, sMin.y, sMax.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMin.x, sMax.y, sMax.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMax.x, sMax.y, sMax.z, 1.0f ) ) < 0.0 ) )
			{
				return false;
			}
		}

		// Check Frustum Outside/Inside Box
		for ( int j = 0; j < 3; j++ )
		{
			int out = 0;
			for ( int i = 0; i < 8; i++ )
			{
				if ( points[ i ][ j ] > sMax[ j ] )
					out++;
			}

			if ( out == 8 )
				return false;

			out = 0;
			for ( int i = 0; i < 8; i++ )
			{
				if ( points[ i ][ j ] < sMin[ j ] )
					out++;
			}

			if ( out == 8 )
				return false;
		}

		return true;
	}

	void get_aabb( glm::vec3& srMin, glm::vec3& srMax ) const
	{
		srMin = points[ 0 ];
		srMax = points[ 0 ];

		for ( int i = 1; i < 8; i++ )
		{
			srMin = glm::min( srMin, points[ i ] );
			srMax = glm::max( srMax, points[ i ] );
		}
	}

	bool is_visible( const frustum_t& other )
	{
		// Just do a aabb test
		glm::vec3 min;
		glm::vec3 max;

		other.get_aabb( min, max );

		return is_box_visible( min, max );
	}
};


struct ch_model_materials_old
{
	u32         vertex_offset;
	u32         vertex_count;

	u32         index_offset;
	u32         index_count;

	ch_handle_t mat;
};


struct ch_model_materials
{
	ch_material_h* mat;
	u32            mat_count;  // could this be u16? is there ever a need for more than 65535 materials on a model?
};


struct mesh_vertex_t
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv;
	glm::vec4 color;
};


//
// physics mesh ideas
// 
// Data we need to create simple physics meshes
// - Vertex Positions
// - Physics Materials
// - An array of Sub Shapes
// 
// Standard mesh loading, like from an obj, usually join all the meshs into a single mesh
// For physics objects, we need to keep those separate, and have them loaded as a Static Compound
// 
// We need physics material data, not shader materials, should make that distinction probably
// 
// 


// In jolt, physics materials seem to only have a name and a debug color
// Maybe there's something we can do where if we contact some point on a physics object,
// we can see the triangle it hit, get the material from it, and map that to a game physics material,
// and then 
struct test_physics_material_t
{
	glm::vec3 debug_color;
};


struct physics_model_sub_shape_t
{
	glm::vec3* pos;
	u32*       index;

	u32        vertex_count;
	u32        index_count;

	// supposedly in jolt physics, individual triangles can have their own material
};


struct physics_model_data_t
{
	u32                        shape_count;
	physics_model_sub_shape_t* shape;
};


// Basic Mesh Structure
// 
// Maybe don't keep this in memory after uploading the mesh to the gpu and potentially creating a physics object from it?
// For physics objects, what do we need to keep from this after creation?
// We may need it for server side work, but we shouldn't need to keep all the vertex data loaded in memory for that
// 
// All I think we need for server side model work:
// - Pre-Computed AABB
// - AABB offsets for all blend shapes
// - maybe max AABB offsets if animations applied to it? hmm
//   instead for testing vis with animation playback, we could have a bounding box for each bone? but then what about adding blend shapes on top of that?
// 
// Though in the end, what's to say we can't just have a server mesh struct for storing what the server actually needs?
// And as for hotloading, we could just store the path somewhere, check if it's modified or something, then reload the mesh in the background,
// and tell all systems using that model that it's been modified, then once each system signals it's finished with the model, then it's freed
// 
// Maybe instead of having a ref count for loading models, we could just have a sort of "mutex" for models
// like `bool usable = graphics_data->model_mark_use( ch_model_h handle );`,
// and then when that system is done, it can call `graphics_data->model_mark_finished( ch_model_h handle );`
// 
// in the main app code or something where this is loading, it could look something like this
// also maybe this code itself should happen in a job maybe, like map loading or creating an entity?
// mainly so it never blocks the main thread, i would like to try and avoid that ideally, not very fun to have that
// 
// ch_model_h model_handle = graphics_data->model_load( "my/cool/model.obj" );
// 
// // Tell systems to load this model (happens async or in a job)
// phys_shape = physics->load_shape( model_handle );
// gpu_mesh   = render->mesh_upload( model_handle );
// 
// // do this in a different job once mesh upload is complete
// renderable = render->renderable_create( gpu_mesh );
// 
// // Now allow the graphics data dll to free this data once the systems above are done with it
// // This marks the model freed internally, and each update, waits for the model to be finished 
// graphics_data->model_free( model_handle );
// 


//
// Model Loading idea
// 
// 
// 



// while not very efficient with pointers, it is a lot easier to use, and it's not like it's used for very long
// also should have better access times than having to search for which data array is positions or something
struct mesh_vertex_data_t
{
	glm::vec3* pos;
	glm::vec3* normal;
	glm::vec2* tex_coord;
	glm::vec4* color;
	glm::vec3* tangent;
	glm::vec3* bi_tangent;
};


struct mesh_surface_t
{
	u32           vertex_offset;
	u32           vertex_count;

	u32           index_offset;
	u32           index_count;

	ch_material_h material;  // base material to copy from
};


// Basic Mesh Structure
// Designed to be used temporarily, and then deleted after it's used in the systems it's needed for
struct mesh_t
{
	// NOTE: could store these pointers as one allocation probably
	// mesh_vertex_t*  vertex;

//	e_vertex_attribute* vertex_attrib;

	// array of data for vertex attributes, each point to an array of vertices
	// the type depending on the type in vertex_attrib
	// so like vertex_data[ 0 ] would be what vertex_attrib[ 0 ] is, could be glm::vec3* positions
//	void**              vertex_data;

	mesh_vertex_data_t  vertex_data;
	u32*                index;
	mesh_surface_t*     surface;

	u32                 vertex_count;
	u32                 index_count;
	u32                 surface_count;
	//f_vertex_format     vertex_format;
	//u8                  vertex_attrib_count;
};


struct mesh_morph_data_t
{

};


struct model_t
{
	mesh_t*            mesh;
	mesh_morph_data_t* morph_data;

	u32                mesh_count;
	u32                morph_data_count;
};


// ------------------------------------------------------------------------------------
// Material Data


// this wastes memory, but easier to manage
struct material_var_t
{
	union
	{
		ch_string val_string;
		int       val_int;
		float     val_float;
		glm::vec2 val_vec2;
		glm::vec3 val_vec3;
		glm::vec4 val_vec4;
	};
};


struct material_t
{
	ch_string*      name;
	// u16*            name;
	material_var_t* var;
	e_mat_var*      type;
	size_t          count;
};


// ------------------------------------------------------------------------------------
// Model Builder Data


struct model_builder_t
{

};


// ------------------------------------------------------------------------------------


inline const char* get_mat_var_str( e_mat_var mat_var )
{
	if ( mat_var >= e_mat_var_count )
		return g_mat_var_str[ e_mat_var_invalid ];

	return g_mat_var_str[ mat_var ];
}


// ------------------------------------------------------------------------------------


class IGraphicsData : public ISystem
{
   public:
	// --------------------------------------------------------------------------------------------
	// General
	// --------------------------------------------------------------------------------------------

	// Creates a new frustum
	virtual frustum_t     frustum_create( const glm::mat4& proj_view )                                                      = 0;

	// --------------------------------------------------------------------------------------------
	// Materials
	// --------------------------------------------------------------------------------------------

	// cleans up the material name/path
	virtual ch_string     material_parse_name( const char* name, size_t len )                                               = 0;

	virtual ch_material_h material_create( const char* name, const char* shader_name )                                      = 0;
	virtual ch_material_h material_load( const char* path )                                                                 = 0;
	virtual void          material_free( ch_material_h handle )                                                             = 0;

	virtual ch_material_h material_find( const char* name, size_t len = 0, bool hide_warning = false )                      = 0;
	virtual ch_material_h material_find_from_path( const char* path, size_t len = 0, bool hide_warning = false )            = 0;

	// get allocated capacity of material handle list
	virtual u32           material_get_capacity()                                                                           = 0;

	virtual material_t*   material_get_data( ch_material_h handle )                                                         = 0;
	virtual u32           material_get_count()                                                                              = 0;
	virtual ch_string     material_get_name( ch_material_h handle )                                                         = 0;
	virtual ch_string     material_get_path( ch_material_h handle )                                                         = 0;
	// virtual ch_material_h material_get_at_index( u32 index )                                                                = 0;

	// to decrement the material ref counter, call material_free
	virtual void          material_ref_increment( ch_material_h handle )                                                    = 0;

	virtual size_t        material_get_dirty_count()                                                                        = 0;
	virtual ch_material_h material_get_dirty( size_t index )                                                                = 0;
	virtual void          material_mark_dirty( ch_material_h handle )                                                       = 0;

	virtual bool          material_set_string( ch_material_h handle, const char* var_name, const char* value )              = 0;
	virtual bool          material_set_float( ch_material_h handle, const char* var_name, float value )                     = 0;

	virtual ch_string     material_get_string( ch_material_h handle, const char* var_name, const char* fallback = nullptr ) = 0;
	virtual float         material_get_float( ch_material_h handle, const char* var_name, float fallback = 0.f )            = 0;

	// --------------------------------------------------------------------------------------------
	// Models - TODO: Have models load async, or on a job
	// --------------------------------------------------------------------------------------------

	virtual ch_model_h    model_load( const char* path )                                                                    = 0;
	virtual ch_model_h*   model_load( const char** paths, size_t count = 1 )                                                = 0;

	virtual void          model_free( ch_model_h handle )                                                                   = 0;

	virtual model_t*      model_get( ch_model_h handle )                                                                    = 0;
	virtual ch_string     model_get_path( ch_model_h handle )                                                               = 0;

	// virtual const ch_model_materials* model_get_material_array( ch_model_h handle ) = 0;

	// virtual const u16            model_get_material_count( ch_model_h handle )      = 0;
	// virtual const ch_material_h* model_get_material_array( ch_model_h handle )      = 0;

#if 0
	// odd idea for model loading to have it potentially non-blocking?

	queue_handle = graphics_data->model_load_queued( "path" );

	// once the graphics data model loading task is done, then these systems read that loaded data
	audio->model_create( queue_handle );
	render->model_create( queue_handle );

#endif

	// --------------------------------------------------------------------------------------------
	// Memory Calculating - Returns memory usage information
	// --------------------------------------------------------------------------------------------

	#if 0

	virtual ch_mem_info_material_t memory_get_material() = 0;
	virtual ch_mem_info_model_t    memory_get_model()    = 0;

	#endif

	// --------------------------------------------------------------------------------------------
	// Mesh Building
	//
	// useful for importing custom models, like .obj or .gltf files
	// or for making models in code, like a skybox cube
	// --------------------------------------------------------------------------------------------
};


#define CH_GRAPHICS_DATA     "ch_graphics_data"
#define CH_GRAPHICS_DATA_VER 3

