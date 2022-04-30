#pragma once

#include "physics.h"

#ifndef JPH_DEBUG_RENDERER
	// Hack to compile DebugRenderer
	#define JPH_DEBUG_RENDERER 1
#endif

#include <Renderer/DebugRenderer.h>

// NOTE: this would need to be implemented in game code or something similar to the bullet physics version, 
//  because this dll could be used on server where graphics is null
//  and i don't want this dll to try and load graphics tbh,
//  feel like it should be isolated and only use ch_core.dll

// class PhysDebugMesh:  public JPH::RefTargetVirtual, public JPH::RefTarget<IMesh>
class PhysDebugMesh:  public JPH::RefTargetVirtual, public IRenderable
{
public:
	PhysDebugMesh();
	~PhysDebugMesh();

	virtual void                    AddRef() override           { IRenderable::AddRef(); }
	virtual void					Release() override          { IRenderable::Release(); }

	IMaterial*                      apMaterial = nullptr;

	std::vector< vertex_debug_t >   aVertices;
	std::vector< uint32_t >         aIndices;

	// --------------------------------------------------------------------------------------
	// Materials

	virtual size_t     GetMaterialCount() override                          { return 1; }
	virtual IMaterial* GetMaterial( size_t i ) override                     { return apMaterial; }
	virtual void       SetMaterial( size_t i, IMaterial* mat ) override     { apMaterial = mat; }

	virtual void SetShader( size_t i, const std::string& name ) override
	{
		if ( apMaterial )
			apMaterial->SetShader( name );
	}

	// --------------------------------------------------------------------------------------
	// Drawing Info

	virtual u32 GetVertexOffset( size_t material ) override     { return 0; }
	virtual u32 GetVertexCount( size_t material ) override      { return aVertices.size(); }

	virtual u32 GetIndexOffset( size_t material ) override      { return 0; }
	virtual u32 GetIndexCount( size_t material ) override       { return aIndices.size(); }

	// --------------------------------------------------------------------------------------
	// Part of IModel only, i still don't know how i would handle different vertex formats
	// maybe store it in some kind of unordered_map containing these models and the vertex type?
	// but, the vertex and index type needs to be determined by the shader pipeline actually, hmm

	virtual VertexFormatFlags                   GetVertexFormatFlags() override     { return vertex_debug_t::GetFormatFlags(); }
	virtual size_t                              GetVertexFormatSize() override      { return sizeof( vertex_debug_t ); }
	virtual void*                               GetVertexData() override            { return aVertices.data(); };
	virtual size_t                              GetTotalVertexCount() override      { return aVertices.size(); };

	virtual std::vector< vertex_debug_t >&      GetVertices()                       { return aVertices; };
	virtual std::vector< uint32_t >&            GetIndices() override               { return aIndices; };
};


class PhysDebugDraw: public JPH::DebugRenderer
{
public:

	PhysDebugDraw();
	~PhysDebugDraw();

	// int aDebugMode = DBG_NoDebug;

	void OnNewFrame();

	void DrawLine( const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &color );

	// overrides
	virtual void    DrawLine(
							const JPH::Float3 &inFrom,
							const JPH::Float3 &inTo,
							JPH::ColorArg inColor ) override;

	virtual void    DrawTriangle(
							JPH::Vec3Arg inV1,
							JPH::Vec3Arg inV2,
							JPH::Vec3Arg inV3,
							JPH::ColorArg inColor ) override;

	virtual Batch   CreateTriangleBatch(
							const Triangle *inTriangles,
							int inTriangleCount ) override;

	virtual Batch   CreateTriangleBatch(
							const Vertex *inVertices,
							int inVertexCount,
							const JPH::uint32 *inIndices,
							int inIndexCount ) override;

	virtual void    DrawGeometry(
							JPH::Mat44Arg inModelMatrix,
							const JPH::AABox &inWorldSpaceBounds,
							float inLODScaleSq,
							JPH::ColorArg inModelColor,
							const GeometryRef &inGeometry,
							ECullMode inCullMode,
							ECastShadow inCastShadow,
							EDrawMode inDrawMode ) override;
	
	virtual void    DrawText3D(
							JPH::Vec3Arg inPosition,
							const std::string &inString,
							JPH::ColorArg inColor,
							float inHeight ) override;

	std::vector< PhysDebugMesh* > aMeshes;
	std::vector< IMaterial* >     aMaterials;

	IMaterial*                    apMatSolid;
	IMaterial*                    apMatWire;
};


// for debugging
extern BaseGraphicsSystem* graphics;
extern IMaterialSystem* matsys;

