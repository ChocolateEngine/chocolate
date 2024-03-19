#include "physics.h"
#include "physics_object.h"

#include <Renderer/DebugRenderer.cpp>

#include "physics_debug.h"


LOG_CHANNEL( Physics );

extern Phys_DebugFuncs_t gDebugFuncs;

CONVAR( phys_dbg, 0 );

// constexpr glm::vec3 vec3_default( 255, 255, 255 );
// constexpr glm::vec4 vec4_default( 255, 255, 255, 255 );

constexpr glm::vec3 vec3_default( 1, 1, 1 );
constexpr glm::vec4 vec4_default( 1, 1, 1, 1 );

static std::string gShader_Debug     = "debug";
static std::string gShader_DebugLine = "debug_line";

// ==============================================================


PhysDebugMesh::PhysDebugMesh()
{
}


PhysDebugMesh::~PhysDebugMesh()
{
}


// ==============================================================


PhysDebugDraw::PhysDebugDraw()
{
}


PhysDebugDraw::~PhysDebugDraw()
{
}


void PhysDebugDraw::Init()
{
	Initialize();
	aValid = true;
}


void PhysDebugDraw::OnNewFrame()
{

}


void PhysDebugDraw::DrawLine(
  JPH::RVec3Arg inFrom,
  JPH::RVec3Arg inTo,
  JPH::ColorArg inColor )
{
	gDebugFuncs.apDrawLine( fromJolt( inFrom ), fromJolt( inTo ), fromJolt( inColor ) );
}


void PhysDebugDraw::DrawTriangle(
  JPH::RVec3Arg inV1,
  JPH::RVec3Arg inV2,
  JPH::RVec3Arg inV3,
  JPH::ColorArg inColor,
  ECastShadow   inCastShadow )
{
	gDebugFuncs.apDrawTriangle( fromJolt( inV1 ), fromJolt( inV2 ), fromJolt( inV3 ), fromJolt4( inColor ) );
}


// vertex_debug_t ToVertDBG( const JPH::DebugRenderer::Vertex& inVert )
// {
// 	return {
// 		.pos        = fromJolt(inVert.mPosition),
// 		.color      = fromJolt(inVert.mColor),
// 		// .texCoord   = fromJolt(inVert.mUV),
// 		// .normal     = fromJolt(inVert.mNormal),
// 	};
// }


PhysDebugDraw::Batch PhysDebugDraw::CreateTriangleBatch(
	const Triangle *inTriangles,
	int inTriangleCount )
{
	std::vector< PhysTriangle_t > tris( inTriangleCount );

	for ( int i = 0; i < inTriangleCount; i++ )
	{
		tris[ i ].aPos[ 0 ] = fromJolt( inTriangles[ i ].mV[ 0 ].mPosition );
		tris[ i ].aPos[ 1 ] = fromJolt( inTriangles[ i ].mV[ 1 ].mPosition );
		tris[ i ].aPos[ 2 ] = fromJolt( inTriangles[ i ].mV[ 2 ].mPosition );
	}

	Handle handle = gDebugFuncs.apCreateTriBatch( tris );

	if ( handle )
	{
		PhysDebugMesh* mesh = new PhysDebugMesh;
		mesh->aModel        = handle;
		return mesh;
	}
	
	return nullptr;
}


PhysDebugDraw::Batch PhysDebugDraw::CreateTriangleBatch(
	const Vertex *inVertices,
	int inVertexCount,
	const JPH::uint32 *inIndices,
	int inIndexCount )
{
	std::vector< PhysVertex_t >   verts( inVertexCount );
	std::vector< u32 >            ind( inIndexCount );

	for ( int i = 0; i < inVertexCount; i++ )
	{
		verts[ i ].aPos = fromJolt( inVertices[ i ].mPosition );
		verts[ i ].aNorm = fromJolt( inVertices[ i ].mNormal );
		verts[ i ].aUV = fromJolt( inVertices[ i ].mUV );
		verts[ i ].aColor = fromJolt( inVertices[ i ].mColor.ToVec4() );
	}

	memcpy( ind.data(), inIndices, inIndexCount * sizeof( JPH::uint32 ) );

	// for ( int i = 0; i < inIndexCount; i++ )
	// {
	// }

	Handle handle = gDebugFuncs.apCreateTriBatchInd( verts, ind );

	if ( handle )
	{
		PhysDebugMesh* mesh = new PhysDebugMesh;
		mesh->aModel        = handle;
		return mesh;
	}

	return nullptr;
}


void PhysDebugDraw::DrawGeometry(
	JPH::Mat44Arg inModelMatrix,
	const JPH::AABox &inWorldSpaceBounds,
	float inLODScaleSq,
	JPH::ColorArg inModelColor,
	const GeometryRef &inGeometry,
	ECullMode inCullMode,
	ECastShadow inCastShadow,
	EDrawMode inDrawMode )
{
	// TODO: handle LODs
	auto&          lod  = inGeometry.GetPtr()->mLODs[ 0 ];
	PhysDebugMesh* mesh = (PhysDebugMesh*)lod.mTriangleBatch.GetPtr();

	gDebugFuncs.apDrawGeometry(
	  fromJolt( inModelMatrix ),
	  inLODScaleSq,
	  fromJolt( inModelColor.ToVec4() ),
	  mesh->aModel,
	  (EPhysCullMode)inCullMode,
	  inCastShadow == ECastShadow::On,
	  inDrawMode == EDrawMode::Wireframe );
}


void PhysDebugDraw::DrawText3D(
	JPH::Vec3Arg inPosition,
	const std::string_view &inString,
	JPH::ColorArg inColor,
	float inHeight )
{
	gDebugFuncs.apDrawText( fromJolt( inPosition ), inString.data(), fromJolt( inColor ), inHeight );
}

