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
#include "types/transform.h"
#include "graphics/sprite.h"
#include "debugprims/primcreator.h"
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

extern GuiSystem* gui;
extern DebugRenderer gDbgDrawer;

Renderer* renderer = nullptr;

static FileMonitor gFileMonitor;

LOG_CHANNEL( Graphics );

const int THREAD_COUNT = std::thread::hardware_concurrency() - 1;


// awful
struct RenderableData_t
{
	BaseShader* shader;
	size_t index;
	IRenderable* renderable;
	size_t matIndex;
	RenderableDrawData drawData;
};


struct RenderWorkerData_t
{
	// std::vector< BaseRenderable* > aRenderables;
	// std::unordered_map< BaseShader*, std::vector< BaseRenderable* > > aDrawList;
	// std::unordered_map< BaseShader*, std::vector< RenderableData_t > > aDrawList;
	std::forward_list< RenderableData_t > aDrawList;
	// DataBuffer< RenderableData_t > aDrawList;
	// u32 aDrawCount;

	uint32_t aCommandBufferCount;
};

constexpr int CH_RENDER_WORKER_TASKS = 11;


struct RenderWorkerThread_t
{
	//RenderWorkerData_t** apData = nullptr;  // list of data on the stack
	std::vector< RenderWorkerData_t* > aData;  // list of data on the stack
	u32                  aCount = 0;        // amount in array
};

// lazy and awful
// std::vector< std::vector< RenderWorkerData_t* > > gTaskQueue;
// use a c array for this and create them on the stack?
// std::vector< std::forward_list< RenderWorkerData_t > > gTaskQueue;
std::vector< RenderWorkerThread_t > gTaskQueue;


// allocate one memory pool containing all draw data
// and each thread gets it's own offset and count into the memory pool

// um
std::mutex gTaskMutex;

// lazy and awful
int gTaskFinishedCount = 0;
bool gThreadsPaused = false;

RenderWorkerData_t* GetNextTask( std::mutex& mutex, int sThreadId )
{
	// RenderWorkerData_t* data = nullptr;
	if ( gThreadsPaused )
		return nullptr;

	if ( gTaskQueue[sThreadId].aCount )
	{
		return gTaskQueue[sThreadId].aData[ --gTaskQueue[sThreadId].aCount ];
	}

	// if ( !gTaskQueue[sThreadId].empty() )
	// {
	// 	if ( gThreadsPaused )
	// 		return nullptr;
	// 
	// 	return &gTaskQueue[sThreadId].front();
	// 	// return &gTaskQueue[sThreadId][gTaskQueue[sThreadId].size() - 1];
	// 	
	// 	// data = gTaskQueue[sThreadId][gTaskQueue[sThreadId].size() - 1];
	// 	// 
	// 	// if ( gThreadsPaused )
	// 	// 	return nullptr;
	// 	// 
	// 	// gTaskQueue[sThreadId].pop_back();
	// }

	return nullptr;
	// return data;
}


void TaskFinished( int sThreadId )
{
	// gTaskQueue[sThreadId].pop_front();

	gTaskMutex.lock();
	gTaskFinishedCount++;
	gTaskMutex.unlock();
}


void RenderWorker( int sThreadId )
{
	// blech
	std::mutex taskMutex;

	while ( true )
	{
		sys_sleep( 0.01 );

		if ( gThreadsPaused )
			continue;

		if ( RenderWorkerData_t* data = GetNextTask( taskMutex, sThreadId ) )
		{
			for ( auto& renderData : data->aDrawList )
			{
				renderData.shader->PrepareDrawData(
					renderData.index,
					renderData.renderable,
					renderData.matIndex,
					renderData.drawData,
					data->aCommandBufferCount
				);
			}

			TaskFinished( sThreadId );
		}
	}
}

// TEST BASIC MULTITHREADING, MOVE TO CH_CORE LATER AND PROPERLY GET CPU THREAD COUNT
std::vector< std::thread > gThreadPool;


// ------------------------------------------------------------
// New Task Testing


struct Task_PrepareDrawData
{
	// CH_DECLARE_TASK(
	//		Task_PrepareDrawData, 
	//		ETaskStackReq::Standard, 
	//		ETaskPriotiry::Normal, 
	//		LogColor::Blue,  // um 
	// );

	void Run()
	{
		// ... do thing here ...
	}
};



// ------------------------------------------------------------


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

	gDbgDrawer.PrepareMeshForDraw();
	
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

	// calculate work for threads
	size_t drawCount = 0;
	for ( auto& [shader, renderList] : matsys->aDrawList )
	{
		auto size = std::distance( renderList.begin(), renderList.end() );
		shader->AllocDrawData( size );
	}

	// reset queue
	for ( int i = 0; i < THREAD_COUNT; i++ )
	{
	 	gTaskQueue[i].aCount = 0;
	}

	// i would like to use a global memory pool for this instead
	// ...but then the forward list not allocated
	// luckily though, the data accessing still works fine (i think)
	RenderWorkerData_t workerData[CH_RENDER_WORKER_TASKS];
	size_t taskCount = 0;

	// calculate memory info for the memory pool
	size_t curTask = 0;
	for ( auto& [shader, renderList]: matsys->aDrawList )
	{
		size_t renderIndex = 0;
		for ( auto& [renderable, matIndex, drawData]: renderList )
		{
			workerData[curTask].aDrawList.emplace_front( shader, renderIndex++, renderable, matIndex, drawData );
			workerData[curTask].aCommandBufferCount = aCommandBuffers.size();

			// RenderWorkerData_t& data = gTaskQueue[curTask].front();
			// data.aDrawList.emplace_front( shader, renderIndex++, renderable );
			// data.aCommandBufferCount = aCommandBuffers.size();

			curTask++;
			taskCount++;

			// if ( curTask >= THREAD_COUNT )
			if ( curTask >= CH_RENDER_WORKER_TASKS )
				curTask = 0;
		}
	}

	curTask = 0;
	for ( int curThread = 0; curTask < CH_RENDER_WORKER_TASKS; curTask++ )
	{
		// hmm, probably a lot of excess tasks here, idk

		gTaskQueue[curThread].aCount++;
		// gTaskQueue[curThread].apData[0] = &workerData[curTask];
		gTaskQueue[curThread].aData.push_back( &workerData[curTask] );

		if ( ++curThread >= THREAD_COUNT )
			curThread = 0;
	}

	gTaskMutex.unlock();
	gThreadsPaused = false;

	// wait for threads to finish
	// while ( gTaskFinishedCount != THREAD_COUNT )
	while ( gTaskFinishedCount != CH_RENDER_WORKER_TASKS )
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
			for ( auto& [renderable, matIndex, drawData] : renderList )
			{
				Material* mat = (Material*)renderable->GetMaterial( matIndex );
				mat->apShader->UpdateBuffers( i, renderIndex++, renderable, matIndex );
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
		IRenderable* prevRenderable = nullptr;
		size_t prevMatIndex = SIZE_MAX;

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

			for ( auto& [renderable, matIndex, drawData]: renderList )
			{
				if ( prevRenderable != renderable || prevMatIndex != matIndex )
				{
					prevRenderable = renderable;
					prevMatIndex = matIndex;

					shader->BindBuffers( renderable, matIndex, aCommandBuffers[i], i );
				}

				matsys->DrawRenderable( renderIndex++, renderable, matIndex, drawData, aCommandBuffers[i], i );
			}
		}

		// HACK HACK: Get skybox to draw last so it's the cheapest
		if ( drawSkybox )
		{
			// NOTE: there should only be one here
			size_t skyCount = std::distance( matsys->aDrawList[gpSkybox].begin(), matsys->aDrawList[gpSkybox].end() );

			AssertMsg( skyCount == 1, "More than one skybox renderable, can only be one!!" );

			gpSkybox->Bind( aCommandBuffers[i], i );

			for ( auto& [renderable, matIndex, drawData] : matsys->aDrawList[gpSkybox] )
			{
				gpSkybox->BindBuffers( renderable, matIndex, aCommandBuffers[i], i );
				matsys->DrawRenderable( 0, renderable, matIndex, drawData, aCommandBuffers[i], i );
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

	matsys->OnReInitSwapChain();

	gLayoutBuilder.BuildLayouts(  );
	gPipelineBuilder.BuildPipelines(  );

	for ( auto& model : aModels )
	{
		matsys->InitUniformBuffer( model );
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

	matsys->OnDestroySwapChain();

	for ( auto& model : aModels )
	{
		matsys->MeshFreeOldResources( model );
	}
	
	for ( auto framebuffer : aSwapChainFramebuffers )
		vkDestroyFramebuffer( DEVICE, framebuffer, NULL );
	
	vkDestroyRenderPass( DEVICE, *gpRenderPass, NULL );
	for ( auto imageView : aSwapChainImageViews )
		vkDestroyImageView( DEVICE, imageView, NULL );
}

#if 0
bool Renderer::LoadSprite( Sprite &srSprite, const String &srSpritePath )
{
	srSprite.SetMaterial( 0, matsys->CreateMaterial() );
	srSprite.GetMaterial( 0 )->SetShader( "basic_2d" );
	srSprite.GetMaterial( 0 )->SetVar( "diffuse", matsys->CreateTexture( srSpritePath ) );

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
#endif

void Renderer::DrawFrame(  )
{
	PROF_SCOPE();

	if ( aCommandBuffers.size() > 0 && r_showDrawCalls.GetBool() )
	{
		int buh = 0;
		gui->InsertDebugMessage( buh++, "Draw Calls: %u (%u / %u Command Buffers)",
			gModelDrawCalls / aCommandBuffers.size(), gModelDrawCalls, aCommandBuffers.size() );

		gui->InsertDebugMessage( buh++, "Vertices:   %u (%u / %u Command Buffers)",
			gVertsDrawn / aCommandBuffers.size(), gVertsDrawn, aCommandBuffers.size() );

		gui->InsertDebugMessage( buh++, "" );  // spacer
	}

	gModelDrawCalls = 0;
	gVertsDrawn = 0;

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
	
	// probably a more effective reminder
#pragma message ("IDEA: maybe change this vkQueueWaitIdle to be called at the start of the DrawFrame function," \
				"cause this does take up some cpu time")

	vkQueueWaitIdle( gpDevice->GetPresentQueue() );

	aCurrentFrame = ++aCurrentFrame % MAX_FRAMES_PROCESSING;
	vkFreeCommandBuffers( DEVICE, gpDevice->GetCommandPool(  ), ( uint32_t )aCommandBuffers.size(  ), aCommandBuffers.data(  ) );

	matsys->aDrawList.clear();
	gDbgDrawer.ResetMesh();
}

/* Create a line and add it to drawing.  */
void Renderer::CreateLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor ) {
	gDbgDrawer.CreateLine( sX, sY, sColor );
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
