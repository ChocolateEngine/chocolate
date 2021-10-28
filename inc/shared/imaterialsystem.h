/*
imaterialsystem.h ( Authored by Demez )

Provides a public interface to the materialsystem so that the game can use this
*/

#pragma once


#include "../core/renderer/shaders.h"
#include "../core/renderer/material.h"
#include "../types/transform.h"


// only 8 bytes
class BaseRenderable
{
public:
	virtual ~BaseRenderable() = default;

	// uh
	virtual glm::mat4               GetModelMatrix() = 0;

	// this stuff will only be on the client (Meshes on the server won't inherit this probably? idk),
	// so i can just use the address of the "this" pointer for it
	inline size_t                   GetID(  ) { return (size_t)this; }

	Material*                       apMaterial = nullptr;
	inline BaseShader*              GetShader() const           { return apMaterial->apShader; }
};

// bruh
// #include "../types/modeldata.h"
class IMesh;


class IMaterialSystem
{
public:

	virtual ~IMaterialSystem() = default;

	virtual Material*	                CreateMaterial() = 0;
	virtual void                        DeleteMaterial( Material* mat ) = 0;

	virtual Material*	                GetErrorMaterial() = 0;

	// ideally this shouldn't need a path, should really be reworked to be closer to thermite, more flexible design
	// also this probably shouldn't use a Material for a parameter, but it's needed right now
	virtual TextureDescriptor*			CreateTexture( Material* material, const std::string path ) = 0;

	/* Create a Vertex and Index buffer for a Mesh. */
	virtual void                        CreateVertexBuffer( IMesh* mesh ) = 0;
	virtual void                        CreateIndexBuffer( IMesh* mesh ) = 0;

	/* Free a Vertex and Index buffer for a Mesh. */
	virtual void                        FreeVertexBuffer( IMesh* mesh ) = 0;
	virtual void                        FreeIndexBuffer( IMesh* mesh ) = 0;

	// Mesh Functions
	// virtual void                        InitMesh( IMesh* mesh ) = 0;

	// Call this on renderable creation to assign it an Id
	virtual void                        RegisterRenderable( BaseRenderable* renderable ) = 0;

	// Add a Renderable to be drawn next frame, list is cleared after drawing
	virtual void                        AddRenderable( BaseRenderable* renderable ) = 0;

	// Get the Renderable ID
	virtual size_t                      GetRenderableID( BaseRenderable* renderable ) = 0;

	virtual BaseShader*                 GetShader( const std::string& name ) = 0;
};

