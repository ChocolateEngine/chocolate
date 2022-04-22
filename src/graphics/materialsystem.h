/*
materialsystem.h ( Authored by Demez )

Manages all materials and shaders.
TODO: move this to graphics level abstraction so the game can use this
*/

#pragma once

#include "types/databuffer.hh"
#include "graphics/imaterialsystem.h"
#include "shaders/shaders.h"
#include "allocator.h"
#include "types/material.h"
#include "textures/itextureloader.h"


// for vertex and index buffers
struct RenderableBuffer
{
	VkBufferUsageFlags      aFlags     = 0;
	VkBuffer                aBuffer    = nullptr;
	VkDeviceMemory          aBufferMem = nullptr;
};


constexpr VkBufferUsageFlags gVertexBufferFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
constexpr VkBufferUsageFlags gIndexBufferFlags  = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;


class MaterialSystem: public IMaterialSystem
{
	using Sets   = std::vector< VkDescriptorSet >;
	using Images = std::vector< TextureDescriptor* >;
public:

	MaterialSystem();
	~MaterialSystem() {}

	friend class Renderer;

	void                        Init();

	/* Create a new empty material */
	IMaterial*	                CreateMaterial() override;

	/* Find an already loaded material by it's name if it exists */
	IMaterial*                  FindMaterial( const std::string &name ) override;

	/* Delete a material */
	void                        DeleteMaterial( IMaterial* mat ) override;

	/* Parse a Chocolate Material File (cmt) */
	IMaterial*                  ParseMaterial( const std::string &path ) override;

	/* Get Error Material */
	IMaterial*	                GetErrorMaterial( const std::string& shader ) override;

	/* Get Error Texture */
	Texture*                    GetMissingTexture() override;

	// ideally this shouldn't need a path, should really be reworked to be closer to thermite, more flexible design
	// also this probably shouldn't use a shader for a parameter
	Texture*                    CreateTexture( const std::string &path ) override;
	int                         GetTextureId( Texture *spTexture ) override;

	/* Find an already loaded texture in vram */
	//TextureDescriptor          *FindTexture( const std::string &path ) override;

	/* Create a Vertex and Index buffer for a Renderable. */
	void                        CreateVertexBuffer( BaseRenderable* renderable ) override;
	void                        CreateIndexBuffer( BaseRenderable* renderable ) override;

	/* Check if a Renderable has a Vertex and/or Index buffer. */
	bool                        HasVertexBuffer( BaseRenderable* renderable ) override;
	bool                        HasIndexBuffer( BaseRenderable* renderable ) override;

	/* Free a Vertex and Index buffer for a Renderable. */
	void                        FreeVertexBuffer( BaseRenderable* renderable ) override;
	void                        FreeIndexBuffer( BaseRenderable* renderable ) override;

	// Free all buffers from this renderable
	void                        FreeAllBuffers( BaseRenderable* renderable ) override;

	// New buffer methods
	// void                        CreateBuffers( BaseRenderable* renderable, RenderableBufferFlags flags ) override;
	// bool                        HasBuffers( BaseRenderable* renderable, RenderableBufferFlags flags ) override;
	// void                        FreeBuffers( BaseRenderable* renderable, RenderableBufferFlags flags ) override;

	/* Get Vector of Buffers for a Renderable. */
	const std::vector< RenderableBuffer* >& GetRenderBuffers( BaseRenderable* renderable );

	// Does a buffer exist with these flags for this renderable already?
	bool                        HasBufferInternal( BaseRenderable* renderable, VkBufferUsageFlags flags );

	void                        ReInitSwapChain();
	void                        DestroySwapChain();

	// BLECH
	void                        InitUniformBuffer( IMesh* mesh );

	inline UniformDescriptor&       GetUniformData( size_t id )            { return aUniformDataMap[id]; }
	inline VkDescriptorSetLayout    GetUniformLayout( size_t id )          { return aUniformLayoutMap[id]; }

	BaseShader*                 GetShader( const std::string& name ) override;
	bool                        AddShader( BaseShader* spShader, const std::string& name );

	template <typename T>
	BaseShader* CreateShader( const std::string& name )
	{
		// check if we already made this shader
		auto search = aShaders.find( name );

		if ( search != aShaders.end() )
			return search->second;

		BaseShader* shader = new T;
		shader->aName = name;
		shader->Init();

		aShaders[name] = shader;

		return shader;
	}

	// Call this on renderable creation to assign it an Id
	void                        RegisterRenderable( BaseRenderable* renderable ) override;

	// Destroy a Renderable's Vertex and Index buffers, and anything else it may use
	void                        DestroyRenderable( BaseRenderable* renderable ) override;
	constexpr void              DestroyRenderable( BaseRenderableGroup *group ) override;

	// Add a Renderable to be drawn next frame, list is cleared after drawing
	void                        AddRenderable( BaseRenderable* renderable ) override;
	constexpr void              AddRenderable( BaseRenderableGroup *group ) override;

	// Get the Renderable ID
	constexpr size_t            GetRenderableID( BaseRenderable* renderable ) override;

	// Draw a renderable (just calls shader draw lmao)
	void                        DrawRenderable( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex );
	void                        DrawRenderable( size_t renderableIndex, BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex );

	void                        DrawRenderable( BaseRenderableGroup *group, VkCommandBuffer c, uint32_t commandBufferIndex );

	// Awful Mesh Functions, here until i abstract what's used in it
	void                        MeshInit( IMesh* mesh ) override;
	void                        MeshReInit( IMesh* mesh ) override;
	void                        MeshFreeOldResources( IMesh* mesh ) override;


	VkSampler                  *apSampler;

	VkDescriptorSetLayout       aImageLayout;
	Sets                        aImageSets;

	VkDescriptorSetLayout       aUniformLayout;
	Sets                        aUniformSets;

	// TODO: remove this
	std::unordered_map< size_t, UniformDescriptor >         aUniformDataMap;
	std::unordered_map< size_t, VkDescriptorSetLayout >     aUniformLayoutMap;

	std::vector< ITextureLoader* >                          aTextureLoaders;
	
private:
	std::unordered_map< std::string, BaseShader* >          aShaders;

	std::vector< BaseRenderable* >                          aRenderables;

	std::unordered_map< size_t, std::vector< RenderableBuffer* > >   aRenderBuffers;

	//std::vector< BaseRenderable* >                          aDrawList;
	std::unordered_map< BaseShader*, std::vector< BaseRenderable* > > aDrawList;

	std::vector< Material* >                                aMaterials;
	std::vector< Material* >                                aErrorMaterials;

	std::vector< TextureDescriptor* >                       aTextures;
	std::unordered_map< std::string, TextureDescriptor* >   aTexturePaths;

	TextureDescriptor*                                      apMissingTex = nullptr;
	size_t                                                  aMissingTexId = 0;
};

extern MaterialSystem* matsys;

MaterialSystem* GetMaterialSystem();


void AddTextureLoader( ITextureLoader* loader );

