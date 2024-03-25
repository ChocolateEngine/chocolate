#include "util.h"
#include "render/irender.h"
#include "graphics_int.h"


// TODO: rename this file and shader to "standard" or "generic"?

static bool                gArgPCF                  = false;  // Args_Register( "Enable PCF Shadow Filtering", "-pcf" );

constexpr const char*      gpFallbackAOPath         = "materials/base/white.ktx";
constexpr const char*      gpFallbackEmissivePath   = "materials/base/black.ktx";

constexpr u32              CH_BASIC3D_MAX_MATERIALS = 2048;


static CreateDescBinding_t gBasic3D_Bindings[]      = {
		 { EDescriptorType_StorageBuffer, ShaderStage_Vertex | ShaderStage_Fragment, 0, CH_BASIC3D_MAX_MATERIALS },
};


struct Basic3D_Push
{
	u32 aRenderable = 0;
	u32 aMaterial   = 0;
	u32 aViewport   = 0;

	// Hack for GTX 1050 Ti until I figure this out properly
	// bool aPCF = false;

	// debugging
	u32 aDebugDraw  = 0;
};


// TODO: remove this, this is a useless struct
// ...except for one tiny, itsy, bitsy issue
// this automatically takes care of alignment for us
struct Basic3D_Material
{
	int   diffuse;
	int   ao;
	int   emissive;
	int   normalMap;

	float aoPower;
	float emissivePower;

	bool  alphaTest;
	bool  useNormalMap;  // TODO: have an option to detect if a texture is in the normal map option or not
};


// static ShaderMaterialVarDesc gBasic3D_MaterialVars[] = {
// 	{ "diffuse", "Diffuse Texture", "", offsetof( Basic3D_Material, diffuse ), sizeof( Basic3D_Material::diffuse ) },
// 	{ "ao", "Ambient Occlusion Texture", gpFallbackAOPath, offsetof( Basic3D_Material, ao ) },
// 	{ "emissive", "Emission Texture", gpFallbackEmissivePath, offsetof( Basic3D_Material, emissive ) },
// 
// 	{ "aoPower", "Ambient Occlusion Strength", 0.f, offsetof( Basic3D_Material, aoPower ) },
// 	{ "emissivePower", "Emission Strength", 0.f, offsetof( Basic3D_Material, emissivePower ) },
// 
// 	{ "alphaTest", "Alpha Testing", false, offsetof( Basic3D_Material, alphaTest ) },
// };


static ShaderMaterialVarDesc gBasic3D_MaterialVars[] = {
	CH_SHADER_MATERIAL_VAR( Basic3D_Material, diffuse, "Diffuse Texture", "" ),
	CH_SHADER_MATERIAL_VAR( Basic3D_Material, ao, "Ambient Occlusion Texture", gpFallbackAOPath ),
	CH_SHADER_MATERIAL_VAR( Basic3D_Material, emissive, "Emission Texture", gpFallbackEmissivePath ),
	CH_SHADER_MATERIAL_VAR( Basic3D_Material, normalMap, "Normal Map Texture", "materials/models/protogen_wip_25d/protogen25d_normal" ),

	CH_SHADER_MATERIAL_VAR( Basic3D_Material, aoPower, "Ambient Occlusion Strength", 0.f ),
	CH_SHADER_MATERIAL_VAR( Basic3D_Material, emissivePower, "Emission Strength", 0.f ),

	CH_SHADER_MATERIAL_VAR( Basic3D_Material, alphaTest, "Alpha Testing", false ),
	CH_SHADER_MATERIAL_VAR( Basic3D_Material, useNormalMap, "Use Normal Map", true ),

	// bullet list option
	// CH_SHADER_MATERIAL_VAR_COMBO();
};


// KEEP IN THE SAME ORDER AS THE MATERIAL VARS ABOVE
enum : u32
{
	EBasic3D_Diffuse,
	EBasic3D_AmbientOcclusion,
	EBasic3D_Emissive,
	EBasic3D_NormalMap,

	EBasic3D_AmbientOcclusionPower,
	EBasic3D_EmissivePower,

	EBasic3D_AlphaTest,
};


static std::unordered_map< u32, Basic3D_Push > gPushData;

CONVAR( r_basic3d_dbg_mode, 0 );


static void Shader_Basic3D_GetPipelineLayoutCreate( PipelineLayoutCreate_t& srPipeline )
{
	// NOTE: maybe create the descriptor set layout for this shader here, then add it? idk

	srPipeline.aPushConstants.push_back( { ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Basic3D_Push ) } );
}


static void Shader_Basic3D_GetGraphicsPipelineCreate( GraphicsPipelineCreate_t& srGraphics )
{
	srGraphics.aShaderModules.push_back( { ShaderStage_Vertex, "shaders/basic3d.vert.spv", "main" } );
	srGraphics.aShaderModules.push_back( { ShaderStage_Fragment, "shaders/basic3d.frag.spv", "main" } );

	srGraphics.aColorBlendAttachments.push_back( { false } ); 

	srGraphics.aPrimTopology   = EPrimTopology_Tri;
	srGraphics.aDynamicState   = EDynamicState_Viewport | EDynamicState_Scissor;
	srGraphics.aCullMode       = ECullMode_Back;
}


// TODO: Move this push constant management into the shader system
// we should only need the SetupPushData function
static void Shader_Basic3D_ResetPushData()
{
	gPushData.clear();
}


static void Shader_Basic3D_SetupPushData( u32 sRenderableIndex, u32 sViewportIndex, Renderable_t* spRenderable, SurfaceDraw_t& srDrawInfo, ShaderMaterialData* spMaterialData )
{
	PROF_SCOPE();

	Basic3D_Push& push = gPushData[ srDrawInfo.aShaderSlot ];
	// push.aModelMatrix  = spModelDraw->aModelMatrix;
	// push.aRenderable   = CH_GET_HANDLE_INDEX( srDrawInfo.aRenderable );
	push.aRenderable   = spRenderable->aIndex;  // or sRenderableIndex, huh
	push.aMaterial     = spMaterialData->matIndex;
	push.aViewport     = sViewportIndex;

	push.aDebugDraw    = r_basic3d_dbg_mode;
	// push.aPCF          = gArgPCF;
}


//static void Shader_Basic3D_SetupPushData2( u32 sSurfaceIndex, u32 sViewportIndex, Renderable_t* spModelDraw, SurfaceDraw_t& srDrawInfo, void* spData )
//{
//	PROF_SCOPE();
//
//	Basic3D_Push* push = static_cast< Basic3D_Push* >( spData );
//
//	// push->aModelMatrix  = spModelDraw->aModelMatrix;
//	push->aRenderable  = CH_GET_HANDLE_INDEX( srDrawInfo.aRenderable );
//	push->aMaterial    = Shader_Basic3D_GetMaterialIndex( sSurfaceIndex, spModelDraw, srDrawInfo );
//	push->aViewport    = sViewportIndex;
//
//	push->aDebugDraw   = r_basic3d_dbg_mode;
//	// push->aPCF          = gArgPCF;
//}


static void Shader_Basic3D_PushConstants( Handle cmd, Handle sLayout, SurfaceDraw_t& srDrawInfo )
{
	PROF_SCOPE();

	Basic3D_Push& push = gPushData.at( srDrawInfo.aShaderSlot );
	render->CmdPushConstants( cmd, sLayout, ShaderStage_Vertex | ShaderStage_Fragment, 0, sizeof( Basic3D_Push ), &push );
}


static IShaderPush gShaderPush_Basic3D = {
	.apReset = Shader_Basic3D_ResetPushData,
	.apSetup = Shader_Basic3D_SetupPushData,
	.apPush  = Shader_Basic3D_PushConstants,
};


// TODO: some binding list saying which ones are materials? or only have one option for saying which binding is a material
ShaderCreate_t gShaderCreate_Basic3D = {
	.apName               = "basic_3d",
	.aStages              = ShaderStage_Vertex | ShaderStage_Fragment,
	.aBindPoint           = EPipelineBindPoint_Graphics,
	.aFlags               = EShaderFlags_PushConstant,
	.aDynamicState        = EDynamicState_Viewport | EDynamicState_Scissor,
	.aVertexFormat        = VertexFormat_Position | VertexFormat_Normal | VertexFormat_TexCoord,

	.apLayoutCreate       = Shader_Basic3D_GetPipelineLayoutCreate,
	.apGraphicsCreate     = Shader_Basic3D_GetGraphicsPipelineCreate,

	// .aPushSize         = sizeof( Basic3D_Push ),
	// .apPushSetup       = Shader_Basic3D_SetupPushData2,

	.apShaderPush         = &gShaderPush_Basic3D,

	.apBindings           = gBasic3D_Bindings,
	.aBindingCount        = CH_ARR_SIZE( gBasic3D_Bindings ),

	.apMaterialVars       = gBasic3D_MaterialVars,
	.aMaterialVarCount    = CH_ARR_SIZE( gBasic3D_MaterialVars ),
	.aMaterialSize        = sizeof( Basic3D_Material ),
	.aUseMaterialBuffer   = true,
	.aMaterialBufferIndex = 0,
};


CH_REGISTER_SHADER( gShaderCreate_Basic3D );

