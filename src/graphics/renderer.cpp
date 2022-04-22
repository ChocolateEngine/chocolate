/*
renderer.cpp ( Authored by p0lyh3dron )

Defines the methods declared in renderer.h,
creating a vulkan interface with which to
draw to the screen.
 */
#include "core/profiler.h"
#include "renderer.h"
#include "initializers.h"
#include "types/material.h"
#include "util/modelloader.h"
#include "types/transform.h"
#include "graphics/sprite.h"
#include "util.h"

#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <set>
#include <fstream>
#include <iostream>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <thread>
#include <mutex>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_sdl.h"

#include "filemonitor.h"

#include "gui/gui.h"


#if RENDER_DOC
#include <renderdoc_app.h>
#endif


#define SWAPCHAIN gpDevice->GetSwapChain(  )

CONVAR( r_showDrawCalls, 1 );

size_t gModelDrawCalls = 0;
size_t gVertsDrawn = 0;
size_t gLinesDrawn = 0;

extern GuiSystem* gui;

Renderer* renderer = nullptr;

static FileMonitor gFileMonitor;

LOG_CHANNEL( Graphics );

const int THREAD_COUNT = std::thread::hardware_concurrency() - 1;


// awful
struct RenderableData_t
{
	size_t index;
	BaseRenderable* renderable;
};


struct RenderWorkerData_t
{
	// std::vector< BaseRenderable* > aRenderables;
	// std::unordered_map< BaseShader*, std::vector< BaseRenderable* > > aDrawList;
	std::unordered_map< BaseShader*, std::vector< RenderableData_t > > aDrawList;

	uint32_t aCommandBufferCount;
};


// lazy and awful
std::vector< std::vector< RenderWorkerData_t* > > gTaskQueue;

// um
std::mutex gTaskMutex;

// lazy and awful
int gTaskFinishedCount = 0;
bool gThreadsPaused = false;

RenderWorkerData_t* GetNextTask( int sThreadId )
{
	RenderWorkerData_t* data = nullptr;

	if ( gTaskQueue[sThreadId].size() )
	{
		if ( gThreadsPaused )
			return nullptr;

		data = gTaskQueue[sThreadId][gTaskQueue[sThreadId].size() - 1];

		if ( gThreadsPaused )
			return nullptr;

		gTaskQueue[sThreadId].pop_back();
	}

	return data;
}


void TaskFinished( int sThreadId )
{
	gTaskMutex.lock();
	gTaskFinishedCount++;
	gTaskMutex.unlock();
}


void RenderWorker( int sThreadId )
{
	while ( true )
	{
		sys_sleep( 0.01 );

		if ( gThreadsPaused )
			continue;

		// NOTE: probably change this to a task queue for each thread
		// this mutex thing is really shit imo
		if ( RenderWorkerData_t* data = GetNextTask( sThreadId ) )
		{
			for ( auto& [shader, renderList]: data->aDrawList )
			{
				for ( auto& renderable : renderList )
				{
					shader->PrepareDrawData( renderable.index, renderable.renderable, data->aCommandBufferCount );
				}
			}

			delete data;
			TaskFinished( sThreadId );
		}
	}
}


// TEST BASIC MULTITHREADING, MOVE TO CH_CORE LATER AND PROPERLY GET CPU THREAD COUNT
std::vector< std::thread > gThreadPool;

// HACK
BaseShader* gpSkybox = nullptr;

void Renderer::EnableImgui(  )
{
	aImGuiInitialized = true;
}

void Renderer::InitVulkan(  )
{
	InitImageViews( aSwapChainImageViews );
	InitRenderPass(  );
	InitColorResources( aColorImage, aColorImageMemory, aColorImageView );
	InitDepthResources( aDepthImage, aDepthImageMemory, aDepthImageView );
	InitFrameBuffer( aSwapChainFramebuffers, aSwapChainImageViews, aDepthImageView, aColorImageView );
	gpDevice->InitTextureSampler( aTextureSampler, VK_SAMPLER_ADDRESS_MODE_REPEAT );
	InitDescPool( { { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000 }, { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000 } } } );
	InitImguiPool( gpDevice->GetWindow(  ) );
	InitSync( aImageAvailableSemaphores, aRenderFinishedSemaphores, aInFlightFences, aImagesInFlight );

	matsys = GetMaterialSystem();
	matsys->apSampler = &aTextureSampler;
	matsys->Init();

	gpSkybox = matsys->GetShader( "skybox" );

	gLayoutBuilder.BuildLayouts(  );
	gPipelineBuilder.BuildPipelines(  );

	aDbgDrawer.Init();

	// start up threads
	gThreadPool.resize( THREAD_COUNT );
	gTaskQueue.resize( THREAD_COUNT );
	for ( int i = 0; i < THREAD_COUNT; i++ )
	{
		gThreadPool[i] = std::thread( RenderWorker, i );
	}

	gui->StyleImGui();
}

bool Renderer::HasStencilComponent( VkFormat sFmt )
{
	return sFmt == VK_FORMAT_D32_SFLOAT_S8_UINT || sFmt == VK_FORMAT_D24_UNORM_S8_UINT;
}


void Renderer::InitCommandBuffers(  )
{
	PROF_SCOPE();

	gFileMonitor.Update();

	aDbgDrawer.PrepareMeshForDraw();
	
	aCommandBuffers.resize( aSwapChainFramebuffers.size(  ) );
	VkCommandBufferAllocateInfo allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool 		= gpDevice->GetCommandPool(  );
	allocInfo.level 		= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount	= ( uint32_t )aCommandBuffers.size(  );

	if ( vkAllocateCommandBuffers( DEVICE, &allocInfo, aCommandBuffers.data(  ) ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to allocate command buffers!" );
	
	// reset and allocate data for worker threads
	gTaskMutex.lock();
	gThreadsPaused = true;

	// reset thread finished state
	gTaskFinishedCount = 0;
	for ( int i = 0; i < THREAD_COUNT; i++ )
	{
		gTaskQueue[i].push_back( new RenderWorkerData_t );
	}

	// calculate work for threads
	for ( auto& [shader, renderList] : matsys->aDrawList )
	{
		shader->AllocDrawData( renderList.size() );
	}

	size_t curTask = 0;
	for ( auto& [shader, renderList]: matsys->aDrawList )
	{
		size_t renderIndex = 0;
		for ( auto& renderable : renderList )
		{
			RenderWorkerData_t* data = gTaskQueue[curTask][0];
			data->aDrawList[shader].emplace_back( renderIndex++, renderable );
			data->aCommandBufferCount = aCommandBuffers.size();

			curTask++;

			if ( curTask >= THREAD_COUNT )
				curTask = 0;
		}
	}

	gTaskMutex.unlock();
	gThreadsPaused = false;

	// wait for threads to finish
	// does this need to be locked?
	while ( gTaskFinishedCount != THREAD_COUNT )
	{
		sys_sleep( 0.01 );
	}

	for ( int i = 0; i < aCommandBuffers.size(); i++ )
	{
		VkCommandBufferBeginInfo beginInfo{  };
		beginInfo.sType 		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags 		= 0; // Optional
		beginInfo.pInheritanceInfo 	= NULL; // Optional

		if ( vkBeginCommandBuffer( aCommandBuffers[ i ], &beginInfo ) != VK_SUCCESS )
			throw std::runtime_error( "Failed to begin recording command buffer!" );
		
		VkRenderPassBeginInfo renderPassInfo{  };
		renderPassInfo.sType 		 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass 	 = *gpRenderPass;
		renderPassInfo.framebuffer 	 = aSwapChainFramebuffers[ i ];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = SWAPCHAIN.GetExtent(  );

		// HACK: check for skybox
		bool drawSkybox = false;

		for ( auto& [shader, renderList]: matsys->aDrawList )
		{
			// if ( shader->aName == strSkybox )
			if ( shader == gpSkybox )
			{
				drawSkybox = true;
				continue;
			}
			
			size_t renderIndex = 0;
			for ( auto& renderable : renderList )
			{
				Material* mat = (Material*)renderable->GetMaterial();
				mat->apShader->UpdateBuffers( i, renderIndex++, renderable );
			}
		}

		// if we have a skybox, don't bother to clear the previous frame
		// um, why does not clearing it crash it
		// maybe something when creating an attachment description idk
		// if ( !drawSkybox )
		if ( true )
		{
			std::array< VkClearValue, 2 > clearValues{  };
			clearValues[ 0 ].color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clearValues[ 1 ].depthStencil = { 1.0f, 0 };

			renderPassInfo.clearValueCount 	 = ( uint32_t )clearValues.size(  );
			renderPassInfo.pClearValues 	 = clearValues.data(  );
		}

		vkCmdBeginRenderPass( aCommandBuffers[ i ], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

		// IDEA: make a batched mesh vector so that way we can bind everything needed, and then just draw draw draw draw

		size_t renderIndex = 0;

		// TODO: still could be better, but it's better than what we used to have
		for ( auto& [shader, renderList]: matsys->aDrawList )
		{
			// HACK HACK
			if ( shader == gpSkybox )
			{
				renderIndex++;
				continue;
			}

			shader->Bind( aCommandBuffers[i], i );

			renderIndex = 0;
			for ( auto& renderable : renderList )
			{
				matsys->DrawRenderable( renderIndex++, renderable, aCommandBuffers[i], i );
			}
		}

		// HACK HACK: Get skybox to draw last so it's the cheapest
		if ( drawSkybox )
		{
			gpSkybox->Bind( aCommandBuffers[i], i );

			for ( auto& renderable: matsys->aDrawList[gpSkybox] )
			{
				matsys->DrawRenderable( 0, renderable, aCommandBuffers[i], i );
			}
		}

	//	aDbgDrawer.RenderPrims( aCommandBuffers[ i ], aView );
		
		if ( aImGuiInitialized )
		{
			ImGui::Render(  );
			ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(  ), aCommandBuffers[ i ] );
		}
		
		vkCmdEndRenderPass( aCommandBuffers[ i ] );

		if ( vkEndCommandBuffer( aCommandBuffers[ i ] ) != VK_SUCCESS )
			throw std::runtime_error( "Failed to record command buffer!" );
	}
}

void Renderer::ReinitSwapChain(  )
{
	int width = 0, height = 0;
	SDL_Vulkan_GetDrawableSize( gpDevice->GetWindow(  ), &width, &height );
	for ( ; width == 0 || height == 0; )
		SDL_Vulkan_GetDrawableSize( gpDevice->GetWindow(  ), &width, &height );
	
	gpDevice->SetResolution( width, height );
	vkDeviceWaitIdle( DEVICE );

	DestroySwapChain(  );
	gpDevice->ReinitSwapChain(  );

	InitImageViews( aSwapChainImageViews );
	InitRenderPass(  );
	InitColorResources( aColorImage, aColorImageMemory, aColorImageView );
	InitDepthResources( aDepthImage, aDepthImageMemory, aDepthImageView );
	InitFrameBuffer( aSwapChainFramebuffers, aSwapChainImageViews, aDepthImageView, aColorImageView );

	matsys->ReInitSwapChain();

	gLayoutBuilder.BuildLayouts(  );
	gPipelineBuilder.BuildPipelines(  );

	for ( auto& model : aModels )
	{
		for ( auto& mesh : model->aMeshes )
		{
			matsys->MeshReInit( mesh );
		}
	}
}

void Renderer::DestroySwapChain(  )
{
	aReinitSwapChain = false;
	
	gShaderCache.ClearCache(  );
	vkDestroyImageView( DEVICE, aDepthImageView, nullptr );
	vkDestroyImage( DEVICE, aDepthImage, nullptr );
	vkFreeMemory( DEVICE, aDepthImageMemory, nullptr );
	vkDestroyImageView( DEVICE, aColorImageView, nullptr );
	vkDestroyImage( DEVICE, aColorImage, nullptr );
	vkFreeMemory( DEVICE, aColorImageMemory, nullptr );

	matsys->DestroySwapChain();

	for ( auto& model : aModels )
	{
		for ( auto& mesh : model->aMeshes )
		{
			matsys->MeshFreeOldResources( mesh );
		}
	}
	
	for ( auto framebuffer : aSwapChainFramebuffers )
		vkDestroyFramebuffer( DEVICE, framebuffer, NULL );
	
	vkDestroyRenderPass( DEVICE, *gpRenderPass, NULL );
	for ( auto imageView : aSwapChainImageViews )
		vkDestroyImageView( DEVICE, imageView, NULL );
}


bool LoadBaseMeshes( Model* sModel, const std::string& srPath, std::vector< MeshPtr* >& meshPtrs )
{
	std::vector< IMesh* > meshes;

	if ( srPath.substr(srPath.size() - 4) == ".obj" )
	{
		LoadObj( srPath, meshes );
	}
	else if ( srPath.substr(srPath.size() - 4) == ".glb" || srPath.substr(srPath.size() - 5) == ".gltf" )
	{
		LogDev( 1, "[Renderer] GLTF currently not supported, on TODO list\n" );
		return false;
	}

	// TODO: load in an error model here somehow?
	if (meshes.empty())
		return false;

	for (std::size_t i = 0; i < meshes.size(); ++i)
	{
		IMesh* mesh = meshes[i];

		matsys->CreateVertexBuffer( mesh );

		// if the vertex count is different than the index count, use the index buffer
		if ( mesh->GetVertices().size() != mesh->GetIndices().size() )
			matsys->CreateIndexBuffer( mesh );

		//mesh->aRadius = glm::distance( mesh->aMinSize, mesh->aMaxSize ) / 2.0f;

#if 0
		sModel->aVertices.insert( sModel->aVertices.end(), mesh->aVertices.begin(), mesh->aVertices.end() );

		size_t indCount = sModel->aIndices.size();
		for ( uint32_t ind: mesh->aIndices )
			sModel->aIndices.push_back( ind + indCount );
#endif

		MeshPtr* meshptr = new MeshPtr;
		meshptr->apMesh = mesh;
		meshptr->apModel = sModel;
		meshPtrs.emplace_back( meshptr );

		matsys->MeshInit( sModel->aMeshes[i] );
	}

	return true;
}


bool Renderer::LoadModel( Model* sModel, const std::string &srPath )
{
	sModel->aPath = srPath;

	Model* dupeModel = nullptr;

	// shitty cache system
	for ( const auto& model: aModels )
	{
		if ( model->aPath == srPath )
		{
			dupeModel = model;
			break;
		}
	}

	if ( dupeModel )  // copy over stuf from the other loaded model
	{
		sModel->aMeshes.resize( dupeModel->aMeshes.size() );

		for (std::size_t i = 0; i < sModel->aMeshes.size(); ++i)
		{
			sModel->aMeshes[i] = new MeshPtr;
			sModel->aMeshes[i]->apModel = sModel;
			sModel->aMeshes[i]->apMesh = dupeModel->aMeshes[i];

			matsys->RegisterRenderable( sModel->aMeshes[i] );
			matsys->MeshInit( sModel->aMeshes[i] );
		}
	}
	else  // we haven't loaded this model yet
	{
		LoadBaseMeshes( sModel, srPath, sModel->aMeshes );
	}

	// TODO: load in an error model here somehow?
	if ( sModel->aMeshes.empty() )
		return false;

	aModels.push_back( sModel );
	return true;
}

bool Renderer::LoadSprite( Sprite &srSprite, const String &srSpritePath )
{
	srSprite.SetMaterial( matsys->CreateMaterial() );
	srSprite.GetMaterial()->SetShader( "basic_2d" );
	srSprite.GetMaterial()->SetVar( "diffuse", matsys->CreateTexture( srSpritePath ) );

	std::vector< vertex_2d_t > aVertices =
	{
		{{-1 * ( srSprite.aWidth / 2.0f ), -1 * ( srSprite.aHeight / 2.0f )}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{( srSprite.aWidth / 2.0f ), -1 * ( srSprite.aHeight / 2.0f )}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{( srSprite.aWidth / 2.0f ), ( srSprite.aHeight / 2.0f )}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-1 * ( srSprite.aWidth / 2.0f ), ( srSprite.aHeight / 2.0f )}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	};

	std::vector< uint32_t > aIndices = {0, 1, 2, 2, 3, 0};

	// blech
	srSprite.GetVertices() = aVertices;
	srSprite.GetIndices()  = aIndices;

	matsys->CreateVertexBuffer( &srSprite );
	matsys->CreateIndexBuffer( &srSprite );

	aSprites.push_back( &srSprite );

	return true;
}

void Renderer::DrawFrame(  )
{
	PROF_SCOPE();

	if ( aCommandBuffers.size() > 0 && r_showDrawCalls.GetBool() )
	{
		int buh = 0;
		gui->InsertDebugMessage( buh++, "Model Draw Calls: %u (%u / %u Command Buffers)",
			gModelDrawCalls / aCommandBuffers.size(), gModelDrawCalls, aCommandBuffers.size() );

		gui->InsertDebugMessage( buh++, "Vertices Drawn: %u (%u / %u Command Buffers)",
			gVertsDrawn / aCommandBuffers.size(), gVertsDrawn, aCommandBuffers.size() );

		gui->InsertDebugMessage( buh++, "Prims Drawn: %u (%u / %u Command Buffers)",
			gLinesDrawn / aCommandBuffers.size(), gLinesDrawn, aCommandBuffers.size() );

		gui->InsertDebugMessage( buh++, "" );  // spacer
	}

	gModelDrawCalls = 0;
	gVertsDrawn = 0;
	gLinesDrawn = 0;

	/*for ( auto& renderable : matsys->aDrawList )
	{
		Material* mat = (Material*)renderable->apMaterial;
		mat->apShader->UpdateBuffers( aCurrentFrame, renderable );
	}*/

	InitCommandBuffers(  );	//	Fucky wucky!!
	
	vkWaitForFences( DEVICE, 1, &aInFlightFences[ aCurrentFrame ], VK_TRUE, UINT64_MAX );
	
	uint32_t imageIndex;
	VkResult res =  vkAcquireNextImageKHR( DEVICE, SWAPCHAIN.GetSwapChain(  ), UINT64_MAX, aImageAvailableSemaphores[ aCurrentFrame ], VK_NULL_HANDLE, &imageIndex );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR || aReinitSwapChain )
	{
		ReinitSwapChain(  );
		return;
	}
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
		throw std::runtime_error( "Failed ot acquire swap chain image!" );
	
	if ( aImagesInFlight[ imageIndex ] != VK_NULL_HANDLE )
		vkWaitForFences( DEVICE, 1, &aImagesInFlight[ imageIndex ], VK_TRUE, UINT64_MAX );
	
	aImagesInFlight[ imageIndex ] = aInFlightFences[ aCurrentFrame ];

	VkSubmitInfo submitInfo{  };
	submitInfo.sType 			= VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[  ] 		= { aImageAvailableSemaphores[ aCurrentFrame ] };
	VkPipelineStageFlags waitStages[  ] 	= { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount 		= 1;
	submitInfo.pWaitSemaphores 		= waitSemaphores;
	submitInfo.pWaitDstStageMask 		= waitStages;
	submitInfo.commandBufferCount 		= 1;
	submitInfo.pCommandBuffers 		= &aCommandBuffers[ imageIndex ];
	VkSemaphore signalSemaphores[  ] 	= { aRenderFinishedSemaphores[ aCurrentFrame ] };
	submitInfo.signalSemaphoreCount 	= 1;
	submitInfo.pSignalSemaphores 		= signalSemaphores;

	vkResetFences( DEVICE, 1, &aInFlightFences[ aCurrentFrame ] );
	if ( vkQueueSubmit( gpDevice->GetGraphicsQueue(  ), 1, &submitInfo, aInFlightFences[ aCurrentFrame ] ) != VK_SUCCESS )
		throw std::runtime_error( "Failed to submit draw command buffer!" );
	
	VkPresentInfoKHR presentInfo{  };
	presentInfo.sType 			= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount 		= 1;
	presentInfo.pWaitSemaphores 		= signalSemaphores;

	VkSwapchainKHR swapChains[  ] 		= { SWAPCHAIN.GetSwapChain(  ) };
	presentInfo.swapchainCount 		= 1;
	presentInfo.pSwapchains 		= swapChains;
	presentInfo.pImageIndices 		= &imageIndex;
	presentInfo.pResults 			= NULL; // Optional

	res = vkQueuePresentKHR( gpDevice->GetPresentQueue(  ), &presentInfo );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
		ReinitSwapChain(  );
	
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
		throw std::runtime_error( "Failed ot present swap chain image!" );
	
	vkQueueWaitIdle( gpDevice->GetPresentQueue(  ) );

	aCurrentFrame = ( aCurrentFrame + 1 ) % MAX_FRAMES_PROCESSING;
	vkFreeCommandBuffers( DEVICE, gpDevice->GetCommandPool(  ), ( uint32_t )aCommandBuffers.size(  ), aCommandBuffers.data(  ) );

	matsys->aDrawList.clear();
	aDbgDrawer.ResetMesh();
}

/* Create a line and add it to drawing.  */
void Renderer::CreateLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor ) {
	aDbgDrawer.CreateLine( sX, sY, sColor );
}

void Renderer::Cleanup(  )
{
	DestroySwapChain(  );
	vkDestroySampler( DEVICE, aTextureSampler, NULL );
	vkDestroyDescriptorPool( DEVICE, *gpPool, NULL );
	for ( int i = 0; i < MAX_FRAMES_PROCESSING; i++ )
	{
		vkDestroySemaphore( DEVICE, aRenderFinishedSemaphores[ i ], NULL );
		vkDestroySemaphore( DEVICE, aImageAvailableSemaphores[ i ], NULL );
		vkDestroyFence( DEVICE, aInFlightFences[ i ], NULL);
	}
}

Renderer::Renderer(  ) :
	aView(0, 0, 640, 480, 0.1, 1000, 90)
{
	renderer = this;
}


#if RENDER_DOC
RENDERDOC_API_1_5_0* gpRenderDoc = nullptr;

void LoadRenderDocAPI()
{
	void* renderdoc = SDL_LoadObject( "renderdoc" EXT_DLL );

	// enable validation layers in renderdoc
	if ( !renderdoc )
	{
		LogWarn( gGraphicsChannel, "(-renderdoc) Renderdoc DLL not found: %s\n", SDL_GetError() );
		return;
	}

	// typedef int(RENDERDOC_CC *pRENDERDOC_GetAPI)(RENDERDOC_Version version, void **outAPIPointers)

	pRENDERDOC_GetAPI rdGet = (pRENDERDOC_GetAPI)SDL_LoadFunction( renderdoc, "RENDERDOC_GetAPI" );

	// pRENDERDOC_SetCaptureOptionU32 RENDERDOC_SetCaptureOptionU32 = (pRENDERDOC_SetCaptureOptionU32)SDL_LoadFunction( renderdoc, "RENDERDOC_SetCaptureOptionU32" );
	// pRENDERDOC_SetCaptureOptionU32 RENDERDOC_SetCaptureOptionU32 = (pRENDERDOC_SetCaptureOptionU32)SDL_LoadFunction( renderdoc, "RENDERDOC_SetCaptureOptionU32" );

	if ( !rdGet )
		return;

	int ret = rdGet( eRENDERDOC_API_Version_1_5_0, (void **)&gpRenderDoc );
	if ( ret != 1 )
	{
		LogWarn( gGraphicsChannel, "(-renderdoc) Failed to Get Renderdoc API\n" );
		SDL_UnloadObject( gpRenderDoc );
		return;
	}

	// Enable Vulkan API Validation Layers
	ret = gpRenderDoc->SetCaptureOptionU32( eRENDERDOC_Option_DebugOutputMute, 0 );
	if ( ret != 1 )
	{
		LogWarn( gGraphicsChannel, "(-renderdoc) Failed to Get Renderdoc API\n" );
		SDL_UnloadObject( gpRenderDoc );
		return;
	}
}

#endif


void Renderer::Init(  )
{
#if RENDER_DOC
	if ( cmdline->Find( "-renderdoc" ) )
		LoadRenderDocAPI();
#endif

	gpDevice = new Device;
	gpDevice->InitDevice(  );

	auto gui = GET_SYSTEM( BaseGuiSystem );
	gui->AssignWindow( gpDevice->GetWindow(  ) );

	uint32_t w, h;
	GetWindowSize( &w, &h );

	aView.Set(0, 0, w, h, 0.1, 1000, 90);

	Transform transform = {};
	aView.viewMatrix = transform.ToViewMatrixZ(  );
}

SDL_Window *Renderer::GetWindow(  )
{
	return gpDevice->GetWindow(  );
}

void Renderer::SetView( View& view )
{
	if ( aView.width != view.width || aView.height != view.height )
		aReinitSwapChain = true;

	aView.Set(view);

	if ( aReinitSwapChain )
		ReinitSwapChain();
}

void Renderer::GetWindowSize( uint32_t* width, uint32_t* height )
{
	SDL_GetWindowSize( gpDevice->GetWindow(  ), (int*)width, (int*)height );
}

Renderer::~Renderer
	(  )
{
	Cleanup(  );
}
