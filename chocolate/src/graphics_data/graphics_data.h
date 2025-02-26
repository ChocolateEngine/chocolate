#pragma once

#include "igraphics_data.h"

LOG_CHANNEL( GraphicsData );

// Path to Handle
extern std::unordered_map< ch_string, ch_model_h >        g_model_paths;
extern std::unordered_map< ch_string, ch_material_h >     g_material_paths;

extern std::unordered_map< ch_string, ch_material_h >     g_material_names;

extern ch_handle_ref_list_32< ch_material_h, material_t > g_materials;


// size_t                                                    material_get_var_count( ch_material_h material );
// const char*                                               material_get_idx_name( ch_material_h material, size_t index );
// const char*                                               material_get_idx_string( ch_material_h material, size_t index );
// e_mat_var                                                 material_get_idx_type( ch_material_h material, size_t index );

bool                                                      model_load_obj( const char* s_base_path, const char* s_full_path, model_t& model );


class GraphicsData final : public IGraphicsData
{
   public:
	// --------------------------------------------------------------------------------------------
	// General
	// --------------------------------------------------------------------------------------------

	// Creates a new frustum
	frustum_t     frustum_create( const glm::mat4& proj_view ) override;

	// --------------------------------------------------------------------------------------------
	// Materials
	// --------------------------------------------------------------------------------------------

	// cleans up the material name/path
	ch_string     material_parse_name( const char* name, size_t len ) override;

	ch_material_h material_create( const char* name, const char* shader_name ) override;
	ch_material_h material_load( const char* path ) override;
	void          material_free( ch_material_h handle ) override;

	ch_material_h material_find( const char* name, size_t len = 0, bool hide_warning = false ) override;
	ch_material_h material_find_from_path( const char* path, size_t len = 0, bool hide_warning = false ) override;

	material_t*   material_get_data( ch_material_h handle ) override;
	u32           material_get_count() override;
	u32           material_get_capacity() override;
	ch_string     material_get_name( ch_material_h handle ) override;
	ch_string     material_get_path( ch_material_h handle ) override;

	// to decrement the material ref counter, call material_free
	void          material_ref_increment( ch_material_h handle ) override;

	size_t        material_get_dirty_count() override;
	ch_material_h material_get_dirty( size_t index ) override;
	void          material_mark_dirty( ch_material_h handle ) override;

//	ch_string*    material_get_var_names( u16 var_name_index ) override;
//	u16           material_get_var_names_count() override;
//	ch_string     material_get_var_name( u16 var_name_index ) override;
//	u16           material_get_var_name_index( const char* name, size_t len ) override;

	bool          material_set_string( ch_material_h handle, const char* var_name, const char* value ) override;
	bool          material_set_float( ch_material_h handle, const char* var_name, float value ) override;

	ch_string     material_get_string( ch_material_h handle, const char* var_name, const char* fallback ) override;
	float         material_get_float( ch_material_h handle, const char* var_name, float fallback ) override;

	// --------------------------------------------------------------------------------------------
	// Models - TODO: Have models load async, or on a job
	// --------------------------------------------------------------------------------------------

	ch_model_h    model_load( const char* path ) override;
	ch_model_h*   model_load( const char** paths, size_t count = 1 ) override;

	void          model_free( ch_model_h handle ) override;

	model_t*      model_get( ch_model_h handle ) override;
	ch_string     model_get_path( ch_model_h handle ) override;

	// const ch_model_materials* model_get_material_array( ch_model_h handle ) override;

	// const u16            model_get_material_count( ch_model_h handle )      override;
	// const ch_material_h* model_get_material_array( ch_model_h handle )      override;

	// --------------------------------------------------------------------------------------------
	// Memory Calculating - Returns memory usage information
	// --------------------------------------------------------------------------------------------

#if 0

	ch_mem_info_material_t memory_get_material() override;
	ch_mem_info_model_t    memory_get_model()    override;

#endif

	// --------------------------------------------------------------------------------------------
	// Mesh Building
	//
	// useful for importing custom models, like .obj or .gltf files
	// or for making models in code, like a skybox cube
	// --------------------------------------------------------------------------------------------
};


extern GraphicsData graphics_data;

