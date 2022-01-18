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


class MaterialSystem: public IMaterialSystem
{
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

	/* Get Error Material */
	IMaterial*	                GetErrorMaterial( const std::string& shader ) override;

	// ideally this shouldn't need a path, should really be reworked to be closer to thermite, more flexible design
	// also this probably shouldn't use a shader for a parameter
	TextureDescriptor          *CreateTexture( IMaterial* material, const std::string &path ) override;

	/* Find an already loaded texture in vram */
	//TextureDescriptor          *FindTexture( const std::string &path ) override;

	/* Create a Vertex and Index buffer for a Renderable. */
	void                        CreateVertexBuffer( BaseRenderable* renderable ) override;
	void                        CreateIndexBuffer( BaseRenderable* renderable ) override;

	/* Free a Vertex and Index buffer for a Renderable. */
	void                        FreeVertexBuffer( BaseRenderable* renderable ) override;
	void                        FreeIndexBuffer( BaseRenderable* renderable ) override;

	void                        ReInitSwapChain();
	void                        DestroySwapChain();

	// BLECH
	void                        InitUniformBuffer( IMesh* mesh );

	inline UniformDescriptor&       GetUniformData( size_t id )            { return aUniformDataMap[id]; }
	inline VkDescriptorSetLayout    GetUniformLayout( size_t id )          { return aUniformLayoutMap[id]; }

	BaseShader*                 GetShader( const std::string& name ) override;

	template <typename T>
	BaseShader* CreateShader( const std::string& name )
	{
		// check if we already made this shader
		if ( BaseShader* shader = GetShader(name) )
			return shader;

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

	// Add a Renderable to be drawn next frame, list is cleared after drawing
	void                        AddRenderable( BaseRenderable* renderable ) override;

	// Get the Renderable ID
	size_t                      GetRenderableID( BaseRenderable* renderable ) override;

	// Draw a renderable (just calls shader draw lmao)
	void                        DrawRenderable( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex );

	// Awful Mesh Functions, here until i abstract what's used in it
	void                        MeshInit( IMesh* mesh ) override;
	void                        MeshReInit( IMesh* mesh ) override;
	void                        MeshFreeOldResources( IMesh* mesh ) override;

private:
	std::unordered_map< std::string, BaseShader* >          aShaders;

	std::unordered_map< size_t, UniformDescriptor >         aUniformDataMap;
	std::unordered_map< size_t, VkDescriptorSetLayout >     aUniformLayoutMap;

	std::vector< BaseRenderable* >                          aRenderables;
	std::vector< BaseRenderable* >                          aDrawList;  // REMOVE THIS CRINGE

	std::vector< Material* >                                aMaterials;
	std::vector< Material* >                                aErrorMaterials;

	std::vector< ITextureLoader* >                          aTextureLoaders;
	std::unordered_map< std::string, TextureDescriptor* >   aTextures;
};

extern MaterialSystem* materialsystem;
extern MaterialSystem* matsys;

