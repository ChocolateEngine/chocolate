/*
imaterialsystem.h ( Authored by Demez )

Provides a public interface to the materialsystem so that the game can use this
*/

#pragma once


#include "imaterial.h"
#include "types/transform.h"

// TODO: maybe make BaseRenderable2D and BaseRenderable3D?

//template<typename VERT_TYPE>
class BaseRenderable
{
public:
	virtual ~BaseRenderable() = default;

	// this stuff will only be on the client (Meshes on the server won't inherit this probably? idk),
	// so i can just use the address of the "this" pointer for it
	inline size_t                   GetID(  ) { return (size_t)this; }

	IMaterial*                      apMaterial = nullptr;

	// Set the shader for the material by the shader name
	inline void                     SetShader( const char* name ) { if ( apMaterial ) apMaterial->SetShader( name ); };

	// TODO: remove this from BaseRenderable, really shouldn't have this,
	// gonna have to have some base types for this so game can use this
	// or we just store this in graphics like how we store uniform buffers
	VkBuffer                        aVertexBuffer = nullptr;
	VkBuffer                        aIndexBuffer = nullptr;
	VkDeviceMemory                  aVertexBufferMem = nullptr;
	VkDeviceMemory                  aIndexBufferMem = nullptr;

	//std::vector< VERT_TYPE >        aVertices;
	std::vector< uint32_t >         aIndices;
};

//template class BaseRenderable< vertex_2d_t >;
//template class BaseRenderable< vertex_3d_t >;

class IMesh;
class Sprite;


class IMaterialSystem
{
public:

	virtual ~IMaterialSystem() = default;

	/* Create a new empty material */
	virtual IMaterial*	                CreateMaterial() = 0;

	/* Find an already loaded material by it's name if it exists */
	virtual IMaterial*	                FindMaterial( const std::string& name ) = 0;

	/* Delete a material */
	virtual void                        DeleteMaterial( IMaterial* mat ) = 0;

	/* Parse a Chocolate Material File (cmt) */
	virtual IMaterial*                  ParseMaterial( const std::string &path ) = 0;

	/* Get Error Material */
	virtual IMaterial*	                GetErrorMaterial( const std::string& shader ) = 0;

	// ideally this shouldn't need a path, should really be reworked to be closer to thermite, more flexible design
	// also this probably shouldn't use a Material for a parameter, but it's needed right now
	// Creates a texture if it doesn't exist yet
	virtual TextureDescriptor          *CreateTexture( IMaterial* material, const std::string &path ) = 0;

	/* Find an already loaded texture in vram (useless, use the above func, it will do the check for you) */
	// virtual TextureDescriptor          *FindTexture( const std::string &path ) = 0;

	/* Create a Vertex and Index buffer for a Renderable. */
	virtual void                        CreateVertexBuffer( BaseRenderable* renderable ) = 0;
	virtual void                        CreateIndexBuffer( BaseRenderable* renderable ) = 0;

	/* Free a Vertex and Index buffer for a Renderable. */
	virtual void                        FreeVertexBuffer( BaseRenderable* renderable ) = 0;
	virtual void                        FreeIndexBuffer( BaseRenderable* renderable ) = 0;

	// Call this on renderable creation to assign it an Id
	virtual void                        RegisterRenderable( BaseRenderable* renderable ) = 0;

	// Destroy a Renderable's Vertex and Index buffers, and anything else it may use
	virtual void                        DestroyRenderable( BaseRenderable* renderable ) = 0;

	// Add a Renderable to be drawn next frame, list is cleared after drawing
	virtual void                        AddRenderable( BaseRenderable* renderable ) = 0;

	// Get the Renderable ID
	virtual size_t                      GetRenderableID( BaseRenderable* renderable ) = 0;

	virtual BaseShader*                 GetShader( const std::string& name ) = 0;

	// Awful Mesh Functions, here until i abstract what's used in it
	virtual void                        MeshInit( IMesh* mesh ) = 0;
	virtual void                        MeshReInit( IMesh* mesh ) = 0;
	virtual void                        MeshFreeOldResources( IMesh* mesh ) = 0;
};

