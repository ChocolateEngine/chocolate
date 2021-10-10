/*
renderer.cpp ( Authored by p0lyh3dron )

Defines the methods declared in renderer.h,
creating a vulkan interface with which to
draw to the screen.
 */
#include "../../../inc/core/renderer/renderer.h"
#include "../../../inc/core/renderer/initializers.h"
#include "../../../inc/core/renderer/material.h"
#include "../../../inc/core/renderer/modelloader.h"
#include "../../../inc/types/transform.h"
#include "../../../inc/shared/util.h"

#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <set>
#include <fstream>
#include <iostream>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

#include "../../../inc/imgui/imgui.h"
#include "../../../inc/imgui/imgui_impl_vulkan.h"
#include "../../../inc/imgui/imgui_impl_sdl.h"

#include "../../../inc/core/gui.h"
#define SPRITE_SUPPORTED 0

#define SWAPCHAIN gpDevice->GetSwapChain(  )

CONVAR( r_showDrawCalls, 1 );

size_t gModelDrawCalls = 0;

extern GuiSystem* gui;

void Renderer::InitCommands(  )
{
	apCommandManager->Add( Renderer::Commands::IMGUI_INITIALIZED, Command<  >( std::bind( &Renderer::EnableImgui, this ) ) );  
	apCommandManager->Add( Renderer::Commands::SET_VIEW, Command< View& >( [ this ]( View& srView ){ aView.Set( srView ); } ) );

//	auto getWindowSize = std::bind( &Renderer::GetWindowSize, this, std::placeholders::_1, std::placeholders::_2 );       
	apCommandManager->Add( Renderer::Commands::GET_WINDOW_SIZE,
			       Command< uint32_t*, uint32_t* >( [ this ]( uint32_t *spWidth, uint32_t *spHeight ){ GetWindowSize( spWidth, spHeight ); } ) );
}

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
	InitShaders(  );
	gLayoutBuilder.BuildLayouts(  );
	gPipelineBuilder.BuildPipelines(  );
}

bool Renderer::HasStencilComponent( VkFormat sFmt )
{
	return sFmt == VK_FORMAT_D32_SFLOAT_S8_UINT || sFmt == VK_FORMAT_D24_UNORM_S8_UINT;
}

void Renderer::InitCommandBuffers(  )
{
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

		for ( auto& model : aModels )
		{
			if ( model->aNoDraw )
				continue;

			model->Draw( aCommandBuffers[ i ], i );
		}

		for ( auto& sprite : aSprites )
		{
			sprite->Bind( aCommandBuffers[ i ], i );
			push_constant_t push{  };
			push.scale	= { 1, 1 };
			push.translate  = { sprite->aPos.x, sprite->aPos.y };

			vkCmdPushConstants
				(
					aCommandBuffers[ i ],
					sprite->aPipelineLayout,
					VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
					0,
					sizeof( push_constant_t ),
					&push
					);
			sprite->Draw( aCommandBuffers[ i ] );
		}

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

void Renderer::InitSpriteVertices( const String &srSpritePath, SpriteData &srSprite )
{
	std::vector< vertex_2d_t > vertices =
	{
		{{-1 * ( srSprite.aWidth / 2.0f ), -1 * ( srSprite.aHeight / 2.0f )}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
		{{( srSprite.aWidth / 2.0f ), -1 * ( srSprite.aHeight / 2.0f )}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
		{{( srSprite.aWidth / 2.0f ), ( srSprite.aHeight / 2.0f )}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{-1 * ( srSprite.aWidth / 2.0f ), ( srSprite.aHeight / 2.0f )}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	};
	std::vector< uint32_t > indices =
	{
		0, 1, 2, 2, 3, 0	
	};
	srSprite.aVertexCount = ( uint32_t )vertices.size(  );
	srSprite.aIndexCount = ( uint32_t )indices.size(  );
	InitTexBuffer( vertices, srSprite.aVertexBuffer, srSprite.aVertexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
	InitTexBuffer( indices, srSprite.aIndexBuffer, srSprite.aIndexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
}

/* Loads all shaders that will be used in rendering.  */
void Renderer::InitShaders(  )
{
	apMaterialSystem = new MaterialSystem;
	apMaterialSystem->apRenderer = this;
	apMaterialSystem->Init();
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

	apMaterialSystem->ReInitSwapChain();

	gLayoutBuilder.BuildLayouts(  );
	gPipelineBuilder.BuildPipelines(  );

	for ( auto& model : aModels )
		model->ReInit(  );
	
	for ( auto& sprite : aSprites )
		sprite->Reinit(  );
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

	apMaterialSystem->DestroySwapChain();

	for ( auto& model : aModels )
		model->FreeOldResources(  );
	
	for ( auto& sprite : aSprites  )
		sprite->FreeOldResources(  );
	
	for ( auto framebuffer : aSwapChainFramebuffers )
		vkDestroyFramebuffer( DEVICE, framebuffer, NULL );
	
	vkDestroyRenderPass( DEVICE, *gpRenderPass, NULL );
	for ( auto imageView : aSwapChainImageViews )
		vkDestroyImageView( DEVICE, imageView, NULL );
}

// TODO: change to LoadModel
void Renderer::InitModel( ModelData &srModelData, const String &srModelPath, const String &srTexturePath )
{
	srModelData.Init(  );
	std::vector< Mesh* > meshes;

	if ( srModelPath.substr(srModelPath.size() - 4) == ".obj" )
	{
		LoadObj( srModelPath, meshes );
	}
	else if ( srModelPath.substr(srModelPath.size() - 4) == ".glb" || srModelPath.substr(srModelPath.size() - 5) == ".gltf" )
	{
		//LoadGltf( srModelPath, meshes );
		Print( "[Renderer] GLTF currently not supported, on TODO list\n" );
		return;
	}

	// TODO: load in an error model here somehow?
	if (meshes.empty())
		return;

	for (std::size_t i = 0; i < meshes.size(); ++i)
	{
		Mesh* mesh = meshes[i];

		mesh->apMaterial->apShader = apMaterialSystem->GetShader( "basic_3d" );
		mesh->apMaterial->apDiffuse = apMaterialSystem->CreateTexture( mesh->apMaterial, mesh->apMaterial->aDiffusePath.string() );

		apMaterialSystem->CreateVertexBuffer( mesh );
		apMaterialSystem->CreateIndexBuffer( mesh );

		mesh->aRadius = glm::distance( mesh->aMinSize, mesh->aMaxSize ) / 2.0f;

		srModelData.aMeshes.push_back( mesh );

		for (const auto& vert: mesh->aVertices)
			srModelData.aVertices.push_back(vert);

		for (const auto& ind: mesh->aIndices)
			srModelData.aIndices.push_back(ind);
	}

	aModels.push_back( &srModelData );
}

// TODO: change to LoadSprite
void Renderer::InitSprite( SpriteData &srSpriteData, const String &srSpritePath )
{
	srSpriteData.Init(  );
	srSpriteData.SetTexture( srSpritePath, aTextureSampler );
	InitSpriteVertices( "", srSpriteData );
	aSprites.push_back( &srSpriteData );
}

void Renderer::DrawFrame(  )
{
	InitCommandBuffers(  );	//	Fucky wucky!!

	if ( r_showDrawCalls.GetBool() )
	{
		gui->InsertDebugMessage( 0, "Model Draw Calls: %u (%u / %u Command Buffers)",
								gModelDrawCalls / aCommandBuffers.size(), gModelDrawCalls, aCommandBuffers.size() );

		gui->InsertDebugMessage( 1, "" );  // spacer
	}

	gModelDrawCalls = 0;
	
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

	for ( auto& model : aModels )
	{
		for ( int i = 0; i < model->aMeshes.size(); i++ )
		{
			model->aMeshes[i]->GetShader()->UpdateUniformBuffers( imageIndex, *model, model->aMeshes[i] );
		}
	}

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
	BaseSystem(  ),
	aView(0, 0, 640, 480, 0.1, 100, 90)
{
	aSystemType = RENDERER_C;
}

void Renderer::Init(  )
{
	BaseSystem::Init(  );

	gpDevice->InitDevice(  );

	uint32_t w, h;
	GetWindowSize( &w, &h );

	aView.Set(0, 0, w, h, 0.1, 100, 90);

	Transform transform = {};
	aView.viewMatrix = transform.ToViewMatrixZ(  );
}

void Renderer::SendMessages(  )
{
	apCommandManager->Execute( GuiSystem::Commands::ASSIGN_WINDOW, gpDevice->GetWindow(  ) );
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
