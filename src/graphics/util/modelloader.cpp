/*
modelloader.cpp ( Authored by Demez )

Class dedicated for loading models, and caches them too for multiple uses
*/

//#include "../../../inc/core/renderer/modelloader.h"
#include "modelloader.h"
#include "../materialsystem.h"
#include "graphics/imesh.h"

#include "util.h"
#include "core/console.h"

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE

#include "io/tiny_obj_loader.h"

extern MaterialSystem* materialsystem;

std::string GetBaseDir( const std::string &srPath )
{
	if ( srPath.find_last_of( "/\\" ) != std::string::npos )
		return srPath.substr( 0, srPath.find_last_of( "/\\" ) );
	return "";
}


// TODO: use std::filesystem::path
void LoadObj( const std::string &srPath, std::vector<Mesh*> &meshes )
{
	auto startTime = std::chrono::high_resolution_clock::now(  );

	tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig reader_config;

	std::string baseDir = GetBaseDir( srPath );

	reader_config.mtl_search_path = baseDir;

	if ( !reader.ParseFromFile( srPath, reader_config ) )
	{
		if ( !reader.Error().empty() )
			Print( "[LoadObj] Error: %s\n", reader.Error().c_str() );

		return;
	}

	startTime = std::chrono::high_resolution_clock::now(  );

	if ( !reader.Warning().empty() )
		Print( "%s\n", reader.Warning().c_str() );

	auto &objAttrib = reader.GetAttrib();
	auto &objShapes = reader.GetShapes();
	auto &objMaterials = reader.GetMaterials();
	
	meshes.resize( objMaterials.size() );

	for (std::size_t i = 0; i < meshes.size(); ++i)
	{
		meshes[i] = new Mesh;
		materialsystem->RegisterRenderable( meshes[i] );
		materialsystem->MeshInit( meshes[i] );
		meshes[i]->apMaterial = nullptr;
	}

	assert(objMaterials.size() > 0);

	for (std::size_t i = 0; i < objMaterials.size(); ++i)
	{
		const tinyobj::material_t &objMaterial = objMaterials[i];

		IMaterial* material = materialsystem->FindMaterial( objMaterial.name );

		if ( material == nullptr )
		{
			material = matsys->ParseMaterial( baseDir + "/" + objMaterial.name );
		}

		// fallback if there is no cmt file
		if ( material == nullptr )
		{
			material = materialsystem->CreateMaterial();
			material->aName = objMaterial.name;
			material->SetShader( "basic_3d" );

			auto SetTexture = [&]( const std::string &param, const std::string &texname )
			{
				if ( !texname.empty() )
				{
					std::filesystem::path path = texname;

					if ( path.is_relative() )
						path = baseDir / path;

					int tex = matsys->GetTextureId( materialsystem->CreateTexture( path.string() ) );

					material->AddVar( param, path.string(), tex );
				}
			};

			SetTexture( "diffuse", objMaterial.diffuse_texname );
			SetTexture( "emissive", objMaterial.emissive_texname );
		}

		meshes[i]->apMaterial = material;
	}

	std::unordered_map< vertex_3d_t, uint32_t > vertIndexes;

	for (std::size_t shapeIndex = 0; shapeIndex < objShapes.size(); ++shapeIndex)
	{
		Print( "Obj Shape Index: %u\n", shapeIndex );

		std::size_t indexOffset = 0;
		for (std::size_t faceIndex = 0; faceIndex < objShapes[shapeIndex].mesh.num_face_vertices.size(); ++faceIndex)
		{
			const std::size_t faceVertexCount = static_cast<std::size_t>(objShapes[shapeIndex].mesh.num_face_vertices[faceIndex]);
			if (faceVertexCount != 3)
			{
				// TODO: This really shouldn't happen, and we should have better error handling in the event it does
				//Log::FatalError("Model is not trianglated!\n");
			}

			const std::size_t materialIndex = objShapes[shapeIndex].mesh.material_ids[faceIndex] < 0 ? objMaterials.size() - 1 : objShapes[shapeIndex].mesh.material_ids[faceIndex];
			assert(materialIndex < objMaterials.size());

			Mesh& mesh = *meshes[materialIndex];

			struct tinyobj_vec2 { tinyobj::real_t x, y; };
			struct tinyobj_vec3 { tinyobj::real_t x, y, z; };

			for (std::size_t v = 0; v < faceVertexCount; ++v)
			{
				glm::vec3 pos = {};
				glm::vec3 norm = {};
				glm::vec2 uv = {};
				//glm::vec4 col = {};
				glm::vec3 col = {};

				constexpr int posStride = 3;
				constexpr int normStride = 3;
				constexpr int uvStride = 2;
				constexpr int colStride = 3;

				#define ToVec3( out, item ) { \
					const tinyobj_vec3& tiny_##out = reinterpret_cast<const tinyobj_vec3&>(item); \
					out = {tiny_##out.x, tiny_##out.y, tiny_##out.z}; }

				const tinyobj::index_t idx = objShapes[shapeIndex].mesh.indices[indexOffset + v];
				ToVec3( pos, objAttrib.vertices[posStride * idx.vertex_index] );

				// wtf do i do if there is no normals (like in the bsp2obj thing)
				if ( objAttrib.normals.size() > 0 && idx.normal_index >= 0 )
					ToVec3( norm, objAttrib.normals[normStride * idx.normal_index] );

				if (idx.texcoord_index >= 0)
				{
					const tinyobj_vec2& texcoord = reinterpret_cast<const tinyobj_vec2&>(objAttrib.texcoords[uvStride * idx.texcoord_index]);
					uv = {texcoord.x, 1.0f - texcoord.y};
				}

				ToVec3( col, objAttrib.colors[colStride * idx.vertex_index] );
				//const tinyobj_vec3& color = reinterpret_cast<const tinyobj_vec3&>(objAttrib.colors[colStride * static_cast<std::size_t>(idx.vertex_index)]);
				//vColor = {color.x, color.y, color.z, /*1.0f*/ };

				#undef ToVec3

				bool uniqueVertex = true;
				uint32_t newIndex = static_cast<uint32_t>(mesh.aVertices.size());
				assert(mesh.aVertices.size() < std::numeric_limits<uint32_t>::max());

				vertex_3d_t vert;

				vert.pos = pos;
				vert.normal = norm;
				vert.texCoord = uv;
				vert.color = col;

				auto iterSavedIndex = vertIndexes.find(vert);

				// Is this a duplicate vertex?
				/*if ( iterSavedIndex != vertIndexes.end() )
				{
					uniqueVertex = false;
					newIndex = iterSavedIndex->second;
				}*/

				if ( uniqueVertex )
				{
					mesh.aVertices.push_back( vert );
					//mesh.aMinSize = glm::min( pos, mesh.aMinSize );
					//mesh.aMaxSize = glm::max( pos, mesh.aMaxSize );
				}

				vertIndexes[ vert ] = newIndex;
				mesh.aIndices.emplace_back(newIndex);
			}

			indexOffset += faceVertexCount;
		}
	}

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count(  );

	Print( "Parsed Obj in %.6f sec: %s\n", time, srPath.c_str() );
}


void LoadGltf( const std::string &srPath, std::vector<Mesh*> &meshes )
{
}

