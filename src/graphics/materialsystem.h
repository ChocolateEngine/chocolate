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


class MaterialSystem: public IMaterialSystem
{
public:

	MaterialSystem();
	~MaterialSystem() {}

	friend class Renderer;

	void                        Init();

	IMaterial*	                CreateMaterial() override;
	void                        DeleteMaterial( IMaterial* mat ) override;

	IMaterial*	                GetErrorMaterial() override { return apErrorMaterial; }

	// ideally this shouldn't need a path, should really be reworked to be closer to thermite, more flexible design
	// also this probably shouldn't use a shader for a parameter
	TextureDescriptor*          CreateTexture( IMaterial* material, const std::string path ) override;

	/* Create a Vertex and Index buffer for a Mesh. */
	void                        CreateVertexBuffer( IMesh* mesh ) override;
	void                        CreateIndexBuffer( IMesh* mesh ) override;

	/* Free a Vertex and Index buffer for a Mesh. */
	void                        FreeVertexBuffer( IMesh* mesh ) override;
	void                        FreeIndexBuffer( IMesh* mesh ) override;

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

	// Add a Renderable to be drawn next frame, list is cleared after drawing
	void                        AddRenderable( BaseRenderable* renderable ) override;

	// Get the Renderable ID
	size_t                      GetRenderableID( BaseRenderable* renderable ) override;

	// Draw a renderable
	void                        DrawRenderable( BaseRenderable* renderable, VkCommandBuffer c, uint32_t commandBufferIndex );

	// This really should be part of the shader
	void                        DrawMesh( IMesh* mesh, VkCommandBuffer c, uint32_t commandBufferIndex );
	// void                        DrawSprite( IMesh* mesh, VkCommandBuffer c, uint32_t commandBufferIndex );

	// Awful Mesh Functions, here until i abstract what's used in it
	void                        MeshInit( IMesh* mesh ) override;
	void                        MeshReInit( IMesh* mesh ) override;
	void                        MeshFreeOldResources( IMesh* mesh ) override;
	void                        MeshDestroy( IMesh* mesh ) override;

private:
	std::unordered_map< std::string, BaseShader* >          aShaders;

	std::unordered_map< size_t, UniformDescriptor >         aUniformDataMap;
	std::unordered_map< size_t, VkDescriptorSetLayout >     aUniformLayoutMap;

	std::vector< BaseRenderable* > aDrawList;

	std::vector< Material* > aMaterials;
	Material* apErrorMaterial = nullptr;
};

extern MaterialSystem* materialsystem;

