/*
modelloader.cpp ( Authored by Demez )

Class dedicated for loading models, and caches them too for multiple uses
*/

#include "core/util.h"
#include "physics.h"
#include "mesh_builder.h"

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj/fast_obj.h"


bool LoadObj_Fast( const std::string &srPath, PhysicsModel* spModel, bool singleShape )
{
	PROF_SCOPE();

	fastObjMesh* obj = fast_obj_read( srPath.c_str() );

	if ( obj == nullptr )
	{
		Log_ErrorF( "Failed to parse obj\n", srPath.c_str() );
		return false;
	}

	u64 totalIndexOffset  = 0;

	spModel->shapes.resize( singleShape ? 1 : obj->object_count );

	for ( u32 objIndex = 0; objIndex < obj->object_count; objIndex++ )
	// for ( u32 objIndex = 0; objIndex < obj->group_count; objIndex++ )
	{
		fastObjGroup&    group = obj->objects[ objIndex ];
		// fastObjGroup& group = obj->groups[objIndex];

		PhysicsSubShape& shape = spModel->shapes[ singleShape ? 0 : objIndex ];

		for ( u32 faceIndex = 0; faceIndex < group.face_count; faceIndex++ )
		// for ( u32 faceIndex = 0; faceIndex < obj->face_count; faceIndex++ )
		{
			u32& faceVertCount = obj->face_vertices[ group.face_offset + faceIndex ];
			u32  faceMat       = 0;

			if ( obj->material_count > 0 )
			{
				faceMat = obj->face_materials[ group.face_offset + faceIndex ];
			}

			u32 currentVertex = shape.vertexCount;
			shape.vertexCount += faceVertCount == 3 ? 3 : 6;

			// JPH::Float3* newVerts = ch_realloc< JPH::Float3 >( shape.vertices, shape.vertexCount );
			// 
			// if ( newVerts == nullptr )
			// {
			// 	for ( )
			// 	// FREREEEEF
			// 	// FREEEEE OTHER DATA
			// 	free( shape.vertices );
			// 	return false;
			// }
			// 
			// shape.vertices = newVerts;

			shape.vertices.resize( shape.vertexCount );

			for ( u32 faceVertIndex = 0; faceVertIndex < faceVertCount; faceVertIndex++ )
			{
				// NOTE: mesh->indices holds each face "fastObjIndex" as three
				// seperate index objects contiguously laid out one after the other
				fastObjIndex objVertIndex = obj->indices[ totalIndexOffset + faceVertIndex ];

				if ( faceVertIndex >= 3 && faceVertCount == 4 )
				{
					Log_ErrorF( "FACE HAS MORE THAT 3 VERTICES !!!!!!!\n" );
					return false;
				}

				const u32 position_index = objVertIndex.p * 3;
				shape.vertices[ currentVertex ].x = obj->positions[ position_index ];
				shape.vertices[ currentVertex ].y = obj->positions[ position_index + 1 ];
				shape.vertices[ currentVertex ].z = obj->positions[ position_index + 2 ];
				currentVertex++;
			}

			// indexOffset += faceVertCount;
			totalIndexOffset += faceVertCount;
		}
	}

	fast_obj_destroy( obj );

	// TODO: calculate indices, this only removes empty shapes
	for ( u32 shapeI = 0; shapeI < spModel->shapes.size(); shapeI++ )
	{
		PhysicsSubShape& shape = spModel->shapes[ shapeI ];

		// what?
		if ( shape.vertexCount == 0 )
		{
			vec_remove_index( spModel->shapes, shapeI );

			if ( shapeI > 0 )
				shapeI--;

			continue;
		}
	}

	if ( spModel->shapes.size() == 0 )
	{
		return false;
	}

	return true;
}


