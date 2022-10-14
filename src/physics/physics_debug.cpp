#include "physics.h"
#include "physics_object.h"

#include <Renderer/DebugRenderer.cpp>

#include "physics_debug.h"


LOG_CHANNEL( Physics );

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
#if 0
	// do we have functions from game code ready

	Initialize();

	aValid = true;
#endif
}


PhysDebugDraw::~PhysDebugDraw()
{
}


void PhysDebugDraw::OnNewFrame()
{

}


void PhysDebugDraw::DrawLine( const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &color )
{
}


void PhysDebugDraw::DrawLine(
	const JPH::Float3 &inFrom,
	const JPH::Float3 &inTo,
	JPH::ColorArg inColor )
{
}


void PhysDebugDraw::DrawTriangle(
	JPH::Vec3Arg inV1,
	JPH::Vec3Arg inV2,
	JPH::Vec3Arg inV3,
	JPH::ColorArg inColor )
{
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
#if 0
	if ( inTriangles == nullptr || inTriangleCount == 0 )
		return nullptr; // mEmptyBatch;

	PhysDebugMesh* mesh = new PhysDebugMesh;
	aMeshes.push_back( mesh );

	MeshBuilder meshBuilder;
	meshBuilder.Start( matsys, mesh );
	meshBuilder.SetMaterial( apMatSolid );

	// convert vertices
	// vert.resize( inTriangleCount * 3 );
	for ( int i = 0; i < inTriangleCount; i++ )
	{
		meshBuilder.SetPos( fromJolt( inTriangles[i].mV[0].mPosition ) );
		// meshBuilder.SetNormal( fromJolt( inTriangles[i].mV[0].mNormal ) );
		meshBuilder.SetColor( fromJolt( inTriangles[i].mV[0].mColor ) );
		// meshBuilder.SetTexCoord( fromJolt( inTriangles[i].mV[0].mUV ) );
		meshBuilder.NextVertex();

		meshBuilder.SetPos( fromJolt( inTriangles[i].mV[1].mPosition ) );
		// meshBuilder.SetNormal( fromJolt( inTriangles[i].mV[1].mNormal ) );
		meshBuilder.SetColor( fromJolt( inTriangles[i].mV[1].mColor ) );
		// meshBuilder.SetTexCoord( fromJolt( inTriangles[i].mV[1].mUV ) );
		meshBuilder.NextVertex();

		meshBuilder.SetPos( fromJolt( inTriangles[i].mV[2].mPosition ) );
		// meshBuilder.SetNormal( fromJolt( inTriangles[i].mV[2].mNormal ) );
		meshBuilder.SetColor( fromJolt( inTriangles[i].mV[2].mColor ) );
		// meshBuilder.SetTexCoord( fromJolt( inTriangles[i].mV[2].mUV ) );
		meshBuilder.NextVertex();

		// vert[i * 3 + 0] = ToVertDBG( inTriangles[i].mV[0] );
		// vert[i * 3 + 1] = ToVertDBG( inTriangles[i].mV[1] );
		// vert[i * 3 + 2] = ToVertDBG( inTriangles[i].mV[2] );
	}

	meshBuilder.End();

	// create buffers
	// matsys->CreateVertexBuffer( mesh );
	// matsys->CreateIndexBuffer( mesh );

	return mesh;
#endif
	return nullptr;
}


PhysDebugDraw::Batch PhysDebugDraw::CreateTriangleBatch(
	const Vertex *inVertices,
	int inVertexCount,
	const JPH::uint32 *inIndices,
	int inIndexCount )
{
#if 0
	if ( inVertices == nullptr || inVertexCount == 0 || inIndices == nullptr || inIndexCount == 0 )
		return nullptr; // mEmptyBatch;

	PhysDebugMesh* mesh = new PhysDebugMesh;
	aMeshes.push_back( mesh );
	
	MeshBuilder meshBuilder;
	meshBuilder.Start( matsys, mesh );
	meshBuilder.SetMaterial( apMatSolid );

	// convert vertices
	for ( int i = 0; i < inIndexCount; i++ )
	{
		meshBuilder.SetPos( fromJolt( inVertices[inIndices[i]].mPosition ) );
		// meshBuilder.SetNormal( fromJolt( inVertices[inIndices[i]].mNormal ) );
		meshBuilder.SetColor( fromJolt( inVertices[inIndices[i]].mColor ) );
		// meshBuilder.SetTexCoord( fromJolt( inVertices[inIndices[i]].mUV ) );
		meshBuilder.NextVertex();
	}

	meshBuilder.End();

	// create material
	// IMaterial* mat = matsys->CreateMaterial();
	// aMaterials.push_back( mat );
	// 
	// mat->SetShader( "debug" );
	// mat->SetVar( "color", vec4_default );
	// 
	// mesh->SetMaterial( 0, mat );

	return mesh;
#endif
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
#if 0
	if ( inGeometry.GetPtr()->mLODs.empty() )
		return;

	// TODO: handle LODs
	auto& lod = inGeometry.GetPtr()->mLODs[0];

	PhysDebugMesh* mesh = (PhysDebugMesh*)lod.mTriangleBatch.GetPtr();

	if ( mesh == nullptr )
		return;

	IMaterial* mat = mesh->GetMaterial( 0 );
	const std::string& shader = mat->GetShaderName();

	// AAAA
	if ( inDrawMode == EDrawMode::Wireframe )
	{
		if ( shader != gShader_DebugLine )
			mat->SetShader( gShader_DebugLine );
	}
	else
	{
		if ( shader != gShader_Debug )
			mat->SetShader( gShader_Debug );
	}

	mat->SetVar(
		"color",
		{
			inModelColor.r,
			inModelColor.g,
			inModelColor.b,
			inModelColor.a,
		}
	);

	// AWFUL
	DefaultRenderable* renderable = new DefaultRenderable;
	renderable->aMatrix = fromJolt( inModelMatrix );
	renderable->apModel = mesh;

	matsys->AddRenderable( renderable );
#endif
}


void PhysDebugDraw::DrawText3D(
	JPH::Vec3Arg inPosition,
	const std::string &inString,
	JPH::ColorArg inColor,
	float inHeight )
{

}

