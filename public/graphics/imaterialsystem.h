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
	virtual                        ~BaseRenderable() = default;

	virtual size_t                  GetID(  ) { return (size_t)this; }

	virtual glm::mat4               GetModelMatrix() { return glm::mat4( 1.f ); };

	virtual IMaterial*              GetMaterial() = 0;
	virtual void                    SetMaterial( IMaterial* mat ) = 0;

	virtual void                    SetShader( const char* name ) = 0;
	
	virtual const std::vector< uint32_t >& GetIndices() = 0;
};


class BaseRenderableGroup
{
public:
	virtual ~BaseRenderableGroup() = default;

	virtual uint32_t        GetRenderableCount() = 0;
	virtual BaseRenderable* GetRenderable( uint32_t index ) = 0;
};


class IMesh;
class Sprite;


using RenderableBufferFlags = u32;


// TODO: use this later probably idk
enum : RenderableBufferFlags
{
	RenderableBuffer_None       = 0,
	RenderableBuffer_Vertex     = (1 << 0),
	RenderableBuffer_Index      = (1 << 1),
	RenderableBuffer_Uniform    = (1 << 2),
};


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

	/* Get Error Texture */
	virtual Texture*                    GetMissingTexture() = 0;

	// ideally this shouldn't need a path, should really be reworked to be closer to thermite, more flexible design
	// also this probably shouldn't use a Material for a parameter, but it's needed right now
	// Creates a texture if it doesn't exist yet
	virtual Texture*                    CreateTexture( const std::string &path ) = 0;
	virtual int                         GetTextureId( Texture *spTexture ) = 0;

	/* Find an already loaded texture in vram (useless, use the above func, it will do the check for you) */
	// virtual TextureDescriptor          *FindTexture( const std::string &path ) = 0;

	// Create a Vertex and Index buffer for a Renderable.
	virtual void                        CreateVertexBuffer( BaseRenderable* renderable ) = 0;
	virtual void                        CreateIndexBuffer( BaseRenderable* renderable ) = 0;

	// Check if a Renderable has a Vertex and/or Index buffer.
	virtual bool                        HasVertexBuffer( BaseRenderable* renderable ) = 0;
	virtual bool                        HasIndexBuffer( BaseRenderable* renderable ) = 0;

	// Free a Vertex and Index buffer for a Renderable.
	virtual void                        FreeVertexBuffer( BaseRenderable* renderable ) = 0;
	virtual void                        FreeIndexBuffer( BaseRenderable* renderable ) = 0;

	virtual void                        FreeAllBuffers( BaseRenderable* renderable ) = 0;

	// New buffer methods
	// virtual void                        CreateBuffers( BaseRenderable* renderable, RenderableBufferFlags flags ) = 0;
	// virtual RenderableBufferFlags       GetBuffers( BaseRenderable* renderable ) = 0;
	// virtual void                        FreeBuffers( BaseRenderable* renderable, RenderableBufferFlags flags ) = 0;

	// Call this on renderable creation to assign it an Id
	virtual void                        RegisterRenderable( BaseRenderable* renderable ) = 0;

	// Destroy a Renderable's Vertex and Index buffers, and anything else it may use
	virtual void                        DestroyRenderable( BaseRenderable* renderable ) = 0;
	virtual void                        DestroyRenderable( BaseRenderableGroup* group ) = 0;

	// Add a Renderable to be drawn next frame, list is cleared after drawing
	virtual void                        AddRenderable( BaseRenderable* renderable ) = 0;
	virtual void                        AddRenderable( BaseRenderableGroup* group ) = 0;

	// Get the Renderable ID
	virtual size_t                      GetRenderableID( BaseRenderable* renderable ) = 0;

	virtual BaseShader*                 GetShader( const std::string& name ) = 0;
	//virtual HShader                     GetShader( const std::string& name ) = 0;

	// Awful Mesh Functions, here until i abstract what's used in it
	virtual void                        MeshInit( IMesh* mesh ) = 0;
	virtual void                        MeshReInit( IMesh* mesh ) = 0;
	virtual void                        MeshFreeOldResources( IMesh* mesh ) = 0;

	// =====================================================================================
	// Material Handle Functions
};

