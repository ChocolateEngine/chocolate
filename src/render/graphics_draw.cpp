#include "core/core.h"
#include "igui.h"
#include "render/irender.h"
#include "graphics_int.h"
#include "lighting.h"
#include "debug_draw.h"
#include "mesh_builder.h"
#include "imgui/imgui.h"

#include <forward_list>
#include <stack>
#include <set>
#include <unordered_set>

// --------------------------------------------------------------------------------------

// shaders, fun
u32                                    Shader_Basic3D_UpdateMaterialData( Handle sMat );


// --------------------------------------------------------------------------------------
// Other

size_t                                 gModelDrawCalls = 0;
size_t                                 gVertsDrawn     = 0;

extern ChVector< Shader_VertexData_t > gDebugLineVerts;

CONVAR( r_vis, 1 );
CONVAR( r_vis_lock, 0 );

CONVAR( r_line_thickness, 2 );

CONVAR( r_show_draw_calls, 0 );

CONVAR( r_random_blend_shapes, 1 );
CONVAR( r_reset_blend_shapes, 0 );


bool Graphics_ViewFrustumTest( Renderable_t* spModelDraw, ViewportShader_t& srViewport )
{
	PROF_SCOPE();

	if ( !spModelDraw )
		return false;

	if ( !spModelDraw->aVisible )
		return false;

	if ( !srViewport.aActive )
		return false;

	// If visibility testing is disabled, or the object doesn't want vis testing, then it is always visible
	if ( !r_vis || !spModelDraw->aTestVis )
		return true;

	return srViewport.aFrustum.IsBoxVisible( spModelDraw->aAABB.aMin, spModelDraw->aAABB.aMax );
}


// TODO: experiment with instanced drawing
void Graphics_CmdDrawSurface( Handle cmd, Model* spModel, size_t sSurface )
{
	PROF_SCOPE();

	Mesh& mesh = spModel->aMeshes[ sSurface ];

	// TODO: figure out a way to use vertex and index offsets with this vertex format stuff
	// ideally, it would be less vertex buffer binding, but would be harder to pull off
	if ( spModel->apBuffers->aIndex )
		render->CmdDraw(
		  cmd,
		  mesh.aIndexCount,
		  1,
		  mesh.aIndexOffset,
		  0 );

		// render->CmdDrawIndexed(
		//   cmd,
		//   mesh.aIndexCount,
		//   1,                  // instance count
		//   mesh.aIndexOffset,
		//   0, // mesh.aVertexOffset,
		//   0 );

	else
		render->CmdDraw(
		  cmd,
		  mesh.aVertexCount,
		  1,
		  mesh.aVertexOffset,
		  0 );

	gModelDrawCalls++;
	gVertsDrawn += mesh.aVertexCount;
}


bool Graphics_BindModel( ChHandle_t cmd, VertexFormat sVertexFormat, Model* spModel, ChHandle_t sVertexBuffer )
{
#if 0
	PROF_SCOPE();

	// Bind the mesh's vertex and index buffers

	// Get Vertex Buffers the shader wants
	// TODO: what about if we don't have an attribute the shader wants???
	ChVector< ChHandle_t > vertexBuffers;

	// lazy hack, blech
	// ChVector< ChHandle_t >& allVertexBuffers = spRenderable->aOutVertexBuffers.size() ? spRenderable->aOutVertexBuffers : spModel->apBuffers->aVertex;

	// TODO: THIS CAN BE DONE WHEN ADDING THE MODEL TO THE MAIN DRAW LIST, AND PUT IN SurfaceDraw_t
	// for ( size_t i = 0; i < spModel->apVertexData->aData.size(); i++ )
	// {
	// 	VertAttribData_t& data = spModel->apVertexData->aData[ i ];
	// 
	// 	if ( sVertexFormat & ( 1 << data.aAttrib ) )
	// 		vertexBuffers.push_back( srVertexBuffers[ i ] );
	// }

	uint64_t                offsets = 0;

	// size_t* offsets = (size_t*)CH_STACK_ALLOC( sizeof( size_t ) * vertexBuffers.size() );
	// if ( offsets == nullptr )
	// {
	// 	Log_Error( gLC_ClientGraphics, "Graphics_BindModel: Failed to allocate vertex buffer offsets!\n" );
	// 	return false;
	// }
	// 
	// // TODO: i could probably use offsets here, i imagine it might actually be faster?
	// memset( offsets, 0, sizeof( size_t ) * vertexBuffers.size() );

	render->CmdBindVertexBuffers( cmd, 0, 1, &sVertexBuffer, &offsets );

	// TODO: store index type here somewhere
	if ( spModel->apBuffers->aIndex )
		render->CmdBindIndexBuffer( cmd, spModel->apBuffers->aIndex, 0, EIndexType_U32 );

	// SHADER: update and bind per object descriptor set?

	// CH_STACK_FREE( offsets );
#endif
	return true;
}


void Graphics_DrawShaderRenderables( Handle cmd, size_t sIndex, Handle shader, ChVector< SurfaceDraw_t >& srRenderList )
{
	PROF_SCOPE();

	if ( srRenderList.empty() )
		return;

	// if ( Log_GetDevLevel() > 2 )
	{
		const char* name = gGraphics.GetShaderName( shader );
		Log_DevF( 2, "Binding Shader: %s", name );
	}

	if ( !Shader_Bind( cmd, sIndex, shader ) )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to bind shader: %s\n", gGraphics.GetShaderName( shader ) );
		return;
	}

	SurfaceDraw_t* prevSurface = nullptr;
	Model*         prevModel   = nullptr;

	ShaderData_t*  shaderData  = Shader_GetData( shader );
	if ( !shaderData )
		return;

	if ( shaderData->aDynamicState & EDynamicState_LineWidth )
		render->CmdSetLineWidth( cmd, r_line_thickness );

	VertexFormat vertexFormat = Shader_GetVertexFormat( shader );

	for ( uint32_t i = 0; i < srRenderList.size(); )
	{
		SurfaceDraw_t& surfaceDraw = srRenderList[ i ];

		Renderable_t* renderable = nullptr;
		if ( !gGraphicsData.aRenderables.Get( surfaceDraw.aRenderable, &renderable ) )
		{
			Log_Warn( gLC_ClientGraphics, "Draw Data does not exist for renderable!\n" );
			srRenderList.remove( i );
			continue;
		}

		// get model and check if it's nullptr
		if ( renderable->aModel == InvalidHandle )
		{
			Log_Error( gLC_ClientGraphics, "Graphics::DrawShaderRenderables: model handle is InvalidHandle\n" );
			srRenderList.remove( i );
			continue;
		}

		// get model data
		Model* model = gGraphics.GetModelData( renderable->aModel );
		if ( !model )
		{
			Log_Error( gLC_ClientGraphics, "Graphics::DrawShaderRenderables: model is nullptr\n" );
			srRenderList.remove( i );
			continue;
		}

		// make sure this model has valid vertex buffers
		if ( model->apBuffers == nullptr || model->apBuffers->aVertex == CH_INVALID_HANDLE )
		{
			Log_Error( gLC_ClientGraphics, "No Vertex/Index Buffers for Model??\n" );
			srRenderList.remove( i );
			continue;
		}

		bool bindModel = !prevSurface;

		if ( prevSurface )
		{
			// bindModel |= prevSurface->apDraw->aModel != renderable->apDraw->aModel;
			// bindModel |= prevSurface->aSurface != renderable->aSurface;

			if ( prevModel )
			{
				bindModel |= prevModel->apBuffers != model->apBuffers;
				bindModel |= prevModel->apVertexData != model->apVertexData;
			}
		}

		if ( bindModel )
		{
			prevModel   = model;
			prevSurface = &surfaceDraw;
			if ( !Graphics_BindModel( cmd, vertexFormat, model, renderable->aVertexBuffer ) )
				continue;
		}

		i++;

		// NOTE: not needed if the material is the same i think
		if ( !Shader_PreRenderableDraw( cmd, sIndex, shaderData, surfaceDraw ) )
			continue;

		Graphics_CmdDrawSurface( cmd, model, surfaceDraw.aSurface );
	}
}


// Do Rendering with shader system and user land meshes
void Graphics_RenderView( Handle cmd, size_t sIndex, ViewportShader_t& srViewport, ViewRenderList_t& srViewList )
{
	PROF_SCOPE();

	if ( srViewport.aSize.x == 0 || srViewport.aSize.y == 0 )
	{
		Log_ErrorF( "Cannot Render View with width and/or height of 0!\n" );
		return;
	}

	// here we go again
	static Handle skybox    = gGraphics.GetShader( "skybox" );
	static Handle gizmo     = gGraphics.GetShader( "gizmo" );

	bool          hasSkybox = false;

	int width = 0, height = 0;
	render->GetSurfaceSize( width, height );

	Rect2D_t rect{};
	rect.aOffset.x = 0;
	rect.aOffset.y = 0;
	rect.aExtent.x = width;
	rect.aExtent.y = height;

	render->CmdSetScissor( cmd, 0, &rect, 1 );

	// flip viewport
	Viewport_t viewPort{};
	// viewPort.x        = 0.f;
	// viewPort.y        = height;

	viewPort.x        = srViewport.aOffset.x;
	viewPort.y        = srViewport.aSize.y + srViewport.aOffset.y;

	// viewPort.width    = width;
	// viewPort.height   = height * -1.f;

	viewPort.width    = srViewport.aSize.x;
	viewPort.height   = srViewport.aSize.y * -1.f;

	auto findGizmo    = srViewList.aRenderLists.find( gizmo );

	if ( findGizmo != srViewList.aRenderLists.end() )
	{
		viewPort.minDepth = 0.000f;
		viewPort.maxDepth = 0.001f;

		render->CmdSetViewport( cmd, 0, &viewPort, 1 );

		Graphics_DrawShaderRenderables( cmd, sIndex, gizmo, srViewList.aRenderLists[ gizmo ] );
	}

	viewPort.minDepth = 0.f;
	viewPort.maxDepth = 1.f;

	render->CmdSetViewport( cmd, 0, &viewPort, 1 );

	for ( auto& [ shader, renderList ] : srViewList.aRenderLists )
	{
		if ( shader == InvalidHandle )
		{
			Log_Warn( gLC_ClientGraphics, "Invalid Shader Handle (0) in View RenderList\n" );
			continue;
		}

		if ( shader == skybox )
		{
			hasSkybox = true;
			continue;
		}

		if ( shader == gizmo )
			continue;

		Graphics_DrawShaderRenderables( cmd, sIndex, shader, renderList );
	}

	// Draw Skybox - and set depth for skybox
	if ( hasSkybox )
	{
		viewPort.minDepth = 0.999f;
		viewPort.maxDepth = 1.f;

		render->CmdSetViewport( cmd, 0, &viewPort, 1 );

		Graphics_DrawShaderRenderables( cmd, sIndex, skybox, srViewList.aRenderLists[ skybox ] );
	}
}


void Graphics_Render( Handle sCmd, size_t sIndex, ERenderPass sRenderPass )
{
	PROF_SCOPE();

	// render->CmdBindDescriptorSets( sCmd, sIndex, EPipelineBindPoint_Graphics, PIPELINE_LAYOUT, SETS, SET_COUNT );

	// TODO: add in some dependency thing here, when you add camera's in the game, you'll need to render those first before the final viewports (VR maybe)
	for ( auto& [ viewHandle, viewRenderList ] : gGraphicsData.aViewRenderLists )
	{
		auto it = gGraphicsData.aViewports.find( viewHandle );

		if ( it == gGraphicsData.aViewports.end() )
		{
			Log_ErrorF( gLC_ClientGraphics, "Failed to get viewport data for rendering\n" );
			continue;
		}

		ViewportShader_t& viewport = it->second;

		// blech
		if ( !viewport.aActive )
			continue;

		// HACK HACK !!!!
		// don't render views with shader overrides here, the only override is the shadow map shader and selection
		// and that is rendered in a separate render pass
		if ( viewport.aShaderOverride )
			continue;

		Graphics_RenderView( sCmd, sIndex, viewport, viewRenderList );
	}
}


ChHandle_t Graphics_CreateRenderPass( EDescriptorType sType )
{
	RenderPassData_t passData{};

	return CH_INVALID_HANDLE;
}


#if 0
void Graphics_UpdateRenderPass( ChHandle_t sRenderPass )
{
	// update the descriptor sets
	WriteDescSet_t update{};

	update.aDescSets.push_back( gShaderDescriptorData.aPerPassSets[ sRenderPass ][ 0 ] );
	update.aDescSets.push_back( gShaderDescriptorData.aPerPassSets[ sRenderPass ][ 1 ] );

	update.aType    = EDescriptorType_StorageBuffer;
	update.aBuffers = gViewportBuffers;
	render->UpdateDescSet( update );
}


void Graphics_UpdateRenderPassBuffers( ERenderPass sRenderPass )
{
	// update the descriptor sets
	WriteDescSet_t update{};

	update.aDescSets.push_back( gShaderDescriptorData.aPerPassSets[ sRenderPass ][ 0 ] );
	update.aDescSets.push_back( gShaderDescriptorData.aPerPassSets[ sRenderPass ][ 1 ] );

	update.aType    = EDescriptorType_StorageBuffer;
	update.aBuffers = gViewportBuffers;
	render->UpdateDescSet( update );
}
#endif


u32 Graphics::CreateViewport( ViewportShader_t** spViewport )
{
	u32 handle = Graphics_AllocateShaderSlot( gGraphicsData.aViewportSlots );

	if ( handle == UINT32_MAX )
	{
		Log_Error( gLC_ClientGraphics, "Failed to allocate viewport\n" );
		return handle;
	}

	ViewportShader_t& viewport = gGraphicsData.aViewports[ handle ];

	if ( spViewport )
	{
		( *spViewport )             = &viewport;
		( *spViewport )->aActive    = true;
	}

	return handle;
}


void Graphics::FreeViewport( u32 sViewportHandle )
{
	// TODO: QUEUE THIS !!!!
	Graphics_FreeShaderSlot( gGraphicsData.aViewportSlots, sViewportHandle );

	// Remove the Viewport Data
	{
		auto it = gGraphicsData.aViewports.find( sViewportHandle );

		if ( it == gGraphicsData.aViewports.end() )
		{
			Log_ErrorF( gLC_ClientGraphics, "Failed to Free Viewport\n" );
			return;
		}

		gGraphicsData.aViewports.erase( it );
	}

	// Remove the View Render List
	{
		auto it = gGraphicsData.aViewRenderLists.find( sViewportHandle );

		if ( it == gGraphicsData.aViewRenderLists.end() )
		{
			Log_ErrorF( gLC_ClientGraphics, "Failed to Free Viewport Render List\n" );
			return;
		}

		gGraphicsData.aViewRenderLists.erase( it );
	}
}


// FUICK THIS WONT WORK !!!!
// DATA WILL BE LOST WHEN SWITCHING INDEX AAAAAAAAAAAAAAAAAAAAAAA


ViewportShader_t* Graphics::GetViewportData( u32 viewportHandle )
{
	auto it = gGraphicsData.aViewports.find( viewportHandle );

	if ( it == gGraphicsData.aViewports.end() )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to Find Viewport Data\n" );
		return nullptr;
	}

	ViewportShader_t* viewport = &it->second;
	return viewport;
}


void Graphics::SetViewportUpdate( bool sUpdate )
{
	gGraphicsData.aCoreDataStaging.aDirty = sUpdate;
}


void Graphics_PushViewInfo( const ViewportShader_t& srViewInfo )
{
	// gViewportStack.push( srViewInfo );
}


void Graphics_PopViewInfo()
{
	// if ( gViewportStack.empty() )
	// {
	// 	Log_Error( "Misplaced View Info Pop!\n" );
	// 	return;
	// }
	// 
	// gViewportStack.pop();
}


// ViewportShader_t& Graphics_GetViewInfo()
// {
// 	// if ( gViewportStack.empty() )
// 	// 	return gViewport[ 0 ];
// 	// 
// 	// return gViewportStack.top();
// }

