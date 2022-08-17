/*
 *    graphics.h    --    Header for Chocolate Graphics
 *
 *    Authored by Karl "p0lyh3dron" Kreuze on February 24, 2022
 *
 *    This header declares the graphics API and all of its
 *    functions.
 */
#pragma once

#include "core/core.h"

#include "graphics/igraphics2.h"

LOG_CHANNEL( Graphics2 );

class Texture2;
class RenderGraph;
class RenderGraphPass;
class CommandBufferHelper;

struct MaterialVk;


struct VertexBuffer_t
{
	std::vector< VkBuffer >       aBuffers;
	std::vector< VkDeviceMemory > aDeviceMemory;
};


struct IndexBuffer_t
{
	VkBuffer        aBuffer;
	VkIndexType     aIndexType = VK_INDEX_TYPE_UINT32;

	// should be somewhere else, since we would be putting it in the cpu cache despite not using it the majority of the time
	VkDeviceMemory  aBufferMem;
};


struct ModelVk
{
	struct Surface
	{
		// hmm, could be slow to access this
		HMaterial aMaterial;

		Handle    aVertexBuffer;
		u32       aVertexOffset;
		u32       aVertexCount;
		
		// you could have an option to have u16 indices or u32 indices, but im unsure how to implement it cleanly here
		Handle    aIndexBuffer;
		u32       aIndexOffset;
		u32       aIndexCount;
	};

	std::vector< Surface > aSurfaces;
};


// model data for rendering purposes only
// if im understanding this correctly, 
// this entire thing should be able to be fit into the L1 cache of the cpu
// so in theory, no random memory accessors
struct GeometryRenderData_t
{
	// MaterialVk*         apMaterial;

	// Shader Info
	// maybe use a handle to the compiled shader material data? idk
	// cause each shader will probably have their own material data
	// and a handle would allow for any shader data type to be here
	
	

	// Vertex Buffer Info
	std::vector< VkBuffer >     aVertexBuffers;
	u32                         aFirstBinding;
	std::vector< VkDeviceSize > aVertexOffsets;
	
	// Index Buffer Info
	VkBuffer     aIndexBuffer;
	VkIndexType  aIndexType = VK_INDEX_TYPE_UINT32;
	VkDeviceSize aIndexOffset;
};


class Graphics : public IGraphics
{
protected:  
public:	

	// --------------------------------------------------------------------------------------------
	// BaseSystem Functions
	// --------------------------------------------------------------------------------------------

	void             Init() override;
	void             Update( float sDT ) override;
	
	// --------------------------------------------------------------------------------------------
	// General Purpose Functions
	// --------------------------------------------------------------------------------------------

	/*
     *    Returns the surface that the Graphics system is using.
     *
     *    @return Surface *    The surface that the Graphics system is using.
     */
	Window          *GetSurface() override;
	
    /*
     *    Draws a line from one point to another.
     *
     *    @param glm::vec3    The starting point.
     *    @param glm::vec3    The ending point.
     *    @param glm::vec3    The color of the line.
     */
    void             DrawLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor ) override;

	// --------------------------------------------------------------------------------------------
	// Built-in Model Support
	// --------------------------------------------------------------------------------------------

	/*
     *    Loads a model from a path.
     *
     *    @param const std::string &    The path to the model.
     *
     *    @return HModel                The handle to the model.
     */
	HModel           LoadModel( const std::string& srModelPath ) override;

	/*
	 *    Register a custom model
	 *
	 *    @return HModel                The handle to the model.
	 */
	HModel           RegisterModel( Model_t* spModel ) override;

	/*
     *    Frees a model.
     *
     *    @param HModel     The handle to the model.
     */
	void 		     FreeModel( HModel sModel ) override;

	// --------------------------------------------------------------------------------------------
	// Materials and Textures
	// --------------------------------------------------------------------------------------------

	/*
	 *    Creates a material.
	 *
	 *	  @return HMaterial    The handle to the material.
	 */
	HMaterial        CreateMat() override;

	/*
	 *    Frees a material.
	 *
	 *	  @param HMaterial    The handle to the material.
	 */
	void 		     FreeMat( HMaterial sMat ) override;

	/*
	 *    Loads a material.
	 *
	 *	  @param const std::string &     The path to the material.
	 *
	 *	  @return HMaterial              The handle to the material.
	 */
	HMaterial        LoadMat( const std::string& srMatPath ) override;

	/*
	 *    Creates a texture from a file located by path specified.
	 *
	 *    @param const std::string &    The path to the texture.
	 * 
	 *	  @return HTexture              The handle to the texture.
	 */
	HTexture         LoadTexture( const std::string& srTexturePath ) override;
	
	/*
	 *   Creates a texture from pixel data.
	 *
	 *   @param TextureCreateInfo_t&    The information about the texture.
	 */
	HTexture         CreateTexture( const TextureCreateInfo_t& srTextureCreateInfo ) override;

	/*
	 *    Frees a texture.
	 *
	 *    @param HTexture    The handle to the texture.
	 */
	void 		     FreeTexture( HTexture sTexture ) override;

	// --------------------------------------------------------------------------------------------
	// Render List
	// --------------------------------------------------------------------------------------------
	
	/*
	 *    Creates a render graph.
	 *
	 *	  @return bool    Whether the render graph was successfully created or not.
	 */
	bool             CreateRenderGraph() override;
	
	/*
	 *    Marks the current render graph as finished and queues it for drawing during the GPU execution stage.
	 * 
	 *    @return bool    Whether the render graph was successfully baked or not.
	 */
	bool             FinishRenderGraph() override;

	/*
	 *    Creates a render pass.
	 * 
	 *    @param const std::string&        The render pass name
	 *    @param RenderGraphQueueFlagBits  The render pass type
	 *
	 *	  @return HRenderPass    The handle to the render pass.
	 */
	HRenderPass      CreateRenderPass( const std::string& srName, RenderGraphQueueFlagBits sStage ) override;
	
	/*
	 *    Frees a render pass.
	 *
	 *    @param HRenderPass    The handle to the render pass.
	 */
	void 		     FreeRenderPass( HRenderPass sRenderPass ) override;

	/*
	 *    Adds a render pass to the render graph.
	 * 
	 *    @param HRenderPass    The handle to the render pass.
	 */
	void 		     AddRenderPass( HRenderPass sRenderPass ) override;
	
	/*
	 *    Adds a model to a render pass.
	 *
	 *    @param HRenderList    The handle to the render list.
	 *    @param HModel         The handle to the model.
	 *    @param glm::mat4      The model matrix.
	 */
	void             AddModel( HRenderPass sRenderPass, HModel sModel, const glm::mat4& sModelMatrix ) override;
	
	/*
	 *    Adds a camera to a render pass.
	 * 
	 *    @param HRenderList      The handle to the render list.
	 *    @param ViewportCamera_t The view to add.
	 */
	void             AddViewportCamera( HRenderPass sRenderPass, const ViewportCamera_t& sViewportCamera ) override;
	
private:

	/*
	*    Initializes ImGui.
	*/
	void             InitImGui();

	// helper function to check for valid render graph and render pass
	RenderGraphPass* GetRenderGraphPass( HRenderPass sRenderPass, const char* sFunc, const char* sError );
	
	// TEMP TEMP TEMP TEMP !!!!!!!!!!!!!!!!!!!!!
	void             AllocImageSets();
	void             UpdateImageSets();

public:
	std::unordered_map< std::string, HModel >    aModelPaths;
	std::unordered_map< std::string, HMaterial > aMatPaths;

	ResourceManager< Model_t >             aModels;   // public model format
	std::unordered_map< Model_t, ModelVk > aModelVk;  // internal rendering model format
	
	// all model vertex/index buffers are stored here
	ResourceManager< VertexBuffer_t > aVertexBuffers;
	ResourceManager< IndexBuffer_t >  aIndexBuffers;
	
	HTexture  aMissingTexHandle = InvalidHandle;
	Texture2* apMissingTex = nullptr;

	std::unordered_map< std::string, HTexture > aTexturePaths;
	ResourceManager< Texture2 >                 aTextures;

	// All render passes created
	ResourceManager< RenderGraphPass >          aPasses;

	// Render Graph being built by the game code
	RenderGraph*                                apRenderGraph = nullptr;

	// Render Graph being drawn by the graphics system
	RenderGraph*                                apRenderGraphDraw = nullptr;
	
	CommandBufferHelper                         aCmdHelper;
	
	// TODO: MOVE THIS TO DESCRIPTOR MANAGER (PROBABLY)
	std::vector< VkDescriptorSet > aImageSets;
	VkDescriptorSetLayout          aImageLayout;
};

extern Graphics graphics;

