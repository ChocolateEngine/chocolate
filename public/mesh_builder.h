#pragma once

#include "core/core.h"
#include "igraphics.h"


// Vertex containing all possible values
struct MeshBuilderVertex
{
	glm::vec3   pos{};
	glm::vec3   normal{};
	glm::vec2   texCoord{};
	// glm::vec2 texCoord2;
	glm::vec3   color{};

	// bones and weights
	// std::vector<int> bones;
	// std::vector<float> weights;

	inline bool operator==( const MeshBuilderVertex& srOther ) const
	{
		// Guard self assignment
		if ( this == &srOther )
			return true;

		return std::memcmp( &pos, &srOther.pos, sizeof( MeshBuilderVertex ) ) == 0;
	}
};


// Vertex containing all possible morph values
struct MeshBuilderMorphVertex
{
	// Vertex Index to affect
	u32         vert;

	// Morph Target it came from
	u32         morph;

	// Position and Normal deltas
	glm::vec3   pos{};
	glm::vec3   normal{};

	inline bool operator==( const MeshBuilderMorphVertex& srOther ) const
	{
		if ( vert != srOther.vert )
			return false;

		// if ( morph != srOther.morph )
		// 	return false;

		if ( pos != srOther.pos )
			return false;

		// if ( normal != srOther.normal )
		// 	return false;

		return true;
	}
};


// Hashing Support for this vertex struct
namespace std
{
	template<  > struct hash< MeshBuilderVertex >
	{
		size_t operator()( MeshBuilderVertex const& vertex ) const
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

			value ^= ( hash< glm::vec2::value_type >()( vertex.texCoord.x ) );
			value ^= ( hash< glm::vec2::value_type >()( vertex.texCoord.y ) );

			return value;
		}
	};
}


// TODO:
// - Add a way to add bones and weights to the vertex
// - Add a way to add morph targets to the vertex
// - probably add a "AddBlendShape( std::string_view name )" function

glm::vec3 Util_CalculatePlaneNormal( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c );
// glm::vec3 Util_CalculateVertexNormal( const glm::vec3& a, const glm::vec3& b, const glm::vec3& c );

// Helper For Mesh Building on User Land
struct MeshBuilder
{
	struct BlendShape
	{
		std::string                   aName;
		ChVector< MeshBuilderVertex > aVertices;
	};

	struct Surface
	{
		ChVector< MeshBuilderVertex > aVertices;
		ChVector< u32 >               aIndices;

		MeshBuilderVertex             aVertex;
		VertexFormat                  aFormat   = VertexFormat_None;
		ChHandle_t                    aMaterial = CH_INVALID_HANDLE;
	};

	MeshBuilder( IGraphics* spGraphics )
	{
		apGraphics = spGraphics;
	}

	MeshBuilder( IGraphics& srGraphics )
	{
		apGraphics = &srGraphics;
	}

	IGraphics*                                                  apGraphics   = nullptr;
	const char*                                                 apDebugName  = nullptr;

	Model*                                                      apMesh       = nullptr;
	ModelBuffers_t*                                             apBuffers    = nullptr;
	VertexData_t*                                               apVertexData = nullptr;

	ChVector< BlendShape >                                      aBlendShapes;
	ChVector< Surface >                                         aSurfaces;
	std::vector< std::unordered_map< MeshBuilderVertex, u32 > > aSurfacesInd;  // kinda weird
	uint32_t                                                    aSurf  = 0;
	Surface*                                                    apSurf = 0;  // pointer to current surface

	// ------------------------------------------------------------------------

	void                     Start( Model* spMesh, const char* spDebugName = nullptr );
	void                     End( bool sCreateBuffers = true );
	void                     Reset();

	uint32_t                 GetVertexCount( uint32_t i ) const;
	uint32_t                 GetVertexCount() const;

	// ------------------------------------------------------------------------
	// Building Functions
	
	void                     PreallocateVertices( uint32_t sCount );
	void                     AllocateVertices( uint32_t sCount );  // adds onto the existing size

	void                     NextVertex();

	void                     SetPos( const glm::vec3& data );
	void                     SetPos( float x, float y, float z );

	void                     SetNormal( const glm::vec3& data );
	void                     SetNormal( float x, float y, float z );

	void                     SetColor( const glm::vec3& data );
	void                     SetColor( float x, float y, float z );

	void                     SetTexCoord( const glm::vec2& data );
	void                     SetTexCoord( float x, float y );

	// ------------------------------------------------------------------------

	void                     SetMaterial( Handle sMaterial );
	void                     SetSurfaceCount( size_t sCount );
	void                     SetCurrentSurface( size_t sIndex );
	void                     AdvanceSurfaceIndex();

	const MeshBuilderVertex& GetLastVertex();

	// ------------------------------------------------------------------------

	void                     CalculateNormals( size_t sIndex );
	void                     CalculateAllNormals();
};


// =============================================================================================
// Mesh Builder 2
// Helper System for loading models, not really meant for creating meshes from scratch in code


struct MeshBuildBlendShapeElement_t
{
	glm::vec3 aPos;
	glm::vec3 aNorm;
	glm::vec2 aUV;
};


struct MeshBuildBlendShape_t
{
	// Blend Shape Data is interleaved - POS|NORM|UV|POS|NORM|UV, instead of POS|POS|POS NORM|NORM|NORM UV|UV|UV
	MeshBuildBlendShapeElement_t* apData;
};


struct MeshBuildMaterial_t
{
	glm::vec3*                        apPos;
	glm::vec3*                        apNorm;
	glm::vec2*                        apUV;

	u32                               aVertexCount;
	ChHandle_t                        aMaterial = CH_INVALID_HANDLE;

	ChVector< MeshBuildBlendShape_t > aBlendShapes;
};


// the reason why i stored vertices in each surface was to organize them by material
// then i combine them all together at the end
// when iterating through the model, they could just be all fragmented out
// we need them grouped together like this so we can use vertex offsets for each material
// 
//  material 0 verts           material 1 verts
// |==========================|==========================================|
// 
// remember we draw each surface with a vertex offset, so we need to group vertices by material in a contiguous list or something
// current mesh builder takes care of this by putting each vertex in a different list depending on the surface
// then when we finish the mesh, we just append the finished lists one by one in the final vertex data
// however, blend shapes don't care about materials at all
// and neither does skeleton bone data, so we can store blend shapes and skeleton data with the same method
// but storing standard vertices would be a bit weird
// 
// this is how i imagine blend shapes will be stored in the single storage buffer we make for it
// 
//   morph 0 - pos       morph 0 - normal     morph 1 - pos       morph 1 - normal
// []===================|===================[]===================|===================[]
// 
// same with skeleton data, whatever data goes in there, maybe this? i know i need 2 buffers
// and one buffer to write to a bunch for updating the position and rotation of each joint somehow
// 
//  bone 0 weights      bone 1 weights      bone 2 weights      bone 3 weights
// |===================|===================|===================|===================|
// 


struct MeshBuildData_t
{
	ChVector< MeshBuildMaterial_t > aMaterials;
	VertexFormat                    aVertexFormat;

	ChVector< std::string >         aBlendShapeNames;
};


// TODO: what if the model we're loading already has indices calculated for it? can't we just use that?
bool MeshBuild_StartMesh( MeshBuildData_t& srMeshBuildData, u32 sMaterialCount, ChHandle_t* spMaterials );
void MeshBuild_FinishMesh( IGraphics* spGraphics, MeshBuildData_t& srMeshBuildData, Model* spModel, bool sCalculateIndices, bool sUploadMesh, const char* spDebugName = "" );

void MeshBuild_AllocateVertices( MeshBuildData_t& srMeshBuildData, u32 sMaterial, u32 sCount );
void MeshBuild_AllocateBlendShapes( MeshBuildMaterial_t& srMeshBuildMaterial, u32 sBlendShapeCount );

// void MeshBuild_SetVertexPos( MeshBuildMaterial_t& srMeshBuildMaterial, u32 sVertIndex, const glm::vec3& data );
// void MeshBuild_FillVertexPosData( MeshBuildData_t& srMeshBuildData, u32 sMaterial, glm::vec3* spData, u32 sCount, u32 sOffset );

