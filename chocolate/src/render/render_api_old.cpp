#include "graphics_int.h"
#include "lighting.h"
#include "debug_draw.h"
#include "imgui/imgui.h"

RenderSystemOld gRenderOld;

extern bool Graphics_ViewFrustumTest( Renderable_t* spModelDraw, ViewportShader_t& srViewport );
extern void Graphics_RenderView( ch_handle_t cmd, size_t sIndex, u32 sViewportIndex, ViewportShader_t& srViewport, ViewRenderList_t& srViewList );


CONVAR_BOOL_EXT( r_vis_lock );
CONVAR_BOOL_EXT( r_reset_blend_shapes );

CONVAR_BOOL( r_wireframe, 0 );


void RenderSystemOld::NewFrame()
{
	gStats.aDrawCalls        = 0;
	gStats.aMaterialsDrawn   = 0;
	gStats.aRenderablesDrawn = 0;
	gStats.aVerticesDrawn    = 0;

	render->NewFrame();

	Graphics_DebugDrawNewFrame();
}


void RenderSystemOld::Reset( ch_handle_t window )
{
	render->Reset( window );
}


void Graphics_DrawSelectionTextureRenderables( ch_handle_t cmd, size_t sIndex )
{
	PROF_SCOPE();

	ViewportShader_t* graphicsViewport = gGraphics.GetViewportData( gRenderOld.aSelectionViewport );

	Rect2D_t rect{};
	rect.aOffset.x = 0.f;
	rect.aOffset.y = 0.f;
	rect.aExtent.x = graphicsViewport->aSize.x;
	rect.aExtent.y = graphicsViewport->aSize.y;

	render->CmdSetScissor( cmd, 0, &rect, 1 );

	// flip viewport
	Viewport_t viewPort{};
	viewPort.x        = graphicsViewport->aOffset.x;
	viewPort.y        = graphicsViewport->aSize.y + graphicsViewport->aOffset.y;
	viewPort.width    = graphicsViewport->aSize.x;
	viewPort.height   = graphicsViewport->aSize.y * -1.f;

	render->CmdSetViewport( cmd, 0, &viewPort, 1 );

	auto it = gGraphicsData.aViewRenderLists.find( gRenderOld.aSelectionViewport );

	if ( it == gGraphicsData.aViewRenderLists.end() )
	{
		Log_Error( gLC_ClientGraphics, "Invalid Viewport Index for Selection Rendering\n" );
		return;
	}

	ViewRenderList_t& viewList = it->second;

	// HACK - EVIL
	// always draw stuff with the gizmo shader on top of everything

	ChVector< SurfaceDraw_t > gizmos;
	ChVector< SurfaceDraw_t > otherRenderables;

	static ch_handle_t     gizmo     = gGraphics.GetShader( "gizmo" );

	for ( auto& [ shader, renderList ] : viewList.aRenderLists )
	{
		for ( SurfaceDraw_t& surface : renderList )
		{
			Renderable_t* renderable = gGraphics.GetRenderableData( surface.aRenderable );
			ch_handle_t    mat        = renderable->apMaterials[ surface.aSurface ];
			ch_handle_t    matShader  = gGraphics.Mat_GetShader( mat );

			if ( matShader == gizmo )
				gizmos.push_back( surface );
			else
				otherRenderables.push_back( surface );
		}
	}

	static ch_handle_t     select    = gGraphics.GetShader( "__select" );

	auto findGizmo = viewList.aRenderLists.find( gizmo );

	// Draw Gizmos
	// if ( gizmos.size() )
	// {
	// 	viewPort.minDepth = 0.000f;
	// 	viewPort.maxDepth = 0.001f;
	// 	render->CmdSetViewport( cmd, 0, &viewPort, 1 );
	// 
	// 	Graphics_DrawShaderRenderables( cmd, sIndex, select, gRenderOld.aSelectionViewport, gizmos );
	// }

	// Draw Normal Renderables

	viewPort.minDepth = 0.f;
	viewPort.maxDepth = 1.f;
	render->CmdSetViewport( cmd, 0, &viewPort, 1 );

	u32 viewIndex = Graphics_GetShaderSlot( gGraphicsData.aViewportSlots, gRenderOld.aSelectionViewport );

	Graphics_DrawShaderRenderables( cmd, sIndex, select, viewIndex, otherRenderables );

	// for ( auto& [ shader, renderList ] : viewList.aRenderLists )
	// {
	// 	if ( shader == gizmo )
	// 		continue;
	// 
	// 	Graphics_DrawShaderRenderables( cmd, sIndex, shader, renderList );
	// }
}


void Graphics_SelectionTexturePass( ch_handle_t sCmd, size_t sIndex )
{
	PROF_SCOPE();

	ViewportShader_t* viewport = gGraphics.GetViewportData( gRenderOld.aSelectionViewport );

	if ( !viewport || viewport->aSize.x == 0 || viewport->aSize.y == 0 )
		return;

	RenderPassBegin_t renderPassBegin{};
	renderPassBegin.aClear.resize( 2 );
	renderPassBegin.aClear[ 0 ].color   = { 0.f, 0.f, 0.f, 1.f };
	renderPassBegin.aClear[ 0 ].aIsDepth = false;
	renderPassBegin.aClear[ 1 ].color   = { 0.f, 0.f, 0.f, 1.f };
	renderPassBegin.aClear[ 1 ].aIsDepth = true;

	renderPassBegin.aRenderPass          = gGraphicsData.aRenderPassSelect;
	renderPassBegin.aFrameBuffer         = gRenderOld.aSelectionFramebuffer;

	if ( renderPassBegin.aFrameBuffer == CH_INVALID_HANDLE )
		return;

	render->BeginRenderPass( sCmd, renderPassBegin );
	Graphics_DrawSelectionTextureRenderables( sCmd, sIndex );
	render->EndRenderPass( sCmd );
}


void Graphics_UpdateShaderBufferDescriptors( ShaderBufferList_t& srBufferList, u32 sBinding )
{
	srBufferList.aDirty = false;

	WriteDescSet_t write{};

	write.aDescSetCount = gShaderDescriptorData.aGlobalSets.aCount;
	write.apDescSets    = gShaderDescriptorData.aGlobalSets.apSets;

	write.aBindingCount = 1;
	write.apBindings    = CH_STACK_NEW( WriteDescSetBinding_t, write.aBindingCount );

	ChVector< ch_handle_t > buffers;
	buffers.reserve( srBufferList.aBuffers.size() );

	for ( auto& [ handle, buffer ] : srBufferList.aBuffers )
		buffers.push_back( buffer );

	write.apBindings[ 0 ].aBinding = sBinding;
	write.apBindings[ 0 ].aType    = EDescriptorType_StorageBuffer;
	write.apBindings[ 0 ].aCount   = buffers.size();
	write.apBindings[ 0 ].apData   = buffers.data();

	render->UpdateDescSets( &write, 1 );

	CH_STACK_FREE( write.apBindings );
}


void Graphics_PrepareDrawData()
{
	PROF_SCOPE();

	render->PreRenderPass();

	Shader_UpdateMaterialVars();

	// update renderable AABB's
	for ( auto& [ renderHandle, bbox ] : gGraphicsData.aRenderAABBUpdate )
	{
		if ( Renderable_t* renderable = gGraphics.GetRenderableData( renderHandle ) )
		{
			if ( glm::length( bbox.aMin ) == 0 && glm::length( bbox.aMax ) == 0 )
			{
				Log_Warn( gLC_ClientGraphics, "Model Bounding Box not calculated, length of min and max is 0\n" );
				bbox = gGraphics.CalcModelBBox( renderable->aModel );
			}

			renderable->aAABB = gGraphics.CreateWorldAABB( renderable->aModelMatrix, bbox );
		}
	}

	gGraphicsData.aRenderAABBUpdate.clear();

	// Update Light Data
	Graphics_PrepareLights();

	// --------------------------------------------------------------------

	Graphics_UpdateDebugDraw();

	bool updateShaderRenderables = gGraphicsData.aRenderableStaging.aDirty || gGraphicsData.aVertexBuffers.aDirty || gGraphicsData.aIndexBuffers.aDirty;

	// Update Vertex Buffer Array SSBO
	if ( gGraphicsData.aVertexBuffers.aDirty )
		Graphics_UpdateShaderBufferDescriptors( gGraphicsData.aVertexBuffers, CH_BINDING_VERTEX_BUFFERS );

	// Update Index Buffer Array SSBO
	if ( gGraphicsData.aIndexBuffers.aDirty )
		Graphics_UpdateShaderBufferDescriptors( gGraphicsData.aIndexBuffers, CH_BINDING_INDEX_BUFFERS );

	// --------------------------------------------------------------------

	Graphics_PrepareShadowRenderLists();

	if ( r_reset_blend_shapes )
		Con_SetConVarValue( "r_reset_blend_shapes", false );

	// --------------------------------------------------------------------
	// Prepare Skinning Compute Shader Buffers

	u32 r = 0;
	for ( ch_handle_t renderHandle : gGraphicsData.aSkinningRenderList )
	{
		Renderable_t* renderable = nullptr;
		if ( !gGraphicsData.aRenderables.Get( renderHandle, &renderable ) )
		{
			Log_Warn( gLC_ClientGraphics, "Renderable does not exist!\n" );
			continue;
		}

		r++;
		render->BufferWrite( renderable->aBlendShapeWeightsBuffer, renderable->aBlendShapeWeights.size_bytes(), renderable->aBlendShapeWeights.data() );
	}

	// Update Core Data SSBO

	if ( gGraphicsData.aCoreDataStaging.aDirty )
	{
		gGraphicsData.aCoreDataStaging.aDirty = false;
		render->BufferWrite( gGraphicsData.aCoreDataStaging.aStagingBuffer, sizeof( Buffer_Core_t ), &gGraphicsData.aCoreData );

		BufferRegionCopy_t copy;
		copy.aSrcOffset = 0;
		copy.aDstOffset = 0;
		copy.aSize      = sizeof( Buffer_Core_t );

		render->BufferCopyQueued( gGraphicsData.aCoreDataStaging.aStagingBuffer, gGraphicsData.aCoreDataStaging.aBuffer, &copy, 1 );
	}

	// Update Viewport SSBO
	{
		for ( auto& [ viewHandle, viewport ] : gGraphicsData.aViewports )
		{
			u32                viewIndex      = Graphics_GetShaderSlot( gGraphicsData.aViewportSlots, viewHandle );
			Shader_Viewport_t& viewportBuffer = gGraphicsData.aViewportData[ viewIndex ];

			viewportBuffer.aProjView          = viewport.aProjView;
			viewportBuffer.aProjection        = viewport.aProjection;
			viewportBuffer.aView              = viewport.aView;
			viewportBuffer.aViewPos           = viewport.aViewPos;
			viewportBuffer.aNearZ             = viewport.aNearZ;
			viewportBuffer.aFarZ              = viewport.aFarZ;
		}

		BufferRegionCopy_t copy;
		copy.aSrcOffset = 0;
		copy.aDstOffset = 0;
		copy.aSize      = sizeof( Shader_Viewport_t ) * CH_R_MAX_VIEWPORTS;

		render->BufferWrite( gGraphicsData.aViewportStaging.aStagingBuffer, copy.aSize, gGraphicsData.aViewportData );
		render->BufferCopyQueued( gGraphicsData.aViewportStaging.aStagingBuffer, gGraphicsData.aViewportStaging.aBuffer, &copy, 1 );
	}

	// Update Renderables SSBO
	// if ( gGraphicsData.aRenderableStaging.aDirty )
	if ( updateShaderRenderables )
	{
		gGraphicsData.aRenderableStaging.aDirty = false;

		// Update Renderable Vertex Buffer Handles
		for ( u32 i = 0; i < gGraphicsData.aRenderables.size(); )
		{
			Renderable_t* renderable = nullptr;
			if ( !gGraphicsData.aRenderables.Get( gGraphicsData.aRenderables.aHandles[ i ], &renderable ) )
			{
				Log_Warn( gLC_ClientGraphics, "Renderable handle is invalid!\n" );
				gGraphicsData.aRenderables.Remove( gGraphicsData.aRenderables.aHandles[ i ] );
				continue;
			}

			// if ( !renderable->aVisible )
			// {
			// 	i++;
			// 	continue;
			// }

			gGraphicsData.aRenderableData[ renderable->aIndex ].aVertexBuffer = Graphics_GetShaderBufferIndex( gGraphicsData.aVertexBuffers, renderable->aVertexIndex );
			gGraphicsData.aRenderableData[ renderable->aIndex ].aIndexBuffer  = Graphics_GetShaderBufferIndex( gGraphicsData.aIndexBuffers, renderable->aIndexHandle );

			i++;
		}

		BufferRegionCopy_t copy;
		copy.aSrcOffset = 0;
		copy.aDstOffset = 0;
		copy.aSize      = sizeof( Shader_Renderable_t ) * CH_R_MAX_RENDERABLES;

		render->BufferWrite( gGraphicsData.aRenderableStaging.aStagingBuffer, copy.aSize, gGraphicsData.aRenderableData );
		render->BufferCopyQueued( gGraphicsData.aRenderableStaging.aStagingBuffer, gGraphicsData.aRenderableStaging.aBuffer, &copy, 1 );
	}

	// Update Model Matrices SSBO
	// if ( gGraphicsData.aRenderableStaging.aDirty )
	{
		BufferRegionCopy_t copy;
		copy.aSrcOffset = 0;
		copy.aDstOffset = 0;
		copy.aSize      = sizeof( glm::mat4 ) * CH_R_MAX_RENDERABLES;

		render->BufferWrite( gGraphicsData.aModelMatrixStaging.aStagingBuffer, copy.aSize, gGraphicsData.aModelMatrixData );
		render->BufferCopyQueued( gGraphicsData.aModelMatrixStaging.aStagingBuffer, gGraphicsData.aModelMatrixStaging.aBuffer, &copy, 1 );
	}

	// Update Shader Draws Data SSBO
	// if ( gGraphicsData.aSurfaceDrawsStaging.aDirty )
	// {
	// 	gGraphicsData.aSurfaceDrawsStaging.aDirty = false;
	// 	render->BufferWrite( gGraphicsData.aSurfaceDrawsStaging.aStagingBuffer, sizeof( gGraphicsData.aSurfaceDraws ), &gGraphicsData.aSurfaceDraws );
	// 	render->BufferCopy( gGraphicsData.aSurfaceDrawsStaging.aStagingBuffer, gGraphicsData.aSurfaceDrawsStaging.aBuffer, sizeof( gGraphicsData.aSurfaceDraws ) );
	// }

	render->CopyQueuedBuffers();
}


void Graphics_DoSkinning( ch_handle_t sCmd, u32 sCmdIndex )
{
#if 1
	static ch_handle_t shaderSkinning = gGraphics.GetShader( "__skinning" );

	if ( shaderSkinning == CH_INVALID_HANDLE )
	{
		Log_Error( gLC_ClientGraphics, "skinning shader not found, can't apply blend shapes and bone transforms!\n" );
		return;
	}

	if ( !Shader_Bind( sCmd, sCmdIndex, shaderSkinning ) )
	{
		Log_Error( gLC_ClientGraphics, "Failed to bind skinning shader, can't apply blend shapes and bone transforms!\n" );
		return;
	}

	ShaderData_t*                             shaderSkinningData = Shader_GetData( shaderSkinning );

	ChVector< GraphicsBufferMemoryBarrier_t > buffers;

	u32                                       i = 0;
	for ( ch_handle_t renderHandle : gGraphicsData.aSkinningRenderList )
	{
		Renderable_t* renderable = nullptr;
		if ( !gGraphicsData.aRenderables.Get( renderHandle, &renderable ) )
		{
			Log_Warn( gLC_ClientGraphics, "Renderable does not exist!\n" );
			continue;
		}

		// get model data
		Model* model = gGraphics.GetModelData( renderable->aModel );
		if ( !model )
		{
			Log_ErrorF( gLC_ClientGraphics, "%s : model is nullptr\n", CH_FUNC_NAME_CLASS );
			continue;
		}

		// make sure this model has valid vertex buffers
		if ( model->apBuffers == nullptr || model->apBuffers->aVertex == CH_INVALID_HANDLE )
		{
			Log_Error( gLC_ClientGraphics, "No Vertex/Index Buffers for Model??\n" );
			continue;
		}

		i++;

		ShaderSkinning_Push push{};
		push.aRenderable            = CH_GET_HANDLE_INDEX( renderHandle );
		push.aSourceVertexBuffer    = Graphics_GetShaderBufferIndex( gGraphicsData.aVertexBuffers, model->apBuffers->aVertexHandle );
		push.aVertexCount           = model->apVertexData->aIndices.empty() ? model->apVertexData->aCount : model->apVertexData->aIndices.size();
		push.aBlendShapeCount       = model->apVertexData->aBlendShapeCount;
		push.aBlendShapeWeightIndex = Graphics_GetShaderBufferIndex( gGraphicsData.aBlendShapeWeightBuffers, renderable->aBlendShapeWeightsIndex );
		push.aBlendShapeDataIndex   = Graphics_GetShaderBufferIndex( gGraphicsData.aBlendShapeDataBuffers, model->apBuffers->aBlendShapeHandle );

		if ( push.aSourceVertexBuffer == UINT32_MAX || push.aBlendShapeWeightIndex == UINT32_MAX || push.aBlendShapeDataIndex == UINT32_MAX )
			continue;

		render->CmdPushConstants( sCmd, shaderSkinningData->aLayout, ShaderStage_Compute, 0, sizeof( push ), &push );
		render->CmdDispatch( sCmd, glm::max( 1U, push.aVertexCount / 64 ), 1, 1 );

	#if 0
		VkBufferMemoryBarrier barrier = vkinit::buffer_barrier(matrixBuffer, _graphicsQueueFamily);
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
	#endif

		GraphicsBufferMemoryBarrier_t& buffer = buffers.emplace_back();
		buffer.aSrcAccessMask                 = EGraphicsAccess_MemoryRead | EGraphicsAccess_MemoryWrite;
		buffer.aDstAccessMask                 = EGraphicsAccess_MemoryRead | EGraphicsAccess_MemoryWrite;
		buffer.aBuffer                        = Graphics_GetShaderBuffer( gGraphicsData.aVertexBuffers, renderable->aVertexIndex );
	}

	PipelineBarrier_t endBarrier{};
	// endBarrier.aSrcStageMask             = EPipelineStage_BottomOfPipe;  // EPipelineStage_ComputeShader;
	// endBarrier.aDstStageMask             = EPipelineStage_TopOfPipe;  // EPipelineStage_VertexShader;
	endBarrier.aSrcStageMask             = EPipelineStage_ComputeShader;
	endBarrier.aDstStageMask             = EPipelineStage_VertexShader;

	endBarrier.aBufferMemoryBarrierCount = buffers.size();
	endBarrier.apBufferMemoryBarriers    = buffers.data();

	render->CmdPipelineBarrier( sCmd, endBarrier );

#endif

	gGraphicsData.aSkinningRenderList.clear();
}



void RenderSystemOld::DoSelectionCompute( ch_handle_t cmd, u32 cmdIndex )
{
#if 0
	static ch_handle_t shaderSelect = gGraphics.GetShader( CH_SHADER_NAME_SELECT );

	if ( aSelectionRenderables.empty() )
		return;

	if ( !aSelectionThisFrame )
		return;

	aSelectionThisFrame = false;

	if ( shaderSelect == CH_INVALID_HANDLE )
	{
		Log_Error( gLC_ClientGraphics, "selection shader not found, can't select renderables under cursor!\n" );
		return;
	}

	if ( !Shader_Bind( cmd, cmdIndex, shaderSelect ) )
	{
		Log_Error( gLC_ClientGraphics, "Failed to bind skinning shader, can't apply blend shapes and bone transforms!\n" );
		return;
	}

	// Reset Data
	ShaderSelect_OutputBuffer                 buffer{};
	u32                                       wrote            = render->BufferWrite( aSelectionBuffer, sizeof( ShaderSelect_OutputBuffer ), &buffer );

	ShaderData_t*                             shaderSelectData = Shader_GetData( shaderSelect );
	IShaderPushComp*                          selectPush       = shaderSelectData->apPushComp;

	ChVector< GraphicsBufferMemoryBarrier_t > buffers;

	u32                                       i = 0;
	// for ( ViewRenderList_t& renderList : gGraphicsData.aViewRenderLists )
	for ( SelectionRenderable& selectRenderable : aSelectionRenderables )
	{
		Renderable_t* renderable = nullptr;
		if ( !gGraphicsData.aRenderables.Get( selectRenderable.renderable, &renderable ) )
		{
			Log_Warn( gLC_ClientGraphics, "Renderable does not exist!\n" );
			continue;
		}

		// get model data
		Model* model = gGraphics.GetModelData( renderable->aModel );
		if ( !model )
		{
			Log_ErrorF( gLC_ClientGraphics, "%s : model is nullptr\n", CH_FUNC_NAME_CLASS );
			continue;
		}

		// make sure this model has valid vertex buffers
		if ( model->apBuffers == nullptr || model->apBuffers->aVertex == CH_INVALID_HANDLE )
		{
			Log_Error( gLC_ClientGraphics, "No Vertex/Index Buffers for Model??\n" );
			continue;
		}

		i++;

		ShaderSelect_Push push{};
		push.aRenderable  = CH_GET_HANDLE_INDEX( selectRenderable.renderable );
		push.aViewport    = aSelectionViewport;
		push.aVertexCount = model->apVertexData->aIndices.empty() ? model->apVertexData->aCount : model->apVertexData->aIndices.size();
		push.color.x     = selectRenderable.color.x;
		push.color.y     = selectRenderable.color.y;
		push.color.z     = selectRenderable.color.z;
		push.aCursorPos   = aSelectionCursorPos;

		render->CmdPushConstants( cmd, shaderSelectData->aLayout, ShaderStage_Compute, 0, sizeof( push ), &push );
		render->CmdDispatch( cmd, push.aVertexCount, 1, 1 );

#if 0
		VkBufferMemoryBarrier barrier = vkinit::buffer_barrier(matrixBuffer, _graphicsQueueFamily);
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
#endif

		GraphicsBufferMemoryBarrier_t& buffer = buffers.emplace_back();
		buffer.aSrcAccessMask                 = EGraphicsAccess_MemoryRead | EGraphicsAccess_MemoryWrite;
		buffer.aDstAccessMask                 = EGraphicsAccess_MemoryRead | EGraphicsAccess_MemoryWrite;
		buffer.aBuffer                        = aSelectionBuffer;
	}
#endif
}


void RenderSystemOld::PrePresent()
{
	render->WaitForQueues();
	render->ResetCommandPool();

	Graphics_FreeQueuedResources();
	Graphics_PrepareDrawData();
}


extern void Graphics_OnResetCallback( ch_handle_t window, ERenderResetFlags sFlags );


void RenderSystemOld::Present( ch_handle_t window, u32* viewports, u32 viewportCount )
{
	PROF_SCOPE();

	if ( viewports == nullptr || viewportCount == 0 )
		return;

	// YEAH THIS SUCKS !!!!!!!!!
	render->SetResetCallback( window, Graphics_OnResetCallback );

	{
		PROF_SCOPE_NAMED( "Imgui Render" );
		ImGui::Render();
	}

	// render->LockGraphicsMutex();

	ch_handle_t backBuffer[ 2 ];
	backBuffer[ 0 ] = render->GetBackBufferColor( window );
	backBuffer[ 1 ] = render->GetBackBufferDepth( window );

	if ( backBuffer[ 0 ] == CH_INVALID_HANDLE || backBuffer[ 1 ] == CH_INVALID_HANDLE )
	{
		Log_Fatal( gLC_ClientGraphics, "Failed to get Back Buffer Handles!\n" );
		return;
	}

	static u32 commandBufferCount = render->GetCommandBufferHandles( window, nullptr );

	if ( commandBufferCount < 1 )
	{
		Log_Fatal( gLC_ClientGraphics, "Failed to get render command buffers!\n" );
		return;
	}

	// cool memory leak
	static ch_handle_t* commandBuffers = ch_malloc< ch_handle_t >( commandBufferCount );
	render->GetCommandBufferHandles( window, commandBuffers );

	u32        imageIndex = render->GetFlightImageIndex( window );

	// For each framebuffer, begin a primary
	// command buffer, and record the commands.
	// for ( size_t cmdIndex = 0; cmdIndex < commandBufferCount; cmdIndex++ )
	// {
		size_t cmdIndex = imageIndex;
		PROF_SCOPE_NAMED( "Primary Command Buffer" );

		ch_handle_t c = commandBuffers[ cmdIndex ];

		render->BeginCommandBuffer( c );

		// Animate Materials in a Compute Shader
		// Run Skinning Compute Shader
		//Graphics_DoSkinning( c, cmdIndex );

		// Do Selection Rendering
		if ( aSelectionEnabled && aSelectionThisFrame )
			Graphics_SelectionTexturePass( c, cmdIndex );

		aSelectionThisFrame = false;

		// Draw Shadow Maps
		Graphics_DrawShadowMaps( c, cmdIndex, viewports, viewportCount );

		// ----------------------------------------------------------
		// Main RenderPass

		RenderPassBegin_t renderPassBegin{};
		renderPassBegin.aRenderPass  = gGraphicsData.aRenderPassGraphics;
		renderPassBegin.aFrameBuffer = backBuffer[ cmdIndex ];
		renderPassBegin.aClear.resize( 2 );
		renderPassBegin.aClear[ 0 ].color   = { 0.f, 0.f, 0.f, 0.f };
		renderPassBegin.aClear[ 0 ].aIsDepth = false;
		renderPassBegin.aClear[ 1 ].color   = { 0.f, 0.f, 0.f, 1.f };
		renderPassBegin.aClear[ 1 ].aIsDepth = true;

		render->BeginRenderPass( c, renderPassBegin );  // VK_SUBPASS_CONTENTS_INLINE

		// Render
		// TODO: add in some dependency thing here, when you add camera's in the game, you'll need to render those first before the final viewports (VR maybe)
		for ( u32 i = 0; i < viewportCount; i++ )
		{
			PROF_SCOPE();

			auto it = gGraphicsData.aViewports.find( viewports[ i ] );

			if ( it == gGraphicsData.aViewports.end() )
			{
				Log_ErrorF( gLC_ClientGraphics, "Failed to get viewport data for rendering\n" );
				continue;
			}

			ViewportShader_t& viewport = it->second;

			// this should probably be moved to app code
			if ( !viewport.aActive )
				continue;

			auto itList = gGraphicsData.aViewRenderLists.find( viewports[ i ] );

			if ( itList == gGraphicsData.aViewRenderLists.end() )
			{
				Log_ErrorF( gLC_ClientGraphics, "Failed to get viewport render list\n" );
				continue;
			}

			ViewRenderList_t& viewRenderList = itList->second;

			// HACK HACK !!!!
			// don't render views with shader overrides here, the only override is the shadow map shader and selection
			// and that is rendered in a separate render pass
			if ( viewport.aShaderOverride )
				continue;

			u32 viewIndex = Graphics_GetShaderSlot( gGraphicsData.aViewportSlots, viewports[ i ] );

			Graphics_RenderView( c, cmdIndex, viewIndex, viewport, viewRenderList );
		}

		render->DrawImGui( ImGui::GetDrawData(), c );

		render->EndRenderPass( c );

		render->EndCommandBuffer( c );
	// }

	render->Present( window, imageIndex );
	// render->UnlockGraphicsMutex();

	// ch_free( commandBuffers );
}


void Graphics_DestroySelectRenderPass()
{
	if ( gGraphicsData.aRenderPassSelect != CH_INVALID_HANDLE )
		render->DestroyRenderPass( gGraphicsData.aRenderPassSelect );
}


bool Graphics_CreateSelectRenderPass()
{
	RenderPassCreate_t create{};

	create.aAttachments.resize( 2 );

	// create.aAttachments[ 0 ].aFormat             = render->GetSwapFormatColor();
	create.aAttachments[ 0 ].aFormat             = GraphicsFmt::BGRA8888_UNORM;
	create.aAttachments[ 0 ].aUseMSAA            = false;
	create.aAttachments[ 0 ].aType               = EAttachmentType_Color;
	create.aAttachments[ 0 ].aLoadOp             = EAttachmentLoadOp_Clear;
	create.aAttachments[ 0 ].aStoreOp            = EAttachmentStoreOp_Store;
	create.aAttachments[ 0 ].aStencilLoadOp      = EAttachmentLoadOp_Load;
	create.aAttachments[ 0 ].aStencilStoreOp     = EAttachmentStoreOp_Store;
	create.aAttachments[ 0 ].aGeneralFinalLayout = true;

	create.aAttachments[ 1 ].aFormat             = render->GetSwapFormatDepth();
	create.aAttachments[ 1 ].aUseMSAA            = false;
	create.aAttachments[ 1 ].aType               = EAttachmentType_Depth;
	create.aAttachments[ 1 ].aLoadOp             = EAttachmentLoadOp_Clear;
	create.aAttachments[ 1 ].aStoreOp            = EAttachmentStoreOp_Store;
	create.aAttachments[ 1 ].aStencilLoadOp      = EAttachmentLoadOp_Load;
	create.aAttachments[ 1 ].aStencilStoreOp     = EAttachmentStoreOp_Store;

	create.aSubpasses.resize( 1 );
	create.aSubpasses[ 0 ].aBindPoint = EPipelineBindPoint_Graphics;

	gGraphicsData.aRenderPassSelect   = render->CreateRenderPass( create );

	return gGraphicsData.aRenderPassSelect;
}


void Graphics_UpdateSelectionViewport()
{
	ViewportShader_t* viewport    = gGraphics.GetViewportData( gRenderOld.aSelectionViewport );
	ViewportShader_t* refViewport = gGraphics.GetViewportData( gRenderOld.aSelectionViewportRef );

	if ( !viewport )
		return;

	if ( !refViewport )
		return;

	viewport->aProjView   = refViewport->aProjView;
	viewport->aProjection = refViewport->aProjection;
	viewport->aView       = refViewport->aView;
	viewport->aViewPos    = refViewport->aViewPos;
	viewport->aNearZ      = refViewport->aNearZ;
	viewport->aFarZ       = refViewport->aFarZ;
	viewport->aOffset     = refViewport->aOffset;
	viewport->aSize       = refViewport->aSize;
}


void RenderSystemOld::FreeSelectionTexture()
{
	if ( aSelectionFramebuffer != CH_INVALID_HANDLE )
		render->DestroyFramebuffer( aSelectionFramebuffer );

	if ( aSelectionTexture != CH_INVALID_HANDLE )
		render->FreeTexture( aSelectionTexture );

	if ( aSelectionDepth != CH_INVALID_HANDLE )
		render->FreeTexture( aSelectionDepth );

	aSelectionFramebuffer = CH_INVALID_HANDLE;
	aSelectionTexture     = CH_INVALID_HANDLE;
	aSelectionDepth       = CH_INVALID_HANDLE;
}


void RenderSystemOld::CreateSelectionTexture()
{
	// TODO: modify CreateTexture to allow us to update the handle
	FreeSelectionTexture();

	if ( !aSelectionEnabled )
		return;

	ViewportShader_t* viewport = gGraphics.GetViewportData( aSelectionViewport );

	if ( viewport == nullptr )
	{
		Log_Error( gLC_ClientGraphics, "Failed to get viewport for selection\n" );
		return;
	}

	viewport->aShaderOverride     = gGraphics.GetShader( CH_SHADER_NAME_SELECT );
	ViewportShader_t* refViewport = gGraphics.GetViewportData( aSelectionViewportRef );

	if ( refViewport->aSize.x == 0 || refViewport->aSize.y == 0 )
		return;

	viewport->aSize   = refViewport->aSize;
	viewport->aActive = true;

	// Create Color Texture
	{
		TextureCreateInfo_t texCreate{};
		texCreate.apName      = "Selection Color Texture";
		texCreate.aSize       = viewport->aSize;
		// texCreate.aFormat     = render->GetSwapFormatColor();
		texCreate.aFormat     = GraphicsFmt::BGRA8888_UNORM;
		texCreate.aViewType   = EImageView_2D;
		texCreate.aMemoryType = EBufferMemory_Host;

		TextureCreateData_t createData{};
		createData.aUsage          = EImageUsage_AttachColor | EImageUsage_Sampled | EImageUsage_TransferSrc;
		createData.aFilter         = EImageFilter_Nearest;
		createData.aSamplerAddress = ESamplerAddressMode_ClampToBorder;
		createData.aDepthCompare   = true;

		aSelectionTexture          = gGraphics.CreateTexture( texCreate, createData );
	}

	// Create Depth Texture
	{
		TextureCreateInfo_t texCreate{};
		texCreate.apName    = "Selection Depth Texture";
		texCreate.aSize     = viewport->aSize;
		texCreate.aFormat   = render->GetSwapFormatDepth();
		texCreate.aViewType = EImageView_2D;

		TextureCreateData_t createData{};
		createData.aUsage          = EImageUsage_AttachDepthStencil | EImageUsage_Sampled;
		createData.aFilter         = EImageFilter_Nearest;
		createData.aSamplerAddress = ESamplerAddressMode_ClampToBorder;
		createData.aDepthCompare   = true;

		aSelectionDepth            = gGraphics.CreateTexture( texCreate, createData );
	}

	// Create Framebuffer
	CreateFramebuffer_t frameBufCreate{};
	frameBufCreate.apName             = "Selection Framebuffer";
	frameBufCreate.aRenderPass        = gGraphicsData.aRenderPassSelect;
	frameBufCreate.aSize              = viewport->aSize;
	frameBufCreate.aPass.aAttachDepth = aSelectionDepth;
	frameBufCreate.aPass.aAttachColors.push_back( aSelectionTexture );

	aSelectionFramebuffer = render->CreateFramebuffer( frameBufCreate );

	if ( aSelectionFramebuffer == CH_INVALID_HANDLE )
	{
		Log_Error( gLC_ClientGraphics, "Failed to create selection framebuffer!\n" );
	}

	Graphics_UpdateSelectionViewport();
}


void RenderSystemOld::EnableSelection( bool enabled, u32 viewport )
{
	if ( enabled )
	{
		aSelectionEnabled     = enabled;
		aSelectionViewportRef = viewport;

		aSelectionViewport    = gGraphics.CreateViewport();

		if ( aSelectionViewport == UINT32_MAX )
		{
			Log_Error( gLC_ClientGraphics, "Failed to allocate viewport for selection\n" );
			aSelectionEnabled = false;
			return;
		}

		Graphics_UpdateSelectionViewport();
		Graphics_CreateSelectRenderPass();
		CreateSelectionTexture();
#if 0
		// Create a storage buffer for this
		aSelectionBuffer = render->CreateBuffer( "Selection Output Buffer", sizeof( ShaderSelect_OutputBuffer ), EBufferFlags_Storage, EBufferMemory_Host );

		if ( aSelectionBuffer == CH_INVALID_HANDLE )
		{
			Log_Error( gLC_ClientGraphics, "Failed to create Selection Output Buffer\n!" );
			return;
		}

		aSelectionViewport = viewport;

		WriteDescSet_t write{};

		write.aDescSetCount = gShaderDescriptorData.aPerShaderSets[ CH_SHADER_NAME_SELECT ].aCount;
		write.apDescSets    = gShaderDescriptorData.aPerShaderSets[ CH_SHADER_NAME_SELECT ].apSets;

		write.aBindingCount = 1;
		write.apBindings    = CH_STACK_NEW( WriteDescSetBinding_t, write.aBindingCount );

		ChVector< ch_handle_t > buffers;
		buffers.push_back( aSelectionBuffer );

		write.apBindings[ 0 ].aBinding = 0;
		write.apBindings[ 0 ].aType    = EDescriptorType_StorageBuffer;
		write.apBindings[ 0 ].aCount   = buffers.size();
		write.apBindings[ 0 ].apData   = buffers.data();

		render->UpdateDescSets( &write, 1 );

		CH_STACK_FREE( write.apBindings );
#endif
	}
	else if ( !enabled && aSelectionEnabled )
	{
#if 0
		// Free the storage buffer if it exists
		if ( aSelectionBuffer != CH_INVALID_HANDLE )
			render->DestroyBuffer( aSelectionBuffer );

		aSelectionBuffer   = CH_INVALID_HANDLE;
#endif

		Graphics_DestroySelectRenderPass();
		FreeSelectionTexture();

		if ( aSelectionViewport != UINT32_MAX )
			gGraphics.FreeViewport( aSelectionViewport );

		aSelectionViewport = CH_R_MAX_VIEWPORTS;
		aSelectionEnabled  = enabled;
	}
}


void RenderSystemOld::SetSelectionRenderablesAndCursorPos( const ChVector< SelectionRenderable >& renderables, glm::ivec2 cursorPos )
{
	if ( !aSelectionEnabled || renderables.empty() )
	{
		return;
	}

	// TODO: resizing before assigning fixes a crash here ????
	// need to look further into this
	aSelectionRenderables.resize( renderables.size() );
	aSelectionRenderables = renderables;
	aSelectionCursorPos   = cursorPos;
	aSelectionThisFrame   = true;

	Graphics_UpdateSelectionViewport();
}


// Gets the selection compute shader result from the last rendered frame
// Returns the color the cursor landed on
bool RenderSystemOld::GetSelectionResult( u8& red, u8& green, u8& blue )
{
	return false;

	if ( !aSelectionEnabled || aSelectionTexture == CH_INVALID_HANDLE )
		return false;

	ReadTexture  readTexture = render->ReadTextureFromDevice( aSelectionTexture );

	const size_t index       = aSelectionCursorPos.y * readTexture.size.x + aSelectionCursorPos.x;
	u32          pixel       = readTexture.pData[ index ];

	red                      = ( pixel >> 16 ) & 0xFF;
	green                    = ( pixel >> 8 ) & 0xFF;
	blue                     = pixel & 0xFF;

	Log_DevF( 1, "Picked Color of (%d, %d, %d)\n", red, green, blue );

	render->FreeReadTexture( &readTexture );

	return true;
}

