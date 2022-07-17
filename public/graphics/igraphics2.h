#pragma once

#include "system.h"
#include <stdio.h>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>


using HModel        = Handle;
using HMaterial     = Handle;
using HTexture      = Handle;
using HShader       = Handle;

using HRenderGraph  = Handle;
using HRenderPass   = Handle;

using VertexFormat  = u16;

struct SDL_Window;


// -----------------------------------------------------------------------------
// Data Types
// -----------------------------------------------------------------------------


enum class GraphicsFmt
{
	INVALID,

	// -------------------------
	// 8 bit depth

	R8_SRGB,
	R8_SINT,
	R8_UINT,

	RG88_SRGB,
	RG88_SINT,
	RG88_UINT,

	RGB888_SRGB,
	RGB888_SINT,
	RGB888_UINT,

	RGBA8888_SRGB,
	RGBA8888_SINT,
	RGBA8888_UINT,

	// -------------------------
	
	// BGR888_SNORM,
	// BGR888_UNORM,

	// -------------------------
	// 16 bit depth

	R16_SFLOAT,
	R16_SINT,
	R16_UINT,

	RG1616_SFLOAT,
	RG1616_SINT,
	RG1616_UINT,

	RGB161616_SFLOAT,
	RGB161616_SINT,
	RGB161616_UINT,

	RGBA16161616_SFLOAT,
	RGBA16161616_SINT,
	RGBA16161616_UINT,

	// -------------------------
	// 32 bit depth

	R32_SFLOAT,
	R32_SINT,
	R32_UINT,

	RG3232_SFLOAT,
	RG3232_SINT,
	RG3232_UINT,

	RGB323232_SFLOAT,
	RGB323232_SINT,
	RGB323232_UINT,

	RGBA32323232_SFLOAT,
	RGBA32323232_SINT,
	RGBA32323232_UINT,

	// -------------------------
	// GPU Compressed Formats

	BC1_RGB_UNORM_BLOCK,
	BC1_RGB_SRGB_BLOCK,
	BC1_RGBA_UNORM_BLOCK,
	BC1_RGBA_SRGB_BLOCK,

	BC2_UNORM_BLOCK,
	BC2_SRGB_BLOCK,

	BC3_UNORM_BLOCK,
	BC3_SRGB_BLOCK,

	BC4_UNORM_BLOCK,
	BC4_SNORM_BLOCK,

	BC5_UNORM_BLOCK,
	BC5_SNORM_BLOCK,

	BC6H_UFLOAT_BLOCK,
	BC6H_SFLOAT_BLOCK,

	BC7_UNORM_BLOCK,
	BC7_SRGB_BLOCK,
};


enum TextureType
{
	TextureType_1D,
	TextureType_2D,
	TextureType_3D,
	TextureType_Cube,
	TextureType_1DArray,
	TextureType_2DArray,
	TextureType_CubeArray,

	TextureType_Count = TextureType_CubeArray
};


enum TextureSamples
{
	TextureSamples_1,
	TextureSamples_2,
	TextureSamples_4,
	TextureSamples_8,
	TextureSamples_16,
	TextureSamples_32,
	TextureSamples_64,
	TextureSamples_Max,
};


// Always in order from top to bottom in terms of order in each vertex
// technically, you could use the above only
enum VertexAttribute : u8
{
	VertexAttribute_Position,         // vec3
	VertexAttribute_Normal,           // vec3
	VertexAttribute_TexCoord,         // vec2
	VertexAttribute_Color,            // vec3 (should be vec4 probably)

	// this and morphs will be calculated in a compute shader
	// VertexAttribute_BoneIndex,        // uvec4
	// VertexAttribute_BoneWeight,       // vec4

	VertexAttribute_Count
};


// Flags to Determine what the Vertex Data contains
enum : VertexFormat
{
	VertexFormat_None           = 0,
	VertexFormat_Position       = (1 << VertexAttribute_Position),
	VertexFormat_Normal         = (1 << VertexAttribute_Normal),
	VertexFormat_TexCoord       = (1 << VertexAttribute_TexCoord),
	VertexFormat_Color          = (1 << VertexAttribute_Color),
};


// used in render resources to mark where it's going to be used
enum RenderGraphQueueFlagBits
{
	RenderGraphQueue_None           = 0,
	RenderGraphQueue_Graphics       = (1 << 0),
	RenderGraphQueue_Compute        = (1 << 1),
	RenderGraphQueue_AsyncGraphics  = (1 << 2),
	RenderGraphQueue_AsyncCompute   = (1 << 3),
};

using RenderGraphQueueFlags = u32;


// -----------------------------------------------------------------------------
// Structs
// -----------------------------------------------------------------------------


struct Window 
{
    uint32_t         aWidth;
    uint32_t         aHeight;
    SDL_Window      *apWindow;
};


struct TextureCreateInfo_t
{
	u32 aWidth;
	u32 aHeight;
	void* apData;

	// TODO: give these two actual enums
	u32 aFormat;
	u32 aType;
};


struct Viewport_t
{
	float x;
	float y;
	float width;
	float height;
	float minDepth;
	float maxDepth;
};


struct ViewportCamera_t
{
	void ComputeProjection( float sWidth, float sHeight )
	{
		const float hAspect = (float)sWidth / (float)sHeight;
		const float vAspect = (float)sHeight / (float)sWidth;

		float V = 2.0f * atanf( tanf( glm::radians( aFOV ) / 2.0f ) * vAspect );

		aProjMat = glm::perspective( V, hAspect, aNearZ, aFarZ );

		aProjViewMat = aProjMat * aViewMat;
	}

	float aNearZ;
	float aFarZ;
	float aFOV;

	glm::mat4 aViewMat;
	glm::mat4 aProjMat;

	// projection matrix * view matrix 
	glm::mat4 aProjViewMat;
};


struct RenderTarget_t
{
	u32 aWidth;
	u32 aHeight;
	u32 aFormat;
	u32 aType;
	void* apData;
};


struct VertAttribData_t
{
	VertexAttribute aAttrib = VertexAttribute_Position;
	void*           apData = nullptr;

	VertAttribData_t() {}

	~VertAttribData_t()
	{
		if ( apData )
			free( apData );

		apData = nullptr;
	}

	// REALLY should be uncommented but idc right now
	// private:
	// 	VertAttribData_t( const VertAttribData_t& other );
};


struct VertexData_t
{
	VertexFormat                        aFormat;
	u32                                 aCount;
	std::vector< VertAttribData_t >     aData;
	bool                                aLocked;  // if locked, we can create a vertex buffer
};


struct Model_t
{
	struct Surface
	{
		HMaterial aMaterial;

		// HVertexBuffer aVertexBuffer;
		// HIndexBuffer aIndexBuffer;

		// NOTE: these could be a pointer, and multiple surfaces could share the same vertex data and indices 
		VertexData_t                    aVertexData;
		std::vector< uint32_t >         aIndices;
	};

	std::vector< Surface* > aSurfaces{};
};


// potential idea for the future for adding a software renderer
enum GraphicsSupportFlagBits : u32
{
	GraphicsSupport_None = 0,
	// GraphicsSupport_Shaders = (1 << 0),
	// GraphicsSupport_Buffers = (1 << 1),
	// GraphicsSupport_Textures = (1 << 2),
	// GraphicsSupport_RenderTargets = (1 << 3),
	// GraphicsSupport_Viewports = (1 << 4),
	// GraphicsSupport_ViewportsCamera = (1 << 5),
	// GraphicsSupport_RenderGraph = (1 << 6),
	// GraphicsSupport_All = 0xFFFFFFFF,
};

using GraphicsSupportFlags = u32;

// use this function to check:
// bool ret = graphics->Supports( GraphicsSupport_Something | GraphicsSupport_Something2 );



class IGraphics : public BaseSystem
{
protected:
public:
	// --------------------------------------------------------------------------------------------
	// General Purpose Functions
	// --------------------------------------------------------------------------------------------

	/*
	*    Returns the surface that the Graphics system is using.
	*
	*    @return Surface *    The surface that the Graphics system is using.
	*/
	virtual Window          *GetSurface() = 0;
	
	/*
     *    Draws a line from one point to another.
     *
     *    @param glm::vec3    The starting point.
     *    @param glm::vec3    The ending point.
     *    @param glm::vec3    The color of the line.
     */
    virtual void             DrawLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor ) = 0;
	
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
	virtual HModel           LoadModel( const std::string& srModelPath ) = 0;

	/*
	 *    Register a custom model
	 *
	 *    @return HModel                The handle to the model.
	 */
	virtual HModel           RegisterModel( Model_t* spModel ) = 0;

	/*
     *    Frees a model.
     *
     *    @param HModel     The handle to the model.
     */
	virtual void 		     FreeModel( HModel sModel ) = 0;
	
	// --------------------------------------------------------------------------------------------
	// Materials and Textures
	// --------------------------------------------------------------------------------------------

	/*
	 *    Creates a material.
	 *
	 *	  @return HMaterial    The handle to the material.
	 */
	virtual HMaterial        CreateMat() = 0;

	/*
	 *    Frees a material.
	 *
	 *	  @param HMaterial    The handle to the material.
	 */
	virtual void 		     FreeMat( HMaterial sMat ) = 0;

	/*
	 *    Loads a material.
	 *
	 *	  @param const std::string &     The path to the material.
	 *
	 *	  @return HMaterial              The handle to the material.
	 */
	virtual HMaterial        LoadMat( const std::string& srMatPath ) = 0;

	/*
	 *    Creates a texture from a file located by path specified.
	 *
	 *    @param const std::string &    The path to the texture.
	 * 
	 *	  @return HTexture              The handle to the texture.
	 */
	virtual HTexture         LoadTexture( const std::string& srTexturePath ) = 0;
	
	/*
	 *   Creates a texture from pixel data.
	 *
	 *   @param TextureCreateInfo_t&    The information about the texture.
	 */
	virtual HTexture         CreateTexture( const TextureCreateInfo_t& srTextureCreateInfo ) = 0;

	/*
	 *    Frees a texture.
	 *
	 *    @param HTexture    The handle to the texture.
	 */
	virtual void 		     FreeTexture( HTexture sTexture ) = 0;

	// --------------------------------------------------------------------------------------------
	// Shader System
	// --------------------------------------------------------------------------------------------

	/*
	 *    Creates a shader.
	 *
	 *	  @return HShader    The handle to the shader.
	 */
	// virtual HShader          CreateShader( const std::string& srShaderPath ) = 0;
	
	// --------------------------------------------------------------------------------------------
	// Render Graph System
	// --------------------------------------------------------------------------------------------

	// instead, have CreateRenderGraph return a bool
	// so the game never needs to really pass around a render graph handle,
	// it's not like it will ever be using multiple handles at once
	// and to "finish" a render graph, you just need to call FinishRenderGraph
	// and it could either
	// A: return a handle to the render graph that you can use to render
	// B: add it to a queue of finished render graphs internally used during the gpu execution frame stage
	// though only one item can be waiting at a time
	// 
	// if we have a new rendergraph completed, but there is still a rendergraph waiting, we can hold off on rendering the new one
	// or we could swap it out, not sure what that would look like, but whatever
	// 
	// so we can have a queue of render graphs to render, and when we have a new one, we can add it to the queue
	// 
	// or, maybe there is no queue at all, and just a single pointer storing the next render graph
	//
	
	/*
	 *    Creates a render graph.
	 *
	 *	  @return bool    Whether the render graph was successfully created or not.
	 */
	virtual bool             CreateRenderGraph() = 0;
	
	/*
	 *    Marks the current render graph as finished and queues it for drawing during the GPU execution stage.
	 * 
	 *    @return bool    Whether the render graph was successfully baked or not.
	 */
	virtual bool 		     FinishRenderGraph() = 0;

	/*
	 *    Creates a render pass in the current render graph.
	 * 
	 *    @param const std::string&        The render pass name
	 *    @param RenderGraphQueueFlagBits  The render pass type
	 *
	 *	  @return HRenderPass    The handle to the render pass.
	 */
	virtual HRenderPass      CreateRenderPass( const std::string& srName, RenderGraphQueueFlagBits sStage ) = 0;
	
	/*
	 *    Adds a model to a render pass.
	 *
	 *    @param HRenderPass    The handle to the render pass.
	 *    @param HModel         The handle to the model.
	 *    @param glm::mat4      The model matrix.
	 */
	virtual void             AddModel( HRenderPass sRenderPass, HModel sModel, const glm::mat4& sModelMatrix ) = 0;
	
	/*
	 *    Adds a camera to a render pass.
	 * 
	 *    @param HRenderPass      The handle to the render pass.
	 *    @param ViewportCamera_t The view to add.
	 */
	virtual void             AddViewportCamera( HRenderPass sRenderPass, const ViewportCamera_t& sViewportCamera ) = 0;
	
#if 0
	/*
	 *    Adds a node to a render graph.
	 *
	 *	  @param HRenderGraph    The handle to the render graph.
	 *	  @param HNode          The handle to the node.
	 */
	virtual void 		     AddNode( HRenderGraph sRenderGraph, HNode sNode ) = 0;
	
	/*
	 *    Removes a node from a render graph.
	 *
	 *	  @param HRenderGraph    The handle to the render graph.
	 *	  @param HNode          The handle to the node.
	 */
	virtual void 		     RemoveNode( HRenderGraph sRenderGraph, HNode sNode ) = 0;
	
	/*
	 *    Adds a pass to a render graph.
	 *
	 *	  @param HRenderGraph    The handle to the render graph.
	 *	  @param HRenderPass    The handle to the pass.
	 */
	virtual void 		     AddPass( HRenderGraph sRenderGraph, HRenderPass sPass ) = 0;
	
	/*
	 *    Removes a pass from a render graph.
	 *
	 *	  @param HRenderGraph    The handle to the render graph.
	 *	  @param HRenderPass    The handle to the pass.
	 */
	virtual void 		     RemovePass( HRenderGraph sRenderGraph, HRenderPass sPass ) = 0;
	
	/*
	 *    Adds a target to a render graph.
	 *
	 *	  @param HRenderGraph    The handle to the render graph.
	 *	  @param HRenderTarget  The handle to the target.
	 */
	virtual void 		     AddTarget( HRenderGraph sRenderGraph, HRenderTarget sTarget ) = 0;
	
	/*
	 *    Removes a target from a render graph.
	 *
	 *	  @param HRenderGraph    The handle to the render graph.
	 *	  @param HRenderTarget  The handle to the target.
	 */
	virtual void 		     RemoveTarget( HRenderGraph sRenderGraph, HRenderTarget sTarget ) = 0;
	
	/*
	 *    Adds a queue to a render graph.
	 *
	 *	  @param HRenderGraph    The handle to the render graph.
	 *	  @param HRenderQueue   The handle to the queue.
	 */
	virtual void 		     AddQueue( HRenderGraph sRenderGraph, HRenderQueue sQueue ) = 0;
	
	/*
	 *    Removes a queue from a render graph.
	 *
	 *	  @param HRenderGraph    The handle to the render graph.
	 *	  @param HRenderQueue   The handle to the queue.
	 */
	virtual void 		     RemoveQueue( HRenderGraph sRenderGraph, HRenderQueue sQueue ) = 0;

	/*
	 *    Adds a pass to a queue.
	 *
	 *	  @param HRenderGraph    The handle to the render graph.
	 *	  @param HRenderQueue   The handle to the queue.
	 *	  @param HRenderPass    The handle to the pass.
	 */
	virtual void 		     AddPassToQueue( HRenderGraph sRenderGraph, HRenderQueue sQueue, HRenderPass sPass ) = 0;
	
	/*
	 *    Removes a pass from a queue.
	 *
	 *	  @param HRenderGraph    The handle to the render graph.
	 *	  @param HRenderQueue   The handle to the queue.
	 *	  @param HRenderPass    The handle to the pass.
	 */
	virtual void 		     RemovePassFromQueue( HRenderGraph sRenderGraph, HRenderQueue sQueue, HRenderPass sPass ) = 0;
#endif
};

