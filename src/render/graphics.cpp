#include "core/core.h"
#include "igui.h"
#include "render/irender.h"
#include "graphics_int.h"
#include "lighting.h"
#include "debug_draw.h"
#include "mesh_builder.h"
#include "imgui/imgui.h"
// #include "rmlui_render.h"

#include <forward_list>
#include <stack>
#include <set>
#include <unordered_set>


LOG_CHANNEL_REGISTER_EX( gLC_ClientGraphics, "ClientGraphics", ELogColor_Green );

bool _Graphics_LoadModel( ch_handle_t& item, const fs::path& srPath );
bool _Graphics_CreateModel( ch_handle_t& item, const fs::path& srInternalPath, void* spData );
void _Graphics_FreeModel( ch_handle_t item );

bool _Graphics_LoadMaterial( ch_handle_t& item, const fs::path& srPath );
bool _Graphics_CreateMaterial( ch_handle_t& item, const fs::path& srInternalPath, void* spData );
void _Graphics_FreeMaterial( ch_handle_t item );


// static ResourceType_t gResourceType_Model = {
// 	.apName       = "Model",
// 	.apFuncLoad   = _Graphics_LoadModel,
// 	.apFuncCreate = _Graphics_CreateModel,
// 	// .apFuncFree   = _Graphics_FreeModel,
// };
// 
// static ResourceType_t gResourceType_Material = {
// 	.apName       = "Material",
// 	.apFuncLoad   = _Graphics_LoadMaterial,
// 	.apFuncCreate = _Graphics_CreateMaterial,
// 	// .apFuncFree   = _Graphics_FreeMaterial,
// };
// 
// 
// ch_handle_t             gResource_Model    = Resource_RegisterType( gResourceType_Model );
// ch_handle_t             gResource_Material = Resource_RegisterType( gResourceType_Material );

// #define CH_REGISTER_RESOURCE_TYPE( name, loadFunc, createFunc, freeFunc ) \
// 	ch_handle_t gResource_##name = Resource_RegisterType( #name, loadFunc, createFunc, freeFunc )
// 
// CH_REGISTER_RESOURCE_TYPE( Model, _Graphics_LoadModel, _Graphics_CreateModel, nullptr );
// CH_REGISTER_RESOURCE_TYPE( Material, _Graphics_LoadMaterial, _Graphics_CreateMaterial, nullptr );

// --------------------------------------------------------------------------------------
// Interfaces

extern IGuiSystem*     gui;
extern IRender*        render;

// --------------------------------------------------------------------------------------

void                   Graphics_LoadObj( const std::string& srBasePath, const std::string& srPath, Model* spModel );
void                   Graphics_LoadGltf( const std::string& srBasePath, const std::string& srPath, const std::string& srExt, Model* spModel );
void                   Graphics_LoadGltfNew( const std::string& srBasePath, const std::string& srPath, const std::string& srExt, Model* spModel );

void                   Graphics_LoadSceneObj( const std::string& srBasePath, const std::string& srPath, Scene_t* spScene );

ch_handle_t                 CreateModelBuffer( const char* spName, void* spData, size_t sBufferSize, EBufferFlags sUsage );

// --------------------------------------------------------------------------------------
// General Rendering
// TODO: group these globals together by data commonly used together

GraphicsData_t           gGraphicsData;
ShaderDescriptorData_t   gShaderDescriptorData;
RenderStats_t          gStats;

IRender*                 render;
Graphics                 gGraphics;

static ModuleInterface_t gInterfaces[] = {
	{ &gGraphics, IGRAPHICS_NAME, IGRAPHICS_VER },
	{ &gRenderOld, IRENDERSYSTEMOLD_NAME, IRENDERSYSTEMOLD_VER },
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& srCount )
	{
		srCount = 2;
		return gInterfaces;
	}
}


// --------------------------------------------------------------------------------------
// Other

const char* gShaderCoreArrayStr[] = {
	"Viewport",
	// "Renderable",

	"LightWorld",
	"LightPoint",
	"LightCone",
	"LightCapsule",
};


static_assert( CH_ARR_SIZE( gShaderCoreArrayStr ) == EShaderCoreArray_Count );


CONCMD( r_reload_textures )
{
	render->ReloadTextures();
	// Graphics_SetAllMaterialsDirty();
}


// TODO: ch_handle_t Blend Shapes and Animations
ModelBBox_t Graphics::CalcModelBBox( ch_handle_t sModel )
{
	PROF_SCOPE();

	ModelBBox_t bbox{};
	bbox.aMax = { INT_MIN, INT_MIN, INT_MIN };
	bbox.aMin = { INT_MAX, INT_MAX, INT_MAX };

	Model* model = gGraphics.GetModelData( sModel );
	if ( !model )
		return bbox;

	auto*      vertData = model->apVertexData;
	glm::vec3* data     = nullptr;

	for ( auto& attrib : vertData->aData )
	{
		if ( attrib.aAttrib == VertexAttribute_Position )
		{
			data = (glm::vec3*)attrib.apData;
			break;
		}
	}

	if ( data == nullptr )
	{
		Log_Error( "Position Vertex Data not found?\n" );
		gGraphicsData.aModelBBox[ sModel ] = bbox;
		return bbox;
	}

	auto UpdateBBox = [ & ]( const glm::vec3& vertex )
	{
		bbox.aMin.x = glm::min( bbox.aMin.x, vertex.x );
		bbox.aMin.y = glm::min( bbox.aMin.y, vertex.y );
		bbox.aMin.z = glm::min( bbox.aMin.z, vertex.z );

		bbox.aMax.x = glm::max( bbox.aMax.x, vertex.x );
		bbox.aMax.y = glm::max( bbox.aMax.y, vertex.y );
		bbox.aMax.z = glm::max( bbox.aMax.z, vertex.z );
	};

	for ( Mesh& mesh : model->aMeshes )
	{
		if ( vertData->aIndices.size() )
		{
			if ( mesh.aIndexCount )
			{
				for ( u32 i = 0; i < mesh.aIndexCount; i++ )
					UpdateBBox( data[ vertData->aIndices[ mesh.aIndexOffset + i ] ] );
			}
			else
			{
				for ( u32 i = 0; i < mesh.aVertexCount; i++ )
					UpdateBBox( data[ mesh.aVertexOffset + i ] );
			}
		}
		else
		{
			for ( u32 i = 0; i < mesh.aVertexCount; i++ )
				UpdateBBox( data[ mesh.aVertexOffset + i ] );
		}
	}

	gGraphicsData.aModelBBox[ sModel ] = bbox;

	return bbox;
}


bool Graphics::GetModelBBox( ch_handle_t sModel, ModelBBox_t& srBBox )
{
	auto it = gGraphicsData.aModelBBox.find( sModel );
	if ( it == gGraphicsData.aModelBBox.end() )
		return false;

	srBBox = it->second;
	return true;
}


ch_handle_t Graphics::LoadModel( const std::string& srPath )
{
	PROF_SCOPE();

	// Have we loaded this model already?
	auto it = gGraphicsData.aModelPaths.find( srPath );

	if ( it != gGraphicsData.aModelPaths.end() )
	{
		// We did load this already, use that model instead
		// Increment the ref count
		Model* model = nullptr;
		if ( !gGraphicsData.aModels.Get( it->second, &model ) )
		{
			Log_Error( gLC_ClientGraphics, "Graphics::LoadModel: Model is nullptr\n" );
			return CH_INVALID_HANDLE;
		}

		model->AddRef();
		return it->second;
	}

	// We have not, so try to load this model in
	ch_string_auto fullPath = FileSys_FindFile( srPath.data(), srPath.size() );

	if ( !fullPath.data )
	{
		if ( FileSys_IsRelative( srPath.data() ) )
		{
			ch_string_auto newPath = ch_str_join( "models" CH_PATH_SEP_STR, 7, (char*)srPath.data(), (s64)srPath.size() );
			fullPath               = FileSys_FindFile( newPath.data, newPath.size );
		}
	}

	if ( !fullPath.data )
	{
		Log_ErrorF( gLC_ClientGraphics, "LoadModel: Failed to Find Model: %s\n", srPath.c_str() );
		return CH_INVALID_HANDLE;
	}

	ch_string_auto fileExt = FileSys_GetFileExt( srPath.data(), srPath.size() );

	Model*      model   = nullptr;
	ch_handle_t      handle  = CH_INVALID_HANDLE;

	// TODO: try to do file header checking
	if ( ch_str_equals( fileExt, "obj", 3 ) )
	{
		handle = gGraphicsData.aModels.Create( &model );

		if ( handle == CH_INVALID_HANDLE )
		{
			Log_ErrorF( gLC_ClientGraphics, "LoadModel: Failed to Allocate Model: %s\n", srPath.c_str() );
			return CH_INVALID_HANDLE;
		}

		// fuck
		std::string temp( fullPath.data, fullPath.size );
		Graphics_LoadObj( srPath, temp, model );
	}
	else if ( ch_str_equals( fileExt, "glb", 3 ) || ch_str_equals( fileExt, "gltf", 4 ) )
	{
		handle = gGraphicsData.aModels.Create( &model );

		if ( handle == CH_INVALID_HANDLE )
		{
			Log_ErrorF( gLC_ClientGraphics, "LoadModel: Failed to Allocate Model: %s\n", srPath.c_str() );
			return CH_INVALID_HANDLE;
		}

		// fuck
		std::string fullPathStd( fullPath.data, fullPath.size );
		std::string fileExtStd( fileExt.data, fileExt.size );
		Graphics_LoadGltfNew( srPath, fullPathStd, fileExtStd, model );
	}
	else
	{
		Log_DevF( gLC_ClientGraphics, 1, "Unknown Model File Extension: %s\n", fileExt.data );
		return CH_INVALID_HANDLE;
	}

	//sModel->aRadius = glm::distance( mesh->aMinSize, mesh->aMaxSize ) / 2.0f;

	// TODO: load in an error model here instead
	if ( model->aMeshes.empty() )
	{
		gGraphicsData.aModels.Remove( handle );
		return CH_INVALID_HANDLE;
	}

	// calculate a bounding box
	Graphics::CalcModelBBox( handle );

	gGraphicsData.aModelPaths[ srPath ] = handle;

	model->AddRef();

	return handle;
}


ch_handle_t Graphics::CreateModel( Model** spModel )
{
	ch_handle_t handle = gGraphicsData.aModels.Create( spModel );

	if ( handle != CH_INVALID_HANDLE )
		( *spModel )->AddRef();

	return handle;
}


void Graphics_FreeQueuedResources()
{
	PROF_SCOPE();

	// Free Renderables
	for ( ch_handle_t renderHandle : gGraphicsData.aRenderablesToFree )
	{
	}

	// Free Models
	for ( ch_handle_t modelHandle : gGraphicsData.aModelsToFree )
	{
		Model* model = nullptr;
		if ( !gGraphicsData.aModels.Get( modelHandle, &model ) )
		{
			Log_Error( gLC_ClientGraphics, "Graphics::FreeModel: Model is nullptr\n" );
			continue;
		}

		model->aRefCount--;
		if ( model->aRefCount == 0 )
		{
			// Free Materials attached to this model
			for ( Mesh& mesh : model->aMeshes )
			{
				if ( mesh.aMaterial )
					gGraphics.FreeMaterial( mesh.aMaterial );
			}

			// Free Vertex Data
			if ( model->apVertexData )
			{
				delete model->apVertexData;
			}

			// Free Vertex and Index Buffers
			if ( model->apBuffers )
			{
				if ( model->apBuffers->aVertexHandle != UINT32_MAX )
				{
					Graphics_RemoveShaderBuffer( gGraphicsData.aVertexBuffers, model->apBuffers->aVertexHandle );
					render->DestroyBuffer( model->apBuffers->aVertex );
				}

				if ( model->apBuffers->aIndexHandle != UINT32_MAX )
				{
					Graphics_RemoveShaderBuffer( gGraphicsData.aIndexBuffers, model->apBuffers->aIndexHandle );
					render->DestroyBuffer( model->apBuffers->aIndex );
				}

				if ( model->apBuffers->aBlendShapeHandle != UINT32_MAX )
				{
					Graphics_RemoveShaderBuffer( gGraphicsData.aBlendShapeDataBuffers, model->apBuffers->aBlendShapeHandle );
					render->DestroyBuffer( model->apBuffers->aBlendShape );
				}

				// if ( model->apBuffers->aBlendShapeHandle != UINT32_MAX )
				// {
				// 	Graphics_RemoveShaderBuffer();
				// 	render->DestroyBuffer( model->apBuffers->aSkin );
				// }

				delete model->apBuffers;
			}

			// If this model was loaded from disk, remove the stored model path
			for ( auto& [ path, modelHandleI ] : gGraphicsData.aModelPaths )
			{
				if ( modelHandle == modelHandleI )
				{
					gGraphicsData.aModelPaths.erase( path );
					break;
				}
			}

			gGraphicsData.aModels.Remove( modelHandle );
			gGraphicsData.aModelBBox.erase( modelHandle );
		}
	}

	gGraphicsData.aModelsToFree.clear();
}


void Graphics::FreeModel( ch_handle_t shModel )
{
	if ( shModel == CH_INVALID_HANDLE )
		return;

	gGraphicsData.aModelsToFree.emplace( shModel );
}


// ---------------------------------------------------
// Resource System Funcs


bool _Graphics_LoadModel( ch_handle_t& item, const fs::path& srPath )
{
	return false;
}


bool _Graphics_CreateModel( ch_handle_t& item, const fs::path& srInternalPath, void* spData )
{
	return false;
}


void _Graphics_FreeModel( ch_handle_t item )
{
}

// ---------------------------------------------------


Model* Graphics::GetModelData( ch_handle_t shModel )
{
	PROF_SCOPE();

	Model* model = nullptr;
	if ( !gGraphicsData.aModels.Get( shModel, &model ) )
	{
		Log_Error( gLC_ClientGraphics, "Graphics::GetModelData: Model is nullptr\n" );
		return nullptr;
	}

	return model;
}


std::string_view Graphics::GetModelPath( ch_handle_t sModel )
{
	for ( auto& [ path, modelHandle ] : gGraphicsData.aModelPaths )
	{
		if ( modelHandle == sModel )
		{
			return path;
		}
	}

	return "";
}


void Graphics::Model_SetMaterial( ch_handle_t shModel, size_t sSurface, ch_handle_t shMat )
{
	Model* model = nullptr;
	if ( !gGraphicsData.aModels.Get( shModel, &model ) )
	{
		Log_Error( gLC_ClientGraphics, "Model_SetMaterial: Model is nullptr\n" );
		return;
	}

	if ( sSurface > model->aMeshes.size() )
	{
		Log_ErrorF( gLC_ClientGraphics, "Model_SetMaterial: surface is out of range: %zu (Surface Count: %zu)\n", sSurface, model->aMeshes.size() );
		return;
	}

	model->aMeshes[ sSurface ].aMaterial = shMat;
}


ch_handle_t Graphics::Model_GetMaterial( ch_handle_t shModel, size_t sSurface )
{
	Model* model = nullptr;
	if ( !gGraphicsData.aModels.Get( shModel, &model ) )
	{
		Log_Error( gLC_ClientGraphics, "Model_GetMaterial: Model is nullptr\n" );
		return CH_INVALID_HANDLE;
	}

	if ( sSurface >= model->aMeshes.size() )
	{
		Log_ErrorF( gLC_ClientGraphics, "Model_GetMaterial: surface is out of range: %zu (Surface Count: %zu)\n", sSurface, model->aMeshes.size() );
		return CH_INVALID_HANDLE;
	}

	return model->aMeshes[ sSurface ].aMaterial;
}


// ---------------------------------------------------------------------------------------
// Scenes


ch_handle_t Graphics::LoadScene( const std::string& srPath )
{
#if 0
	// Have we loaded this scene already?
	auto it = gGraphicsData.aScenePaths.find( srPath );

	if ( it != gGraphicsData.aScenePaths.end() )
	{
		// We have, use that scene instead
		return it->second;
	}

	// We have not, so try to load this model in
	std::string fullPath = FileSys_FindFile( srPath );

	if ( fullPath.empty() )
	{
		Log_ErrorF( gLC_ClientGraphics, "LoadScene: Failed to Find Scene: %s\n", srPath.c_str() );
		return CH_INVALID_HANDLE;
	}

	std::string fileExt = FileSys_GetFileExt( srPath );

	// Scene_t*    scene   = new Scene_t;
	Scene_t*    scene   = nullptr;
	ch_handle_t      handle  = CH_INVALID_HANDLE;

	// TODO: try to do file header checking
	if ( fileExt == "obj" )
	{
		handle = gGraphicsData.aScenes.Create( &scene );
		
		if ( handle == CH_INVALID_HANDLE )
		{
			Log_ErrorF( gLC_ClientGraphics, "LoadScene: Failed to Allocate Scene: %s\n", srPath.c_str() );
			return CH_INVALID_HANDLE;
		}

		memset( &scene->aModels, 0, sizeof( scene->aModels ) );
		Graphics_LoadSceneObj( srPath, fullPath, scene );
	}
	// else if ( fileExt == "glb" || fileExt == "gltf" )
	// {
	// 	// handle = gScenes.Add( scene );
	// 	// Graphics_LoadGltf( srPath, fullPath, fileExt, model );
	// }
	else
	{
		Log_DevF( gLC_ClientGraphics, 1, "Unknown Model File Extension: %s\n", fileExt.c_str() );
		return CH_INVALID_HANDLE;
	}

	//sModel->aRadius = glm::distance( mesh->aMinSize, mesh->aMaxSize ) / 2.0f;

	// TODO: load in an error model here instead
	if ( scene->aModels.empty() )
	{
		gGraphicsData.aScenes.Remove( handle );
		// delete scene;
		return CH_INVALID_HANDLE;
	}

	// Calculate Bounding Boxes for Models
	for ( const auto& modelHandle : scene->aModels )
	{
		Graphics::CalcModelBBox( modelHandle );
	}

	gGraphicsData.aScenePaths[ srPath ] = handle;
	return handle;
#else
	return CH_INVALID_HANDLE;
#endif
}


void Graphics::FreeScene( ch_handle_t sScene )
{
	// HACK HACK PERF: we have to wait for queues to finish, so we could just free this model later
	// maybe right before the next draw?
	render->WaitForQueues();

	Scene_t* scene = nullptr;
	if ( !gGraphicsData.aScenes.Get( sScene, &scene ) )
	{
		Log_Error( gLC_ClientGraphics, "Graphics_FreeScene: Failed to find Scene\n" );
		return;
	}

	for ( auto& model : scene->aModels )
	{
		gGraphics.FreeModel( model );
	}
	
	for ( auto& [ path, sceneHandle ] : gGraphicsData.aScenePaths )
	{
		if ( sceneHandle == sScene )
		{
			gGraphicsData.aScenePaths.erase( path );
			break;
		}
	}

	// delete scene;
	gGraphicsData.aScenes.Remove( sScene );
}


SceneDraw_t* Graphics::AddSceneDraw( ch_handle_t sScene )
{
	if ( !sScene )
		return nullptr;

	Scene_t* scene = nullptr;
	if ( !gGraphicsData.aScenes.Get( sScene, &scene ) )
	{
		Log_Error( gLC_ClientGraphics, "Graphics_DrawScene: Failed to find Scene\n" );
		return nullptr;
	}

	SceneDraw_t* sceneDraw = new SceneDraw_t;
	sceneDraw->aScene      = sScene;
	sceneDraw->aDraw.resize( scene->aModels.size() );

	for ( uint32_t i = 0; i < scene->aModels.size(); i++ )
	{
		sceneDraw->aDraw[ i ] = gGraphics.CreateRenderable( scene->aModels[ i ] );
	}

	return sceneDraw;
}


void Graphics::RemoveSceneDraw( SceneDraw_t* spScene )
{
	if ( !spScene )
		return;

	// Scene_t* scene = nullptr;
	// if ( !gScenes.Get( spScene->aScene, &scene ) )
	// {
	// 	Log_Error( gLC_ClientGraphics, "Graphics_DrawScene: Failed to find Scene\n" );
	// 	return;
	// }

	for ( auto& modelDraw : spScene->aDraw )
	{
		gGraphics.FreeRenderable( modelDraw );
	}

	delete spScene;
}


size_t Graphics::GetSceneModelCount( ch_handle_t sScene )
{
	Scene_t* scene = nullptr;
	if ( !gGraphicsData.aScenes.Get( sScene, &scene ) )
	{
		Log_Error( gLC_ClientGraphics, "Graphics::GetSceneModelCount: Failed to find Scene\n" );
		return 0;
	}

	return scene->aModels.size();
}


ch_handle_t Graphics::GetSceneModel( ch_handle_t sScene, size_t sIndex )
{
	Scene_t* scene = nullptr;
	if ( !gGraphicsData.aScenes.Get( sScene, &scene ) )
	{
		Log_Error( gLC_ClientGraphics, "Graphics::GetSceneModel: Failed to find Scene\n" );
		return 0;
	}

	if ( sIndex >= scene->aModels.size() )
	{
		Log_Error( gLC_ClientGraphics, "Graphics::GetSceneModel: Index out of range\n" );
		return CH_INVALID_HANDLE;
	}

	return scene->aModels[ sIndex ];
}


// ---------------------------------------------------------------------------------------
// Render Lists
//
// Render Lists are a Vector of Renderables to draw
// which contain viewport properties and material overrides
//

// Create a RenderList
RenderList* Graphics::RenderListCreate()
{
	RenderList* renderList = new RenderList;
	gGraphicsData.aRenderLists.push_back( renderList );
	return renderList;
}


// Free a RenderList
void Graphics::RenderListFree( RenderList* pRenderList )
{
	gGraphicsData.aRenderLists.erase( pRenderList );
	delete pRenderList;
}


// Copy all data from one renderlist to another
// return true on success
bool Graphics::RenderListCopy( RenderList* pSrc, RenderList* pDest )
{
	if ( !pSrc )
		Log_Error( gLC_ClientGraphics, "RenderListCopy: pSrc is nullptr!\n" );

	if ( !pDest )
		Log_Error( gLC_ClientGraphics, "RenderListCopy: pDest is nullptr!\n" );

	if ( !pSrc || !pDest )
		return false;

	// Copy Renderables
	if ( pSrc->renderables.size() )
	{
		pDest->renderables.resize( pSrc->renderables.size() );
		memcpy( pDest->renderables.apData, pSrc->renderables.apData, pSrc->renderables.size_bytes() );
	}

	// Copy Culled Renderables
	if ( pSrc->culledRenderables.size() )
	{
		pDest->culledRenderables.resize( pSrc->culledRenderables.size() );
		memcpy( pDest->culledRenderables.apData, pSrc->culledRenderables.apData, pSrc->culledRenderables.size_bytes() );
	}

	// Copy Renderable Overrides
	if ( pSrc->renderableOverrides.size() )
	{
		pDest->renderableOverrides.resize( pSrc->renderableOverrides.size() );
		memcpy( pDest->renderableOverrides.apData, pSrc->renderableOverrides.apData, pSrc->renderableOverrides.size_bytes() );
	}

	pDest->useInShadowMap   = pSrc->useInShadowMap;
	pDest->minDepth         = pSrc->minDepth;
	pDest->maxDepth         = pSrc->maxDepth;
	pDest->viewportOverride = pSrc->viewportOverride;

	return true;
}


// Draw a Render List to a Frame Buffer
// if CH_INVALID_HANDLE is passed in as the Frame Buffer, it defaults to the backbuffer
// TODO: what if we want to pass in a render list to a compute shader?
void Graphics::RenderListDraw( RenderList* pRenderList, ch_handle_t framebuffer )
{
	if ( framebuffer == CH_INVALID_HANDLE )
	{
	}

	// Find Framebuffer and put this renderlist in a vector for it

	// ---------------------------------------------------------------

	if ( pRenderList->useInShadowMap )
	{
	}
}


// Do Frustum Culling based on the projection/view matrix of a viewport
// This is exposed so you can do basic frustum culling, and then more advanced vis of your own, then draw it
// The result is stored in "RenderList.culledRenderables"
void Graphics::RenderListDoFrustumCulling( RenderList* pRenderList, u32 viewportIndex )
{
}


// ---------------------------------------------------------------------------------------


ch_handle_t Graphics::LoadTexture( ch_handle_t& srHandle, const std::string& srTexturePath, const TextureCreateData_t& srCreateData )
{
	//gGraphicsData.aTexturesDirty = true;
	return render->LoadTexture( srHandle, srTexturePath, srCreateData );
}


ch_handle_t Graphics::CreateTexture( const TextureCreateInfo_t& srTextureCreateInfo, const TextureCreateData_t& srCreateData )
{
	//gGraphicsData.aTexturesDirty = true;
	return render->CreateTexture( srTextureCreateInfo, srCreateData );
}


void Graphics::FreeTexture( ch_handle_t shTexture )
{
	//gGraphicsData.aTexturesDirty = true;
	render->FreeTexture( shTexture );
}


// ---------------------------------------------------------------------------------------


void Graphics_DestroyRenderPasses()
{
	Graphics_DestroyShadowRenderPass();
}


bool Graphics_CreateRenderPasses()
{
	// Shadow Map Render Pass
	if ( !Graphics_CreateShadowRenderPass() )
	{
		Log_Error( "Failed to create Shadow Map Render Pass\n" );
		return false;
	}

	// Selection Render Pass
	if ( !Graphics_CreateSelectRenderPass() )
	{
		Log_Error( "Failed to create Selection Render Pass\n" );
		return false;
	}

	return true;
}


#if 0
bool Graphics_CreateVariableDescLayout( CreateDescLayout_t& srCreate, ch_handle_t& srLayout, ch_handle_t* spSets, u32 sSetCount, const char* spSetName, int sCount )
{
	srLayout = render->CreateDescLayout( srCreate );

	if ( srLayout == CH_INVALID_HANDLE )
	{
		Log_Error( gLC_ClientGraphics, "Failed to create variable descriptor layout\n" );
		return false;
	}

	AllocVariableDescLayout_t allocLayout{};
	allocLayout.apName    = spSetName;
	allocLayout.aLayout   = srLayout;
	allocLayout.aCount    = sCount;
	allocLayout.aSetCount = sSetCount;

	if ( !render->AllocateVariableDescLayout( allocLayout, spSets ) )
	{
		Log_Error( gLC_ClientGraphics, "Failed to allocate variable descriptor layout\n" );
		return false;
	}

	return true;
}
#endif


bool Graphics_CreateDescLayout( CreateDescLayout_t& srCreate, ch_handle_t& srLayout, ch_handle_t* spSets, u32 sSetCount, const char* spSetName )
{
	srLayout = render->CreateDescLayout( srCreate );

	if ( srLayout == CH_INVALID_HANDLE )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to create descriptor layout - %s\n", spSetName );
		return false;
	}

	AllocDescLayout_t allocLayout{};
	allocLayout.apName    = spSetName;
	allocLayout.aLayout   = srLayout;
	// allocLayout.aCount    = sCount;
	allocLayout.aSetCount = sSetCount;

	if ( !render->AllocateDescLayout( allocLayout, spSets ) )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to allocate descriptor layout - %s\n", spSetName );
		return false;
	}

	return true;
}


// bool Graphics_CreateVariableUniformLayout( ShaderDescriptor_t& srBuffer, const char* spLayoutName, const char* spSetName, int sCount )
// {
// 	return Graphics_CreateVariableDescLayout( EDescriptorType_UniformBuffer, srBuffer, spLayoutName, spSetName, sCount );
// }


// bool Graphics_CreateVariableStorageLayout( ShaderDescriptor_t& srBuffer, const char* spLayoutName, const char* spSetName, int sCount )
// {
// 	return Graphics_CreateVariableDescLayout( EDescriptorType_StorageBuffer, srBuffer, spLayoutName, spSetName, sCount );
// }


bool Graphics_CreateShaderBuffers( EBufferFlags sFlags, std::vector< ch_handle_t >& srBuffers, const char* spBufferName, size_t sBufferSize )
{
	// create buffers for it
	for ( size_t i = 0; i < srBuffers.size(); i++ )
	{
		ch_handle_t buffer = render->CreateBuffer( spBufferName, sBufferSize, sFlags, EBufferMemory_Host );

		if ( buffer == CH_INVALID_HANDLE )
		{
			Log_Error( gLC_ClientGraphics, "Failed to Create Light Uniform Buffer\n" );
			return false;
		}

		srBuffers[ i ] = buffer;
	}

	// update the descriptor sets
	// WriteDescSet_t update{};
	// update.apDescSets    = srUniform.apSets;
	// update.aDescSetCount = srUniform.aCount;
	// update.aType    = sType;
	// update.apData = srBuffers.data();
	// render->UpdateDescSet( update );

	return true;
}


bool Graphics_CreateUniformBuffers( std::vector< ch_handle_t >& srBuffers, const char* spBufferName, size_t sBufferSize )
{
	return Graphics_CreateShaderBuffers( EDescriptorType_UniformBuffer, srBuffers, spBufferName, sBufferSize );
}


bool Graphics_CreateStorageBuffers( std::vector< ch_handle_t >& srBuffers, const char* spBufferName, size_t sBufferSize )
{
	return Graphics_CreateShaderBuffers( EDescriptorType_StorageBuffer, srBuffers, spBufferName, sBufferSize );
}


static void Graphics_AllocateShaderArray( ShaderArrayAllocator_t& srAllocator, u32 sCount, const char* spName )
{
	srAllocator.apName     = spName;
	srAllocator.aAllocated = sCount;
	srAllocator.aUsed      = 0;

	srAllocator.apUsed     = ch_calloc< u32 >( sCount );
	srAllocator.apFree     = ch_calloc< u32 >( sCount );

	// Fill the free list with handles, and the used list with invalid handles
	for ( u32 index = 0; index < sCount; index++ )
	{
		srAllocator.apFree[ index ] = ( rand() % 0xFFFFFFFE ) + 1;  // Each slot gets a magic number
		srAllocator.apUsed[ index ] = UINT32_MAX;
	}
}


static void Graphics_FreeShaderArray( ShaderArrayAllocator_t& srAllocator )
{
	if ( srAllocator.apFree )
		free( srAllocator.apFree );

	if ( srAllocator.apUsed )
		free( srAllocator.apUsed );

	memset( &srAllocator, 0, sizeof( ShaderArrayAllocator_t ) );
}


// writes data in regions to a staging host buffer, and then copies those regions to the device buffer
void Graphics_WriteDeviceBufferRegions()
{
}


bool Graphics_CreateStagingBuffer( DeviceBufferStaging_t& srStaging, size_t sSize, const char* spStagingName, const char* spName )
{
	srStaging.aStagingBuffer = render->CreateBuffer( spStagingName, sSize, EBufferFlags_TransferSrc, EBufferMemory_Host );
	srStaging.aBuffer        = render->CreateBuffer( spName, sSize, EBufferFlags_Storage | EBufferFlags_TransferDst, EBufferMemory_Device );

	if ( !srStaging.aBuffer || !srStaging.aStagingBuffer )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to Create %s\n", spName );
		return false;
	}

	return true;
}


bool Graphics_CreateStagingUniformBuffer( DeviceBufferStaging_t& srStaging, size_t sSize, const char* spStagingName, const char* spName )
{
	srStaging.aStagingBuffer = render->CreateBuffer( spStagingName, sSize, EBufferFlags_TransferSrc, EBufferMemory_Host );
	srStaging.aBuffer        = render->CreateBuffer( spName, sSize, EBufferFlags_Uniform | EBufferFlags_TransferDst, EBufferMemory_Device );

	if ( !srStaging.aBuffer || !srStaging.aStagingBuffer )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to Create %s\n", spName );
		return false;
	}

	return true;
}


void Graphics_FreeStagingBuffer( DeviceBufferStaging_t& srStaging )
{
	if ( srStaging.aStagingBuffer )
		render->DestroyBuffer( srStaging.aStagingBuffer );

	if ( srStaging.aBuffer )
		render->DestroyBuffer( srStaging.aBuffer );

	srStaging.aStagingBuffer = CH_INVALID_HANDLE;
	srStaging.aBuffer        = CH_INVALID_HANDLE;
}


bool Graphics_CreateDescriptorSets( ShaderRequirmentsList_t& srRequire )
{
	// ------------------------------------------------------
	// Create Core Data Array Slots

	Graphics_AllocateShaderArray( gGraphicsData.aCoreDataSlots[ EShaderCoreArray_LightWorld ], CH_R_MAX_LIGHT_TYPE, "LightWorld" );
	Graphics_AllocateShaderArray( gGraphicsData.aCoreDataSlots[ EShaderCoreArray_LightPoint ], CH_R_MAX_LIGHT_TYPE, "LightPoint" );
	Graphics_AllocateShaderArray( gGraphicsData.aCoreDataSlots[ EShaderCoreArray_LightCone ], CH_R_MAX_LIGHT_TYPE, "LightCone" );
	Graphics_AllocateShaderArray( gGraphicsData.aCoreDataSlots[ EShaderCoreArray_LightCapsule ], CH_R_MAX_LIGHT_TYPE, "LightCapsule" );

	Graphics_AllocateShaderArray( gGraphicsData.aViewportSlots, CH_R_MAX_VIEWPORTS, "Viewports" );

	gGraphicsData.aRenderableData  = ch_calloc< Shader_Renderable_t >( CH_R_MAX_RENDERABLES );
	gGraphicsData.aModelMatrixData = ch_calloc< glm::mat4 >( CH_R_MAX_RENDERABLES );
	gGraphicsData.aViewportData    = ch_calloc< Shader_Viewport_t >( CH_R_MAX_VIEWPORTS );

	gGraphicsData.aVertexBuffers.aBuffers.reserve( CH_R_MAX_VERTEX_BUFFERS );
	gGraphicsData.aIndexBuffers.aBuffers.reserve( CH_R_MAX_INDEX_BUFFERS );
	gGraphicsData.aBlendShapeWeightBuffers.aBuffers.reserve( CH_R_MAX_BLEND_SHAPE_WEIGHT_BUFFERS );
	gGraphicsData.aBlendShapeDataBuffers.aBuffers.reserve( CH_R_MAX_BLEND_SHAPE_DATA_BUFFERS );
	
	// ------------------------------------------------------
	// Create Core Data Buffer

	if ( !Graphics_CreateStagingBuffer( gGraphicsData.aCoreDataStaging, sizeof( Buffer_Core_t ), "Core Staging", "Core" ) )
		return false;
	
	// ------------------------------------------------------
	// Create Viewport Data Buffer

	if ( !Graphics_CreateStagingUniformBuffer( gGraphicsData.aViewportStaging, sizeof( Shader_Viewport_t ) * CH_R_MAX_VIEWPORTS, "Viewport Staging", "Viewport" ) )
		return false;

	// ------------------------------------------------------
	// Create Model Matrix Data Buffer

	if ( !Graphics_CreateStagingBuffer( gGraphicsData.aModelMatrixStaging, sizeof( glm::mat4 ) * CH_R_MAX_RENDERABLES, "Model Matrix Staging", "Model Matrix" ) )
		return false;
	
	// ------------------------------------------------------
	// Create Renderable Data Buffer

	if ( !Graphics_CreateStagingBuffer( gGraphicsData.aRenderableStaging, sizeof( Shader_Renderable_t ) * CH_R_MAX_RENDERABLES, "Renderable Staging", "Renderable" ) )
		return false;

	// ------------------------------------------------------
	// Create Core Descriptor Set
	{
		CreateDescLayout_t createLayout{};
		createLayout.apName                      = "Global Layout";

		CreateDescBinding_t& texBinding          = createLayout.aBindings.emplace_back();
		texBinding.aBinding                      = CH_BINDING_TEXTURES;
		texBinding.aCount                        = CH_R_MAX_TEXTURES;
		texBinding.aStages                       = ShaderStage_All;
		texBinding.aType                         = EDescriptorType_CombinedImageSampler;

		CreateDescBinding_t& validation          = createLayout.aBindings.emplace_back();
		validation.aBinding                      = CH_BINDING_CORE;
		validation.aCount                        = 1;
		validation.aStages                       = ShaderStage_All;
		validation.aType                         = EDescriptorType_StorageBuffer;

		CreateDescBinding_t& viewports           = createLayout.aBindings.emplace_back();
		viewports.aBinding                       = CH_BINDING_VIEWPORTS;
		viewports.aCount                         = 1;
		viewports.aStages                        = ShaderStage_All;
		viewports.aType                          = EDescriptorType_UniformBuffer;

		CreateDescBinding_t& renderables         = createLayout.aBindings.emplace_back();
		renderables.aBinding                     = CH_BINDING_RENDERABLES;
		renderables.aCount                       = 1;
		renderables.aStages                      = ShaderStage_All;
		renderables.aType                        = EDescriptorType_StorageBuffer;

		CreateDescBinding_t& modelMatrices       = createLayout.aBindings.emplace_back();
		modelMatrices.aBinding                   = CH_BINDING_MODEL_MATRICES;
		modelMatrices.aCount                     = 1;
		modelMatrices.aStages                    = ShaderStage_All;
		modelMatrices.aType                      = EDescriptorType_StorageBuffer;

		CreateDescBinding_t& vertexBuffers       = createLayout.aBindings.emplace_back();
		vertexBuffers.aBinding                   = CH_BINDING_VERTEX_BUFFERS;
		vertexBuffers.aCount                     = CH_R_MAX_VERTEX_BUFFERS;
		vertexBuffers.aStages                    = ShaderStage_All;
		vertexBuffers.aType                      = EDescriptorType_StorageBuffer;
		
		CreateDescBinding_t& indexBuffers        = createLayout.aBindings.emplace_back();
		indexBuffers.aBinding                    = CH_BINDING_INDEX_BUFFERS;
		indexBuffers.aCount                      = CH_R_MAX_INDEX_BUFFERS;
		indexBuffers.aStages                     = ShaderStage_All;
		indexBuffers.aType                       = EDescriptorType_StorageBuffer;

		// TODO: this is for 2 swap chain images, but the swap chain image count could be different
		gShaderDescriptorData.aGlobalSets.aCount = 2;
		gShaderDescriptorData.aGlobalSets.apSets = ch_calloc< ch_handle_t >( gShaderDescriptorData.aGlobalSets.aCount );

		if ( !Graphics_CreateDescLayout( createLayout, gShaderDescriptorData.aGlobalLayout, gShaderDescriptorData.aGlobalSets.apSets, 2, "Global Sets" ) )
			return false;

		// update the descriptor sets
		WriteDescSet_t update{};

		update.aDescSetCount = gShaderDescriptorData.aGlobalSets.aCount;
		update.apDescSets    = gShaderDescriptorData.aGlobalSets.apSets;

		update.aBindingCount = static_cast< u32 >( createLayout.aBindings.size() - 2 );  // don't write anything for vertex and index buffers
		update.apBindings    = ch_calloc< WriteDescSetBinding_t >( update.aBindingCount );

		size_t i             = 0;
		for ( const CreateDescBinding_t& binding : createLayout.aBindings )
		{
			update.apBindings[ i ].aBinding = binding.aBinding;
			update.apBindings[ i ].aType    = binding.aType;
			update.apBindings[ i ].aCount   = binding.aCount;
			i++;

			if ( i == update.aBindingCount )
				break;
		}

		update.apBindings[ CH_BINDING_TEXTURES ].apData       = ch_calloc< ch_handle_t >( CH_R_MAX_TEXTURES );
		update.apBindings[ CH_BINDING_CORE ].apData           = &gGraphicsData.aCoreDataStaging.aBuffer;

		update.apBindings[ CH_BINDING_VIEWPORTS ].apData      = &gGraphicsData.aViewportStaging.aBuffer;
		update.apBindings[ CH_BINDING_RENDERABLES ].apData    = &gGraphicsData.aRenderableStaging.aBuffer;
		update.apBindings[ CH_BINDING_MODEL_MATRICES ].apData = &gGraphicsData.aModelMatrixStaging.aBuffer;

		// update.aImages = gViewportBuffers;
		render->UpdateDescSets( &update, 1 );

		free( update.apBindings[ CH_BINDING_TEXTURES ].apData );
		free( update.apBindings );
	}

	render->SetTextureDescSet( gShaderDescriptorData.aGlobalSets.apSets, gShaderDescriptorData.aGlobalSets.aCount, 0 );

	// ------------------------------------------------------
	// Create Per Shader Descriptor Sets
	for ( ShaderRequirement_t& requirement : srRequire.aItems )
	{
		CreateDescLayout_t createLayout{};
		createLayout.apName = requirement.aShader.data();
		createLayout.aBindings.resize( requirement.aBindingCount );

		for ( u32 i = 0; i < requirement.aBindingCount; i++ )
		{
			createLayout.aBindings[ i ] = requirement.apBindings[ i ];
		}

		ShaderDescriptor_t& descriptor = gShaderDescriptorData.aPerShaderSets[ requirement.aShader ];
		descriptor.aCount              = 2;
		descriptor.apSets              = ch_calloc< ch_handle_t >( descriptor.aCount );

		if ( !Graphics_CreateDescLayout( createLayout, gShaderDescriptorData.aPerShaderLayout[ requirement.aShader ], descriptor.apSets, descriptor.aCount, "Shader Sets" ) )
		{
			Log_ErrorF( "Failed to Create Descriptor Set Layout for shader \"%s\"\n", requirement.aShader.data() );
			return false;
		}
	}

	return true;
}


u32 Graphics_AllocateCoreSlot( EShaderCoreArray sSlot )
{
	if ( sSlot >= EShaderCoreArray_Count )
	{
		Log_ErrorF( gLC_ClientGraphics, "Invalid core shader data array (%zd), only %d arrays\n", sSlot, EShaderCoreArray_Count );
		return CH_SHADER_CORE_SLOT_INVALID;
	}

	return Graphics_AllocateShaderSlot( gGraphicsData.aCoreDataSlots[ sSlot ] );
}


void Graphics_FreeCoreSlot( EShaderCoreArray sSlot, u32 sIndex )
{
	if ( sSlot >= EShaderCoreArray_Count )
	{
		Log_ErrorF( gLC_ClientGraphics, "Invalid core shader data array (%d), only %d arrays\n", sSlot, EShaderCoreArray_Count );
		return;
	}

	Graphics_FreeShaderSlot( gGraphicsData.aCoreDataSlots[ sSlot ], sIndex );
}


u32 Graphics_GetCoreSlot( EShaderCoreArray sSlot, u32 sIndex )
{
	return Graphics_GetShaderSlot( gGraphicsData.aCoreDataSlots[ sSlot ], sIndex );
}


u32 Graphics_AllocateShaderSlot( ShaderArrayAllocator_t& srAllocator )
{
	if ( srAllocator.aUsed == srAllocator.aAllocated )
	{
		Log_ErrorF( gLC_ClientGraphics, "Out of slots for allocating shader data \"%s\", max of %zd\n",
		            srAllocator.apName ? srAllocator.apName : "UNKNOWN", srAllocator.aAllocated );

		return UINT32_MAX;
	}

	CH_ASSERT( srAllocator.apFree );

	// Get the base of this free list
	u32 handle = srAllocator.apFree[ 0 ];

	if ( handle == UINT32_MAX )
	{
		Log_ErrorF( gLC_ClientGraphics, "Out of slots in shader array \"%s\"\n", srAllocator.apName );
		return UINT32_MAX;
	}

	// Copy this to the used list
	srAllocator.apUsed[ srAllocator.aUsed++ ] = handle;

	// shift everything down by one
	memcpy( &srAllocator.apFree[ 0 ], &srAllocator.apFree[ 1 ], sizeof( u32 ) * ( srAllocator.aAllocated - 1 ) );

	// mark the very end of the list as invalid
	srAllocator.apFree[ srAllocator.aAllocated - srAllocator.aUsed ] = UINT32_MAX;

	// mark this as dirty
	srAllocator.aDirty                                               = true;

	return handle;
}


void Graphics_FreeShaderSlot( ShaderArrayAllocator_t& srAllocator, u32 sHandle )
{
	if ( srAllocator.aUsed == 0 )
	{
		Log_ErrorF( gLC_ClientGraphics, "No slots in use in shader data for %s type, can't free this slot\n", srAllocator.apName ? srAllocator.apName : "UNKNOWN" );
		return;
	}

	CH_ASSERT( srAllocator.apFree );
	CH_ASSERT( srAllocator.apFree[ srAllocator.aAllocated - srAllocator.aUsed ] == UINT32_MAX );

	// Find the index of this handle in the used list
	u32 index = 0;
	for ( ; index < srAllocator.aUsed; index++ )
	{
		if ( srAllocator.apUsed[ index ] == sHandle )
			break;
	}

	if ( index == srAllocator.aAllocated )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to find handle in shader slot \"%s\"\n", srAllocator.apName );
		return;
	}

	// shift everything down by one if there's anything after
	// don't shift anything if this is the last handle
	if ( index + 1 < srAllocator.aAllocated )
	{
		memcpy( &srAllocator.apUsed[ index ], &srAllocator.apUsed[ index + 1 ], sizeof( u32 ) * ( srAllocator.aAllocated - 1 ) );
	}

	// write this free handle
	srAllocator.apFree[ srAllocator.aAllocated - srAllocator.aUsed ] = sHandle;
	srAllocator.apUsed[ srAllocator.aUsed ]                          = 0;
	srAllocator.aUsed--;

	// mark this as dirty
	srAllocator.aDirty = true;
}


u32 Graphics_GetShaderSlot( ShaderArrayAllocator_t& srAllocator, u32 sHandle )
{
	if ( srAllocator.aUsed == 0 )
	{
		Log_ErrorF( gLC_ClientGraphics, "No slots in use in shader data for %s type, can't free this slot\n", srAllocator.apName ? srAllocator.apName : "UNKNOWN" );
		return UINT32_MAX;
	}

	for ( u32 index = 0; index < srAllocator.aUsed; index++ )
	{
		if ( srAllocator.apUsed[ index ] == sHandle )
			return index;
	}

	Log_ErrorF( gLC_ClientGraphics, "Failed to find handle in shader slot \"%s\"\n", srAllocator.apName );
	return UINT32_MAX;
}


// return a magic number
u32 Graphics_AddShaderBuffer( ShaderBufferList_t& srBufferList, ch_handle_t sBuffer )
{
	// Generate a handle magic number.
	u32 magic = ( rand() % 0xFFFFFFFE ) + 1;

	srBufferList.aBuffers[ magic ] = sBuffer;
	srBufferList.aDirty            = true;

	return magic;
}


void Graphics_RemoveShaderBuffer( ShaderBufferList_t& srBufferList, u32 sHandle )
{
	srBufferList.aBuffers.erase( sHandle );
	srBufferList.aDirty = true;
}


ch_handle_t Graphics_GetShaderBuffer( const ShaderBufferList_t& srBufferList, u32 sHandle )
{
	if ( sHandle == UINT32_MAX )
		return CH_INVALID_HANDLE;

	auto it = srBufferList.aBuffers.find( sHandle );
	if ( it == srBufferList.aBuffers.end() )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to Find Buffer in ShaderBufferList_t! - %u\n", sHandle );
		return CH_INVALID_HANDLE;
	}

	return it->second;
}


u32 Graphics_GetShaderBufferIndex( const ShaderBufferList_t& srBufferList, u32 sHandle )
{
	if ( sHandle == UINT32_MAX )
		return sHandle;

	u32 i = 0;
	for ( const auto& [ handle, buffer ] : srBufferList.aBuffers )
	{
		if ( handle == sHandle )
			return i;

		i++;
	}

	return UINT32_MAX;

	// oh god no
	// return std::distance( std::begin( srBufferList.aBuffers ), srBufferList.aBuffers.find( sHandle ) );
}


void Graphics_OnTextureIndexUpdate()
{
	gGraphics.SetAllMaterialsDirty();
}


void Graphics_OnResetCallback( ch_handle_t window, ERenderResetFlags sFlags )
{
	// int width, height;
	// render->GetSurfaceSize( width, height );
	// 
	// if ( gGraphicsData.aViewData.aViewports.size() )
	// 	gGraphicsData.aViewData.aViewports[ 0 ].aSize = { width, height };

	if ( sFlags & ERenderResetFlags_MSAA )
	{
		Graphics_DestroyRenderPasses();

		if ( !Graphics_CreateRenderPasses() )
		{
			Log_Error( gLC_ClientGraphics, "Failed to create render passes\n" );
			return;
		}

		render->ShutdownImGui();
		if ( !render->InitImGui( gGraphicsData.aRenderPassGraphics ) )
		{
			Log_Error( gLC_ClientGraphics, "Failed to re-init ImGui for Vulkan\n" );
			return;
		}

		if ( !Graphics_ShaderInit( true ) )
		{
			Log_Error( gLC_ClientGraphics, "Failed to Recreate Shaders!\n" );
			return;
		}
	}

	gRenderOld.CreateSelectionTexture();
}


bool Graphics::Init()
{
	// Make sure we have the render dll
	render = Mod_GetSystemCast< IRender >( IRENDER_NAME, IRENDER_VER );
	if ( render == nullptr )
	{
		Log_Error( gLC_ClientGraphics, "Failed to load Renderer\n" );
		return false;
	}

	render->SetTextureIndexUpdateCallback( Graphics_OnTextureIndexUpdate );

	if ( !Graphics_CreateRenderPasses() )
	{
		return false;
	}

	// Get information about the shaders we need for creating descriptor sets
	ShaderRequirmentsList_t shaderRequire{};
	if ( !Shader_ParseRequirements( shaderRequire ) )
	{
		Log_Error( gLC_ClientGraphics, "Failed to Parse Shader Requirements!\n" );
		return false;
	}

	if ( !Graphics_CreateDescriptorSets( shaderRequire ) )
	{
		return false;
	}

	if ( !Graphics_ShaderInit( false ) )
	{
		Log_Error( gLC_ClientGraphics, "Failed to Create Shaders!\n" );
		return false;
	}

	if ( !Graphics_DebugDrawInit() )
		return false;

	// Rml::SetRenderInterface( &gRmlRender );

	// return render->InitImGui( gGraphicsData.aRenderPassGraphics );
	return true;
}


void Graphics_FreeBufferList( ShaderBufferList_t& bufferList )
{
	for ( auto& [ magic, buffer ] : bufferList.aBuffers )
	{
		render->DestroyBuffer( buffer );
	}

	bufferList.aBuffers.clear();
	bufferList.aDirty = true;
}


void Graphics_FreeBufferStaging( DeviceBufferStaging_t& bufferStaging )
{
	// The staging buffer and device buffer are right next to each other in memory here, so this works
	render->DestroyBuffers( &bufferStaging.aStagingBuffer, 2 );
}


void Graphics::Shutdown()
{
	// TODO: Free Descriptor Set allocations

	// Free Renderables
	for ( u32 i = 0; i < gGraphicsData.aRenderables.GetHandleCount(); i++ )
	{
		Renderable_t* renderable = nullptr;
		if ( !gGraphicsData.aRenderables.GetByIndex( i, &renderable ) )
			continue;

		if ( renderable->apDebugName )
			ch_str_free( renderable->apDebugName );
	}
	
	Graphics_DestroyLights();

	for ( u32 i = 0; i < EShaderCoreArray_Count; i++ )
	{
		Graphics_FreeShaderArray( gGraphicsData.aCoreDataSlots[ i ] );
	}

	Graphics_FreeShaderArray( gGraphicsData.aViewportSlots );

	if ( gGraphicsData.aModelMatrixData )
		free( gGraphicsData.aModelMatrixData );

	if ( gGraphicsData.aRenderableData )
		free( gGraphicsData.aRenderableData );

	if ( gGraphicsData.aViewportData )
		free( gGraphicsData.aViewportData );

	// Free Buffers
	Graphics_FreeBufferList( gGraphicsData.aBlendShapeDataBuffers );
	Graphics_FreeBufferList( gGraphicsData.aBlendShapeWeightBuffers );
	Graphics_FreeBufferList( gGraphicsData.aIndexBuffers );
	Graphics_FreeBufferList( gGraphicsData.aVertexBuffers );

	Graphics_FreeBufferStaging( gGraphicsData.aCoreDataStaging );
	Graphics_FreeBufferStaging( gGraphicsData.aModelMatrixStaging );
	Graphics_FreeBufferStaging( gGraphicsData.aRenderableStaging );
	Graphics_FreeBufferStaging( gGraphicsData.aViewportStaging );

	// if ( gGraphicsData.aVertexBufferSlots.apFree )
	// 	free( gGraphicsData.aVertexBufferSlots.apFree );
	// 
	// if ( gGraphicsData.aIndexBufferSlots.apFree )
	// 	free( gGraphicsData.aIndexBufferSlots.apFree );
	// 
	// if ( gGraphicsData.aBlendShapeWeightSlots.apFree )
	// 	free( gGraphicsData.aBlendShapeWeightSlots.apFree );
	// 
	// // free model buffer arrays
	// if ( gGraphicsData.aVertexBuffers )
	// 	free( gGraphicsData.aVertexBuffers );
	// 
	// if ( gGraphicsData.aIndexBuffers )
	// 	free( gGraphicsData.aIndexBuffers );
	// 
	// if ( gGraphicsData.aBlendShapeWeightBuffers )
	// 	free( gGraphicsData.aBlendShapeWeightBuffers );
}


RenderStats_t Graphics::GetStats()
{
	return gStats;
}


void Graphics::CreateFrustum( Frustum_t& srFrustum, const glm::mat4& srViewMat )
{
	PROF_SCOPE();

	glm::mat4 m                          = glm::transpose( srViewMat );
	glm::mat4 inv                        = glm::inverse( srViewMat );

	srFrustum.aPlanes[ EFrustum_Left ]   = m[ 3 ] + m[ 0 ];
	srFrustum.aPlanes[ EFrustum_Right ]  = m[ 3 ] - m[ 0 ];
	srFrustum.aPlanes[ EFrustum_Bottom ] = m[ 3 ] + m[ 1 ];
	srFrustum.aPlanes[ EFrustum_Top ]    = m[ 3 ] - m[ 1 ];
	srFrustum.aPlanes[ EFrustum_Near ]   = m[ 3 ] + m[ 2 ];
	srFrustum.aPlanes[ EFrustum_Far ]    = m[ 3 ] - m[ 2 ];

	// Calculate Frustum Points
	for ( int i = 0; i < 8; i++ )
	{
		glm::vec4 ff             = inv * gFrustumFaceData[ i ];
		srFrustum.aPoints[ i ].x = ff.x / ff.w;
		srFrustum.aPoints[ i ].y = ff.y / ff.w;
		srFrustum.aPoints[ i ].z = ff.z / ff.w;
	}
}


Frustum_t Graphics::CreateFrustum( const glm::mat4& srViewInfo )
{
	Frustum_t frustum;
	gGraphics.CreateFrustum( frustum, srViewInfo );
	return frustum;
}


ModelBBox_t Graphics::CreateWorldAABB( glm::mat4& srMatrix, const ModelBBox_t& srBBox )
{
	PROF_SCOPE();

	glm::vec4 corners[ 8 ];

	// Fill array with the corners of the AABB 
	corners[ 0 ] = { srBBox.aMin.x, srBBox.aMin.y, srBBox.aMin.z, 1.f };
	corners[ 1 ] = { srBBox.aMin.x, srBBox.aMin.y, srBBox.aMax.z, 1.f };
	corners[ 2 ] = { srBBox.aMin.x, srBBox.aMax.y, srBBox.aMin.z, 1.f };
	corners[ 3 ] = { srBBox.aMax.x, srBBox.aMin.y, srBBox.aMin.z, 1.f };
	corners[ 4 ] = { srBBox.aMin.x, srBBox.aMax.y, srBBox.aMax.z, 1.f };
	corners[ 5 ] = { srBBox.aMax.x, srBBox.aMin.y, srBBox.aMax.z, 1.f };
	corners[ 6 ] = { srBBox.aMax.x, srBBox.aMax.y, srBBox.aMin.z, 1.f };
	corners[ 7 ] = { srBBox.aMax.x, srBBox.aMax.y, srBBox.aMax.z, 1.f };

	glm::vec3 globalMin;
	glm::vec3 globalMax;

	// Transform all of the corners, and keep track of the greatest and least
	// values we see on each coordinate axis.
	for ( int i = 0; i < 8; i++ )
	{
		glm::vec3 transformed = srMatrix * corners[ i ];

		if ( i > 0 )
		{
			globalMin = glm::min( globalMin, transformed );
			globalMax = glm::max( globalMax, transformed );
		}
		else
		{
			globalMin = transformed;
			globalMax = transformed;
		}
	}

	ModelBBox_t aabb( globalMin, globalMax );
	return aabb;
}


void UpdateRenderableDescSets()
{
	// Skinning Shader
}


void FreeRenderableModel( Renderable_t* renderable )
{
	if ( !renderable->aModel )
		return;

	for ( u32 i = 0; i < renderable->aMaterialCount; i++ )
	{
		// Decrement the ref count
		gGraphics.Mat_RemoveRef( renderable->apMaterials[ i ] );
	}

	if ( renderable->aBlendShapeWeightsBuffer )
	{
		if ( renderable->aVertexIndex != UINT32_MAX )
		{
			Graphics_RemoveShaderBuffer( gGraphicsData.aVertexBuffers, renderable->aVertexIndex );
		}

		if ( renderable->aBlendShapeWeightsIndex != UINT32_MAX )
		{
			Graphics_RemoveShaderBuffer( gGraphicsData.aBlendShapeWeightBuffers, renderable->aBlendShapeWeightsIndex );
		}

		render->DestroyBuffer( renderable->aBlendShapeWeightsBuffer );
		renderable->aBlendShapeWeightsBuffer = CH_INVALID_HANDLE;

		// If we have a blend shape weights buffer, then we have custom vertex buffers for this renderable
		render->DestroyBuffer( renderable->aVertexBuffer );
	}

	renderable->aVertexBuffer = CH_INVALID_HANDLE;
	renderable->aVertexIndex  = UINT32_MAX;
	renderable->aIndexHandle  = UINT32_MAX;
}


void SetRenderableModel( ch_handle_t modelHandle, Model* model, Renderable_t* renderable )
{
	// HACK - REMOVE WHEN WE ADD QUEUED DELETION FOR ASSETS
	render->WaitForQueues();

	if ( renderable->aModel )
	{
		FreeRenderableModel( renderable );
	}

	renderable->aModel = modelHandle;
	
	// Setup Materials
	renderable->aMaterialCount = model->aMeshes.size();

	if ( renderable->aMaterialCount )
	{
		renderable->apMaterials = ch_malloc< ch_handle_t >( renderable->aMaterialCount );

		for ( u32 i = 0; i < renderable->aMaterialCount; i++ )
		{
			// Increase the ref count
			gGraphics.Mat_AddRef( model->aMeshes[ i ].aMaterial );
			renderable->apMaterials[ i ] = model->aMeshes[ i ].aMaterial;
		}
	}

	// memset( &modelDraw->aAABB, 0, sizeof( ModelBBox_t ) );
	// Graphics_UpdateModelAABB( modelDraw );
	renderable->aAABB = gGraphics.CreateWorldAABB( renderable->aModelMatrix, gGraphicsData.aModelBBox[ renderable->aModel ] );

	renderable->aBlendShapeWeights.resize( model->apVertexData->aBlendShapeCount );

	if ( renderable->aBlendShapeWeights.size() )
	{
		// we need new vertex buffers for the modified vertices
		size_t bufferSize         = sizeof( Shader_VertexData_t ) * model->apVertexData->aCount;

		renderable->aVertexBuffer = render->CreateBuffer(
		  "output renderable vertices idfk",
		  bufferSize,
		  EBufferFlags_Storage | EBufferFlags_Vertex | EBufferFlags_TransferDst,
		  EBufferMemory_Device );

		BufferRegionCopy_t copy;
		copy.aSrcOffset = 0;
		copy.aDstOffset = 0;
		copy.aSize      = bufferSize;

		render->BufferCopyQueued( model->apBuffers->aVertex, renderable->aVertexBuffer, &copy, 1 );

		// Now Create a Blend Shape Weights Storage Buffer
		renderable->aBlendShapeWeightsBuffer = render->CreateBuffer(
		  "BlendShape Weights",
		  sizeof( float ) * renderable->aBlendShapeWeights.size(),
		  EBufferFlags_Storage,
		  EBufferMemory_Host );

		// Allocate Indexes for these
		renderable->aVertexIndex            = Graphics_AddShaderBuffer( gGraphicsData.aVertexBuffers, renderable->aVertexBuffer );
		renderable->aBlendShapeWeightsIndex = Graphics_AddShaderBuffer( gGraphicsData.aBlendShapeWeightBuffers, renderable->aBlendShapeWeightsBuffer );

		// update the descriptor sets for the skinning shader
		WriteDescSet_t update{};

		update.aDescSetCount            = gShaderDescriptorData.aPerShaderSets[ "__skinning" ].aCount;
		update.apDescSets               = gShaderDescriptorData.aPerShaderSets[ "__skinning" ].apSets;

		update.aBindingCount            = 2;
		update.apBindings               = ch_calloc< WriteDescSetBinding_t >( update.aBindingCount );

		update.apBindings[ 0 ].aBinding = 0;
		update.apBindings[ 0 ].aType    = EDescriptorType_StorageBuffer;
		update.apBindings[ 0 ].aCount   = gGraphicsData.aBlendShapeWeightBuffers.aBuffers.size();
		update.apBindings[ 0 ].apData   = ch_calloc< ch_handle_t >( gGraphicsData.aBlendShapeWeightBuffers.aBuffers.size() );

		update.apBindings[ 1 ].aBinding = 1;
		update.apBindings[ 1 ].aType    = EDescriptorType_StorageBuffer;
		update.apBindings[ 1 ].aCount   = gGraphicsData.aBlendShapeDataBuffers.aBuffers.size();
		update.apBindings[ 1 ].apData   = ch_calloc< ch_handle_t >( gGraphicsData.aBlendShapeDataBuffers.aBuffers.size() );

		int i                           = 0;
		for ( const auto& [ index, buffer ] : gGraphicsData.aBlendShapeWeightBuffers.aBuffers )
		{
			update.apBindings[ 0 ].apData[ i++ ] = buffer;
		}

		i = 0;
		for ( const auto& [ index, buffer ] : gGraphicsData.aBlendShapeDataBuffers.aBuffers )
		{
			update.apBindings[ 1 ].apData[ i++ ] = buffer;
		}

		// update.aImages = gViewportBuffers;
		render->UpdateDescSets( &update, 1 );

		free( update.apBindings[ 0 ].apData );
		free( update.apBindings[ 1 ].apData );
		free( update.apBindings );
	}
	else
	{
		renderable->aVertexBuffer = model->apBuffers->aVertex;
		renderable->aVertexIndex  = model->apBuffers->aVertexHandle;
	}

	renderable->aIndexHandle                = model->apBuffers->aIndexHandle;

	gGraphicsData.aRenderableStaging.aDirty = true;
}


ch_handle_t Graphics::CreateRenderable( ch_handle_t sModel )
{
	Model* model = nullptr;
	if ( !gGraphicsData.aModels.Get( sModel, &model ) )
	{
		Log_Warn( gLC_ClientGraphics, "Renderable has no model!\n" );
		return CH_INVALID_HANDLE;
	}

	// Log_Dev( gLC_ClientGraphics, 2, "Created Renderable\n" );

	Renderable_t* renderable  = nullptr;
	ch_handle_t    drawHandle = CH_INVALID_HANDLE;

	if ( !( drawHandle = gGraphicsData.aRenderables.Create( &renderable ) ) )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to create Renderable_t\n" );
		return CH_INVALID_HANDLE;
	}

	renderable->aModelMatrix   = glm::identity< glm::mat4 >();
	renderable->aTestVis       = true;
	renderable->aCastShadow    = true;
	renderable->aVisible       = true;

	::SetRenderableModel( sModel, model, renderable );

	std::string_view modelPath = gGraphics.GetModelPath( sModel );

	if ( modelPath.size() )
	{
		SetRenderableDebugName( drawHandle, modelPath );
	}

	return drawHandle;
}


Renderable_t* Graphics::GetRenderableData( ch_handle_t sRenderable )
{
	PROF_SCOPE();

	Renderable_t* renderable = nullptr;
	if ( !gGraphicsData.aRenderables.Get( sRenderable, &renderable ) )
	{
		Log_Warn( gLC_ClientGraphics, "Failed to find Renderable!\n" );
		return nullptr;
	}

	return renderable;
}


void Graphics::SetRenderableModel( ch_handle_t sRenderable, ch_handle_t sModel )
{
	// TODO: queue this stuff for when the model is finished loading
	Renderable_t* renderable = nullptr;
	if ( !gGraphicsData.aRenderables.Get( sRenderable, &renderable ) )
	{
		Log_Warn( gLC_ClientGraphics, "Failed to find Renderable!\n" );
		return;
	}
	
	Model* model = nullptr;
	if ( !gGraphicsData.aModels.Get( sModel, &model ) )
	{
		// TODO: ERROR Model
		Log_Warn( gLC_ClientGraphics, "Renderable has no model!\n" );
		return;
	}

	::SetRenderableModel( sModel, model, renderable );
}


void Graphics::FreeRenderable( ch_handle_t sRenderable )
{
	// TODO: QUEUE THIS RENDERABLE FOR DELETION, DON'T DELETE THIS NOW, SAME WITH MODELS, MATERIALS, AND TEXTURES!!!

	if ( !sRenderable )
		return;

	Renderable_t* renderable = nullptr;
	if ( !gGraphicsData.aRenderables.Get( sRenderable, &renderable ) )
	{
		Log_Warn( gLC_ClientGraphics, "Failed to find Renderable to delete!\n" );
		return;
	}

	// HACK - REMOVE WHEN WE ADD QUEUED DELETION FOR ASSETS
	render->WaitForQueues();

	::FreeRenderableModel( renderable );

	// Reset this renderable in the shader
	gGraphicsData.aRenderableData[ renderable->aIndex ].aVertexBuffer = 0;
	gGraphicsData.aRenderableData[ renderable->aIndex ].aIndexBuffer  = 0;
	gGraphicsData.aRenderableData[ renderable->aIndex ].aLightCount   = 0;

	gGraphicsData.aRenderables.Remove( sRenderable );
	gGraphicsData.aRenderAABBUpdate.erase( sRenderable );
	gGraphicsData.aRenderableStaging.aDirty = true;

	Log_Dev( gLC_ClientGraphics, 1, "Freed Renderable\n" );
}


void Graphics::ResetRenderableMaterials( ch_handle_t sRenderable )
{
	Renderable_t* renderable = nullptr;
	if ( !gGraphicsData.aRenderables.Get( sRenderable, &renderable ) )
	{
		Log_Warn( gLC_ClientGraphics, "Failed to find Renderable to reset materials on!\n" );
		return;
	}

	Model* model = nullptr;
	if ( !gGraphicsData.aModels.Get( renderable->aModel, &model ) )
	{
		Log_Warn( gLC_ClientGraphics, "Renderable has no model!\n" );
		return;
	}

	for ( u32 i = 0; i < renderable->aMaterialCount; i++ )
	{
		// Decrement the ref count
		gGraphics.Mat_RemoveRef( renderable->apMaterials[ i ] );

		// Increase the ref count
		gGraphics.Mat_AddRef( model->aMeshes[ i ].aMaterial );
		renderable->apMaterials[ i ] = model->aMeshes[ i ].aMaterial;
	}
}


void Graphics::UpdateRenderableAABB( ch_handle_t sRenderable )
{
	PROF_SCOPE();

	if ( !sRenderable )
		return;

	if ( Renderable_t* renderable = gGraphics.GetRenderableData( sRenderable ) )
		gGraphicsData.aRenderAABBUpdate.emplace( sRenderable, gGraphicsData.aModelBBox[ renderable->aModel ] );
}


ModelBBox_t Graphics::GetRenderableAABB( ch_handle_t sRenderable )
{
	PROF_SCOPE();

	if ( !sRenderable )
		return {};

	if ( Renderable_t* renderable = gGraphics.GetRenderableData( sRenderable ) )
		return renderable->aAABB;
}


void FreeRenderableDebugName( Renderable_t* renderable )
{
	if ( !renderable->apDebugName )
		return;

	free( renderable->apDebugName );
}


void Graphics::SetRenderableDebugName( ch_handle_t sRenderable, std::string_view sName )
{
	if ( Renderable_t* renderable = gGraphics.GetRenderableData( sRenderable ) )
	{
		FreeRenderableDebugName( renderable );

		if ( sName.empty() )
			return;

		renderable->apDebugName = ch_str_copy( sName.data(), sName.size() ).data;
	}
}


u32 Graphics::GetRenderableCount()
{
	return gGraphicsData.aRenderables.size();
}


ch_handle_t Graphics::GetRenderableByIndex( u32 i )
{
	return gGraphicsData.aRenderables.GetHandleByIndex( i );
}


void Graphics_ConsolidateRenderables()
{
	gGraphicsData.aRenderables.Consolidate();
}


// ---------------------------------------------------------------------------------------
// Vertex Format/Attributes


GraphicsFmt Graphics::GetVertexAttributeFormat( VertexAttribute attrib )
{
	switch ( attrib )
	{
		default:
			Log_ErrorF( gLC_ClientGraphics, "GetVertexAttributeFormat: Invalid VertexAttribute specified: %d\n", attrib );
			return GraphicsFmt::INVALID;

		case VertexAttribute_Position:
			return GraphicsFmt::RGB323232_SFLOAT;

		// NOTE: could be smaller probably
		case VertexAttribute_Normal:
			return GraphicsFmt::RGB323232_SFLOAT;

		case VertexAttribute_Tangent:
		case VertexAttribute_BiTangent:
			return GraphicsFmt::RGB323232_SFLOAT;

		case VertexAttribute_Color:
			return GraphicsFmt::RGBA32323232_SFLOAT;

		case VertexAttribute_TexCoord:
			return GraphicsFmt::RG3232_SFLOAT;
	}
}


size_t Graphics::GetVertexAttributeTypeSize( VertexAttribute attrib )
{
	GraphicsFmt format = gGraphics.GetVertexAttributeFormat( attrib );

	switch ( format )
	{
		default:
			Log_ErrorF( gLC_ClientGraphics, "GetVertexAttributeTypeSize: Invalid DataFormat specified from vertex attribute: %d\n", format );
			return 0;

		case GraphicsFmt::INVALID:
			return 0;

		case GraphicsFmt::RGBA32323232_SFLOAT:
		case GraphicsFmt::RGB323232_SFLOAT:
		case GraphicsFmt::RG3232_SFLOAT:
			return sizeof( float );
	}
}


size_t Graphics::GetVertexAttributeSize( VertexAttribute attrib )
{
	GraphicsFmt format = gGraphics.GetVertexAttributeFormat( attrib );

	switch ( format )
	{
		default:
			Log_ErrorF( gLC_ClientGraphics, "GetVertexAttributeSize: Invalid DataFormat specified from vertex attribute: %d\n", format );
			return 0;

		case GraphicsFmt::INVALID:
			return 0;

		case GraphicsFmt::RGBA32323232_SFLOAT:
			return ( 4 * sizeof( float ) );

		case GraphicsFmt::RGB323232_SFLOAT:
			return ( 3 * sizeof( float ) );

		case GraphicsFmt::RG3232_SFLOAT:
			return ( 2 * sizeof( float ) );

		case GraphicsFmt::R32_SFLOAT:
			return sizeof( float );
	}
}


size_t Graphics::GetVertexFormatSize( VertexFormat format )
{
	size_t size = 0;

	for ( int attrib = 0; attrib < VertexAttribute_Count; attrib++ )
	{
		// does this format contain this attribute?
		// if so, add the attribute size to it
		if ( format & ( 1 << attrib ) )
			size += gGraphics.GetVertexAttributeSize( (VertexAttribute)attrib );
	}

	return size;
}


void Graphics::GetVertexBindingDesc( VertexFormat sFormat, std::vector< VertexInputBinding_t >& srAttrib )
{
	size_t formatSize = gGraphics.GetVertexFormatSize( sFormat );
	srAttrib.emplace_back( 0, formatSize, false );

	// for ( u8 attrib = 0; attrib < VertexAttribute_Count; attrib++ )
	// {
	// 	// does this format contain this attribute?
	// 	// if so, add this attribute to the vector
	// 	if ( format & ( 1 << attrib ) )
	// 	{
	// 		srAttrib.emplace_back(
	// 		  0,
	// 		  (u32)Graphics_GetVertexAttributeSize( (VertexAttribute)attrib ),
	// 		  false );
	// 	}
	// }
}


void Graphics::GetVertexAttributeDesc( VertexFormat format, std::vector< VertexInputAttribute_t >& srAttrib )
{
	u32  location   = 0;
	u32  binding    = 0;
	u32  offset     = 0;

	for ( u8 attrib = 0; attrib < VertexAttribute_Count; attrib++ )
	{
		// does this format contain this attribute?
		// if so, add this attribute to the vector
		if ( format & ( 1 << attrib ) )
		{
			u32 attribSize = (u32)gGraphics.GetVertexAttributeSize( (VertexAttribute)attrib );

			srAttrib.emplace_back(
			  location++,
			  0,
			  gGraphics.GetVertexAttributeFormat( (VertexAttribute)attrib ),
			  offset
			);

			offset += attribSize;
		}
	}
}


const char* Graphics_GetVertexAttributeName( VertexAttribute attrib )
{
	switch ( attrib )
	{
		default:
		case VertexAttribute_Count:
			return "ERROR";

		case VertexAttribute_Position:
			return "Position";

		case VertexAttribute_Normal:
			return "Normal";

		case VertexAttribute_TexCoord:
			return "TexCoord";

		case VertexAttribute_Color:
			return "Color";
	}
}


// ---------------------------------------------------------------------------------------
// Buffers

// sBufferSize is sizeof(element) * count
ch_handle_t CreateModelBuffer( const char* spName, void* spData, size_t sBufferSize, EBufferFlags sUsage )
{
	PROF_SCOPE();

	ch_handle_t stagingBuffer = render->CreateBuffer( "Staging Model Buffer", sBufferSize, sUsage | EBufferFlags_TransferSrc, EBufferMemory_Host );

	// Copy Data to Buffer
	render->BufferWrite( stagingBuffer, sBufferSize, spData );

	ch_handle_t deviceBuffer = render->CreateBuffer( spName, sBufferSize, sUsage | EBufferFlags_TransferDst, EBufferMemory_Device );

	// Copy Local Buffer data to Device
	BufferRegionCopy_t copy;
	copy.aSrcOffset = 0;
	copy.aDstOffset = 0;
	copy.aSize      = sBufferSize;

	render->BufferCopy( stagingBuffer, deviceBuffer, &copy, 1 );

	render->DestroyBuffer( stagingBuffer );

	return deviceBuffer;
}


void Graphics::CreateVertexBuffers( ModelBuffers_t* spBuffer, VertexData_t* spVertexData, const char* spDebugName )
{
	PROF_SCOPE();

	if ( spVertexData == nullptr || spVertexData->aCount == 0 )
	{
		Log_Warn( gLC_ClientGraphics, "Trying to create Vertex Buffers for mesh with no vertices!\n" );
		return;
	}

	if ( spBuffer == nullptr )
	{
		Log_Warn( gLC_ClientGraphics, "Graphics_CreateVertexBuffers: ModelBuffers_t is nullptr!\n" );
		return;
	}

	size_t attribSize  = sizeof( Shader_VertexData_t );
	size_t attribCount = spVertexData->aData.size();

	// for ( size_t j = 0; j < spVertexData->aData.size(); j++ )
	// {
	// 	attribSize += Graphics_GetVertexAttributeSize( spVertexData->aData[ j ].aAttrib );
	// }

	size_t               bufferSize = attribSize * spVertexData->aCount;

	// HACK HACK HACK !!!!!!
	// We append all the data together for now just because i don't want to deal with changing a ton of code
	// Maybe later on we can do that
	Shader_VertexData_t* dataHack   = ch_calloc< Shader_VertexData_t >( spVertexData->aCount );

	if ( dataHack == nullptr )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to allocate vertex data array to copy to the gpu for \"%s\"\n", spDebugName );
		return;
	}

	size_t formatSize = gGraphics.GetVertexFormatSize( spVertexData->aFormat );

	// Slow as hell probably
	for ( size_t v = 0; v < spVertexData->aCount; v++ )
	{
		size_t dataOffset = 0;
		for ( size_t j = 0; j < spVertexData->aData.size(); j++ )
		{
			VertAttribData_t& data     = spVertexData->aData[ j ];
			size_t            elemSize = gGraphics.GetVertexAttributeSize( data.aAttrib );
			char*             dataSrc  = static_cast< char* >( data.apData ) + ( v * elemSize );

			switch ( data.aAttrib )
			{
				case VertexAttribute_Position:
				{
					memcpy( &dataHack[ v ].aPosNormX, dataSrc, elemSize );
					break;
				}

				case VertexAttribute_Normal:
				{
					memcpy( &dataHack[ v ].aPosNormX.w, dataSrc, 4 );
					memcpy( &dataHack[ v ].aNormYZ_UV, dataSrc + 4, 8 );
					break;
				}

				case VertexAttribute_TexCoord:
				{
					memcpy( &dataHack[ v ].aNormYZ_UV.z, dataSrc, 8 );
					break;
				}

				case VertexAttribute_Color:
				{
					memcpy( &dataHack[ v ].aColor, dataSrc, elemSize );
					break;
				}

				//case VertexAttribute_Tangent:
				//{
				//	memcpy( &dataHack[ v ].aTangentXYZ_BiTanX, dataSrc, elemSize );
				//	break;
				//}
				//
				//case VertexAttribute_BiTangent:
				//{
				//	memcpy( &dataHack[ v ].aBiTangentYZ, dataSrc, elemSize );
				//	break;
				//}
			}

			dataOffset += elemSize;
		}
	}

	char* bufferName = nullptr;

	if ( spDebugName )
	{
		size_t len = strlen( spDebugName );
		bufferName = new char[ len + 5 ];  // MEMORY LEAK - need string memory pool

		snprintf( bufferName, len + 5, "VB | %s", spDebugName );
	}

	// transfer source needed if using blend shapes or has a skeleton
	spBuffer->aVertex = CreateModelBuffer(
		bufferName ? bufferName : "VB",
		dataHack,
		bufferSize,
		EBufferFlags_Storage | EBufferFlags_Vertex | EBufferFlags_TransferSrc );

	free( dataHack );

	// Allocate an Index for this
	spBuffer->aVertexHandle = Graphics_AddShaderBuffer( gGraphicsData.aVertexBuffers, spBuffer->aVertex );

	// ch_handle_t Blend Shapes
	
	// TODO: this expects each vertex attribute to have it's own vertex buffer
	// but we want the blend shapes to all be in one huge storage buffer
	// what this expects is a buffer for each vertex attribute, for each blend shape
	// so we would need like (blend shape count) * (vertex attribute count) buffers for blend shapes,
	// that's not happening and needs to change

	if ( spVertexData->aBlendShapeCount == 0 )
		return;

	spBuffer->aBlendShape = CreateModelBuffer(
	  spDebugName ? spDebugName : "Blend Shapes",
	  spVertexData->apBlendShapeData,
	  attribSize * spVertexData->aCount * spVertexData->aBlendShapeCount,
	  EBufferFlags_Storage );

	spBuffer->aBlendShapeHandle = Graphics_AddShaderBuffer( gGraphicsData.aBlendShapeDataBuffers, spBuffer->aBlendShape );
}


void Graphics::CreateIndexBuffer( ModelBuffers_t* spBuffer, VertexData_t* spVertexData, const char* spDebugName )
{
	PROF_SCOPE();

	char* bufferName = nullptr;

	if ( spVertexData->aIndices.empty() )
	{
		Log_Warn( gLC_ClientGraphics, "Trying to create Index Buffer for mesh with no indices!\n" );
		return;
	}

	if ( spDebugName )
	{
		size_t len = strlen( spDebugName );
		bufferName = new char[ len + 7 ];  // MEMORY LEAK - need string memory pool

		snprintf( bufferName, len + 7, "IB | %s", spDebugName );
	}

	spBuffer->aIndex = CreateModelBuffer(
	  bufferName ? bufferName : "IB",
	  spVertexData->aIndices.data(),
	  // sizeof( u32 ) * spVertexData->aIndices.size(),
	  spVertexData->aIndices.size_bytes(),
	  EBufferFlags_Storage | EBufferFlags_Index );

	// Allocate an Index for this
	spBuffer->aIndexHandle = Graphics_AddShaderBuffer( gGraphicsData.aIndexBuffers, spBuffer->aIndex );
}


#if 0
void Graphics_CreateModelBuffers( ModelBuffers_t* spBuffers, VertexData_t* spVertexData, bool sCreateIndex, const char* spDebugName )
{
	if ( !spBuffers )
	{
		Log_Error( gLC_ClientGraphics, "ModelBuffers_t is nullptr\n" );
		return;
	}
	else if ( spBuffers->aVertex.size() )
	{
		Log_Error( gLC_ClientGraphics, "Model Vertex Buffers already created\n" );
		return;
	}

	char* debugName = nullptr;

	if ( spDebugName )
	{
		size_t nameLen = strlen( spDebugName );
		debugName      = new char[ nameLen ];  // MEMORY LEAK - need string memory pool
		snprintf( debugName, nameLen, "%s", spDebugName );
	}

	Graphics_CreateVertexBuffers( spBuffers, spVertexData, debugName );

	if ( sCreateIndex )
		Graphics_CreateIndexBuffer( spBuffers, spVertexData, debugName );
}
#endif


void DumpRenderableInfo( ch_handle_t renderHandle )
{
	Renderable_t* renderable = nullptr;
	if ( !gGraphicsData.aRenderables.Get( renderHandle, &renderable ) )
	{
		Log_ErrorF( gLC_ClientGraphics, "Invalid Handle Found for Renderable: %zd\n", renderHandle );
		return;
	}

	Log_MsgF(
	  gLC_ClientGraphics, "\n------------------------------------------------------------\n"
						  "Renderable %zd\n", renderHandle );

	// TODO: give models debug names
	std::string_view modelPath = gGraphics.GetModelPath( renderable->aModel );

	Log_MsgF( gLC_ClientGraphics, "    Name: \"%s\"\n", renderable->apDebugName );

	if ( modelPath.size() )
		Log_MsgF( gLC_ClientGraphics, "    Model: %zd - \"%s\"\n", renderable->aModel, modelPath.data() );
	else
		Log_MsgF( gLC_ClientGraphics, "    Model: %zd\n", renderable->aModel );

	Log_MsgF( gLC_ClientGraphics, "    Vertex Buffer: %zd\n", renderable->aVertexBuffer );

	Log_MsgF( gLC_ClientGraphics, "    Vertex Buffer Shader Handle: %d\n", renderable->aVertexIndex );
	Log_MsgF( gLC_ClientGraphics, "    Index Buffer Shader Handle: %d\n\n", renderable->aIndexHandle );

	Log_MsgF( gLC_ClientGraphics, "    Test Visibility: %s\n", renderable->aTestVis ? "True" : "False" );
	Log_MsgF( gLC_ClientGraphics, "    Cast Shadow:     %s\n", renderable->aCastShadow ? "True" : "False" );
	Log_MsgF( gLC_ClientGraphics, "    Visible:         %s\n", renderable->aVisible ? "True" : "False" );

	Log_MsgF( gLC_ClientGraphics, "\n    Material Count %d\n", renderable->aMaterialCount );

	for ( u32 matI = 0; matI < renderable->aMaterialCount; matI++ )
	{
		ch_handle_t material = renderable->apMaterials[ matI ];
		Log_MsgF( gLC_ClientGraphics, "        %d - %s\n", matI, gGraphics.Mat_GetName( material ) );
	}
}


CONCMD( r_dump_renderables )
{
	for ( u32 i = 0; i < gGraphicsData.aRenderables.size(); i++ )
	{
		ch_handle_t renderHandle = gGraphicsData.aRenderables.GetHandleByIndex( i );
		DumpRenderableInfo( renderHandle );
	}

	Log_MsgF( gLC_ClientGraphics, "\nRenderable Count %d\n", gGraphicsData.aRenderables.size() );
}


// CONCMD( r_dump_renderable_at_cursor )
// {
// 	// TODO: will need to enable selection
// }
