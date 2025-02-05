#include "graphics_data.h"


#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj/fast_obj.h"


static const char* gDefaultShader  = "basic_3d";

static const char* MatVar_Diffuse  = "diffuse";
static const char* MatVar_Emissive = "emissive";



static std::string GetBaseDir( const std::string& s_full_path )
{
	if ( s_full_path.find_last_of( "/\\" ) != std::string::npos )
		return s_full_path.substr( 0, s_full_path.find_last_of( "/\\" ) );

	return "";
}


// static ch_handle_t load_material( const std::string& path )
// {
// 	std::string matPath = path;
//
// 	if ( !matPath.ends_with( ".cmt" ) )
// 		matPath += ".cmt";
//
// 	if ( FileSys_IsFile( matPath.data(), matPath.size() ) )
// 		return gGraphics.LoadMaterial( matPath.data(), matPath.size() );
//
// 	std::string modelPath = "models/" + matPath;
// 	if ( FileSys_IsFile( modelPath.data(), modelPath.size() ) )
// 		return gGraphics.LoadMaterial( modelPath.data(), modelPath.size() );
//
// 	return CH_INVALID_HANDLE;
// }



// Vertex containing all possible values
struct mesh_build_vertex_t
{
	glm::vec3   pos{};
	glm::vec3   normal{};
	glm::vec2   tex_coord{};
	// glm::vec2 texCoord2;
	glm::vec3   color{};

	// bones and weights
	// std::vector<int> bones;
	// std::vector<float> weights;

	inline bool operator==( const mesh_build_vertex_t& other ) const
	{
		// Guard self assignment
		if ( this == &other )
			return true;

		return std::memcmp( &pos, &other.pos, sizeof( mesh_build_vertex_t ) ) == 0;
	}
};


// Hashing Support for this vertex struct
namespace std
{
template<>
struct hash< mesh_build_vertex_t >
{
	size_t operator()( mesh_build_vertex_t const& vertex ) const
	{
		size_t value = 0;

		// for some reason just doing hash< glm::vec3 > doesn't work anymore
		value ^= ( hash< glm::vec3::value_type >()( vertex.pos.x ) );
		value ^= ( hash< glm::vec3::value_type >()( vertex.pos.y ) );
		value ^= ( hash< glm::vec3::value_type >()( vertex.pos.z ) );

		value ^= ( hash< glm::vec3::value_type >()( vertex.color.x ) );
		value ^= ( hash< glm::vec3::value_type >()( vertex.color.y ) );
		value ^= ( hash< glm::vec3::value_type >()( vertex.color.z ) );

		value ^= ( hash< glm::vec3::value_type >()( vertex.normal.x ) );
		value ^= ( hash< glm::vec3::value_type >()( vertex.normal.y ) );
		value ^= ( hash< glm::vec3::value_type >()( vertex.normal.z ) );

		value ^= ( hash< glm::vec2::value_type >()( vertex.tex_coord.x ) );
		value ^= ( hash< glm::vec2::value_type >()( vertex.tex_coord.y ) );

		return value;
	}
};
}


struct mesh_build_surface_t
{
	ChVector< mesh_build_vertex_t >                vertices;
	ChVector< u32 >                                indices{};

	// stores the vertex again, and the value is the index to the vertex in the vertices variable
	std::unordered_map< mesh_build_vertex_t, u32 > vertices_map{};

	ch_material_h                                  material;
};


bool model_load_obj( const char* s_base_path, const char* s_full_path, model_t& model )
{
	PROF_SCOPE();

	fastObjMesh* obj = fast_obj_read( s_full_path );

	if ( obj == nullptr )
	{
		Log_ErrorF( "Failed to parse obj\n", s_full_path );
		return false;
	}

	// ch_string_auto base_dir_full = FileSys_GetDirName( s_full_path );
	ch_string_auto base_dir      = FileSys_GetDirName( s_base_path );

	// std::string baseDir   = GetBaseDir( s_full_path );
	// std::string baseDir2  = GetBaseDir( s_base_path );

	u32         mat_count = glm::max( 1U, obj->material_count );

	// static ch_handle_t defaultShader = gGraphics.GetShader( gDefaultShader.data() );

	// one material for now
	ch_material_h  my_cool_material = graphics_data.material_create( "test", "standard" );
	// ch_material_h  my_cool_material{};

#if 0
	for ( unsigned int i = 0; i < obj->material_count; i++ )
	{
		fastObjMaterial& objMat   = obj->materials[ i ];
		std::string      matName  = base_dir + "/" + objMat.name;
		ch_handle_t      material = gGraphics.FindMaterial( matName.c_str() );

		if ( material == CH_INVALID_HANDLE )
		{
			// kinda strange this setup

			// Search without "base_dir"
			material = LoadMaterial( objMat.name );

			if ( material == CH_INVALID_HANDLE )
			{
				material = LoadMaterial( matName );
			}
		}

		// fallback if there is no cmt file
		if ( material == CH_INVALID_HANDLE )
		{
			material = gGraphics.CreateMaterial( objMat.name, defaultShader );

			TextureCreateData_t createData{};
			createData.aUsage  = EImageUsage_Sampled;
			createData.aFilter = EImageFilter_Linear;

			auto SetTexture    = [ & ]( const std::string& param, u32 tex_index )
			{
				if ( tex_index > obj->texture_count )
					return;

				fastObjTexture& obj_texture = obj->textures[ tex_index ];

				if ( obj_texture.path == nullptr )
					return;

				ch_handle_t texture = CH_INVALID_HANDLE;

				// check if the name is an absolute path
				const char* texName = nullptr;

				//if ( FileSys_IsAbsolute( fastTexture.name ) )
				//{
				//	texName = obj_texture.name;
				//}
				//else
				//{
				texName             = obj_texture.path;
				//}

				if ( FileSys_IsRelative( texName ) )
					gGraphics.LoadTexture( texture, base_dir + "/" + texName, createData );
				else
					gGraphics.LoadTexture( texture, texName, createData );

				gGraphics.Mat_SetVar( material, param, texture );
			};

			SetTexture( MatVar_Diffuse, objMat.map_Kd );
			SetTexture( MatVar_Emissive, objMat.map_Ke );
		}

		meshBuilder.SetCurrentSurface( i );
		meshBuilder.SetMaterial( material );
	}

	if ( obj->material_count == 0 )
	{
		ch_handle_t material = gGraphics.CreateMaterial( s_full_path, gGraphics.GetShader( gDefaultShader.data() ) );
		meshBuilder.SetCurrentSurface( 0 );
		meshBuilder.SetMaterial( material );
	}
#endif

	// u64 vertexOffset = 0;
	// u64 indexOffset  = 0;
	//
	// u64 totalVerts = 0;
	u64 totalIndexOffset                 = 0;

	model.mesh                           = ch_calloc< mesh_t >( obj->object_count );
	model.mesh_count                     = obj->object_count;

	for ( u32 mesh_index = 0; mesh_index < obj->object_count; mesh_index++ )
	// for ( u32 mesh_index = 0; mesh_index < obj->group_count; mesh_index++ )
	{
		fastObjGroup& group = obj->objects[ mesh_index ];
		// fastObjGroup& group = obj->groups[mesh_index];

		// index_offset += group.index_offset;

		// mesh_build_surface_t* build_surfaces = ch_calloc< mesh_build_surface_t >( mat_count );
		mesh_build_surface_t* build_surfaces = new mesh_build_surface_t[ mat_count ];

		// we don't know how many materials this group has, so just allocate all of it i guess
		//mesh.surface_count  = mat_count;
		//mesh.surface        = ch_calloc< mesh_surface_t >( mat_count );
		//
		//mesh.vertex_data.pos = ch_calloc< glm::vec3 >( group.face_count * 3 );

		for ( u32 faceIndex = 0; faceIndex < group.face_count; faceIndex++ )
		// for ( u32 faceIndex = 0; faceIndex < obj->face_count; faceIndex++ )
		{
			u32& faceVertCount = obj->face_vertices[ group.face_offset + faceIndex ];
			u32  face_mat       = 0;

			if ( obj->material_count > 0 )
			{
				face_mat = obj->face_materials[ group.face_offset + faceIndex ];
			}

			mesh_build_surface_t& surface = build_surfaces[ face_mat ];
			surface.material              = my_cool_material;

			surface.indices.reserve( surface.indices.size() + ( group.face_count * ( faceVertCount == 3 ? 3 : 6 ) ) );
			//surface.vertices.reserve( surface.vertices.size() + ( group.face_count * ( faceVertCount == 3 ? 3 : 6 ) ) );

			// meshBuilder.AllocateVertices( faceVertCount == 3 ? 3 : 6 );

			for ( u32 faceVertIndex = 0; faceVertIndex < faceVertCount; faceVertIndex++ )
			{
				// NOTE: mesh->indices holds each face "fastObjIndex" as three
				// seperate index objects contiguously laid out one after the other
				fastObjIndex objVertIndex = obj->indices[ totalIndexOffset + faceVertIndex ];

				if ( faceVertIndex >= 3 )
				{
					auto ind0 = surface.indices[ surface.indices.size() - 3 ];
					auto ind1 = surface.indices[ surface.indices.size() - 1 ];

					surface.indices.push_back( ind0 );
					surface.indices.push_back( ind1 );
				}

				const u32 position_index = objVertIndex.p * 3;
				const u32 texcoord_index = objVertIndex.t * 2;
				const u32 normal_index   = objVertIndex.n * 3;

				mesh_build_vertex_t current_vert{};

				current_vert.pos    = { obj->positions[ position_index ],
					                    obj->positions[ position_index + 1 ],
					                    obj->positions[ position_index + 2 ] };

				current_vert.normal = { obj->normals[ normal_index ],
					                    obj->normals[ normal_index + 1 ],
					                    obj->normals[ normal_index + 2 ] };

				if ( obj->material_count > 0 )
				{
					current_vert.tex_coord = {
						obj->texcoords[ texcoord_index ],
						1.0f - obj->texcoords[ texcoord_index + 1 ],
					};
				}

				// Now add the vertex to the current surface
				auto vert_index = surface.vertices_map.find( current_vert );
					
				if ( vert_index == surface.vertices_map.end() )
				{
					surface.indices.push_back( surface.vertices.size() );
					surface.vertices_map[ current_vert ] = surface.vertices.size();
					surface.vertices.push_back( current_vert );
				}
				else
				{
					surface.indices.push_back( vert_index->second );
				}
			}

			totalIndexOffset += faceVertCount;
		}

		// now add this to the mesh
		mesh_t& mesh = model.mesh[ mesh_index ];

		// find the total amount of verts and indices
		u32     total_verts = 0;
		u32     total_ind   = 0;

		u32     total_mats  = 0;

		mesh.surface        = ch_calloc< mesh_surface_t >( mat_count );

		for ( u32 surf_i = 0, real_surf_i = 0; surf_i < mat_count; surf_i++ )
		{
			mesh_build_surface_t& surface = build_surfaces[ surf_i ];

			if ( surface.vertices.size() == 0 )
				continue;

			mesh.surface[ mesh.surface_count ].material      = surface.material;

			mesh.surface[ mesh.surface_count ].vertex_count  = surface.vertices.size();
			mesh.surface[ mesh.surface_count ].index_count   = surface.indices.size();

			mesh.surface[ mesh.surface_count ].vertex_offset = total_verts;
			mesh.surface[ mesh.surface_count ].index_offset  = total_ind;

			mesh.surface_count++;

			total_mats++;
			total_verts += surface.vertices.size();
			total_ind   += surface.indices.size();
		}

		mesh.vertex_data.pos       = ch_calloc< glm::vec3 >( total_verts );
		mesh.vertex_data.normal    = ch_calloc< glm::vec3 >( total_verts );
		mesh.vertex_data.tex_coord = ch_calloc< glm::vec2 >( total_verts );

		mesh.vertex_count          = total_verts;
		mesh.index_count           = total_ind;
		mesh.index                 = ch_malloc< u32 >( total_ind );

		u32 current_vert_offset    = 0;
		u32 current_ind_offset     = 0;
		u32 ind_value_offset       = 0;

		for ( u32 surf_i = 0; surf_i < mat_count; surf_i++ )
		{
			mesh_build_surface_t& surface = build_surfaces[ surf_i ];

			if ( build_surfaces[ surf_i ].vertices.size() == 0 )
				continue;

			for ( auto it = surface.vertices.begin(); it != surface.vertices.end(); ++it )
			{
				mesh.vertex_data.pos[ current_vert_offset ]       = it->pos;
				mesh.vertex_data.normal[ current_vert_offset ]    = it->normal;
				mesh.vertex_data.tex_coord[ current_vert_offset ] = it->tex_coord;
				current_vert_offset++;
			}

			for ( u32 ind_i = 0; ind_i < surface.indices.size(); ind_i++ )
			{
				// Have to offset the index values since were appending these to an existing array
				mesh.index[ current_ind_offset++ ] = surface.indices.apData[ ind_i ] + ind_value_offset;
			}

			ind_value_offset = current_vert_offset;
		}

		delete[] build_surfaces;
	}

	fast_obj_destroy( obj );

	return true;
}

