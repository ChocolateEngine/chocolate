/*
materialsystem.h ( Authored by Demez )

Manages all materials and shaders.
TODO: move this to graphics level abstraction so the game can use this
*/

#pragma once

#include "../../types/databuffer.hh"
#include "shaders.h"
#include "allocator.h"
#include "material.h"

class MaterialSystem
{
public:

	MaterialSystem();
	~MaterialSystem() {}

	friend class Renderer;

	void                        Init();

	Material*	                CreateMaterial();
	void                        DeleteMaterial( Material* mat );

	Material*	                GetErrorMaterial() { return apErrorMaterial; }

	// ideally this shouldn't need a path, should really be reworked to be closer to thermite, more flexible design
	// also this probably shouldn't use a shader for a parameter
	TextureDescriptor*			CreateTexture( Material* material, const std::string path );

	/* Create a Vertex and Index buffer for a Mesh. */
	void                        CreateVertexBuffer( Mesh* mesh );
	void                        CreateIndexBuffer( Mesh* mesh );

	void                        ReInitSwapChain();
	void                        DestroySwapChain();

	BaseShader*                 GetShader( const std::string& name );

	template <typename T>
	BaseShader* CreateShader( const std::string& name )
	{
		// check if we already made this shader
		if ( BaseShader* shader = GetShader(name) )
			return shader;

		BaseShader* shader = new T;
		shader->aName = name;
		shader->apMaterialSystem = this;
		shader->apRenderer = apRenderer;
		shader->Init();

		aShaders[name] = shader;

		return shader;
	}

private:
	// this REALLY should be a global at this point
	// we don't need to be passing this around to like everything in here
	Renderer* apRenderer = nullptr;

	std::unordered_map< std::string, BaseShader* > aShaders;

	std::vector< Material* > aMaterials;
	Material* apErrorMaterial = nullptr;
};

