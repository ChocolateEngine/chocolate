#include "physics.h"
#include "physics_object.h"


#include <Renderer/DebugRenderer.cpp>

#include "physics_debug.h"

BaseGraphicsSystem* graphics = nullptr;
IMaterialSystem* matsys = nullptr;

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
	matsys = graphics->GetMaterialSystem();

	apMatSolid = matsys->CreateMaterial();
	apMatWire = matsys->CreateMaterial();

	// apMatSolid->SetShader( "basic_3d" );
	apMatSolid->SetShader( "debug" );
	apMatWire->SetShader( "debug_line" );

	Initialize();
}


PhysDebugDraw::~PhysDebugDraw()
{
}


void PhysDebugDraw::OnNewFrame()
{

}


void PhysDebugDraw::DrawLine( const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &color )
{
	graphics->DrawLine( from, to, color );
}


void PhysDebugDraw::DrawLine(
	const JPH::Float3 &inFrom,
	const JPH::Float3 &inTo,
	JPH::ColorArg inColor )
{
	graphics->DrawLine( fromJolt(inFrom), fromJolt(inTo), fromJolt(inColor) );
}


void PhysDebugDraw::DrawTriangle(
	JPH::Vec3Arg inV1,
	JPH::Vec3Arg inV2,
	JPH::Vec3Arg inV3,
	JPH::ColorArg inColor )
{
	graphics->DrawLine( fromJolt(inV1), fromJolt(inV2), fromJolt(inColor) );
	graphics->DrawLine( fromJolt(inV1), fromJolt(inV3), fromJolt(inColor) );
	graphics->DrawLine( fromJolt(inV2), fromJolt(inV3), fromJolt(inColor) );
}


vertex_3d_t ToVert3D( const JPH::DebugRenderer::Vertex& inVert )
{
	return {
		.pos        = fromJolt(inVert.mPosition),
		.color      = fromJolt(inVert.mColor),
		.texCoord   = fromJolt(inVert.mUV),
		.normal     = fromJolt(inVert.mNormal),
	};
}


PhysDebugDraw::Batch PhysDebugDraw::CreateTriangleBatch(
	const Triangle *inTriangles,
	int inTriangleCount )
{
	if ( inTriangles == nullptr || inTriangleCount == 0 )
		return nullptr; // mEmptyBatch;

	PhysDebugMesh* mesh = aMeshes.emplace_back( new PhysDebugMesh );

	mesh->SetMaterial( apMatSolid );

	auto& vert = mesh->GetVertices();
	auto& ind = mesh->GetIndices();

	// convert vertices
	vert.resize( inTriangleCount * 3 );
	for ( int i = 0, t = 0; t < inTriangleCount; i++, t++ )
	{
		vert[i * 3 + 0] = ToVert3D( inTriangles[i].mV[0] );
		vert[i * 3 + 1] = ToVert3D( inTriangles[i].mV[1] );
		vert[i * 3 + 2] = ToVert3D( inTriangles[i].mV[2] );
	}

	// create buffers
	matsys->MeshInit( mesh );
	matsys->CreateVertexBuffer( mesh );
	// matsys->CreateIndexBuffer( mesh );

	return mesh;
}


PhysDebugDraw::Batch PhysDebugDraw::CreateTriangleBatch(
	const Vertex *inVertices,
	int inVertexCount,
	const JPH::uint32 *inIndices,
	int inIndexCount )
{
	if ( inVertices == nullptr || inVertexCount == 0 || inIndices == nullptr || inIndexCount == 0 )
		return nullptr; // mEmptyBatch;

	PhysDebugMesh* mesh = aMeshes.emplace_back( new PhysDebugMesh );

	mesh->SetMaterial( apMatWire );

	auto& vert = mesh->GetVertices();
	auto& ind = mesh->GetIndices();

	// convert vertices
	vert.resize( inVertexCount );
	for ( int i = 0; i < inVertexCount; i++ )
	{
		vert[i] = ToVert3D( inVertices[i] );
	}

	// convert indices
	ind.resize( inIndexCount );
	for ( int i = 0; i < inIndexCount; i++ )
	{
		ind[i] = inIndices[i];
	}

	// create material
	IMaterial* mat = aMaterials.emplace_back( matsys->CreateMaterial() );
	mat->SetShader( "debug" );
	mat->SetVar( "color", vec4_default );

	// create buffers
	matsys->CreateVertexBuffer( mesh );
	matsys->CreateIndexBuffer( mesh );

	return mesh;
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
	if ( inGeometry.GetPtr()->mLODs.empty() )
		return;

	// TODO: handle LODs
	auto& lod = inGeometry.GetPtr()->mLODs[0];

	PhysDebugMesh* mesh = (PhysDebugMesh*)lod.mTriangleBatch.GetPtr();

	if ( mesh == nullptr )
		return;

	IMaterial* mat = mesh->GetMaterial();
	const std::string& shader = mat->GetShaderName();

	// AAAA
	if ( inDrawMode == EDrawMode::Wireframe )
	{
		if ( shader != gShader_DebugLine )
			mat->SetShader( gShader_DebugLine.c_str() );

		auto& verts = mesh->GetVertices();
		for ( auto& vert : verts )
		{
			vert.color.x = inModelColor.r;
			vert.color.y = inModelColor.g;
			vert.color.z = inModelColor.b;
		}
	}
	else
	{
		if ( shader != gShader_Debug )
			mat->SetShader( gShader_Debug.c_str() );

		mat->SetVar(
			"color",
			{
				inModelColor.r,
				inModelColor.g,
				inModelColor.b,
				inModelColor.a,
			}
		);
	}

	mesh->aMatrix = fromJolt( inModelMatrix );

	matsys->AddRenderable( mesh );

	return;
}


void PhysDebugDraw::DrawText3D(
	JPH::Vec3Arg inPosition,
	const std::string &inString,
	JPH::ColorArg inColor,
	float inHeight )
{

}

