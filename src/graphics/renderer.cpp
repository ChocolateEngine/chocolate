/*
renderer.cpp ( Authored by p0lyh3dron )

Defines the methods declared in renderer.h,
creating a vulkan interface with which to
draw to the screen.
 */
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

#include "imgui/imgui.h"
#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_sdl.h"

#include "filemonitor.h"

#include "gui.h"
#define SPRITE_SUPPORTED 0

#define SWAPCHAIN gpDevice->GetSwapChain(  )

CONVAR( r_showDrawCalls, 1 );

size_t gModelDrawCalls = 0;
size_t gVertsDrawn = 0;
size_t gLinesDrawn = 0;

extern GuiSystem* gui;

Renderer* renderer = nullptr;

static FileMonitor gFileMonitor;

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

	materialsystem = new MaterialSystem;
	materialsystem->Init();
	materialsystem->apSampler = &aTextureSampler;

	gLayoutBuilder.BuildLayouts(  );
	gPipelineBuilder.BuildPipelines(  );

	aDbgDrawer.Init();
}

bool Renderer::HasStencilComponent( VkFormat sFmt )
{
	return sFmt == VK_FORMAT_D32_SFLOAT_S8_UINT || sFmt == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Renderer::InitCommandBuffers(  )
{
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

	for ( int i = 0; i < aCommandBuffers.size(  ); i++ )
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

		std::array< VkClearValue, 2 > clearValues{  };
		clearValues[ 0 ].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[ 1 ].depthStencil = { 1.0f, 0 };
			
		renderPassInfo.clearValueCount 	 = ( uint32_t )clearValues.size(  );
		renderPassInfo.pClearValues 	 = clearValues.data(  );

		vkCmdBeginRenderPass( aCommandBuffers[ i ], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );

		// IDEA: make a batched mesh vector so that way we can bind everything needed, and then just draw draw draw draw

		for ( auto& renderable : materialsystem->aDrawList )
		{
			materialsystem->DrawRenderable( renderable, aCommandBuffers[i], i );
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

void Renderer::InitSpriteVertices( const String &srSpritePath, Sprite &srSprite )
{
	srSprite.aVertices =
	{
		{{-1 * ( srSprite.aWidth / 2.0f ), -1 * ( srSprite.aHeight / 2.0f )}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{( srSprite.aWidth / 2.0f ), -1 * ( srSprite.aHeight / 2.0f )}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{( srSprite.aWidth / 2.0f ), ( srSprite.aHeight / 2.0f )}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-1 * ( srSprite.aWidth / 2.0f ), ( srSprite.aHeight / 2.0f )}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	};

	srSprite.aIndices =
	{
		0, 1, 2, 2, 3, 0	
	};

	materialsystem->CreateVertexBuffer( &srSprite );
	materialsystem->CreateIndexBuffer( &srSprite );
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

	materialsystem->ReInitSwapChain();

	gLayoutBuilder.BuildLayouts(  );
	gPipelineBuilder.BuildPipelines(  );

	for ( auto& model : aModels )
	{
		for ( auto& mesh : model->aMeshes )
		{
			materialsystem->MeshReInit( mesh );
		}
	}
}

void Renderer::DestroySwapChain(  )
{
	gShaderCache.ClearCache(  );
	vkDestroyImageView( DEVICE, aDepthImageView, nullptr );
	vkDestroyImage( DEVICE, aDepthImage, nullptr );
	vkFreeMemory( DEVICE, aDepthImageMemory, nullptr );
	vkDestroyImageView( DEVICE, aColorImageView, nullptr );
	vkDestroyImage( DEVICE, aColorImage, nullptr );
	vkFreeMemory( DEVICE, aColorImageMemory, nullptr );

	materialsystem->DestroySwapChain();

	for ( auto& model : aModels )
	{
		for ( auto& mesh : model->aMeshes )
		{
			materialsystem->MeshFreeOldResources( mesh );
		}
	}
	
	for ( auto framebuffer : aSwapChainFramebuffers )
		vkDestroyFramebuffer( DEVICE, framebuffer, NULL );
	
	vkDestroyRenderPass( DEVICE, *gpRenderPass, NULL );
	for ( auto imageView : aSwapChainImageViews )
		vkDestroyImageView( DEVICE, imageView, NULL );
}


bool Renderer::LoadModel( Model* sModel, const String &srPath )
{
	sModel->aPath = srPath;
	std::vector< Mesh* > meshes;

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
		meshes.resize( dupeModel->aMeshes.size() );

		for (std::size_t i = 0; i < meshes.size(); ++i)
		{
			meshes[i] = new Mesh;
			meshes[i]->apModel = sModel;
			materialsystem->RegisterRenderable( meshes[i] );
			materialsystem->MeshInit( meshes[i] );
			meshes[i]->apMaterial = dupeModel->aMeshes[i]->apMaterial;
			meshes[i]->aIndices = dupeModel->aMeshes[i]->aIndices;
			meshes[i]->aVertices = dupeModel->aMeshes[i]->aVertices;
		}
	}
	else  // we haven't loaded this model yet
	{
		if ( srPath.substr(srPath.size() - 4) == ".obj" )
		{
			LoadObj( srPath, meshes );
		}
		else if ( srPath.substr(srPath.size() - 4) == ".glb" || srPath.substr(srPath.size() - 5) == ".gltf" )
		{
			Print( "[Renderer] GLTF currently not supported, on TODO list\n" );
			return false;
		}
	}

	// TODO: load in an error model here somehow?
	if (meshes.empty())
		return false;

	for (std::size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh* mesh = meshes[i];
		mesh->apModel = sModel;

		matsys->CreateVertexBuffer( mesh );

		// if the vertex count is different than the index count, use the index buffer
		if ( mesh->aVertices.size() != mesh->aIndices.size() )
			matsys->CreateIndexBuffer( mesh );

		//mesh->aRadius = glm::distance( mesh->aMinSize, mesh->aMaxSize ) / 2.0f;

		sModel->aMeshes.push_back( mesh );

		sModel->aVertices.insert( sModel->aVertices.end(), mesh->aVertices.begin(), mesh->aVertices.end() );

		size_t indCount = sModel->aIndices.size();
		for ( uint32_t ind: mesh->aIndices )
			sModel->aIndices.push_back( ind + indCount );
	}

	aModels.push_back( sModel );
	return true;
}

bool Renderer::LoadSprite( Sprite &srSprite, const String &srSpritePath )
{
	srSprite.apMaterial = matsys->CreateMaterial(  );
	srSprite.apMaterial->SetShader( "basic_2d" );

	srSprite.apMaterial->AddVar( "diffuse", srSpritePath, matsys->CreateTexture( srSpritePath ) );

	InitSpriteVertices( "", srSprite );
	aSprites.push_back( &srSprite );

	return true;
}

void Renderer::DrawFrame(  )
{
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

	for ( auto& renderable : materialsystem->aDrawList )
	{
		Material* mat = (Material*)renderable->apMaterial;
		mat->apShader->UpdateBuffers( aCurrentFrame, renderable );
	}

	InitCommandBuffers(  );	//	Fucky wucky!!
	
	vkWaitForFences( DEVICE, 1, &aInFlightFences[ aCurrentFrame ], VK_TRUE, UINT64_MAX );
	
	uint32_t imageIndex;
	VkResult res =  vkAcquireNextImageKHR( DEVICE, SWAPCHAIN.GetSwapChain(  ), UINT64_MAX, aImageAvailableSemaphores[ aCurrentFrame ], VK_NULL_HANDLE, &imageIndex );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
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

	materialsystem->aDrawList.clear();
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

void Renderer::Init(  )
{
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
	aView.Set(view);
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
