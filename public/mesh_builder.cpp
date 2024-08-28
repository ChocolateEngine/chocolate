#if 0

#include "core/core.h"
#include "types/transform.h"
#include "irender.h"
#include "mesh_builder.h"

#include <glm/vec3.hpp>


#define MESH_BUILDER_USE_IND 1

// TODO:
// - Add a way to add bones and weights to the vertex
// - Add a way to add morph targets to the vertex
// - probably add a "AddBlendShape( std::string_view name )" function
// 
// - Replace the "MeshBuilderVertex" struct with individual vertex vectors
//   This way, you can just fill the contents of a vertex attribute right away
//   The current interface will still remain
// 


// std::vector< Vertex >   aVertices;
// VertexFormat            aFormat = VertexFormat_None;
// IMaterial*              apMaterial = nullptr;


// ------------------------------------------------------------------------


LOG_REGISTER_CHANNEL_EX( gLC_MeshBuilder, "MeshBuilder", LogColor::Green );


void MeshBuild_ComputeTangentBasis()
{
#if 0
	for ( int i = 0; i < vertices.size(); i += 3 )
	{
		glm::vec3& v0        = vertices[ i + 0 ];
		glm::vec3& v1        = vertices[ i + 1 ];
		glm::vec3& v2        = vertices[ i + 2 ];

		glm::vec2& uv0       = uvs[ i + 0 ];
		glm::vec2& uv1       = uvs[ i + 1 ];
		glm::vec2& uv2       = uvs[ i + 2 ];

		// Edges of the triangle : position delta
		glm::vec3  deltaPos1 = v1 - v0;
		glm::vec3  deltaPos2 = v2 - v0;

		// UV delta
		glm::vec2  deltaUV1  = uv1 - uv0;
		glm::vec2  deltaUV2  = uv2 - uv0;

		float      r         = 1.0f / ( deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x );
		glm::vec3  tangent   = ( deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y ) * r;
		glm::vec3  bitangent = ( deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x ) * r;
		
		// Set the same tangent for all three vertices of the triangle.
        // They will be merged later
		tangents.push_back( tangent );
		tangents.push_back( tangent );
		tangents.push_back( tangent );

		// Same thing for bitangents
		bitangents.push_back( bitangent );
		bitangents.push_back( bitangent );
		bitangents.push_back( bitangent );
	}
#endif
}


// source uses some IVertexBuffer and IIndexBuffer class here? hmm
void MeshBuilder::Start( Model* spMesh, const char* spDebugName )
{
	apMesh = spMesh;

#ifdef _DEBUG
	apDebugName = spDebugName;
#endif

	SetSurfaceCount( 1 );
	SetCurrentSurface( 0 );
}


void MeshBuilder::End( bool sCreateBuffers )
{
	PROF_SCOPE();

#if 0
	if ( apMesh->aMeshes.size() )
	{
		Log_WarnF( gLC_MeshBuilder, "Meshes already created for Model: \"%s\"\n", apDebugName ? apDebugName : "internal" );
	}

	// apMesh->SetSurfaceCount( aSurfaces.size() );

	VertexData_t* vertData = nullptr;

	if ( apMesh->apVertexData )
	{
		vertData = apMesh->apVertexData;
	}
	else if ( !vertData )
	{
		vertData             = new VertexData_t;
		apMesh->apVertexData = vertData;
	}

	apMesh->aMeshes.reserve( aSurfaces.size() );
	for ( size_t i = 0; i < aSurfaces.size(); i++ )
	{
		Surface& surf = aSurfaces[ i ];

		if ( surf.aVertices.empty() )
		{
			Log_DevF( gLC_MeshBuilder, 1, "Model Surface %zd has no vertices, skipping: \"%s\"\n", i, apDebugName ? apDebugName : "internal" );
			continue;
		}

		Mesh& mesh = apMesh->aMeshes.emplace_back();

		size_t origVertCount = vertData->aCount;

		vertData->aFormat |= surf.aFormat;
		vertData->aCount += surf.aVertices.size();

		mesh.aVertexOffset = origVertCount;
		mesh.aVertexCount  = surf.aVertices.size();

		ChVector< VertexAttribute > attribs;
		for ( int attrib = 0; attrib < VertexAttribute_Count; attrib++ )
		{
			// does this format contain this attribute?
			// if so, add the attribute size to it
			if ( surf.aFormat & (1 << attrib) )
			{
				attribs.push_back( (VertexAttribute)attrib );
			}
		}

		vertData->aData.resize( std::max( vertData->aData.size(), attribs.size() ), true );

		size_t attribIndex = 0;

		for ( VertexAttribute attrib : attribs )
		{
			size_t size = apGraphics->GetVertexAttributeSize( attrib );

			for ( ; attribIndex < vertData->aData.size(); attribIndex++ )
			{
				if ( attrib == VertexAttribute_Position && vertData->aData[ attribIndex ].aAttrib == attrib )
				{
					if ( attribIndex == 0 )
						break;
				}

				if ( attribIndex == 0 )
					continue;

				// default is set to position
				if ( vertData->aData[ attribIndex ].aAttrib == attrib || vertData->aData[ attribIndex ].aAttrib == VertexAttribute_Position )
					break;
			}

			CH_ASSERT( attribIndex < vertData->aData.size() );

			if ( attribIndex >= vertData->aData.size() )
				continue;

			VertAttribData_t& attribData = vertData->aData[ attribIndex++ ];
			attribData.aAttrib           = attrib;

			if ( origVertCount > 0 )
			{
				void* newBuffer = realloc( attribData.apData, size * vertData->aCount );
				if ( newBuffer == nullptr )
				{
					Log_ErrorF( gLC_MeshBuilder, "Failed to allocate memory for vertex attribute buffer (%zd bytes)\n", size * vertData->aCount );
					return;
				}

				attribData.apData = newBuffer;
			}
			else
			{
				attribData.apData = malloc( size * vertData->aCount );
			}

			// char* data = (char*)attribData.apData;

			#define MOVE_VERT_DATA( vertAttrib, attribName, elemCount ) \
				if ( attrib == vertAttrib ) \
				{ \
					for ( size_t v = 0; v < surf.aVertices.size(); v++ ) \
					{ \
						memcpy( (float*)attribData.apData + ( origVertCount * elemCount ) + ( v * elemCount ), &surf.aVertices[ v ].attribName, size ); \
					} \
				}

			if ( attrib == VertexAttribute_Position )
			{
				float* data = (float*)attribData.apData + origVertCount * 3;
				for ( size_t v = 0; v < surf.aVertices.size(); v++ )
				{
					// memcpy( data + v * size, &surf.aVertices[v].pos, size );
					memcpy( data + ( v * 3 ), &surf.aVertices[ v ].pos, size );
			
					// memcpy( data, &surf.aVertices[v].pos, size );
					// data += size;
				}
			}

			// MOVE_VERT_DATA( VertexAttribute_Position, pos, 3 )
			else MOVE_VERT_DATA( VertexAttribute_Normal,   normal,   3 )
			else MOVE_VERT_DATA( VertexAttribute_Color,    color,    3 )
			else MOVE_VERT_DATA( VertexAttribute_TexCoord, texCoord, 2 )

			#undef MOVE_VERT_DATA
		}

#if MESH_BUILDER_USE_IND
		// Now Copy Indices
		ChVector< u32 >& ind      = vertData->aIndices;
		size_t           origSize = ind.size();

		mesh.aIndexOffset         = origSize;
		// mesh.aVertexOffset           = 0;
		mesh.aIndexCount          = surf.aIndices.size();

		ind.resize( ind.size() + surf.aIndices.size() );

		for ( size_t v = 0; v < surf.aIndices.size(); v++ )
		{
			ind[ origSize + v ] = surf.aIndices[ v ] + origVertCount;
		}

		// memcpy( &ind[ origIndex ], surf.aIndices.data(), sizeof( u32 ) * surf.aIndices.size() );
#endif

		// apMesh->SetVertexDataLocked( i, true );
		apMesh->aMeshes[ i ].aMaterial = surf.aMaterial;
	}

	if ( !sCreateBuffers )
		return;

	if ( apMesh->apBuffers == nullptr )
		apMesh->apBuffers = new ModelBuffers_t;

	apGraphics->CreateVertexBuffers( apMesh->apBuffers, vertData, apDebugName );

#if MESH_BUILDER_USE_IND
	apGraphics->CreateIndexBuffer( apMesh->apBuffers, vertData, apDebugName );
#endif
#endif
}


void MeshBuilder::Reset()
{
	apMesh = nullptr;
	aSurfaces.clear();
	aSurf = 0;
	apSurf = 0;
}


// ------------------------------------------------------------------------


uint32_t MeshBuilder::GetVertexCount( uint32_t i ) const
{
	CH_ASSERT( i > aSurfaces.size() );
	return aSurfaces[ i ].aVertices.size();
}


uint32_t MeshBuilder::GetVertexCount() const
{
	CH_ASSERT( aSurfaces.size() );
	return aSurfaces[ aSurf ].aVertices.size();
}


// constexpr size_t GetVertexSize()
// {
// 	return aVertexSize;
// }


// ------------------------------------------------------------------------
// Building Functions


void MeshBuilder::PreallocateVertices( uint32_t sCount )
{
	CH_ASSERT( aSurfaces.size() );
	CH_ASSERT( apSurf );

	if ( !apSurf )
		return;

	apSurf->aVertices.reserve( sCount );
	apSurf->aIndices.reserve( sCount );
	aSurfacesInd[ aSurf ].reserve( sCount );
}


void MeshBuilder::AllocateVertices( uint32_t sCount )
{
	CH_ASSERT( aSurfaces.size() );
	CH_ASSERT( apSurf );

	if ( !apSurf )
		return;

	apSurf->aVertices.reserve( apSurf->aVertices.size() + sCount );
	apSurf->aIndices.reserve( apSurf->aIndices.size() + sCount );
	aSurfacesInd[ aSurf ].reserve( aSurfacesInd[ aSurf ].size() + sCount );
}


void MeshBuilder::NextVertex()
{
	PROF_SCOPE();

	CH_ASSERT( aSurfaces.size() );
	CH_ASSERT( apSurf );

	if ( apSurf->aVertices.empty() )
	{
		apSurf->aVertices.push_back( apSurf->aVertex );
		apSurf->aIndices.push_back( 0 );

		aSurfacesInd[ aSurf ][ apSurf->aVertex ] = 0;

		return;
	}

#if MESH_BUILDER_USE_IND
	auto it = aSurfacesInd[ aSurf ].find( apSurf->aVertex );

	// not a unique vertex
	if ( it != aSurfacesInd[ aSurf ].end() )
	{
		apSurf->aIndices.push_back( it->second );
		return;
	}
#endif

	// this is a unique vertex, up the index
	apSurf->aVertices.push_back( apSurf->aVertex );

#if MESH_BUILDER_USE_IND
	u32 indCount = aSurfacesInd[ aSurf ].size();
#else
	u32 indCount = apSurf->aIndices.size();
#endif

	apSurf->aIndices.push_back( indCount );
	aSurfacesInd[ aSurf ][ apSurf->aVertex ] = indCount;
}


#define VERT_EMPTY_CHECK() \
	CH_ASSERT( aSurfaces.size() ); \
	CH_ASSERT( apSurf );


void MeshBuilder::SetPos( const glm::vec3& data )
{
	VERT_EMPTY_CHECK();

	apSurf->aFormat |= VertexFormat_Position;
	apSurf->aVertex.pos = data;
}


void MeshBuilder::SetPos( float x, float y, float z )
{
	VERT_EMPTY_CHECK();

	apSurf->aFormat |= VertexFormat_Position;
	apSurf->aVertex.pos.x = x;
	apSurf->aVertex.pos.y = y;
	apSurf->aVertex.pos.z = z;
}


void MeshBuilder::SetNormal( const glm::vec3& data )
{
	VERT_EMPTY_CHECK();

	apSurf->aFormat |= VertexFormat_Normal;
	apSurf->aVertex.normal = data;
}


void MeshBuilder::SetNormal( float x, float y, float z )
{
	VERT_EMPTY_CHECK();

	apSurf->aFormat |= VertexFormat_Normal;
	apSurf->aVertex.normal.x = x;
	apSurf->aVertex.normal.y = y;
	apSurf->aVertex.normal.z = z;
}


void MeshBuilder::SetColor( const glm::vec3& data )
{
	VERT_EMPTY_CHECK();

	apSurf->aFormat |= VertexFormat_Color;
	apSurf->aVertex.color = data;
}


void MeshBuilder::SetColor( float x, float y, float z )
{
	VERT_EMPTY_CHECK();

	apSurf->aFormat |= VertexFormat_Color;
	apSurf->aVertex.color.x = x;
	apSurf->aVertex.color.y = y;
	apSurf->aVertex.color.z = z;
}


void MeshBuilder::SetTexCoord( const glm::vec2& data )
{
	VERT_EMPTY_CHECK();

	apSurf->aFormat |= VertexFormat_TexCoord;
	apSurf->aVertex.texCoord = data;
}


void MeshBuilder::SetTexCoord( float x, float y )
{
	VERT_EMPTY_CHECK();

	apSurf->aFormat |= VertexFormat_TexCoord;
	apSurf->aVertex.texCoord.x = x;
	apSurf->aVertex.texCoord.y = y;
}


// ------------------------------------------------------------------------


void MeshBuilder::SetMaterial( Handle sMaterial )
{
	CH_ASSERT( aSurfaces.size() );
	CH_ASSERT( apSurf );

	apSurf->aMaterial = sMaterial;
}


void MeshBuilder::SetSurfaceCount( size_t sCount )
{
	aSurfaces.resize( sCount, true );
	aSurfacesInd.resize( sCount );

	// kind of a hack? not really supposed to do this but im lazy
	// apMesh->SetSurfaceCount( aSurfaces.size() );
}
	

void MeshBuilder::SetCurrentSurface( size_t sIndex )
{
	CH_ASSERT( sIndex < aSurfaces.size() );

	aSurf = sIndex;
	apSurf = &aSurfaces[sIndex];
}


void MeshBuilder::AdvanceSurfaceIndex()
{
	aSurf++;
	CH_ASSERT( aSurf < aSurfaces.size() );
	apSurf = &aSurfaces[aSurf];
}


const MeshBuilderVertex& MeshBuilder::GetLastVertex()
{
	CH_ASSERT( aSurfaces.size() );
	CH_ASSERT( apSurf );
	return *( apSurf->aVertices.end() );
}


// ------------------------------------------------------------------------


glm::vec3 Util_CalculatePlaneNormal( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c )
{
	// glm::vec3 v1( b.x - a.x, b.y - a.y, b.z - a.z );
	// glm::vec3 v2( c.x - a.x, c.y - a.y, c.z - a.z );
	glm::vec3 v1( b - a );
	glm::vec3 v2( c - a );

	return glm::cross( v1, v2 );
}


#if 0
inline glm::vec3 CalculateVertexNormal( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c )
{
	glm::vec3 v1( b - a );
	glm::vec3 v2( c - a );

	glm::vec3 faceNorm = glm::cross( v1, v2 );

	glm::vec3 temp = faceNorm / glm::length( faceNorm );
}
#endif


void MeshBuilder::CalculateNormals( size_t sIndex )
{
	CH_ASSERT( aSurfaces.size() );

	Surface& surf = aSurfaces[sIndex];
//	VertexData_t* vertData = apMesh->apVertexData;

#if 0
	vec3_t v1 = ( vec3_t ){ c->x - b->x, c->y - b->y, c->z - b->z };
	vec3_t v2 = ( vec3_t ){ d->x - b->x, d->y - b->y, d->z - b->z };

	vec3_cross( &a->aNormal, &v1, &v2 );

	a->aDistance = vec3_dot( &a->aNormal, b );
#endif

	// http://web.missouri.edu/~duanye/course/cs4610-spring-2017/assignment/ComputeVertexNormal.pdf

	size_t faceI = 0;
	for ( size_t v = 0, faceI = 0; v < surf.aVertices.size(); v++, faceI += 3 )
	{
	}
}


void MeshBuilder::CalculateAllNormals()
{
	for ( size_t i = 0; i < aSurfaces.size(); i++ )
	{
		CalculateNormals( i );
	}
}


#undef VERT_EMPTY_CHECK


// =============================================================================================
// Mesh Builder 2
// Helper System for loading models, not really meant for creating meshes from scratch in code


bool MeshBuild_StartMesh( MeshBuildData_t& srMeshBuildData, u32 sMaterialCount, ChHandle_t* spMaterials )
{
	if ( sMaterialCount == 0 )
		return false;

	srMeshBuildData.aMaterials.resize( sMaterialCount );

	for ( u32 i = 0; i < sMaterialCount; i++ )
	{
		srMeshBuildData.aMaterials[ i ].aMaterial = spMaterials[ i ];
	}

	return true;
}


static void MeshBuild_CopyVerts( void* spDest, u32 sBaseSize, u32 sStride, void* spData, u32 sVertexCount )
{
	float* data = (float*)spDest + sBaseSize;
	memcpy( data, spData, sVertexCount * sizeof( float ) * sStride );
}


static bool realloc_free( void*& spData, size_t sCount )
{
	void* data = realloc( spData, sCount );

	if ( !data )
	{
		free( spData );
		return false;
	}
		
	spData = data;
	return true;
}


template< typename T >
static bool realloc_free_cast( T*& spData, size_t sCount )
{
	void* data = realloc( spData, sCount * sizeof( T ) );

	if ( !data )
	{
		free( spData );
		return false;
	}
		
	spData = static_cast< T* >( data );
	return true;
}


void MeshBuild_FinishMesh( IRender3* render_api, MeshBuildData_t& srMeshBuildData, Model* spModel, bool sCalculateIndices, bool sUploadMesh, const char* spDebugName )
{
	if ( spModel->aMeshes.size() )
	{
		Log_WarnF( gLC_MeshBuilder, "Meshes already created for Model: \"%s\"\n", spDebugName ? spDebugName : "internal" );
		return;
	}

	VertexData_t* vertData                       = new VertexData_t;
	spModel->apVertexData                        = vertData;
	vertData->aFormat                            = VertexFormat_Position | VertexFormat_Normal | VertexFormat_TexCoord;
	ChVector< u32 >&              indexList      = vertData->aIndices;
	ChVector< VertAttribData_t >& vertAttribs    = vertData->aData;
	vertData->apBlendShapeData                   = nullptr;

	vertAttribs.resize( 3 );
	vertAttribs[ 0 ].aAttrib = VertexAttribute_Position;
	vertAttribs[ 1 ].aAttrib = VertexAttribute_Normal;
	vertAttribs[ 2 ].aAttrib = VertexAttribute_TexCoord;

	int strides[]            = { 3, 3, 2 };

	spModel->aMeshes.resize( srMeshBuildData.aMaterials.size() );

	if ( sCalculateIndices )
	{
	}
	else
	{
		u32 baseBlendSize = 0;
		for ( size_t matI = 0; matI < srMeshBuildData.aMaterials.size(); matI++ )
		{
			MeshBuildMaterial_t& material = srMeshBuildData.aMaterials[ matI ];
			Mesh&                mesh     = spModel->aMeshes[ matI ];
			mesh.aMaterial                = material.aMaterial;

			u32 baseSize                  = indexList.size();

			mesh.aVertexOffset            = baseSize;
			mesh.aVertexCount             = material.aVertexCount;

			indexList.resize( baseSize + material.aVertexCount );

			if ( !realloc_free( vertAttribs[ 0 ].apData, baseSize + material.aVertexCount * sizeof( float ) * 3 ) )
				return;

			if ( !realloc_free( vertAttribs[ 1 ].apData, baseSize + material.aVertexCount * sizeof( float ) * 3 ) )
				return;

			if ( !realloc_free( vertAttribs[ 2 ].apData, baseSize + material.aVertexCount * sizeof( float ) * 2 ) )
				return;

			u32 vertI = 0;
			for ( u32 newIndex = baseSize; newIndex < indexList.size(); newIndex++, vertI++ )
			{
				indexList[ newIndex ] = newIndex;
				// posList[ newIndex ]   = material.apPos[ vertI ];
				// normList[ newIndex ]  = material.apNorm[ vertI ];
				// uvList[ newIndex ]    = material.apUV[ vertI ];
			}

			// Copy Position
			MeshBuild_CopyVerts( vertAttribs[ 0 ].apData, baseSize, 3, material.apPos, material.aVertexCount );

			// Copy Normals
			MeshBuild_CopyVerts( vertAttribs[ 1 ].apData, baseSize, 3, material.apNorm, material.aVertexCount );

			// Copy UV's
			MeshBuild_CopyVerts( vertAttribs[ 2 ].apData, baseSize, 2, material.apUV, material.aVertexCount );

			// Copy Blend Shapes
			if ( material.aBlendShapes.empty() )
				continue;

			if ( !realloc_free_cast( vertData->apBlendShapeData, baseBlendSize + material.aVertexCount * material.aBlendShapes.size() ) )
				return;

			baseBlendSize += material.aVertexCount * material.aBlendShapes.size();

			// u32 baseBlendCount = vertData->aBlendShapeCount;
			// vertData->aBlendShapeCount += material.aBlendShapes.size();
			vertData->aBlendShapeCount = material.aBlendShapes.size();

			// Blend Shape Data is interleaved - POS|NORM|UV|POS|NORM|UV, instead of POS|POS|POS NORM|NORM|NORM UV|UV|UV

#if 0
			for ( size_t blendI = 0; blendI < material.aBlendShapes.size(); blendI++ )
			{
				MeshBuildBlendShape_t& blendShape = material.aBlendShapes[ blendI ];

				for ( size_t v = 0; v < material.aVertexCount; v++ )
				{
					MeshBuildBlendShapeElement_t& element = blendShape.apData[ v ];
					
					// Copy Position Data
					memcpy( &vertData->apBlendShapeData[ v ].aPosNormX, &element.aPos, sizeof( element.aPos ) );

					// Copy Normal Data
					vertData->apBlendShapeData[ v ].aPosNormX.w = element.aNorm.x;
					memcpy( &vertData->apBlendShapeData[ v ].aNormYZ_UV, &element.aNorm.y, 8 );

					// Copy UV Data
					memcpy( &vertData->apBlendShapeData[ v ].aNormYZ_UV.z, &element.aUV, 8 );
				}
			}
#else
			size_t blendV             = 0;
			for ( size_t blendI = 0; blendI < material.aBlendShapes.size(); blendI++ )
			{
				MeshBuildBlendShape_t& blendShape = material.aBlendShapes[ blendI ];

				for ( size_t v = 0; v < material.aVertexCount; v++ )
				{
					MeshBuildBlendShapeElement_t& element = blendShape.apData[ v ];
					
					// Copy Position Data
					memcpy( &vertData->apBlendShapeData[ blendV ].aPosNormX, &element.aPos, sizeof( element.aPos ) );

					// Copy Normal Data
					vertData->apBlendShapeData[ blendV ].aPosNormX.w = element.aNorm.x;
					memcpy( &vertData->apBlendShapeData[ blendV ].aNormYZ_UV, &element.aNorm.y, 8 );

					// Copy UV Data
					memcpy( &vertData->apBlendShapeData[ blendV ].aNormYZ_UV.z, &element.aUV, 8 );
					blendV++;
				}
			}
#endif


			// for ( u32 blendI = 0; blendI < material.aBlendShapes.size(); blendI++ )
			// {
			// 	u32    dataOffset = blendI * material.aVertexCount;
			// 	float* data       = (float*)blendShapeData.apData + baseSize + dataOffset;
			// 
			// 	memcpy( data, material.aBlendShapes[ blendI ].apData, material.aVertexCount * sizeof( Shader_VertexData_t ) );
			// }
		}

		vertData->aCount = indexList.size();
	}

	if ( !sUploadMesh )
		return;

	if ( spModel->apBuffers == nullptr )
		spModel->apBuffers = new ModelBuffers_t;

	spGraphics->CreateVertexBuffers( spModel->apBuffers, vertData, spDebugName );

	if ( sCalculateIndices )
		spGraphics->CreateIndexBuffer( spModel->apBuffers, vertData, spDebugName );
}


void MeshBuild_AllocateVertices( MeshBuildData_t& srMeshBuildData, u32 sMaterial, u32 sCount )
{
	if ( sMaterial >= srMeshBuildData.aMaterials.size() )
	{
		Log_WarnF( gLC_MeshBuilder, "Invalid Mesh Builder Material Index: %u - only %u allocated\n", sMaterial, srMeshBuildData.aMaterials.size() );
		return;
	}

	MeshBuildMaterial_t& material   = srMeshBuildData.aMaterials[ sMaterial ];
	u32                  startCount = material.aVertexCount;

	material.apPos                  = ch_realloc_count( material.apPos, startCount + sCount );
	material.apNorm                 = ch_realloc_count( material.apNorm, startCount + sCount );
	material.apUV                   = ch_realloc_count( material.apUV, startCount + sCount );

	memset( material.apPos + startCount, 0, sCount );
	memset( material.apNorm + startCount, 0, sCount );
	memset( material.apUV + startCount, 0, sCount );

	material.aVertexCount += sCount;
}


void MeshBuild_AllocateBlendShapes( MeshBuildMaterial_t& srMeshBuildMaterial, u32 sBlendShapeCount )
{
	srMeshBuildMaterial.aBlendShapes.resize( sBlendShapeCount );

	u32           vertCount     = srMeshBuildMaterial.aVertexCount;
	constexpr int blendElemSize = 3 + 3 + 2;

	for ( MeshBuildBlendShape_t& blendShape : srMeshBuildMaterial.aBlendShapes )
	{
		// blendShape.apPos  = ch_calloc_count< glm::vec3 >( vertCount );
		// blendShape.apNorm = ch_calloc_count< glm::vec3 >( vertCount );
		// blendShape.apUV   = ch_calloc_count< glm::vec2 >( vertCount );
		
		blendShape.apData = ch_calloc_count< MeshBuildBlendShapeElement_t >( vertCount );

		// blendShape.apData = calloc( vertCount, sizeof( float ) * blendElemSize );
	}
}


void MeshBuild_SetVertexPos( MeshBuildMaterial_t& srMeshBuildMaterial, u32 sVertIndex, const glm::vec3& data )
{
	srMeshBuildMaterial.apPos[ sVertIndex ] = data;
}


void MeshBuild_FillVertexPosData( MeshBuildData_t& srMeshBuildData, u32 sMaterial, glm::vec3* spData, u32 sCount, u32 sOffset )
{
	if ( sMaterial <= srMeshBuildData.aMaterials.size() )
	{
		Log_WarnF( gLC_MeshBuilder, "Invalid Mesh Builder Material Index: %u - only %u allocated\n", sMaterial, srMeshBuildData.aMaterials.size() );
		return;
	}

	MeshBuildMaterial_t& material = srMeshBuildData.aMaterials[ sMaterial ];
}

#endif