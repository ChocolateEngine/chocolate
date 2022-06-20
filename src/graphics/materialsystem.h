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


struct VertexBuffer
{
	VkBufferUsageFlags              aFlags     = 0;

	std::vector< VkBuffer >         aBuffers;
	std::vector< VkDeviceMemory >   aBufferMem;
	std::vector< VkDeviceSize >     aOffsets;

	inline void Bind( VkCommandBuffer c )
	{
		// NOTE: maybe you could make use of this first binding thing?
		vkCmdBindVertexBuffers( c, 0, aBuffers.size(), aBuffers.data(), aOffsets.data() );
	}

	inline void Free()
	{
		for ( size_t v = 0; v < aBuffers.size(); v++ )
		{
			vkDestroyBuffer( DEVICE, aBuffers[v], nullptr );
			vkFreeMemory( DEVICE, aBufferMem[v], nullptr );
		}

		aBuffers.clear();
		aBufferMem.clear();
		aOffsets.clear();
	}
};


struct IndexBuffer
{
	VkBufferUsageFlags      aFlags     = 0;
	VkBuffer                aBuffer    = nullptr;
	VkDeviceMemory          aBufferMem = nullptr;
	// VkDeviceSize            aOffset    = 0;
	VkIndexType             aIndexType = VK_INDEX_TYPE_UINT32;

	inline void Bind( VkCommandBuffer c )
	{
		// NOTE: in godot when vkCmdDrawIndexed is called, it uses the offset stored in the index buffer
		// so, how does this offset actually work anyway?
		vkCmdBindIndexBuffer( c, aBuffer, 0, aIndexType );
	}

	inline void Free()
	{
		if ( aBuffer )
			vkDestroyBuffer( DEVICE, aBuffer, nullptr );

		if ( aBufferMem )
			vkFreeMemory( DEVICE, aBufferMem, nullptr );

		aBuffer = nullptr;
		aBufferMem = nullptr;
	}
};


// two other types to put here eventually:
// - shader storage buffers
// - uniform buffers


// Data for each mesh surface
struct InternalMeshData_t
{
	// std::vector< VertexBuffer >     aVertexBuffers;
	// std::vector< IndexBuffer >      aIndexBuffers;

	VertexBuffer*    apVertexBuffer;
	IndexBuffer*     apIndexBuffer;
	
	// TODO: figure out a better way to do this
	// for custom vertex buffers modified by a compute shader
	VertexBuffer*    apCustomVertexBuffer;
};

constexpr VkBufferUsageFlags gVertexBufferFlags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
constexpr VkBufferUsageFlags gIndexBufferFlags  = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;


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

	// Create a Vertex and Index buffer for a Renderable.
	void                        CreateVertexBuffer( IModel* renderable, size_t surface ) override;
	void                        CreateIndexBuffer( IModel* renderable, size_t surface ) override;

	void                        CreateVertexBuffers( IModel* renderable ) override;
	void                        CreateIndexBuffers( IModel* renderable ) override;

	void                        CreateVertexBufferInt( IModel* renderable, size_t surface, InternalMeshData_t& meshData );
	void                        CreateIndexBufferInt( IModel* renderable, size_t surface, InternalMeshData_t& meshData );

	// Check if a Renderable has a Vertex and/or Index buffer. 
	bool                        HasVertexBuffer( IModel* renderable, size_t surface ) override;
	bool                        HasIndexBuffer( IModel* renderable, size_t surface ) override;

	// Free a Vertex and Index buffer for a Renderable.
	void                        FreeVertexBuffer( IModel* renderable, size_t surface ) override;
	void                        FreeIndexBuffer( IModel* renderable, size_t surface ) override;

	void                        FreeVertexBuffers( IModel* renderable ) override;
	void                        FreeIndexBuffers( IModel* renderable ) override;

	// Free all buffers from this renderable
	void                        FreeAllBuffers( IModel* renderable ) override;

	// New buffer methods
	// void                        CreateBuffers( BaseRenderable* renderable, RenderableBufferFlags flags ) override;
	// bool                        HasBuffers( BaseRenderable* renderable, RenderableBufferFlags flags ) override;
	// void                        FreeBuffers( BaseRenderable* renderable, RenderableBufferFlags flags ) override;

	/* Get Internal Mesh Data for a Renderable. */
	const std::vector< InternalMeshData_t >& GetMeshData( IModel* renderable );

	void                        OnReInitSwapChain();
	void                        OnDestroySwapChain();

	// BLECH
	void                        InitUniformBuffer( IModel* mesh ) override;

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
	// Handle                      RegisterRenderable( IModel* renderable ) override;

	// Destroy a Renderable's Vertex and Index buffers, and anything else it may use
	void                        DestroyModel( IModel* renderable ) override;

	// Add a Renderable to be drawn next frame, list is cleared after drawing
	void                        AddRenderable( IRenderable* renderable ) override;

	// Draw a renderable (just calls shader draw lmao)
	void                        DrawRenderable( size_t renderableIndex, IRenderable* renderable, size_t matIndex, VkCommandBuffer c, uint32_t commandBufferIndex );

	// Awful Mesh Functions, here until i abstract what's used in it
	void                        MeshFreeOldResources( IModel* mesh ) override;

	VkFormat                    ToVkFormat( GraphicsFormat colorFmt );

	VkFormat                    GetVertexAttributeVkFormat( VertexAttribute attrib );
	GraphicsFormat              GetVertexAttributeFormat( VertexAttribute attrib ) override;
	size_t                      GetVertexAttributeTypeSize( VertexAttribute attrib ) override;
	size_t                      GetVertexAttributeSize( VertexAttribute attrib ) override;
	size_t                      GetVertexFormatSize( VertexFormat format ) override;

	// size_t                      GetFormatSize( DataFormat format ) override;

	// VkVertexInputBindingDescription GetVertexBindingDesc( VertexFormat format );
	void                            GetVertexBindingDesc( VertexFormat format, std::vector< VkVertexInputBindingDescription >& srAttrib );
	void                            GetVertexAttributeDesc( VertexFormat format, std::vector< VkVertexInputAttributeDescription >& srAttrib );

	// -----------------------------------------------------------------------------------------

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

	std::vector< IModel* >                             aRenderables;

	std::unordered_map< size_t, std::vector< InternalMeshData_t > > aMeshData;

	std::unordered_map<
		BaseShader*,
		// renderable, material index to draw (WHY)
		std::forward_list< std::tuple< IRenderable*, size_t > >
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


// remove me
static inline void Vertex3D_GetBindingDesc( std::vector< VkVertexInputBindingDescription >& srDescs )
{
	return matsys->GetVertexBindingDesc( VertexFormat_Position | VertexFormat_Normal | VertexFormat_TexCoord, srDescs );
}

