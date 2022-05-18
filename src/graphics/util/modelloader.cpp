/*
modelloader.cpp ( Authored by Demez )

Class dedicated for loading models, and caches them too for multiple uses
*/

#include "modelloader.h"
#include "../materialsystem.h"
#include "../graphics.h"

#include "util.h"
#include "core/console.h"
#include "graphics/meshbuilder.hpp"


#define TINYOBJLOADER_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE

#include "io/tiny_obj_loader.h"

#define USE_FAST_OBJ 0

#if USE_FAST_OBJ
#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj/fast_obj.h"
#endif

static std::string GetBaseDir( const std::string &srPath )
{
	if ( srPath.find_last_of( "/\\" ) != std::string::npos )
		return srPath.substr( 0, srPath.find_last_of( "/\\" ) );
	return "";
}


static std::string gDefaultShader = "basic_3d";

static std::string MatVar_Diffuse = "diffuse";
static std::string MatVar_Emissive = "emissive";


void LoadObj_Tiny( const std::string &srPath, Model* spModel )
{
	std::string baseDir = GetBaseDir( srPath );
	auto startTime = std::chrono::high_resolution_clock::now();

	tinyobj::ObjReader reader;
	tinyobj::ObjReaderConfig reader_config;

	reader_config.mtl_search_path = baseDir;

	if ( !reader.ParseFromFile( srPath, reader_config ) )
	{
		if ( !reader.Error().empty() )
			LogError( gGraphicsChannel, "Obj %s\n", reader.Error().c_str() );

		return;
	}

	startTime = std::chrono::high_resolution_clock::now(  );

	if ( !reader.Warning().empty() )
		LogWarn( gGraphicsChannel, "Obj %s\n", reader.Warning().c_str() );

	auto &objAttrib = reader.GetAttrib();
	auto &objShapes = reader.GetShapes();
	auto &objMaterials = reader.GetMaterials();
	
	spModel->aMeshes.resize( objMaterials.size() );

	// for ( size_t i = 0; i < spModel->aMeshes.size(); i++ )
	// 	spModel->aMeshes[i] = new Model::MaterialGroup;

	// assert( objMaterials.size() > 0 );
	Assert( objMaterials.size() > 0 );

	// --------------------------------------------------------
	// Parse Materials

	MeshBuilder meshBuilder;
	meshBuilder.Start( matsys, spModel );
	meshBuilder.SetSurfaceCount( objMaterials.size() );

	for ( size_t i = 0; i < objMaterials.size(); ++i )
	{
		const tinyobj::material_t &objMaterial = objMaterials[i];

		IMaterial* material = matsys->FindMaterial( objMaterial.name );

		if ( material == nullptr )
		{
			std::string matPath = baseDir + "/" + objMaterial.name + ".cmt";
			if ( filesys->IsFile( matPath ) )
				material = matsys->ParseMaterial( matPath );
		}

		// fallback if there is no cmt file
		if ( material == nullptr )
		{
			material = matsys->CreateMaterial();
			material->aName = objMaterial.name;
			material->SetShader( gDefaultShader );

			auto SetTexture = [&]( const std::string& param, const std::string &texname )
			{
				if ( !texname.empty() )
				{
					if ( filesys->IsRelative( texname ) )
						material->SetVar( param, matsys->CreateTexture( baseDir + "/" + texname ) );
					else
						material->SetVar( param, matsys->CreateTexture( texname ) );
				}
			};

			SetTexture( MatVar_Diffuse, objMaterial.diffuse_texname );
			SetTexture( MatVar_Emissive, objMaterial.emissive_texname );
		}

		meshBuilder.SetCurrentSurface( i );
		meshBuilder.SetMaterial( material );
	}

	meshBuilder.SetCurrentSurface( 0 );

	// --------------------------------------------------------
	// Parse Model Data

	// std::unordered_map< vertex_3d_t, uint32_t > vertIndexes;

	for (std::size_t shapeIndex = 0; shapeIndex < objShapes.size(); ++shapeIndex)
	{
		LogDev( gGraphicsChannel, 1, "Obj Shape Index: %u\n", shapeIndex );

		size_t indexOffset = 0;

		size_t mdlVertexOffset = 0;
		size_t mdlIndexOffset = 0;
		size_t prevMaterialIndex = 0;
		// Model::MaterialGroup* prevMaterialGroup = nullptr;
		// Model::MaterialGroup* materialGroup = nullptr;

		for (size_t faceIndex = 0; faceIndex < objShapes[shapeIndex].mesh.num_face_vertices.size(); ++faceIndex)
		{
			const std::size_t faceVertexCount = static_cast<size_t>(objShapes[shapeIndex].mesh.num_face_vertices[faceIndex]);
			if (faceVertexCount != 3)
			{
				// TODO: This really shouldn't happen, and we should have better error handling in the event it does
				LogWarn( "Model is not trianglated: \"%s\"\n", srPath.c_str() );
			}

			const size_t materialIndex = objShapes[shapeIndex].mesh.material_ids[faceIndex] < 0 ? objMaterials.size() - 1 : objShapes[shapeIndex].mesh.material_ids[faceIndex];
			
			// assert(materialIndex < objMaterials.size());
			Assert(materialIndex < objMaterials.size());

			meshBuilder.SetCurrentSurface( materialIndex );

			// NOTE: this should only be set once per group
			// if ( prevMaterialIndex != materialIndex )
			// {
			// 	if ( materialGroup )
			// 	{
			// 		// materialGroup->aIndexOffset  = materialGroup->aIndexCount;
			// 		// materialGroup->aVertexOffset = materialGroup->aVertexOffset + materialGroup->aVertexCount;
			// 	}
			// 
			// 	prevMaterialIndex = materialIndex;
			// }

			// materialGroup = spModel->aMeshes[materialIndex];
			// 
			// if ( prevMaterialGroup && prevMaterialGroup != materialGroup )
			// {
			// 	materialGroup->aIndexOffset  = prevMaterialGroup->aIndexOffset + prevMaterialGroup->aIndexCount;
			// 	materialGroup->aVertexOffset = prevMaterialGroup->aVertexOffset + prevMaterialGroup->aVertexCount;
			// 	materialGroup->aVertexCount = 0;
			// 	materialGroup->aIndexCount = 0;
			// }
			// 
			// prevMaterialGroup = materialGroup;

			struct tinyobj_vec2 { tinyobj::real_t x, y; };
			struct tinyobj_vec3 { tinyobj::real_t x, y, z; };

			for (std::size_t v = 0; v < faceVertexCount; ++v)
			{

				// vertex_3d_t vert{};

				constexpr int posStride = 3;
				constexpr int normStride = 3;
				constexpr int uvStride = 2;
				constexpr int colStride = 3;

				#define ToVec3( attrib, out, item ) { \
					const tinyobj_vec3& tiny_##out = reinterpret_cast<const tinyobj_vec3&>(item); \
					vert.attrib = {tiny_##out.x, tiny_##out.y, tiny_##out.z}; }

				const tinyobj::index_t idx = objShapes[shapeIndex].mesh.indices[indexOffset + v];
				const tinyobj_vec3& tiny_pos = reinterpret_cast<const tinyobj_vec3&>(objAttrib.vertices[posStride * idx.vertex_index]);
				meshBuilder.SetPos( tiny_pos.x, tiny_pos.y, tiny_pos.z );

				// ToVec3( pos, pos, objAttrib.vertices[posStride * idx.vertex_index] );

				// wtf do i do if there is no normals (like in the bsp2obj thing)
				if ( objAttrib.normals.size() > 0 && idx.normal_index >= 0 )
				{
					const tinyobj_vec3& tiny_norm = reinterpret_cast<const tinyobj_vec3&>(objAttrib.normals[posStride * idx.normal_index]);
					meshBuilder.SetNormal( tiny_norm.x, tiny_norm.y, tiny_norm.z );
					// ToVec3( normal, norm, objAttrib.normals[normStride * idx.normal_index] );
				}
				else
				{
					meshBuilder.SetNormal( 0.f, 0.f, 0.f );
				}

				if (idx.texcoord_index >= 0)
				{
					const tinyobj_vec2& texcoord = reinterpret_cast<const tinyobj_vec2&>(objAttrib.texcoords[uvStride * idx.texcoord_index]);
					meshBuilder.SetTexCoord( texcoord.x, 1.0f - texcoord.y );
					// vert.texCoord = {texcoord.x, 1.0f - texcoord.y};
				}
				else
				{
					meshBuilder.SetTexCoord( 0.f, 0.f );
				}

				// ToVec3( color, col, objAttrib.colors[colStride * idx.vertex_index] );
				const tinyobj_vec3& color = reinterpret_cast<const tinyobj_vec3&>(objAttrib.colors[colStride * static_cast<std::size_t>(idx.vertex_index)]);
				meshBuilder.SetColor( color.x, color.y, color.z /*1.0f*/ );

				meshBuilder.NextVertex();

				// bool uniqueVertex = true;
				// uint32_t newIndex = static_cast<uint32_t>(spModel->GetVertices().size());

				// assert( spModel->GetVertices().size() < std::numeric_limits<uint32_t>::max() );
				// Assert( spModel->GetVertices().size() < std::numeric_limits<uint32_t>::max() );

				// auto iterSavedIndex = vertIndexes.find(vert);

				// Is this a duplicate vertex?
				/*if ( iterSavedIndex != vertIndexes.end() )
				{
					uniqueVertex = false;
					newIndex = iterSavedIndex->second;
				}*/

				// if ( uniqueVertex )
				// {
				// 	spModel->GetVertices().push_back( vert );
				// 	materialGroup->aVertexCount++;
				// 	// vertexOffset++;
				// 	//mesh.aMinSize = glm::min( pos, mesh.aMinSize );
				// 	//mesh.aMaxSize = glm::max( pos, mesh.aMaxSize );
				// }
				// 
				// vertIndexes[vert] = newIndex;
				// spModel->GetIndices().emplace_back( newIndex );
				// materialGroup->aIndexCount++;
			}

			indexOffset += faceVertexCount;
		}

		// if ( materialGroup )
		// {
		// 	// materialGroup->aIndexOffset  = indexOffset - materialGroup->aIndexCount;
		// 	// materialGroup->aVertexOffset = vertexOffset - materialGroup->aVertexCount;
		// }
	}

	meshBuilder.End();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count(  );

	LogDev( gGraphicsChannel, 1, "Parsed Obj in %.6f sec: %s\n", time, srPath.c_str() );
}


#if USE_FAST_OBJ
void LoadObj_Fast( const std::string &srPath, Model* spModel )
{
	// fastObjMesh*                    fast_obj_read(const char* path);
	// fastObjMesh*                    fast_obj_read_with_callbacks(const char* path, const fastObjCallbacks* callbacks, void* user_data);
	// void                            fast_obj_destroy(fastObjMesh* mesh);

	auto startTime = std::chrono::high_resolution_clock::now();

	fastObjMesh* obj = fast_obj_read( srPath.c_str() );

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count();

	LogDev( gGraphicsChannel, 1, "Fast Obj:        %.6f sec: %s\n", time, srPath.c_str() );

	if ( obj == nullptr )
	{
		LogError( gGraphicsChannel, "Failed to parse obj\n" );
		return;
	}

	std::string baseDir = GetBaseDir( srPath );

	spModel->aMeshes.resize( obj->material_count );

	for ( size_t i = 0; i < spModel->aMeshes.size(); i++ )
	// for ( size_t i = 0, j = obj->material_count-1; i < spModel->aMeshes.size(); i++, j-- )
	// for ( size_t i = spModel->aMeshes.size() - 1; i < -1; i-- )
	{
		spModel->aMeshes[i] = new Model::MaterialGroup;

		fastObjMaterial& objMat = obj->materials[i];

		IMaterial* material = matsys->FindMaterial( objMat.name );

		if ( material == nullptr )
		{
			std::string matPath = baseDir + "/" + objMat.name + ".cmt";
			if ( filesys->IsFile( matPath ) )
				material = matsys->ParseMaterial( matPath );
		}

		// fallback if there is no cmt file
		if ( material == nullptr )
		{
			material = matsys->CreateMaterial();
			material->aName = objMat.name;
			material->SetShader( gDefaultShader );

			auto SetTexture = [&]( const std::string& param, const char* texname )
			{
				if ( !texname )
					return;

				material->SetVar( param, matsys->CreateTexture( texname ) );
			};

			SetTexture( MatVar_Diffuse, objMat.map_Kd.path );
			SetTexture( MatVar_Emissive, objMat.map_Ke.path );
		}

		spModel->SetMaterial( i, material );
	}

	matsys->MeshInit( spModel );

	// https://github.com/LoganKilby/raytracer/blob/9f55659860aa7f907b386eb40f5a8295fa7e9c0d/main.cpp

#if 0

	u32 triangle_count = meshes[0].face_count;
	result.tri = (triangle *)malloc(sizeof(triangle) * triangle_count);
	result.count = triangle_count;

	triangle tri;
	u32 tri_index = 0;
	for(u32 mesh_index = 0; mesh_index < mesh_count; ++mesh_index)
	{
		fastObjMesh *mesh = mesh_buffer[mesh_index];
		for(u32 group_index = 0; group_index < mesh->group_count; ++group_index)
		{
			lane_u32 u32_max = lane_u32_from_u32(0xFFFFFFFF);
			fastObjGroup *group = &mesh->groups[group_index];
			Assert(mesh->face_vertices[group->face_offset] == 3);
			for(u32 face_index = 0; face_index < group->face_count; ++face_index)
			{
				// NOTE: mesh->indices holds each face "fastObjIndex" as three
				// seperate index objects contiguously laid out one after the other
				fastObjIndex m0 = mesh->indices[group->index_offset + 3 * face_index + 0];
				fastObjIndex m1 = mesh->indices[group->index_offset + 3 * face_index + 1];
				fastObjIndex m2 = mesh->indices[group->index_offset + 3 * face_index + 2];

				tri.v0 = *(v3 *)&mesh->positions[3 * m0.p];
				tri.v1 = *(v3 *)&mesh->positions[3 * m1.p];
				tri.v2 = *(v3 *)&mesh->positions[3 * m2.p];
				tri.normal = *(v3 *)&mesh->normals[3 * m0.n];

				tri.v0.z += 1;
				tri.v1.z += 1;
				tri.v2.z += 1;

				result.tri[tri_index++] = tri;
			}
		}
	}





	u64 vertex_offset = 0;
	u64 index_offset = 0;


	for ( u32 faceIndex = 0; faceIndex < obj->face_count; faceIndex++ )
	{
		u32 faceVertices = obj->face_vertices[faceIndex];

		for ( u32 faceVert = 0; faceVert < faceVertices; faceVert++ )
		{
			fastObjIndex objIndex = obj->indices[index_offset + faceVert];



			// bruh
			if ( faceVert >= 3 )
			{
				mesh.GetVertices().emplace_back();

				vertices[vertex_offset + 0] = vertices[vertex_offset - 3];
				vertices[vertex_offset + 1] = vertices[vertex_offset - 1];
				vertex_offset += 2;
			}
		}

	}
#endif

	// u64 index_offset = 0;
	// std::vector< u64 > index_offset( meshes.size() );

	u64 indexCount = 0;

	// for( u32 i = 0; i < obj->face_count; i++ )
	// 	index_count += 3 * (obj->face_vertices[i] - 2);

	// std::vector<vertex> vertices(index_count);


	for ( u32 objIndex = 0; objIndex < obj->object_count; objIndex++ )
	{
		fastObjGroup& group = obj->objects[objIndex];

		for ( u32 faceIndex = 0; faceIndex < group.face_count; faceIndex++ )
		{
			u32& faceMat = obj->face_materials[group.face_offset + faceIndex];
			indexCount += 3 * (obj->face_vertices[group.face_offset + faceIndex] - 2);
			// spModel->aMeshes[faceMat]->aIndexCount += 3 * (obj->face_vertices[group.face_offset + faceIndex] - 2);
		}
	}

	// std::unordered_map< vertex_3d_t, uint32_t > vertIndexes;

	std::vector< std::unordered_map< vertex_3d_t, uint32_t > > vertIndexes;
	vertIndexes.resize( spModel->aMeshes.size() );

	std::vector< std::unordered_map< uint32_t, vertex_3d_t > > vertIndexes2( spModel->aMeshes.size() );

	// maybe split this up by object?
	// might be easier to manage lol
	struct object_t
	{
		uint32_t                    mat;
		std::vector< uint32_t >     ind;
		std::vector< vertex_3d_t >  vert;
	};

	std::vector< object_t > objects( spModel->aMeshes.size() );

	// std::unordered_map< uint32_t, vertex_3d_t > vertIndexes2;
	// std::unordered_map< fastObjUInt, vertex_3d_t > vertIndexes;

	u64 vertexOffset = 0;
	u64 indexOffset  = 0;

	u64 totalVerts = 0;
	u64 totalIndexOffset  = 0;

	Model::MaterialGroup* prevMaterialGroup = nullptr;
	Model::MaterialGroup* materialGroup = nullptr;

	auto& verts = spModel->GetVertices();
	auto& ind = spModel->GetIndices();
	// verts.resize( totalVerts );
	ind.resize( indexCount );

	object_t* object = nullptr;

	for ( u32 objIndex = 0; objIndex < obj->object_count; objIndex++ )
	// for ( u32 objIndex = 0; objIndex < obj->group_count; objIndex++ )
	{
		fastObjGroup& group = obj->objects[objIndex];
		// fastObjGroup& group = obj->groups[objIndex];

		object_t* object = nullptr;

		// index_offset += group.index_offset;

		for ( u32 faceIndex = 0; faceIndex < group.face_count; faceIndex++ )
			// for ( u32 faceIndex = 0; faceIndex < obj->face_count; faceIndex++ )
		{
			u32& faceVertCount = obj->face_vertices[group.face_offset + faceIndex];
			u32& faceMat = obj->face_materials[group.face_offset + faceIndex];

			materialGroup = spModel->aMeshes[faceMat];

			if ( !object || object->mat != faceMat )
			{
				object = &objects[faceMat];
				object->mat = faceMat;

				if ( prevMaterialGroup && prevMaterialGroup != materialGroup )
				{
					materialGroup->aIndexOffset  = prevMaterialGroup->aIndexOffset  + prevMaterialGroup->aIndexCount;
					materialGroup->aVertexOffset = prevMaterialGroup->aVertexOffset + prevMaterialGroup->aVertexCount;
				}
			}

			prevMaterialGroup = materialGroup;

			// something is wrong, im missing like a third of the indices and verts
			for ( u32 faceVertIndex = 0; faceVertIndex < faceVertCount; faceVertIndex++ )
			{
				// NOTE: mesh->indices holds each face "fastObjIndex" as three
				// seperate index objects contiguously laid out one after the other
				// fastObjIndex objIndex = obj->indices[totalIndexOffset + faceVertIndex];
				fastObjIndex objIndex = obj->indices[totalIndexOffset + faceVertIndex];

				if ( faceVertIndex >= 3 )
				{
					auto ind0 = object->ind[ object->ind.size() - 3 ];
					auto ind1 = object->ind[ object->ind.size() - 1 ];

					object->ind.push_back( ind0 );
					object->ind.push_back( ind1 );

					object->vert.push_back( object->vert[ind0] );
					object->vert.push_back( object->vert[ind1] );

					verts.push_back( object->vert[ind0] );
					verts.push_back( object->vert[ind1] );

					materialGroup->aIndexCount += 2;
					materialGroup->aVertexCount += 2;
					totalVerts += 2;
				}

				const u32 position_index = objIndex.p * 3;
				const u32 texcoord_index = objIndex.t * 2;
				const u32 normal_index = objIndex.n * 3;

				vertex_3d_t vert{};
				vert.pos.x = obj->positions[position_index];
				vert.pos.y = obj->positions[position_index + 1];
				vert.pos.z = obj->positions[position_index + 2];

				vert.normal.x = obj->normals[normal_index];
				vert.normal.y = obj->normals[normal_index + 1];
				vert.normal.z = obj->normals[normal_index + 2];

				vert.texCoord.x = obj->texcoords[texcoord_index];
				vert.texCoord.y = 1.0f - obj->texcoords[texcoord_index + 1];

				// uint32_t newIndex = (uint32_t)(totalVerts);
				uint32_t newIndex = (uint32_t)(object->vert.size());

				Assert( totalVerts < std::numeric_limits<uint32_t>::max() );

				auto iterSavedIndex = vertIndexes[faceMat].find(vert);

				// Is this a duplicate vertex?
				if ( true )
				// if ( iterSavedIndex == vertIndexes[faceMat].end() )
				// if ( vertexOffset <= objIndex.p-1 )
				{
					// nope, add it
					// matVerts.push_back(vert);
					//mesh.aMinSize = glm::min( pos, mesh.aMinSize );
					//mesh.aMaxSize = glm::max( pos, mesh.aMaxSize );

					verts.push_back( vert );

					object->vert.push_back( vert );

					vertIndexes[faceMat][ vert ] = newIndex;
					vertIndexes2[faceMat][ newIndex ] = vert;

					materialGroup->aVertexCount++;
					totalVerts++;
				}
				else
				{
					newIndex = iterSavedIndex->second;
				// 	// newIndex = objIndex.p-1;
				}

				object->ind.push_back( newIndex );
				// matInd[faceMat].push_back( newIndex );
				// matIndOrig[faceMat].push_back();
				// ind[indexOffset++] = newIndex;

				materialGroup->aIndexCount++;

#if 0
				// KINDA SLOW
				auto it = vertIndexes.find( vert );
				if ( it == vertIndexes.end() )
				{
					// create a new vertex
					verts.resize( objIndex.p );

					verts[objIndex.p - 1].pos.x = obj->positions[position_index];
					verts[objIndex.p - 1].pos.y = obj->positions[position_index + 1];
					verts[objIndex.p - 1].pos.z = obj->positions[position_index + 2];

					verts[objIndex.p - 1].texCoord.x = obj->texcoords[texcoord_index];
					verts[objIndex.p - 1].texCoord.y = obj->texcoords[texcoord_index + 1];
				}

				ind[index] = objIndex.p - 1;
#endif
			}

			// indexOffset += faceVertCount;
			totalIndexOffset += faceVertCount;

#if 0
			// NOTE: mesh->indices holds each face "fastObjIndex" as three
			// seperate index objects contiguously laid out one after the other
			fastObjIndex m0 = obj->indices[group.index_offset + 3 * faceIndex + 0];
			fastObjIndex m1 = obj->indices[group.index_offset + 3 * faceIndex + 1];
			fastObjIndex m2 = obj->indices[group.index_offset + 3 * faceIndex + 2];

			auto it = vertIndexes2.find( m0.p );

			vertex_3d_t vert0;
			vertex_3d_t vert1;
			vertex_3d_t vert2;

			tri.v0 = *(v3 *)&obj->positions[3 * m0.p];
			tri.v1 = *(v3 *)&obj->positions[3 * m1.p];
			tri.v2 = *(v3 *)&obj->positions[3 * m2.p];
			tri.normal = *(v3 *)&obj->normals[3 * m0.n];

			tri.v0.z += 1;
			tri.v1.z += 1;
			tri.v2.z += 1;

			result.tri[tri_index++] = tri;


			for ( u32 faceVertIndex = 0; faceVertIndex < faceVert; faceVertIndex++ )
			{
				u32 index = index_offset[faceMat] + faceVertIndex;
				// const fastObjIndex& index = obj->indices[index_offset + faceVertIndex];
				const fastObjIndex& objIndex = obj->indices[index];
				// const fastObjIndex& index = obj->indices[ind.size() + faceVertIndex];

#if 0
				if(j >= 3)
				{
					vertices[vertex_offset] = vertices[vertex_offset - 3];
					vertices[vertex_offset + 1] = vertices[vertex_offset - 1];
					vertex_offset += 2;
				}
#endif

				const u32 position_index = objIndex.p * 3;
				const u32 texcoord_index = objIndex.t * 2;
				const u32 normal_index = objIndex.n * 3;

				// glm::vec3 pos{};
				// glm::vec3 color{};
				// glm::vec2 texCoord{};
				// glm::vec3 normal{};

#if 1
				vertex_3d_t& vert = verts.emplace_back(
					// pos
					glm::vec3(
						obj->positions[position_index],
						obj->positions[position_index + 1],
						obj->positions[position_index + 2]
					),

					// color
					glm::vec3( 0.f, 0.f, 0.f ),

					// texCoord
					glm::vec2(
						obj->texcoords[texcoord_index],
						obj->texcoords[texcoord_index + 1]
					),

					// normal
					glm::vec3(
						obj->normals[normal_index],
						obj->normals[normal_index + 1],
						obj->normals[normal_index + 2]
					)
				);
#endif

				// ind.emplace_back( index_offset + faceVertIndex );
				// ind.emplace_back( index_offset[faceMat] + faceVertIndex );
				// ind[index] = index;
				ind[index] = verts.size();
				// ind.emplace_back( ind.size() + faceVertIndex);
			}

			index_offset[faceMat] += faceVert;
#endif
		}
	}

	// sorting indices and vertices in a contiguous order by material

#if 1
	u32 indOffset = 0;
	u32 vertOffset = 0;
	for ( size_t matI = 0; matI < objects.size(); matI++ )
	{
		auto& object = objects[matI];

		materialGroup = spModel->aMeshes[matI];

		materialGroup->aIndexOffset = indOffset;
		materialGroup->aVertexOffset = vertOffset;

		for ( size_t i = 0; i < object.ind.size(); i++ )
		{
			ind[indOffset + i] = vertOffset + object.ind[i];
			// verts[vertOffset + object.ind[i]] = vertIndexes2[matI][object.ind[i]];
			// verts.push_back( object.vert[object.ind[i]] );
			// verts.push_back( vertIndexes2[indList[i]] );
			// verts[indOffset + indList[i]] = vertIndexes2[indOffset + indList[i]];
			// verts[indList[i]] = matVerts[indList[i]];
			// vertOffset++;
		}

		materialGroup->aIndexCount = object.ind.size();
		// materialGroup->aVertexOffset = verts.size() - materialGroup->aVertexOffset;

		indOffset += object.ind.size();
		vertOffset += object.vert.size();
	}
#endif

	fast_obj_destroy( obj );
}
#endif


void LoadObj( const std::string &srPath, Model* spModel )
{
	//auto startTime = std::chrono::high_resolution_clock::now();
	//float time = 0.f;

#if USE_FAST_OBJ
	LoadObj_Fast( srPath, spModel );
#else
	LoadObj_Tiny( srPath, spModel );
#endif

	// auto currentTime = std::chrono::high_resolution_clock::now();
	// time = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count();
	// 
	// //LogDev( gGraphicsChannel, 1, "Tiny Obj Loader: %.6f sec: %s\n", time, srPath.c_str() );
	// LogDev( gGraphicsChannel, 1, "Fast Obj:        %.6f sec: %s\n", time, srPath.c_str() );
	// 
	// 
	// // for ( auto mesh : meshes )
	// // 	delete mesh;
	// // meshes.clear();
	// 
	// 
	// startTime = std::chrono::high_resolution_clock::now();
	// 
	// // LoadObj_Fast( srPath, meshes );
	// // 
	// 
	// currentTime = std::chrono::high_resolution_clock::now();
	// time = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count();
	// 
	// LogDev( gGraphicsChannel, 1, "Tiny Obj Loader: %.6f sec: %s\n", time, srPath.c_str() );
	//LogDev( gGraphicsChannel, 1, "Fast Obj:        %.6f sec: %s\n", time, srPath.c_str() );
}


void LoadGltf( const std::string &srPath, Model* spModel )
{
}

