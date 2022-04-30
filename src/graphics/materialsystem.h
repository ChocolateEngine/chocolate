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


// uh
static inline VkVertexInputBindingDescription Vertex3D_GetBindingDesc()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof( vertex_3d_t );
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	return bindingDescription;
}


static inline std::array< VkVertexInputAttributeDescription, 3 > Vertex3D_GetAttributeDesc()
{
	std::array< VkVertexInputAttributeDescription, 3 >attributeDescriptions{  };
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = VertexElement_Position;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof( vertex_3d_t, pos );

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = VertexElement_Normal;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof( vertex_3d_t, normal );

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = VertexElement_TexCoord;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof( vertex_3d_t, texCoord );

	return attributeDescriptions;
}


struct UniformData
{
	VkDescriptorSetLayout   aLayout;
	UniformDescriptor       aDesc;
};


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
	IMaterial*	                CreateMaterial( const std::string& srShader ) override;

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
	void                        CreateVertexBuffer( IRenderable* renderable ) override;
	void                        CreateIndexBuffer( IRenderable* renderable ) override;

	/* Check if a Renderable has a Vertex and/or Index buffer. */
	bool                        HasVertexBuffer( IRenderable* renderable ) override;
	bool                        HasIndexBuffer( IRenderable* renderable ) override;

	/* Free a Vertex and Index buffer for a Renderable. */
	void                        FreeVertexBuffer( IRenderable* renderable ) override;
	void                        FreeIndexBuffer( IRenderable* renderable ) override;

	// Free all buffers from this renderable
	void                        FreeAllBuffers( IRenderable* renderable ) override;

	// New buffer methods
	// void                        CreateBuffers( BaseRenderable* renderable, RenderableBufferFlags flags ) override;
	// bool                        HasBuffers( BaseRenderable* renderable, RenderableBufferFlags flags ) override;
	// void                        FreeBuffers( BaseRenderable* renderable, RenderableBufferFlags flags ) override;

	/* Get Vector of Buffers for a Renderable. */
	const std::vector< RenderableBuffer* >& GetRenderBuffers( IRenderable* renderable );

	// Does a buffer exist with these flags for this renderable already?
	bool                        HasBufferInternal( IRenderable* renderable, VkBufferUsageFlags flags );

	void                        ReInitSwapChain();
	void                        DestroySwapChain();

	// BLECH
	void                        InitUniformBuffer( IRenderable* mesh );

	// inline UniformDescriptor&       GetUniformData( size_t id )            { return aUniformDataMap[id]; }
	// inline VkDescriptorSetLayout    GetUniformLayout( size_t id )          { return aUniformLayoutMap[id]; }

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
	void                        RegisterRenderable( IRenderable* renderable ) override;

	// Destroy a Renderable's Vertex and Index buffers, and anything else it may use
	void                        DestroyRenderable( IRenderable* renderable ) override;

	// Add a Renderable to be drawn next frame, list is cleared after drawing
	void                        AddRenderable( IRenderable* renderable ) override;
	void                        AddRenderable( IRenderable* renderable, const RenderableDrawData& srDrawData ) override;

	// Bind Vertex/Index Buffers of a Renderable (idk if i should have this here but oh well)
	void                        BindRenderBuffers( IRenderable* renderable, VkCommandBuffer c, uint32_t cIndex );

	// Draw a renderable (just calls shader draw lmao)
	void                        DrawRenderable( size_t renderableIndex, IRenderable* renderable, size_t matIndex, const RenderableDrawData& srDrawData, VkCommandBuffer c, uint32_t commandBufferIndex );

	// Awful Mesh Functions, here until i abstract what's used in it
	void                        MeshInit( Model* mesh ) override;
	void                        MeshReInit( Model* mesh ) override;
	void                        MeshFreeOldResources( IRenderable* mesh ) override;

	VkFormat                    ToVkFormat( ColorFormat colorFmt );

	VkFormat                    GetVertexElementVkFormat( VertexElement element );
	ColorFormat                 GetVertexElementFormat( VertexElement element ) override;
	size_t                      GetVertexElementSize( VertexElement element ) override;


	VkSampler                  *apSampler;

	VkDescriptorSetLayout       aImageLayout;
	Sets                        aImageSets;

	VkDescriptorSetLayout       aUniformLayout;
	Sets                        aUniformSets;

	// TODO: remove this
	std::unordered_map< size_t, std::vector< UniformData > > aUniformMap;
	// std::unordered_map< size_t, VkDescriptorSetLayout >     aUniformLayoutMap;

	std::vector< ITextureLoader* >                          aTextureLoaders;
	
private:
	std::unordered_map< std::string, BaseShader* >          aShaders;

	std::vector< IRenderable* >                             aRenderables;

	std::unordered_map< size_t, std::vector< RenderableBuffer* > >   aRenderBuffers;

	std::unordered_map<
		BaseShader*,
		// renderable, material index to draw, and draw data (WHY)
		std::forward_list< std::tuple< IRenderable*, size_t, RenderableDrawData > >
	> aDrawList;

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

