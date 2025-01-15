#pragma once

#include "igraphics_data.h"


ch_material_h  material_load( const char* path );
ch_material_h* material_load_multi( const char** paths, u16 path_count );

ch_material_h  material_create();

bool           model_load_obj( const char* s_base_path, const char* s_full_path, model_t& model );

