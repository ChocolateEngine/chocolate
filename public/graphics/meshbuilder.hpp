/*
sprite.h ( Auhored by Demez )

Declares the Sprite class
*/
#pragma once

#include "core/core.h"

#include "types/transform.h"
#include "graphics/renderertypes.h"
#include "graphics/imaterialsystem.h"


// Vertex containing all possible values
struct MeshBuilderVertex
{
	glm::vec3 pos{};
	glm::vec3 normal{};
	glm::vec2 texCoord{};
	// glm::vec2 texCoord2;
	glm::vec3 color{};

	// only one thing for morph data at the moment
	glm::vec3 morphPos{};
	
	// bones and weights
	// std::vector<int> bones;
	// std::vector<float> weights;

	inline bool operator==( const MeshBuilderVertex& srOther ) const
	{
		if ( pos != srOther.pos )
			return false;

		if ( normal != srOther.normal )
			return false;

		if ( texCoord != srOther.texCoord )
			return false;

		if ( color != srOther.color )
			return false;

		return true;
	}
};


// Vertex containing all possible morph values
struct MeshBuilderMorphVertex
{
	// Vertex Index to affect
	int       vert;

	// Morph Target it came from
	int       morph;
	
	// Position and Normal deltas
	glm::vec3 pos{};
	glm::vec3 normal{};

	inline bool operator==( const MeshBuilderMorphVertex& srOther ) const
	{
		if ( vert != srOther.vert )
			return false;
		
		if ( morph != srOther.morph )
			return false;

		if ( pos != srOther.pos )
			return false;

		if ( normal != srOther.normal )
			return false;

		return true;
	}
};


// Hashing Support for this vertex struct
namespace std
{
	template<  > struct hash< MeshBuilderVertex >
	{
		size_t operator(  )( MeshBuilderVertex const& vertex ) const
		{
			size_t value = (hash< glm::vec3 >()(vertex.pos));
			value ^= (hash< glm::vec3 >()(vertex.color));
			value ^= (hash< glm::vec3 >()(vertex.normal));
			value ^= (hash< glm::vec2 >()(vertex.texCoord));
			return value;
		}
	};
	
	template<  > struct hash< MeshBuilderMorphVertex >
	{
		size_t operator(  )( MeshBuilderMorphVertex const& vertex ) const
		{
			size_t value = (hash< int >()(vertex.vert));
			value ^= (hash< int >()(vertex.morph));
			value ^= (hash< glm::vec3 >()(vertex.pos));
			value ^= (hash< glm::vec3 >()(vertex.normal));
			return value;
		}
	};
}


#define MESH_BUILDER_USE_IND 1

// TODO:
// - Add a way to add bones and weights to the vertex
// - Add a way to add morph targets to the vertex
// - probably add a "AddBlendShape( std::string_view name )" function

class MeshBuilder
{
public:

	struct BlendShape
	{
		std::string aName;
	};

	struct BlendShapeData
	{
		size_t                              aIndex;
		std::vector< MeshBuilderMorphVertex >    aVertices;
		MeshBuilderVertex                   aVertex;
	};
	
	struct Surface
	{
		std::vector< MeshBuilderVertex >    aVertices;
		std::vector< u32 >                  aIndices;

		// uh
		std::unordered_map< MeshBuilderVertex, u32 >   aVertInd;

		MeshBuilderVertex                   aVertex;
		VertexFormat                        aFormat    = VertexFormat_None;
		IMaterial*                          apMaterial = nullptr;

		// is this a per surface thing? i would imagine so
		std::vector< BlendShapeData >       aBlendShapes;
	};

	IMaterialSystem*            apMatSys = nullptr;
	IModel*                apMesh = nullptr;

	std::vector< BlendShape >   aBlendShapes;
	std::vector< Surface >      aSurfaces;
	size_t                      aSurf = 0;
	Surface*                    apSurf = 0;  // pointer to current surface

	// std::vector< Vertex >   aVertices;
	// VertexFormat            aFormat = VertexFormat_None;
	// IMaterial*              apMaterial = nullptr;


	// ------------------------------------------------------------------------


	// source uses some IVertexBuffer and IIndexBuffer class here? hmm
	inline void Start( IMaterialSystem* spMatSys, IModel* spMesh )
	{
		apMatSys = spMatSys;
		apMesh = spMesh;

		SetSurfaceCount( 1 );
		SetCurrentSurface( 0 );
	}


	inline void End()
	{
		apMesh->SetSurfaceCount( aSurfaces.size() );

		for ( size_t i = 0; i < aSurfaces.size(); i++ )
		{
			Surface& surf = aSurfaces[i];
			VertexData_t& vertData = apMesh->GetSurfaceVertexData( i );

			size_t formatSize = apMatSys->GetVertexFormatSize( surf.aFormat );

			vertData.aFormat = surf.aFormat;
			vertData.aCount = surf.aVertices.size();

			std::vector< VertexAttribute > attribs;
			for ( int attrib = 0; attrib < VertexAttribute_Count; attrib++ )
			{
				// does this format contain this attribute?
				// if so, add the attribute size to it
				if ( surf.aFormat & (1 << attrib) )
				{
					attribs.push_back( (VertexAttribute)attrib );
				}
			}

			vertData.aData.resize( attribs.size() );

			size_t j = 0;
			for ( VertexAttribute attrib : attribs )
			{
				size_t size = apMatSys->GetVertexAttributeSize( attrib );

				// VertAttribData_t* attribData = &vertData.aData.emplace_back();
				// vertData.aData.push_back( {} );
				VertAttribData_t& attribData = vertData.aData[j];
				// VertAttribData_t* attribData = &vertData.aData.back();

				attribData.aAttrib = attrib;
				attribData.apData = malloc( size * vertData.aCount );

				// char* data = (char*)attribData.apData;

				#define MOVE_VERT_DATA( vertAttrib, attribName, elemCount ) \
					if ( attrib == vertAttrib ) \
					{ \
						for ( size_t v = 0; v < surf.aVertices.size(); v++ ) \
						{ \
							memcpy( (float*)attribData.apData + (v * elemCount), &surf.aVertices[v].attribName, size ); \
						} \
					}

				if ( attrib == VertexAttribute_Position )
				{
					float* data = (float*)attribData.apData;
					for ( size_t v = 0; v < surf.aVertices.size(); v++ )
					{
						// memcpy( data + v * size, &surf.aVertices[v].pos, size );
						memcpy( (float*)attribData.apData + (v * 3), &surf.aVertices[v].pos, size);

						// memcpy( data, &surf.aVertices[v].pos, size );
						// data += size;
					}
				}

				// MOVE_VERT_DATA( VertexAttribute_Position, pos )
				else MOVE_VERT_DATA( VertexAttribute_Normal,   normal,   3 )
				else MOVE_VERT_DATA( VertexAttribute_Color,    color,    3 )
				else MOVE_VERT_DATA( VertexAttribute_TexCoord, texCoord, 2 )
				
				else MOVE_VERT_DATA( VertexAttribute_MorphPos, morphPos, 3 )

				#undef MOVE_VERT_DATA
				j++;
			}

#if MESH_BUILDER_USE_IND
			// Now Copy Indices
			std::vector< u32 >& ind = apMesh->GetSurfaceIndices( i );
			ind.resize( surf.aIndices.size() );

			memcpy( ind.data(), surf.aIndices.data(), sizeof( u32 ) * ind.size() );
#endif

			apMesh->SetVertexDataLocked( i, true );
			apMesh->SetMaterial( i, surf.apMaterial );
		}

#if MESH_BUILDER_USE_IND
		apMatSys->CreateIndexBuffers( apMesh );
#endif

		apMatSys->InitUniformBuffer( apMesh );
	}


	inline void Reset()
	{
		apMesh = nullptr;
		aSurfaces.clear();
		aSurf = 0;
		apSurf = 0;
	}


	// ------------------------------------------------------------------------


	inline size_t GetVertexCount( size_t i ) const
	{
		Assert( i > aSurfaces.size() );
		return aSurfaces[i].aVertices.size();
	}


	inline size_t GetVertexCount() const
	{
		Assert( aSurfaces.size() );
		return aSurfaces[aSurf].aVertices.size();
	}


	// constexpr size_t GetVertexSize()
	// {
	// 	return aVertexSize;
	// }


	// ------------------------------------------------------------------------
	// Building Functions


	inline void NextVertex()
	{
		Assert( aSurfaces.size() );
		Assert( apSurf );

		if ( apSurf->aVertices.empty() )
		{
			apSurf->aVertices.push_back( apSurf->aVertex );
			apSurf->aIndices.push_back( 0 );

			apSurf->aVertInd[apSurf->aVertex] = 0;

			return;
		}

#if MESH_BUILDER_USE_IND
		auto it = apSurf->aVertInd.find( apSurf->aVertex );

		// not a unique vertex
		if ( it != apSurf->aVertInd.end() )
		{
			apSurf->aIndices.push_back( it->second );
			return;
		}
#endif

		// this is a unique vertex, up the index
		apSurf->aVertices.push_back( apSurf->aVertex );

#if MESH_BUILDER_USE_IND
		u32 indCount = apSurf->aVertInd.size();
#else
		u32 indCount = apSurf->aIndices.size();
#endif

		apSurf->aIndices.push_back( indCount );
		apSurf->aVertInd[apSurf->aVertex] = indCount;
	}


	#define VERT_EMPTY_CHECK() \
		Assert( aSurfaces.size() ); \
		Assert( apSurf );


	inline void SetPos( const glm::vec3& data )
	{
		VERT_EMPTY_CHECK();

		apSurf->aFormat |= VertexFormat_Position;
		apSurf->aVertex.pos = data;
	}


	inline void SetPos( float x, float y, float z )
	{
		VERT_EMPTY_CHECK();

		apSurf->aFormat |= VertexFormat_Position;
		apSurf->aVertex.pos.x = x;
		apSurf->aVertex.pos.y = y;
		apSurf->aVertex.pos.z = z;
	}


	inline void SetNormal( const glm::vec3& data )
	{
		VERT_EMPTY_CHECK();

		apSurf->aFormat |= VertexFormat_Normal;
		apSurf->aVertex.normal = data;
	}


	inline void SetNormal( float x, float y, float z )
	{
		VERT_EMPTY_CHECK();

		apSurf->aFormat |= VertexFormat_Normal;
		apSurf->aVertex.normal.x = x;
		apSurf->aVertex.normal.y = y;
		apSurf->aVertex.normal.z = z;
	}


	inline void SetColor( const glm::vec3& data )
	{
		VERT_EMPTY_CHECK();

		apSurf->aFormat |= VertexFormat_Color;
		apSurf->aVertex.color = data;
	}


	inline void SetColor( float x, float y, float z )
	{
		VERT_EMPTY_CHECK();

		apSurf->aFormat |= VertexFormat_Color;
		apSurf->aVertex.color.x = x;
		apSurf->aVertex.color.y = y;
		apSurf->aVertex.color.z = z;
	}


	inline void SetTexCoord( const glm::vec2& data )
	{
		VERT_EMPTY_CHECK();

		apSurf->aFormat |= VertexFormat_TexCoord;
		apSurf->aVertex.texCoord = data;
	}


	inline void SetTexCoord( float x, float y )
	{
		VERT_EMPTY_CHECK();

		apSurf->aFormat |= VertexFormat_TexCoord;
		apSurf->aVertex.texCoord.x = x;
		apSurf->aVertex.texCoord.y = y;
	}


	// ------------------------------------------------------------------------
	// Blend Shapes


	inline void SetMorphPos( const glm::vec3& data )
	{
		VERT_EMPTY_CHECK();

		apSurf->aFormat |= VertexFormat_MorphPos;
		apSurf->aVertex.morphPos = data;
	}


	inline void SetMorphPos( float x, float y, float z )
	{
		VERT_EMPTY_CHECK();

		apSurf->aFormat |= VertexFormat_MorphPos;
		apSurf->aVertex.morphPos.x = x;
		apSurf->aVertex.morphPos.y = y;
		apSurf->aVertex.morphPos.z = z;
	}


	// ------------------------------------------------------------------------


	inline void SetMaterial( IMaterial* spMaterial )
	{
		Assert( aSurfaces.size() );
		Assert( apSurf );

		apSurf->apMaterial = spMaterial;
	}


	inline void SetSurfaceCount( size_t sCount )
	{
		aSurfaces.resize( sCount );

		// kind of a hack? not really supposed to do this but im lazy
		apMesh->SetSurfaceCount( aSurfaces.size() );
	}
	

	inline void SetCurrentSurface( size_t sIndex )
	{
		Assert( sIndex < aSurfaces.size() );

		aSurf = sIndex;
		apSurf = &aSurfaces[sIndex];
	}


	inline void AdvanceSurfaceIndex()
	{
		aSurf++;
		Assert( aSurf < aSurfaces.size() );
		apSurf = &aSurfaces[aSurf];
	}


	inline const MeshBuilderVertex& GetLastVertex()
	{
		Assert( aSurfaces.size() );
		Assert( apSurf );
		return *( apSurf->aVertices.end() );
	}


	// ------------------------------------------------------------------------


	inline glm::vec3 CalculatePlaneNormal( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c )
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


	inline void CalculateNormals( size_t sIndex )
	{
		Assert( aSurfaces.size() );

		Surface& surf = aSurfaces[sIndex];
		VertexData_t& vertData = apMesh->GetSurfaceVertexData( sIndex );

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


	inline void CalculateAllNormals()
	{
		for ( size_t i = 0; i < aSurfaces.size(); i++ )
		{
			CalculateNormals( i );
		}
	}


	#undef VERT_EMPTY_CHECK
};


