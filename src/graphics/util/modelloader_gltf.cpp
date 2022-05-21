/*
modelloader_gtlf.cpp ( Authored by Demez )

File dedicated for loading gltf models
*/

#include "modelloader.h"
#include "../materialsystem.h"
#include "../graphics.h"
#include "graphics/meshbuilder.hpp"

#include "util.h"
#include "core/console.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"


inline const char* Result2Str( cgltf_result result )
{
	switch ( result )
	{
		default:
			return "Unknown";

		case cgltf_result_success:
			return "Success";

		case cgltf_result_data_too_short:
			return "Data Too Short";

		case cgltf_result_unknown_format:
			return "Unknown Format";

		case cgltf_result_invalid_json:
			return "Invalid json";

		case cgltf_result_invalid_gltf:
			return "Invalid gltf";

		case cgltf_result_invalid_options:
			return "Invalid Options";

		case cgltf_result_file_not_found:
			return "File Not Found";

		case cgltf_result_io_error:
			return "I/O Error";

		case cgltf_result_out_of_memory:
			return "Out Of Memory";

		case cgltf_result_legacy_gltf:
			return "Legacy gltf";
	}
}


static std::string GetBaseDir( const std::string &srPath )
{
	if ( srPath.find_last_of( "/\\" ) != std::string::npos )
		return srPath.substr( 0, srPath.find_last_of( "/\\" ) );
	return "";
}


static std::string gDefaultShader       = "basic_3d";
// static std::string gDefaultShader       = "debug";

static std::string MatVar_Diffuse       = "diffuse";
static std::string MatVar_Emissive      = "emissive";
static std::string MatVar_EmissivePower = "emissive_power";
static std::string MatVar_Normal        = "normal";


// lazy string comparisons lol
static std::string Attrib_Position  = "POSITION";
static std::string Attrib_Normal    = "NORMAL";
static std::string Attrib_Tangent   = "TANGENT";

static std::string Attrib_TexCoord0 = "TEXCOORD_0";
static std::string Attrib_Color0    = "COLOR_0";
static std::string Attrib_Joints0   = "JOINTS_0";
static std::string Attrib_Weights0  = "WEIGHTS_0";


void* GetBufferData( cgltf_accessor* accessor )
{
	unsigned char* buffer = (unsigned char*)accessor->buffer_view->buffer->data + accessor->buffer_view->offset;
	void* data = buffer + accessor->offset;
	return data;
}


// TODO: only loads animations, materials, meshes, and textures
// gltf can load a lot more, but this is not at all handled in the engine, or have any support for it
// so we'll have to do this one day
void LoadGltf( const std::string &srPath, const std::string &srExt, Model* spModel )
{
	cgltf_options options{};
	cgltf_data* gltf = NULL;
	cgltf_result result = cgltf_parse_file( &options, srPath.c_str(), &gltf );

	if ( result != cgltf_result_success )
	{
		LogError( "Failed Loading GLTF File: \"%d\" - \"%s\"", Result2Str( result ), srPath.c_str() );
		return;
	}

	// Force reading data buffers (fills buffer_view->buffer->data)
	result = cgltf_load_buffers( &options, gltf, srPath.c_str() );

	if ( result != cgltf_result_success )
	{
		LogError( "Failed Loading GLTF Buffers: \"%d\" - \"%s\"", Result2Str( result ), srPath.c_str() );
		return;
	}

	std::string baseDir = GetBaseDir( srPath );

	MeshBuilder meshBuilder;
	meshBuilder.Start( matsys, spModel );
	meshBuilder.SetSurfaceCount( gltf->materials_count );

	// --------------------------------------------------------
	// Parse Materials

	for ( size_t i = 0; i < gltf->materials_count; ++i )
	{
		cgltf_material& gltfMat = gltf->materials[i];

		IMaterial* material = matsys->FindMaterial( gltfMat.name );

		if ( material == nullptr )
		{
			std::string matPath = baseDir + "/" + gltfMat.name + ".cmt";
			if ( filesys->IsFile( matPath ) )
				material = matsys->ParseMaterial( matPath );
		}

		// fallback if there is no cmt file
		if ( material == nullptr )
		{
			material = matsys->CreateMaterial();
			material->aName = gltfMat.name;
			material->SetShader( gDefaultShader );

			// auto SetTexture = [&]( const std::string& param, const std::string &texname )
			auto SetTexture = [&]( const std::string& param, cgltf_texture* texture )
			{
				if ( !texture || !texture->name )
					return;

				// TODO: use std::string_view
				const std::string texName = texture->name;

				if ( filesys->IsRelative( texName ) )
					material->SetVar( param, matsys->CreateTexture( baseDir + "/" + texName ) );
				else
					material->SetVar( param, matsys->CreateTexture( texName ) );
			};

			if ( gltfMat.has_pbr_metallic_roughness )
				SetTexture( MatVar_Diffuse, gltfMat.pbr_metallic_roughness.base_color_texture.texture );

			SetTexture( MatVar_Emissive, gltfMat.emissive_texture.texture );
			SetTexture( MatVar_Normal,   gltfMat.normal_texture.texture );

			if ( gltfMat.has_emissive_strength )
				material->SetVar( MatVar_EmissivePower, gltfMat.emissive_strength.emissive_strength );
		}

		meshBuilder.SetCurrentSurface( i );
		meshBuilder.SetMaterial( material );
	}

	meshBuilder.SetCurrentSurface( 0 );

	// --------------------------------------------------------
	// Parse Model Data

	// NOTE: try only changing the count of every material group first
	// then at the end, set all the offsets from the count of everything

	for ( size_t i = 0; i < gltf->meshes_count; i++ )
	{
		cgltf_mesh& mesh = gltf->meshes[i];

		for ( size_t p = 0; p < mesh.primitives_count; p++ )
		{
			cgltf_primitive& prim = mesh.primitives[p];

			if ( prim.type != cgltf_primitive_type_triangles )
			{
				LogWarn( "uh skipping non tri's for now lol\n" );
				continue;
			}

			// bruh
			for ( size_t matIndex = 0; matIndex < gltf->materials_count; matIndex++ )
			{
				if ( prim.material == &gltf->materials[i] )
				{
					meshBuilder.SetCurrentSurface( matIndex );
					break;
				}
			}

			// uh
			cgltf_accessor* vertexBuffer;
			cgltf_accessor* normalBuffer;
			cgltf_accessor* texBuffer;
			cgltf_accessor* colorBuffer;
			cgltf_accessor* jointsBuffer;
			cgltf_accessor* weightsBuffer;

			for ( size_t a = 0; a < prim.attributes_count; a++ )
			{
				cgltf_attribute& attrib = prim.attributes[a];

				if ( attrib.name == Attrib_Position )
				{
					vertexBuffer = attrib.data;
				}
				else if ( attrib.name == Attrib_Normal )
				{
					normalBuffer = attrib.data;
				}
				else if ( attrib.name == Attrib_Tangent )
				{
					// tangentBuffer = attrib.data;
				}
				else if ( attrib.name == Attrib_TexCoord0 )
				{
					texBuffer = attrib.data;
				}
				// NOTE: probably not setup right lol
				else if ( attrib.name == Attrib_Color0 )
				{
					colorBuffer = attrib.data;
				}
				else if ( attrib.name == Attrib_Joints0 )
				{
					jointsBuffer = attrib.data;
				}
				else if ( attrib.name == Attrib_Weights0 )
				{
					weightsBuffer = attrib.data;
				}
				else
				{
					LogWarn( "Unknown GLTF attribute: %s\n", attrib.name );
				}
			}

			if ( prim.indices )
			{
				// u8* indices = (u8*)prim.indices->buffer_view->buffer->data + prim.indices->offset;
				u8* indices = (u8*)GetBufferData( prim.indices );

				for ( size_t i = 0; i < prim.indices->count; i++ )
				{
					// TODO: CHECK COMPONENT TYPE ON EACH BUFFER

					u8* buf = indices + i * prim.indices->stride;
					int index = 0;

					switch ( prim.indices->component_type )
					{
						case cgltf_component_type_r_8u:
							index = (int)*buf;
							break;

						case cgltf_component_type_r_16u:
							index = (int)*(u16*)buf;
							break;

						case cgltf_component_type_r_32u:
							index = (int)*(u32*)buf;
							break;
					}

					// Kinda ugly having this duplicate code, but this works for now, maybe one day i'll find a better way of doing it
					// like filling up the buffers directly in vertices and being able to set indices in meshbuilder instead of it calculating it's own

					Assert( vertexBuffer->component_type == cgltf_component_type_r_32f );
					Assert( vertexBuffer->type == cgltf_type_vec3 );
					
					// Position
					if ( vertexBuffer )
					{
						if ( index < vertexBuffer->count )
						{
							meshBuilder.SetPos( *(glm::vec3*)((float*)GetBufferData( vertexBuffer ) + (index * 3)) );
						}
						else
						{
							LogWarn( "gltf position index out of bounds\n" );
							meshBuilder.SetPos( 0.f, 0.f, 0.f );
						}
					}
					else
					{
						meshBuilder.SetPos( 0.f, 0.f, 0.f );
					}

					// Normals
					Assert( normalBuffer->component_type == cgltf_component_type_r_32f );
					Assert( normalBuffer->type == cgltf_type_vec3 );

					if ( normalBuffer )
					{
						if ( index < normalBuffer->count )
						{
							meshBuilder.SetNormal( *(glm::vec3*)((float*)GetBufferData( normalBuffer ) + (index * 3)) );
						}
						else
						{
							LogWarn( "gltf normal index out of bounds\n" );
							meshBuilder.SetNormal( 0.f, 0.f, 0.f );
						}
					}
					else
					{
						meshBuilder.SetNormal( 0.f, 0.f, 0.f );
					}

					// Color - disabled cause format doesn't match our float color format, maybe change in the future? idk
#if 0
					Assert( colorBuffer->component_type == cgltf_component_type_r_16u );
					Assert( colorBuffer->type == cgltf_type_vec4 );

					if ( colorBuffer )
					{
						if ( index < colorBuffer->count )
						{
							// index * 4 because colors contain alpha, but we don't use alpha here at the moment
							meshBuilder.SetColor( *(glm::vec3*)((float*)GetBufferData( colorBuffer ) + (index * 4)) );
						}
						else
						{
							LogWarn( "gltf color index out of bounds\n" );
							meshBuilder.SetColor( 1.f, 1.f, 1.f );
						}
					}
					else
#endif
					{
						meshBuilder.SetColor( 1.f, 1.f, 1.f );
					}

					// Texture Coordinates
					Assert( texBuffer->component_type == cgltf_component_type_r_32f );
					Assert( texBuffer->type == cgltf_type_vec2 );

					if ( texBuffer )
					{
						if ( index < texBuffer->count )
						{
							meshBuilder.SetTexCoord( *(glm::vec2*)((float*)GetBufferData( texBuffer ) + (index * 2)) );
						}
						else
						{
							LogWarn( "gltf texture coordinate index out of bounds\n" );
							meshBuilder.SetTexCoord( 0.f, 0.f );
						}
					}
					else
					{
						meshBuilder.SetTexCoord( 0.f, 0.f );
					}

					// For Later
#if 0
					// Joints
					Assert( jointsBuffer->component_type == cgltf_component_type_r_8u );
					Assert( jointsBuffer->type == cgltf_type_vec4 );

					if ( jointsBuffer )
					{
						if ( index < jointsBuffer->count )
						{
							meshBuilder.SetJoint0( *(glm::vec4*)((float*)GetBufferData( jointsBuffer ) + (index * 4)) );
						}
						else
						{
							LogWarn( "gltf joint 0 index out of bounds\n" );
							meshBuilder.SetJoint0( 0.f, 0.f, 0.f, 0.f );
						}
					}
					else
					{
						meshBuilder.SetJoint0( 0.f, 0.f, 0.f, 0.f );
					}

					// Weights
					Assert( weightsBuffer->component_type == cgltf_component_type_r_32f );
					Assert( weightsBuffer->type == cgltf_type_vec3 );

					if ( weightsBuffer )
					{
						if ( index < weightsBuffer->count )
						{
							meshBuilder.SetWeight0( *(glm::vec4*)((float*)GetBufferData( weightsBuffer ) + (index * 4)) );
						}
						else
						{
							LogWarn( "gltf weight 0 index out of bounds\n" );
							meshBuilder.SetWeight0( 0.f, 0.f, 0.f, 0.f );
						}
					}
					else
					{
						meshBuilder.SetWeight0( 0.f, 0.f, 0.f, 0.f );
					}
#endif

					meshBuilder.NextVertex();
				}
			}
			else
			{
				// NOTE: not very good lol, needs checks from above
				size_t v = 0, n = 0, t = 0, c = 0;

				for ( ; v < vertexBuffer->count; )
				{
					// meshBuilder.SetPos( *(glm::vec3*)((float*)vertexBuffer->buffer_view->buffer->data + (vertexBuffer->stride * v++)) );
					meshBuilder.SetPos( *(glm::vec3*)((float*)GetBufferData( vertexBuffer )+ (3 * v++)) );

					if ( normalBuffer && n < normalBuffer->count )
					 	meshBuilder.SetNormal( *(glm::vec3*)((float*)GetBufferData( normalBuffer )+ (3 * n++)) );
					else
						meshBuilder.SetNormal( 0.f, 0.f, 0.f );

					if ( colorBuffer && c < colorBuffer->count )
						meshBuilder.SetColor( *(glm::vec3*)((float*)GetBufferData( colorBuffer )+ (3 * c++)) );
					else
						meshBuilder.SetColor( 1.f, 1.f, 1.f );

					if ( texBuffer && t < texBuffer->count )
						meshBuilder.SetTexCoord( *(glm::vec2*)((float*)GetBufferData( texBuffer )+ (2 * t++)) );
					else
						meshBuilder.SetTexCoord( 0.f, 0.f );
					
					meshBuilder.NextVertex();
				}
			}
		}
	}

	meshBuilder.End();

	cgltf_free( gltf );
}

