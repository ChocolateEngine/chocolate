#pragma once

#include <unordered_set>
#include "igraphics.h"

#include <map>


LOG_CHANNEL( ClientGraphics )


// KEEP IN SYNC WITH core.glsl
constexpr u32 CH_R_MAX_TEXTURES                   = 4096;
constexpr u32 CH_R_MAX_VIEWPORTS                  = 32;
constexpr u32 CH_R_MAX_RENDERABLES                = 4096;
constexpr u32 CH_R_MAX_MATERIALS                  = 64;  // max materials per renderable

constexpr u32 CH_R_MAX_VERTEX_BUFFERS             = CH_R_MAX_RENDERABLES * 2;
constexpr u32 CH_R_MAX_INDEX_BUFFERS              = CH_R_MAX_RENDERABLES;
constexpr u32 CH_R_MAX_BLEND_SHAPE_WEIGHT_BUFFERS = CH_R_MAX_RENDERABLES;
constexpr u32 CH_R_MAX_BLEND_SHAPE_DATA_BUFFERS   = CH_R_MAX_RENDERABLES;

constexpr u32 CH_R_MAX_LIGHT_TYPE                 = 256;
constexpr u32 CH_R_MAX_LIGHTS                     = CH_R_MAX_LIGHT_TYPE * ELightType_Count;

constexpr u32 CH_R_LIGHT_LIST_SIZE                = 16;

constexpr u32 CH_BINDING_TEXTURES                 = 0;
constexpr u32 CH_BINDING_CORE                     = 1;
constexpr u32 CH_BINDING_VIEWPORTS                = 2;
constexpr u32 CH_BINDING_RENDERABLES              = 3;
constexpr u32 CH_BINDING_MODEL_MATRICES           = 4;
constexpr u32 CH_BINDING_VERTEX_BUFFERS           = 5;
constexpr u32 CH_BINDING_INDEX_BUFFERS            = 6;


// Contains a built list of renderable surfaces to draw this frame, grouped by shader
struct ViewRenderList_t
{
	// Viewport ch_handle_t
	u32                                                         aHandle;

	// TODO: needs improvement and further sorting
	// [ Shader ] = vector of surfaces to draw
	std::unordered_map< ch_handle_t, ChVector< SurfaceDraw_t > > aRenderLists;

	// Lights in the scene
	//ChVector< Light_t* >                                        aLights;

	//ch_handle_t                                                  aDebugDrawRenderable = CH_INVALID_HANDLE;
};


struct GraphicsSceneViewport_t
{
	u32                    viewport;
	ChVector< ch_handle_t > renderables;
};


struct RenderScene_t
{
	// list of lights in a scene
	ChVector< Light_t* >                lights;

	// multiple viewports in a scene
	// ChVector< GraphicsSceneViewport_t > viewports;

	ChVector< ViewRenderList_t >        renderLists;
};


struct ShaderDescriptorData_t
{
	// Descriptor Set Layouts for the Shader Slots
	// ch_handle_t                                           aLayouts[ EShaderSlot_Count ];
	ch_handle_t                                                 aGlobalLayout;

	// Descriptor Sets Used by all shaders
	ShaderDescriptor_t                                         aGlobalSets;

	// Sets per render pass
	// ShaderDescriptor_t                                   aPerPassSets[ ERenderPass_Count ];

	// Shader Descriptor Set Layout, [shader name] = layout
	std::unordered_map< std::string_view, ch_handle_t >         aPerShaderLayout;

	// Sets per shader, [shader handle] = descriptor sets
	std::unordered_map< std::string_view, ShaderDescriptor_t > aPerShaderSets;

	// Sets per renderable, [renderable handle] = descriptor sets
	// std::unordered_map< ch_handle_t, ShaderDescriptor_t > aPerObjectSets;
};


extern ShaderDescriptorData_t gShaderDescriptorData;


// Match core.glsl!
struct Shader_Viewport_t
{
	glm::mat4 aProjView{};
	glm::mat4 aProjection{};
	glm::mat4 aView{};
	glm::vec3 aViewPos{};
	float     aNearZ = 0.f;
	float     aFarZ  = 0.f;

	// padding for shader????
	glm::vec3 weirdPaddingIdk;
};


// shared renderable data
struct Shader_Renderable_t
{
	// alignas( 16 ) u32 aMaterialCount;
	// u32 aMaterials[ CH_R_MAX_MATERIALS ];
	
	u32 aVertexBuffer;
	u32 aIndexBuffer;

	u32 aLightCount;
	// u32 aLightList[ CH_R_LIGHT_LIST_SIZE ];
};


// surface index and renderable index
// unique to each viewport
// basically, "Draw the current renderable surface with this material"
struct Shader_SurfaceDraw_t
{
	u32 aRenderable;  // index into gCore.aRenderables
	u32 aMaterial;    // index into the shader material array
};


// Light Types
struct UBO_LightDirectional_t
{
	alignas( 16 ) glm::vec4 color{};
	alignas( 16 ) glm::vec3 aDir{};
	alignas( 16 ) glm::mat4 aProjView{};
	int aShadow = -1;
};


struct UBO_LightPoint_t
{
	alignas( 16 ) glm::vec4 color{};
	alignas( 16 ) glm::vec3 aPos{};
	float aRadius = 0.f;
	// int   aShadow = -1;
};


struct UBO_LightCone_t
{
	alignas( 16 ) glm::vec4 color{};
	alignas( 16 ) glm::vec3 aPos{};
	alignas( 16 ) glm::vec3 aDir{};
	alignas( 16 ) glm::vec2 aFov{};
	alignas( 16 ) glm::mat4 aProjView{};
	int aShadow = -1;
};


struct UBO_LightCapsule_t
{
	alignas( 16 ) glm::vec4 color{};
	alignas( 16 ) glm::vec3 aPos{};
	alignas( 16 ) glm::vec3 aDir{};
	float aLength    = 0.f;
	float aThickness = 0.f;
};


struct Buffer_Core_t
{
	u32                    aNumTextures      = 0;
	u32                    aNumViewports     = 0;
	u32                    aNumVertexBuffers = 0;
	u32                    aNumIndexBuffers  = 0;
	
	u32                    aNumLights[ ELightType_Count ];

	// NOTE: this could probably be brought down to one light struct, with packed data to unpack in the shader
	UBO_LightDirectional_t aLightWorld[ CH_R_MAX_LIGHT_TYPE ];
	UBO_LightPoint_t       aLightPoint[ CH_R_MAX_LIGHT_TYPE ];
	UBO_LightCone_t        aLightCone[ CH_R_MAX_LIGHT_TYPE ];
	UBO_LightCapsule_t     aLightCapsule[ CH_R_MAX_LIGHT_TYPE ];
};


struct DeviceBufferStaging_t
{
	ch_handle_t aStagingBuffer = CH_INVALID_HANDLE;
	ch_handle_t aBuffer        = CH_INVALID_HANDLE;
	bool       aDirty         = true;
};


struct ModelAABBUpdate_t
{
	Renderable_t* apModelDraw;
	ModelBBox_t   aBBox;
};


// This is for allocating data list indexes for shaders, like a list of viewports
// With this, we can figure out what index to write data to for the shader
// This is used when using one big buffer for a list
// like one buffer for a list of viewports or lights
struct ShaderArrayAllocator_t
{
	const char* apName     = nullptr;
	u32         aAllocated = 0;        // Total amount of handles allocated
	u32         aUsed      = 0;        // How many handles are in use
	u32*        apFree     = nullptr;  // List of empty handles
	u32*        apUsed     = nullptr;  // List of handles in use, Graphics_GetShaderSlot() returns the index of the handle in this list
	bool        aDirty     = false;
};


struct ShaderBufferList_t
{
	// Stores a magic number handle as the key, and buffer handle as the value
	std::unordered_map< u32, ch_handle_t > aBuffers;
	bool                                  aDirty = false;  // ew
};


enum EShaderCoreArray : u32
{
	EShaderCoreArray_LightWorld   = 1,
	EShaderCoreArray_LightPoint   = 2,
	EShaderCoreArray_LightCone    = 3,
	EShaderCoreArray_LightCapsule = 4,

	EShaderCoreArray_Count,
};


// :skull:
struct ShaderSkinning_Push
{
	u32 aRenderable            = 0;
	u32 aSourceVertexBuffer    = 0;
	u32 aVertexCount           = 0;
	u32 aBlendShapeCount       = 0;
	u32 aBlendShapeWeightIndex = 0;
	u32 aBlendShapeDataIndex   = 0;
};


struct ShaderSelect_Push
{
	u32       aRenderable  = 0;
	u32       aViewport    = 0;
	u32       aVertexCount = 0;
	u32       aDiffuse;
	glm::vec4 color{};
	glm::vec2 aCursorPos{};
};


struct ShaderSelectResult_Push
{
	glm::vec2 aCursorPos{};
};


struct ShaderSelect_OutputBuffer
{
	bool      aSelected;
	glm::vec3 aOutColor;
};


constexpr u32         CH_SHADER_CORE_SLOT_INVALID  = UINT32_MAX;

constexpr const char* CH_SHADER_NAME_SKINNING      = "__skinning";
constexpr const char* CH_SHADER_NAME_SELECT        = "__select";
constexpr const char* CH_SHADER_NAME_SELECT_RESULT = "__select_result";


// I don't like this at all
struct GraphicsData_t
{
	ch_handle_t                                    aCurrentWindow;

	ResourceList< Renderable_t >                  aRenderables;
	std::unordered_map< ch_handle_t, ModelBBox_t > aRenderAABBUpdate;

	std::unordered_map< u32, ViewRenderList_t >   aViewRenderLists;

	// Renderables that need skinning applied to them
	//ChVector< ch_handle_t >                        aSkinningRenderList;
	std::unordered_set< ch_handle_t >              aSkinningRenderList;

	ch_handle_t                                    aRenderPassGraphics;
	ch_handle_t                                    aRenderPassShadow;
	ch_handle_t                                    aRenderPassSelect;

	std::unordered_set< ch_handle_t >              aDirtyMaterials;

	// --------------------------------------------------------------------------------------
	// RenderLists

	ChVector< RenderList* >                       aRenderLists;

	// --------------------------------------------------------------------------------------
	// Descriptor Sets

	ShaderDescriptorData_t                        aShaderDescriptorData;
	// ResourceList< RenderPassData_t >              aRenderPassData;

	// --------------------------------------------------------------------------------------
	// Textures

	// std::vector< ch_handle_t >                     aTextureBuffers;
	// bool                                          aTexturesDirty = true;

	// --------------------------------------------------------------------------------------
	// Viewports
	
	// Viewport ch_handle_t - Viewport Data 
	std::unordered_map < u32, ViewportShader_t >  aViewports;

	// --------------------------------------------------------------------------------------
	// Assets

	ResourceList< Model >                         aModels;
	std::unordered_map< std::string, ch_handle_t > aModelPaths;
	std::unordered_map< ch_handle_t, ModelBBox_t > aModelBBox;

	std::unordered_set< ch_handle_t >              aModelsToFree;
	std::unordered_set< ch_handle_t >              aRenderablesToFree;

	ResourceList< Scene_t >                       aScenes;
	std::unordered_map< std::string, ch_handle_t > aScenePaths;

	// --------------------------------------------------------------------------------------
	// Shader Buffers

	Buffer_Core_t                                 aCoreData;
	DeviceBufferStaging_t                         aCoreDataStaging;

	// Free Indexes
	ShaderArrayAllocator_t                        aCoreDataSlots[ EShaderCoreArray_Count ];

	ShaderBufferList_t                            aVertexBuffers;
	ShaderBufferList_t                            aIndexBuffers;
	ShaderBufferList_t                            aBlendShapeWeightBuffers;
	ShaderBufferList_t                            aBlendShapeDataBuffers;
	
	glm::mat4*                                    aModelMatrixData;  // shares the same slot index as renderables, which is the handle index
	Shader_Renderable_t*                          aRenderableData;

	DeviceBufferStaging_t                         aModelMatrixStaging;
	DeviceBufferStaging_t                         aRenderableStaging;

	Shader_Viewport_t*                            aViewportData;
	ShaderArrayAllocator_t                        aViewportSlots;
	DeviceBufferStaging_t                         aViewportStaging;
};


constexpr const char* gVertexBufferStr      = "Vertex Buffer";
constexpr const char* gIndexBufferStr       = "Vertex Buffer";
constexpr const char* gBlendShapeWeightsStr = "Blend Shape Weights Buffer";


extern GraphicsData_t gGraphicsData;
extern RenderStats_t  gStats;


bool                  Graphics_CreateVariableUniformLayout( ShaderDescriptor_t& srBuffer, const char* spLayoutName, const char* spSetName, int sCount );

void                  Graphics_DrawShaderRenderables( ch_handle_t cmd, size_t sIndex, ch_handle_t shader, u32 sViewportIndex, ChVector< SurfaceDraw_t >& srRenderList );

u32                   Graphics_AllocateShaderSlot( ShaderArrayAllocator_t& srAllocator );
void                  Graphics_FreeShaderSlot( ShaderArrayAllocator_t& srAllocator, u32 sIndex );
u32                   Graphics_GetShaderSlot( ShaderArrayAllocator_t& srAllocator, u32 sHandle );

u32                   Graphics_AllocateCoreSlot( EShaderCoreArray sSlot );
void                  Graphics_FreeCoreSlot( EShaderCoreArray sSlot, u32 sIndex );
u32                   Graphics_GetCoreSlot( EShaderCoreArray sSlot, u32 sIndex );

// return a magic number
u32                   Graphics_AddShaderBuffer( ShaderBufferList_t& srBufferList, ch_handle_t sBuffer );
void                  Graphics_RemoveShaderBuffer( ShaderBufferList_t& srBufferList, u32 sHandle );
ch_handle_t            Graphics_GetShaderBuffer( const ShaderBufferList_t& srBufferList, u32 sHandle );
u32                   Graphics_GetShaderBufferIndex( const ShaderBufferList_t& srBufferList, u32 sHandle );

bool                  Graphics_CreateStagingBuffer( DeviceBufferStaging_t& srStaging, size_t sSize, const char* spStagingName, const char* spName );
bool                  Graphics_CreateStagingUniformBuffer( DeviceBufferStaging_t& srStaging, size_t sSize, const char* spStagingName, const char* spName );
void                  Graphics_FreeStagingBuffer( DeviceBufferStaging_t& srStaging );

void                  Graphics_FreeQueuedResources();

bool                  Graphics_ShaderInit( bool sRecreate );

void                  Graphics_DestroySelectRenderPass();
bool                  Graphics_CreateSelectRenderPass();
void                  Graphics_SelectionTexturePass( ch_handle_t sCmd, size_t sIndex );

bool                  Shader_Bind( ch_handle_t sCmd, u32 sIndex, ch_handle_t sShader );
bool                  Shader_PreMaterialDraw( ch_handle_t sCmd, u32 sIndex, ShaderData_t* spShaderData, ShaderPushData_t& sPushData );
bool                  Shader_ParseRequirements( ShaderRequirmentsList_t& srOutput );

VertexFormat          Shader_GetVertexFormat( ch_handle_t sShader );
ShaderData_t*         Shader_GetData( ch_handle_t sShader );
ShaderSets_t*         Shader_GetSets( ch_handle_t sShader );
ShaderSets_t*         Shader_GetSets( std::string_view sShader );

ch_handle_t                Shader_RegisterDescriptorData( EShaderSlot sSlot, FShader_DescriptorData* sCallback );

void                  Shader_RemoveMaterial( ch_handle_t sMat );
void                  Shader_AddMaterial( ch_handle_t sMat );
void                  Shader_UpdateMaterialVars();
ShaderMaterialData*   Shader_GetMaterialData( ch_handle_t sShader, ch_handle_t sMat );

// Material = Shader Material Data
const std::unordered_map< ch_handle_t, ShaderMaterialData >* Shader_GetMaterialDataMap( ch_handle_t sShader );


// interface

class Graphics : public IGraphics
{
   public:
	virtual void                   Update( float sDT ){};

	// ---------------------------------------------------------------------------------------
	// Models

	virtual ch_handle_t                 LoadModel( const std::string& srPath ) override;
	virtual ch_handle_t                 CreateModel( Model** spModel ) override;
	virtual void                   FreeModel( ch_handle_t hModel ) override;
	virtual Model*                 GetModelData( ch_handle_t hModel ) override;
	virtual std::string_view       GetModelPath( ch_handle_t sModel ) override;
	virtual ModelBBox_t            CalcModelBBox( ch_handle_t sModel ) override;
	virtual bool                   GetModelBBox( ch_handle_t sModel, ModelBBox_t& srBBox ) override;

	virtual void                   Model_SetMaterial( ch_handle_t shModel, size_t sSurface, ch_handle_t shMat ) override;
	virtual ch_handle_t                 Model_GetMaterial( ch_handle_t shModel, size_t sSurface ) override;

	// ---------------------------------------------------------------------------------------
	// Scenes

	virtual ch_handle_t                 LoadScene( const std::string& srPath ) override;
	virtual void                   FreeScene( ch_handle_t sScene ) override;

	virtual SceneDraw_t*           AddSceneDraw( ch_handle_t sScene ) override;
	virtual void                   RemoveSceneDraw( SceneDraw_t* spScene ) override;

	virtual size_t                 GetSceneModelCount( ch_handle_t sScene ) override;
	virtual ch_handle_t                 GetSceneModel( ch_handle_t sScene, size_t sIndex ) override;

	// ---------------------------------------------------------------------------------------
	// Render Lists
	//
	// Render Lists are a Vector of Renderables to draw
	// which contain viewport properties and material overrides
	//

	// Create a RenderList
	virtual RenderList*            RenderListCreate() override;

	// Free a RenderList
	virtual void                   RenderListFree( RenderList* pRenderList ) override;

	// Copy all data from one renderlist to another
	// return true on success
	virtual bool                   RenderListCopy( RenderList* pSrc, RenderList* pDest ) override;

	// Draw a Render List to a Frame Buffer
	// if CH_INVALID_HANDLE is passed in as the Frame Buffer, it defaults to the backbuffer
	// TODO: what if we want to pass in a render list to a compute shader?
	virtual void                   RenderListDraw( RenderList* pRenderList, ch_handle_t framebuffer ) override;

	// Do Frustum Culling based on the projection/view matrix of a viewport
	// This is exposed so you can do basic frustum culling, and then more advanced vis of your own, then draw it
	// The result is stored in "RenderList.culledRenderables"
	virtual void                   RenderListDoFrustumCulling( RenderList* pRenderList, u32 viewportIndex ) override;

	// ---------------------------------------------------------------------------------------

	// HACK HACK - for editor, hopefully i can come up with a better solution later
	// Maybe you can be able to register graphics and compute shaders in game/editor code
	// and we can have functions for running compute shaders, or just use the GraphicsAPI/RenderVK directly
	//
	// What this does is add a render list to the compute shader for selecting entities in the editor
	// The Selection Compute shader renders each renderable with a different solid color
	// then it takes in your mouse position, and picks the color underneath it to find out what renderable you selected
	//virtual void               AddRenderListToSelectionCompute( RenderList* pRenderList )                                                                    override;

	// Gets the selection compute shader result from the last rendered frame
	// Returns a glm::vec3 for the color the cursor landed on
	//virtual glm::vec3          GetSelectionComputeResult()                                                                                                   override;

	// ---------------------------------------------------------------------------------------
	// Render Layers

	// virtual RenderLayer_t*     Graphics_CreateRenderLayer() override;
	// virtual void               Graphics_DestroyRenderLayer( RenderLayer_t* spLayer ) override;

	// control ordering of them somehow?
	// maybe do Graphics_DrawRenderLayer() ?

	// render layers, to the basic degree that i want, is something contains a list of models to render
	// you can manually sort these render layers to draw them in a specific order
	// (viewmodel first, standard view next, skybox last)
	// and is also able to control depth, for skybox and viewmodel drawing
	// or should that be done outside of that? hmm, no idea
	//
	// maybe don't add items to the render list struct directly
	// or, just straight up make the "RenderLayer_t" thing as a ch_handle_t
	// how would this work with immediate mode style drawing? good for a rythem like game
	// and how would it work for VR, or multiple viewports in a level editor or something?
	// and shadowmapping? overthinking this? probably

	// ---------------------------------------------------------------------------------------
	// Textures

	virtual ch_handle_t             LoadTexture( ch_handle_t& srHandle, const std::string& srTexturePath, const TextureCreateData_t& srCreateData ) override;

	virtual ch_handle_t             CreateTexture( const TextureCreateInfo_t& srTextureCreateInfo, const TextureCreateData_t& srCreateData ) override;

	virtual void                   FreeTexture( ch_handle_t shTexture ) override;

	// ---------------------------------------------------------------------------------------
	// Materials

	// Load a cmt file from disk, increments ref count
	virtual ch_handle_t                 LoadMaterial( const char* srPath, s32 sLen ) override;

	// Create a new material with a name and a shader
	virtual ch_handle_t                 CreateMaterial( const std::string& srName, ch_handle_t shShader ) override;

	// Free a material
	virtual void                   FreeMaterial( ch_handle_t sMaterial ) override;

	// Find a material by name
	// Name is a path to the cmt file if it was loaded on disk
	// EXAMPLE: C:/chocolate/sidury/materials/dev/grid01.cmt
	// NAME: materials/dev/grid01
	virtual ch_handle_t                 FindMaterial( const char* spName ) override;

	// Get the total amount of materials created
	virtual u32                    GetMaterialCount() override;

	// Get a material by index
	virtual ch_handle_t             GetMaterialByIndex( u32 sIndex ) override;

	// Get the path to the material
	virtual const std::string&     GetMaterialPath( ch_handle_t sMaterial ) override;

	// Tell all materials to rebuild
	virtual void                   SetAllMaterialsDirty() override;

	// Modifying Material Data
	virtual const char*            Mat_GetName( ch_handle_t mat ) override;
	virtual size_t                 Mat_GetVarCount( ch_handle_t mat ) override;
	virtual EMatVar                Mat_GetVarType( ch_handle_t mat, size_t sIndex ) override;
	virtual const char*            Mat_GetVarName( ch_handle_t mat, size_t sIndex ) override;

	virtual ch_handle_t                 Mat_GetShader( ch_handle_t mat ) override;
	virtual void                   Mat_SetShader( ch_handle_t mat, ch_handle_t shShader ) override;

	virtual VertexFormat           Mat_GetVertexFormat( ch_handle_t mat ) override;

	// Increments Reference Count for material
	virtual void                   Mat_AddRef( ch_handle_t sMat ) override;

	// Decrements Reference Count for material - returns true if the material is deleted
	virtual bool                   Mat_RemoveRef( ch_handle_t sMat ) override;

	virtual void                   Mat_SetVar( ch_handle_t mat, const std::string& name, ch_handle_t texture ) override;
	virtual void                   Mat_SetVar( ch_handle_t mat, const std::string& name, float data ) override;
	virtual void                   Mat_SetVar( ch_handle_t mat, const std::string& name, int data ) override;
	virtual void                   Mat_SetVar( ch_handle_t mat, const std::string& name, bool data ) override;
	virtual void                   Mat_SetVar( ch_handle_t mat, const std::string& name, const glm::vec2& data ) override;
	virtual void                   Mat_SetVar( ch_handle_t mat, const std::string& name, const glm::vec3& data ) override;
	virtual void                   Mat_SetVar( ch_handle_t mat, const std::string& name, const glm::vec4& data ) override;

	virtual int                    Mat_GetTextureIndex( ch_handle_t mat, std::string_view name, ch_handle_t fallback = CH_INVALID_HANDLE ) override;
	virtual ch_handle_t                 Mat_GetTexture( ch_handle_t mat, std::string_view name, ch_handle_t fallback = CH_INVALID_HANDLE ) override;
	virtual float                  Mat_GetFloat( ch_handle_t mat, std::string_view name, float fallback = 0.f ) override;
	virtual int                    Mat_GetInt( ch_handle_t mat, std::string_view name, int fallback = 0 ) override;
	virtual bool                   Mat_GetBool( ch_handle_t mat, std::string_view name, bool fallback = false ) override;
	virtual const glm::vec2&       Mat_GetVec2( ch_handle_t mat, std::string_view name, const glm::vec2& fallback = {} ) override;
	virtual const glm::vec3&       Mat_GetVec3( ch_handle_t mat, std::string_view name, const glm::vec3& fallback = {} ) override;
	virtual const glm::vec4&       Mat_GetVec4( ch_handle_t mat, std::string_view name, const glm::vec4& fallback = {} ) override;

	virtual int                    Mat_GetTextureIndex( ch_handle_t mat, u32 sIndex, ch_handle_t fallback = CH_INVALID_HANDLE ) override;
	virtual ch_handle_t                 Mat_GetTexture( ch_handle_t mat, u32 sIndex, ch_handle_t fallback = CH_INVALID_HANDLE ) override;
	virtual float                  Mat_GetFloat( ch_handle_t mat, u32 sIndex, float fallback = 0.f ) override;
	virtual int                    Mat_GetInt( ch_handle_t mat, u32 sIndex, int fallback = 0 ) override;
	virtual bool                   Mat_GetBool( ch_handle_t mat, u32 sIndex, bool fallback = false ) override;
	virtual const glm::vec2&       Mat_GetVec2( ch_handle_t mat, u32 sIndex, const glm::vec2& fallback = {} ) override;
	virtual const glm::vec3&       Mat_GetVec3( ch_handle_t mat, u32 sIndex, const glm::vec3& fallback = {} ) override;
	virtual const glm::vec4&       Mat_GetVec4( ch_handle_t mat, u32 sIndex, const glm::vec4& fallback = {} ) override;

	// ---------------------------------------------------------------------------------------
	// Shaders

	// virtual bool               Shader_Init( bool sRecreate )                                                                                                override;
	virtual ch_handle_t                 GetShader( const char* sName ) override;
	virtual const char*            GetShaderName( ch_handle_t sShader ) override;

	virtual u32                    GetShaderCount() override;
	virtual ch_handle_t             GetShaderByIndex( u32 sIndex ) override;

	virtual u32                    GetGraphicsShaderCount() override;
	virtual ch_handle_t             GetGraphicsShaderByIndex( u32 sIndex ) override;

	virtual u32                    GetComputeShaderCount() override;
	virtual ch_handle_t             GetComputeShaderByIndex( u32 sIndex ) override;

	virtual u32                    GetShaderVarCount( ch_handle_t shader ) override;
	virtual ShaderMaterialVarDesc* GetShaderVars( ch_handle_t shader ) override;

	// Used to know if this material needs to be ordered and drawn after all opaque ones are drawn
	// virtual bool               Shader_IsMaterialTransparent( ch_handle_t sMat ) override;

	// Used to get all material vars the shader can use, and throw warnings on unknown material vars
	// virtual void               Shader_GetMaterialVars( ch_handle_t sShader ) override;

	// ---------------------------------------------------------------------------------------
	// Buffers

	virtual void                   CreateVertexBuffers( ModelBuffers_t* spBuffer, VertexData_t* spVertexData, const char* spDebugName = nullptr ) override;
	virtual void                   CreateIndexBuffer( ModelBuffers_t* spBuffer, VertexData_t* spVertexData, const char* spDebugName = nullptr ) override;
	// void               CreateModelBuffers( ModelBuffers_t* spBuffers, VertexData_t* spVertexData, bool sCreateIndex, const char* spDebugName ) override;

	// ---------------------------------------------------------------------------------------
	// Lighting

	virtual Light_t*               CreateLight( ELightType sType ) override;
	virtual void                   UpdateLight( Light_t* spLight ) override;
	virtual void                   DestroyLight( Light_t* spLight ) override;

	virtual u32                    GetLightCount() override;
	virtual Light_t*               GetLightByIndex( u32 index ) override;

	// ---------------------------------------------------------------------------------------
	// Rendering

	virtual bool                   Init() override;
	virtual void                   Shutdown() override;

	virtual RenderStats_t        GetStats() override;

	// ch_handle_t         CreateRenderPass() override;
	// virtual void               UpdateRenderPass( ch_handle_t sRenderPass ) override;

	// Returns the Viewport Index - input is the address of a pointer
	virtual u32                    CreateViewport( ViewportShader_t** spViewport = nullptr ) override;
	virtual void                   FreeViewport( u32 sViewportIndex ) override;

	virtual u32                    GetViewportCount() override;
	virtual u32                    GetViewportHandleByIndex( u32 index ) override;
	virtual ViewportShader_t*      GetViewportDataByIndex( u32 index ) override;

	virtual ViewportShader_t*      GetViewportData( u32 sViewportIndex ) override;
	virtual void                   SetViewportUpdate( bool sUpdate ) override;

	virtual void                   SetViewportRenderList( u32 sViewport, ch_handle_t* srRenderables, size_t sCount ) override;

	// virtual void               PushViewInfo( const ViewportShader_t& srViewInfo ) override;
	// virtual void               PopViewInfo() override;
	// virtual ViewportShader_t&  GetViewInfo() override;

	virtual ch_handle_t             CreateRenderable( ch_handle_t sModel ) override;
	virtual Renderable_t*          GetRenderableData( ch_handle_t sRenderable ) override;
	virtual void                   SetRenderableModel( ch_handle_t sRenderable, ch_handle_t sModel ) override;
	virtual void                   FreeRenderable( ch_handle_t sRenderable ) override;
	virtual void                   ResetRenderableMaterials( ch_handle_t sRenderable ) override;
	virtual void                   UpdateRenderableAABB( ch_handle_t sRenderable ) override;
	virtual ModelBBox_t            GetRenderableAABB( ch_handle_t sRenderable ) override;

	virtual void                   SetRenderableDebugName( ch_handle_t sRenderable, std::string_view sName ) override;

	virtual u32                    GetRenderableCount() override;
	virtual ch_handle_t             GetRenderableByIndex( u32 i ) override;

	// virtual void               ConsolidateRenderables()                                                                                                     override;

	virtual void                   CreateFrustum( Frustum_t& srFrustum, const glm::mat4& srViewMat ) override;
	virtual Frustum_t              CreateFrustum( const glm::mat4& srViewMat ) override;

	virtual ModelBBox_t            CreateWorldAABB( glm::mat4& srMatrix, const ModelBBox_t& srBBox ) override;

	// ---------------------------------------------------------------------------------------
	// Debug Rendering

	virtual void                   DrawLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec3& sColor ) override;
	virtual void                   DrawLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec4& sColor ) override;
	virtual void                   DrawLine( const glm::vec3& sX, const glm::vec3& sY, const glm::vec3& sColorX, const glm::vec3& sColorY ) override;
	virtual void                   DrawAxis( const glm::vec3& sPos, const glm::vec3& sAng, const glm::vec3& sScale ) override;
	virtual void                   DrawAxis( const glm::mat4& sMat, const glm::vec3& sScale ) override;
	virtual void                   DrawAxis( const glm::mat4& sMat ) override;
	virtual void                   DrawBBox( const glm::vec3& sX, const glm::vec3& sY, const glm::vec3& sColor ) override;
	virtual void                   DrawProjView( const glm::mat4& srProjView ) override;
	virtual void                   DrawFrustum( const Frustum_t& srFrustum ) override;
	virtual void                   DrawNormals( ch_handle_t sModel, const glm::mat4& srMatrix ) override;

	// Reserve X amount of vertices to draw
	virtual void                   DebugDrawReserve( u32 count ) override;

	// ---------------------------------------------------------------------------------------
	// Vertex Format/Attributes

	virtual GraphicsFmt            GetVertexAttributeFormat( VertexAttribute attrib ) override;
	virtual size_t                 GetVertexAttributeTypeSize( VertexAttribute attrib ) override;
	virtual size_t                 GetVertexAttributeSize( VertexAttribute attrib ) override;
	virtual size_t                 GetVertexFormatSize( VertexFormat format ) override;

	virtual void                   GetVertexBindingDesc( VertexFormat format, std::vector< VertexInputBinding_t >& srAttrib ) override;
	virtual void                   GetVertexAttributeDesc( VertexFormat format, std::vector< VertexInputAttribute_t >& srAttrib ) override;
};


extern Graphics gGraphics;


class RenderSystemOld : public IRenderSystemOld
{
   public:
	virtual bool                    Init() override { return true; };
	virtual void                    Shutdown() override{};
	virtual void                    Update( float sDT ) override{};

	virtual void                    NewFrame() override;
	virtual void                    Reset( ch_handle_t window ) override;
	virtual void                    PrePresent() override;
	virtual void                    Present( ch_handle_t window, u32* sViewports, u32 viewportCount ) override;

	virtual void                    EnableSelection( bool enabled, u32 viewport ) override;

	// Set Renderables to use for selecting this frame
	virtual void                    SetSelectionRenderablesAndCursorPos( const ChVector< SelectionRenderable >& renderables, glm::ivec2 cursorPos ) override;

	// Gets the selection compute shader result from the last rendered frame
	// Returns the color the cursor landed on
	virtual bool                    GetSelectionResult( u8& red, u8& green, u8& blue ) override;

	void                            DoSelectionCompute( ch_handle_t cmd, u32 cmdIndex );
	void                            DoSelection( ch_handle_t cmd, u32 cmdIndex );

	void                            FreeSelectionTexture();
	void                            CreateSelectionTexture();

	// ----------------------------------------------------------

	bool                            aSelectionEnabled = false;
	// ch_handle_t                      aSelectionBuffer      = CH_INVALID_HANDLE;
	;
	ch_handle_t                      aSelectionFramebuffer = CH_INVALID_HANDLE;
	ch_handle_t                      aSelectionTexture     = CH_INVALID_HANDLE;
	ch_handle_t                      aSelectionDepth       = CH_INVALID_HANDLE;
	glm::ivec2                      aSelectionTextureSize{};

	u32                             aSelectionViewportRef = 0;  // viewport data to copy from
	u32                             aSelectionViewport    = CH_R_MAX_VIEWPORTS;
	glm::ivec2                      aSelectionCursorPos{};
	bool                            aSelectionThisFrame = false;

	ChVector< SelectionRenderable > aSelectionRenderables;
};


extern RenderSystemOld gRenderOld;

