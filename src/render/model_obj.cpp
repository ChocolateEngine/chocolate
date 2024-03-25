/*
modelloader.cpp ( Authored by Demez )

Class dedicated for loading models, and caches them too for multiple uses
*/

#include "util.h"
#include "core/console.h"
#include "render/irender.h"

#include "graphics_int.h"
#include "mesh_builder.h"

#include <chrono>
#include <set>

extern IRender* render;

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj/fast_obj.h"

static std::string GetBaseDir( const std::string &srPath )
{
	if ( srPath.find_last_of( "/\\" ) != std::string::npos )
		return srPath.substr( 0, srPath.find_last_of( "/\\" ) );
	return "";
}


static std::string gDefaultShader = "basic_3d";

static std::string MatVar_Diffuse = "diffuse";
static std::string MatVar_Emissive = "emissive";


void LoadObj_Fast( const std::string &srBasePath, const std::string &srPath, Model* spModel )
{
	PROF_SCOPE();

	fastObjMesh* obj = fast_obj_read( srPath.c_str() );

	if ( obj == nullptr )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to parse obj\n", srPath.c_str() );
		return;
	}

	std::string baseDir  = GetBaseDir( srPath );
	std::string baseDir2 = GetBaseDir( srBasePath );

	u32         matCount = glm::max( 1U, obj->material_count );

	MeshBuilder meshBuilder( gGraphics );
  #ifdef _DEBUG
	meshBuilder.Start( spModel, srBasePath.c_str() );
  #else
	meshBuilder.Start( spModel );
  #endif
	meshBuilder.SetSurfaceCount( matCount );
	
	for ( unsigned int i = 0; i < obj->material_count; i++ )
	{
		fastObjMaterial& objMat   = obj->materials[ i ];
		std::string      matName  = baseDir2 + "/" + objMat.name;
		Handle           material = gGraphics.FindMaterial( matName.c_str() );

		if ( material == InvalidHandle )
		{
			std::string matPath = matName + ".cmt";
			if ( FileSys_IsFile( matPath ) )
				material = gGraphics.LoadMaterial( matPath );

			else if ( FileSys_IsFile( "models/" + matPath ) )
				material = gGraphics.LoadMaterial( "models/" + matPath );
		}

		// fallback if there is no cmt file
		if ( material == InvalidHandle )
		{
			material = gGraphics.CreateMaterial( objMat.name, gGraphics.GetShader( gDefaultShader ) );

			TextureCreateData_t createData{};
			createData.aUsage  = EImageUsage_Sampled;
			createData.aFilter = EImageFilter_Linear;

			auto SetTexture    = [ & ]( const std::string& param, const char* texname )
			{
				if ( texname == nullptr )
					return;

				Handle texture = InvalidHandle;

				if ( FileSys_IsRelative( texname ) )
					gGraphics.LoadTexture( texture, baseDir2 + "/" + texname, createData );
				else
					gGraphics.LoadTexture( texture, texname, createData );

				gGraphics.Mat_SetVar( material, param, texture );
			};

			SetTexture( MatVar_Diffuse, objMat.map_Kd.path );
			SetTexture( MatVar_Emissive, objMat.map_Ke.path );
		}

		meshBuilder.SetCurrentSurface( i );
		meshBuilder.SetMaterial( material );
	}

	if ( obj->material_count == 0 )
	{
		Handle material = gGraphics.CreateMaterial( srPath, gGraphics.GetShader( gDefaultShader ) );
		meshBuilder.SetCurrentSurface( 0 );
		meshBuilder.SetMaterial( material );
	}

	// u64 vertexOffset = 0;
	// u64 indexOffset  = 0;
	// 
	// u64 totalVerts = 0;
	u64 totalIndexOffset  = 0;

	for ( u32 objIndex = 0; objIndex < obj->object_count; objIndex++ )
	// for ( u32 objIndex = 0; objIndex < obj->group_count; objIndex++ )
	{
		fastObjGroup& group = obj->objects[objIndex];
		// fastObjGroup& group = obj->groups[objIndex];

		// index_offset += group.index_offset;

		for ( u32 faceIndex = 0; faceIndex < group.face_count; faceIndex++ )
		// for ( u32 faceIndex = 0; faceIndex < obj->face_count; faceIndex++ )
		{
			u32& faceVertCount = obj->face_vertices[ group.face_offset + faceIndex ];
			u32  faceMat       = 0;

			if ( obj->material_count > 0 )
			{
				faceMat = obj->face_materials[ group.face_offset + faceIndex ];
			}

			meshBuilder.SetCurrentSurface( faceMat );
			meshBuilder.AllocateVertices( faceVertCount == 3 ? 3 : 6 );

			for ( u32 faceVertIndex = 0; faceVertIndex < faceVertCount; faceVertIndex++ )
			{
				// NOTE: mesh->indices holds each face "fastObjIndex" as three
				// seperate index objects contiguously laid out one after the other
				fastObjIndex objVertIndex = obj->indices[ totalIndexOffset + faceVertIndex ];

				if ( faceVertIndex >= 3 )
				{
					auto ind0                   = meshBuilder.apSurf->aIndices[ meshBuilder.apSurf->aIndices.size() - 3 ];
					auto ind1                   = meshBuilder.apSurf->aIndices[ meshBuilder.apSurf->aIndices.size() - 1 ];

					meshBuilder.apSurf->aVertex = meshBuilder.apSurf->aVertices[ ind0 ];
					meshBuilder.NextVertex();

					meshBuilder.apSurf->aVertex = meshBuilder.apSurf->aVertices[ ind1 ];
					meshBuilder.NextVertex();
				}

				const u32 position_index = objVertIndex.p * 3;
				const u32 texcoord_index = objVertIndex.t * 2;
				const u32 normal_index   = objVertIndex.n * 3;

				meshBuilder.SetPos(
				  obj->positions[ position_index ],
				  obj->positions[ position_index + 1 ],
				  obj->positions[ position_index + 2 ] );

				meshBuilder.SetNormal(
				  obj->normals[ normal_index ],
				  obj->normals[ normal_index + 1 ],
				  obj->normals[ normal_index + 2 ] );

				if ( obj->material_count > 0 )
				{
					meshBuilder.SetTexCoord(
					  obj->texcoords[ texcoord_index ],
					  1.0f - obj->texcoords[ texcoord_index + 1 ] );
				}

				meshBuilder.NextVertex();
			}

			// indexOffset += faceVertCount;
			totalIndexOffset += faceVertCount;
		}
	}

	meshBuilder.End();
	fast_obj_destroy( obj );
}


void Graphics_LoadObj( const std::string& srBasePath, const std::string& srPath, Model* spModel )
{
	PROF_SCOPE();

	auto startTime = std::chrono::high_resolution_clock::now();
	float time      = 0.f;

	LoadObj_Fast( srBasePath, srPath, spModel );

	auto currentTime = std::chrono::high_resolution_clock::now();
	time = std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count();

	Log_DevF( gLC_ClientGraphics, 1, "Obj Load Time: %.6f sec: %s\n", time, srBasePath.c_str() );
}


void Graphics_LoadSceneObj( const std::string& srBasePath, const std::string& srPath, Scene_t* spScene )
{
	PROF_SCOPE();

	fastObjMesh* obj = fast_obj_read( srPath.c_str() );

	if ( obj == nullptr )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to parse obj\n", srPath.c_str() );
		return;
	}

	std::string baseDir  = GetBaseDir( srPath );
	std::string baseDir2 = GetBaseDir( srBasePath );

	std::vector< Handle > materials;

	for ( unsigned int i = 0; i < obj->material_count; i++ )
	{
		fastObjMaterial& objMat   = obj->materials[ i ];
		std::string      matName  = baseDir2 + "/" + objMat.name;
		Handle           material = gGraphics.FindMaterial( matName.c_str() );

		if ( material == InvalidHandle )
		{
			std::string matPath = matName + ".cmt";
			if ( FileSys_IsFile( matPath ) )
				material = gGraphics.LoadMaterial( matPath );
		}

		// fallback if there is no cmt file
		if ( material == InvalidHandle )
		{
			material = gGraphics.CreateMaterial( matName, gGraphics.GetShader( gDefaultShader ) );

			TextureCreateData_t createData{};
			createData.aUsage  = EImageUsage_Sampled;
			createData.aFilter = EImageFilter_Linear;

			auto SetTexture    = [ & ]( const std::string& param, const char* texname )
			{
				if ( texname == nullptr )
					return;

				Handle texture = InvalidHandle;

				if ( FileSys_IsRelative( texname ) )
					gGraphics.LoadTexture( texture, baseDir2 + "/" + texname, createData );
				else
					gGraphics.LoadTexture( texture, texname, createData );

				gGraphics.Mat_SetVar( material, param, texture );
			};

			SetTexture( MatVar_Diffuse, objMat.map_Kd.path );
			SetTexture( MatVar_Emissive, objMat.map_Ke.path );
		}

		materials.push_back( material );
	}

	u64             totalIndexOffset = 0;

	VertexData_t*   vertData         = new VertexData_t;
	ModelBuffers_t* modelBuffers     = new ModelBuffers_t;

	// std::vector< MeshBuilder > builders( obj->object_count );
	for ( u32 objIndex = 0; objIndex < obj->object_count; objIndex++ )
	// for ( u32 objIndex = 0; objIndex < obj->group_count; objIndex++ )
	{
		fastObjGroup& group = obj->objects[ objIndex ];
		// fastObjGroup& group = obj->groups[objIndex];

		// index_offset += group.index_offset;
		Model* model        = nullptr;
		spScene->aModels.push_back( gGraphics.CreateModel( &model ) );
		model->apBuffers    = modelBuffers;
		model->apVertexData = vertData;

		char* groupName = nullptr;

#ifdef _DEBUG
		if ( group.name )
		{
			groupName = new char[ strlen( group.name ) ];
			strcpy( groupName, group.name );
		}
#endif

		MeshBuilder meshBuilder( gGraphics );
		meshBuilder.Start( model, groupName );
		// meshBuilder.SetSurfaceCount( obj->material_count );

		// uh
		std::vector< u32 > mats;

		for ( u32 faceIndex = 0; faceIndex < group.face_count; faceIndex++ )
		// for ( u32 faceIndex = 0; faceIndex < obj->face_count; faceIndex++ )
		{
			u32& faceVertCount = obj->face_vertices[ group.face_offset + faceIndex ];
			u32& faceMat       = obj->face_materials[ group.face_offset + faceIndex ];

			size_t index = vec_index( mats, faceMat );
			if ( index == SIZE_MAX )
			{
				index = mats.size();
				mats.push_back( faceMat );
			}

			meshBuilder.SetSurfaceCount( mats.size() );
			meshBuilder.SetCurrentSurface( index );
			meshBuilder.SetMaterial( materials[ faceMat ] );
			meshBuilder.AllocateVertices( faceVertCount == 3 ? 3 : 6 );

			bool needNormalCalc = false;
			for ( u32 faceVertIndex = 0; faceVertIndex < faceVertCount; faceVertIndex++ )
			{
				// NOTE: mesh->indices holds each face "fastObjIndex" as three
				// seperate index objects contiguously laid out one after the other
				fastObjIndex vertIndex = obj->indices[ totalIndexOffset + faceVertIndex ];

				if ( faceVertIndex >= 3 )
				{
					auto ind0                   = meshBuilder.apSurf->aIndices[ meshBuilder.apSurf->aIndices.size() - 3 ];
					auto ind1                   = meshBuilder.apSurf->aIndices[ meshBuilder.apSurf->aIndices.size() - 1 ];

					meshBuilder.apSurf->aVertex = meshBuilder.apSurf->aVertices[ ind0 ];
					meshBuilder.NextVertex();

					meshBuilder.apSurf->aVertex = meshBuilder.apSurf->aVertices[ ind1 ];
					meshBuilder.NextVertex();
				}

				const u32 position_index = vertIndex.p * 3;
				const u32 texcoord_index = vertIndex.t * 2;
				const u32 normal_index   = vertIndex.n * 3;

				meshBuilder.SetPos(
				  obj->positions[ position_index ],
				  obj->positions[ position_index + 1 ],
				  obj->positions[ position_index + 2 ] );

				// Check to see if normal index is within valid bounds
				if ( normal_index + 0 < obj->normal_count * 3 )
				{
					meshBuilder.SetNormal(
					  obj->normals[ normal_index ],
					  obj->normals[ normal_index + 1 ],
					  obj->normals[ normal_index + 2 ] );
				}
				else
				{
					needNormalCalc |= true;

					// TEMP: set to positions to keep them unique
					// need to have index buffer calculation be at the end
					meshBuilder.SetNormal(
					  obj->positions[ position_index ],
					  obj->positions[ position_index + 1 ],
					  obj->positions[ position_index + 2 ] );
				}

				meshBuilder.SetTexCoord(
				  obj->texcoords[ texcoord_index ],
				  1.0f - obj->texcoords[ texcoord_index + 1 ] );

				meshBuilder.NextVertex();
			}

			if ( needNormalCalc )
			{
				u32       ind[ 3 ];
				glm::vec3 pos[ 3 ];

				ind[ 0 ] = meshBuilder.apSurf->aIndices[ meshBuilder.apSurf->aIndices.size() - 3 ];
				ind[ 1 ] = meshBuilder.apSurf->aIndices[ meshBuilder.apSurf->aIndices.size() - 2 ];
				ind[ 2 ] = meshBuilder.apSurf->aIndices[ meshBuilder.apSurf->aIndices.size() - 1 ];

				pos[ 0 ] = meshBuilder.apSurf->aVertices[ ind[ 0 ] ].pos;
				pos[ 1 ] = meshBuilder.apSurf->aVertices[ ind[ 1 ] ].pos;
				pos[ 2 ] = meshBuilder.apSurf->aVertices[ ind[ 2 ] ].pos;

				// Calcluate a New Face normal for the vertices
				glm::vec3 normal = glm::normalize( glm::cross( ( pos[ 1 ] - pos[ 0 ] ), ( pos[ 2 ] - pos[ 0 ] ) ) );

				// TODO: we could remove junk faces here, like single line ones

				// set the new face normal for all verts
				meshBuilder.apSurf->aVertices[ ind[ 0 ] ].normal = normal;
				meshBuilder.apSurf->aVertices[ ind[ 1 ] ].normal = normal;
				meshBuilder.apSurf->aVertices[ ind[ 2 ] ].normal = normal;
			}

			// indexOffset += faceVertCount;
			totalIndexOffset += faceVertCount;
		}

		meshBuilder.End( false );
	}

	gGraphics.CreateVertexBuffers( modelBuffers, vertData, srBasePath.c_str() );
	gGraphics.CreateIndexBuffer( modelBuffers, vertData, srBasePath.c_str() );

	fast_obj_destroy( obj );
}

