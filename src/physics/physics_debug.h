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
class PhysDebugMesh:  public JPH::RefTargetVirtual, public RefCounted
{
public:
	PhysDebugMesh();
	~PhysDebugMesh();

	Handle       aModel;

	virtual void AddRef() override { RefCounted::AddRef(); }
	virtual void Release() override { RefCounted::Release(); }
};


class PhysDebugDraw: public JPH::DebugRenderer
{
public:

	PhysDebugDraw();
	~PhysDebugDraw();

	// int aDebugMode = DBG_NoDebug;

	void OnNewFrame();

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

	std::vector< PhysDebugMesh* >     aMeshes;

	bool                              aValid = false;
};

