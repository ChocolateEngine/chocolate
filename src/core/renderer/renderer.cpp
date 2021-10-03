/*
renderer.cpp ( Authored by p0lyh3dron )

Defines the methods declared in renderer.h,
creating a vulkan interface with which to
draw to the screen.
 */
#include "../../../inc/core/renderer/renderer.h"
#include "../../../inc/core/renderer/initializers.h"
#include "../../../inc/types/transform.h"

#define GLM_FORCE_RADIANS
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_INCLUDE_STB_IMAGE
#define TINYGLTF_NO_INCLUDE_STB_IMAGE_WRITE

#include <set>
#include <fstream>
#include <iostream>
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

#include "../../../lib/io/tiny_obj_loader.h"

#include "../../../inc/imgui/imgui.h"
#include "../../../inc/imgui/imgui_impl_vulkan.h"
#include "../../../inc/imgui/imgui_impl_sdl.h"

#include "../../../inc/core/gui.h"
#define SPRITE_SUPPORTED 0

#define SWAPCHAIN gpDevice->GetSwapChain(  )

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

			model->Bind( aCommandBuffers[ i ], i );
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

/* A.  */
std::string Renderer::GetBaseDir( const String &srPath )
{
	if ( srPath.find_last_of( "/\\" ) != std::string::npos )
		return srPath.substr( 0, srPath.find_last_of( "/\\" ) );
	return "";
}

void Renderer::LoadObj( const String &srObjPath, ModelData &srModel )
{
	tinyobj::attrib_t 				attrib;
	std::vector< tinyobj::shape_t > 		shapes;
	std::vector< tinyobj::material_t > 		materials;
	std::string 					warn;
	std::string					err;
	std::vector< vertex_3d_t > 			vertices;
	std::vector< uint32_t > 			indices;
	std::vector< uint32_t >				materialIndices;
        std::unordered_map< vertex_3d_t, uint32_t  >	uniqueVertices{  };
	std::string					baseDir = GetBaseDir( srObjPath );
	std::vector< Mesh >				meshes;
	int loops = 0;

	if ( baseDir.empty(  ) )
		baseDir = ".";
	baseDir += "/";
	
	if ( !tinyobj::LoadObj( &attrib, &shapes, &materials, &warn, &err, srObjPath.c_str(  ), baseDir.c_str(  ) ) )
		throw std::runtime_error( warn + err );

	for ( const auto& shape : shapes )
	{
		uint32_t indexOffset = 0;
		/* TODO: Figure out what this variable does, I have no idea what it does, but it is needed.  */
		uint32_t onion_ring = 0;

		for ( const auto& index : shape.mesh.num_face_vertices )
		{		      
			vertex_3d_t vertex{  };
			uint32_t faceVertices = index;

			for ( uint32_t i = 0; i < faceVertices; ++i )
			{
			        std::vector< uint32_t > shapeIndices;
				tinyobj::index_t tempIndex = shape.mesh.indices[ indexOffset + i ];

				vertex.pos = {
					attrib.vertices[ 3 * tempIndex.vertex_index + 0 ],
					attrib.vertices[ 3 * tempIndex.vertex_index + 1 ],
					attrib.vertices[ 3 * tempIndex.vertex_index + 2 ]
				};

				vertex.texCoord = {
					attrib.texcoords[ 2 * tempIndex.texcoord_index + 0 ],
					1.0f - attrib.texcoords[ 2 * tempIndex.texcoord_index + 1 ]
				};

				vertex.color = { 1.0f, 1.0f, 1.0f };
				if ( uniqueVertices.count( vertex ) == 0 )
				{
					uniqueVertices[ vertex ] = ( uint32_t )vertices.size(  );
					vertices.push_back( vertex );
				}
				shapeIndices.push_back( uniqueVertices[ vertex ] );
				indices.push_back( uniqueVertices[ vertex ] );
				materialIndices.push_back( shape.mesh.material_ids[ onion_ring ] );
				loops = i;
			}
			onion_ring++;
			indexOffset += faceVertices;
		}
		srModel.AddIndexGroup( materialIndices );
	}

	uint32_t search = 0;
	uint32_t numIndices = 0;
	uint32_t j = 0;
	if ( materials.size(  ) )
		for ( j = 0; j < materialIndices.size(  ); )
		{
			for ( numIndices = 0, search = materialIndices[ j ]; j < materialIndices.size(  ) && search == materialIndices[ j ]; ++j, ++numIndices );
			srModel.AddMesh( baseDir + materials[ search ].diffuse_texname, numIndices, j - numIndices, aTextureSampler );
		}
	
	srModel.aVertexCount 	= ( uint32_t )vertices.size(  );
	srModel.aIndexCount 	= ( uint32_t )indices.size(  );
	InitTexBuffer( vertices, srModel.aVertexBuffer, srModel.aVertexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
	InitTexBuffer( indices, srModel.aIndexBuffer, srModel.aIndexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT );

	srModel.apVertices = new vertex_3d_t[ vertices.size(  ) ];
	srModel.apIndices  = new uint32_t[ indices.size(  ) ];
	
	for ( int i = 0; i < vertices.size(  ); ++i )
		srModel.apVertices[ i ] = vertices[ i ];
	for ( int i = 0; i < indices.size(  ); ++i )
		srModel.apIndices[ i ] = indices[ i ];
}

void Renderer::LoadGltf( const String &srGltfPath, ModelData &srModel )
{
	/* ... */
}

void Renderer::InitModelVertices( const String &srModelPath, ModelData &srModel )
{
	if ( srModelPath.substr( srModelPath.size(  ) - 4 ) == ".obj" )
	{
		LoadObj( srModelPath, srModel );
		return;
	}
	if ( srModelPath.substr( srModelPath.size(  ) - 4 ) == ".glb" )
	{
		LoadGltf( srModelPath, srModel );
		return;
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
	for ( auto& model : aModels )
		model->Reinit(  );
	
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

void Renderer::UpdateUniformBuffers( uint32_t sCurrentImage, ModelData &srModelData )
{
	ubo_3d_t ubo{  };

	ubo.model = glm::scale( glm::translate( glm::mat4( 1.0f ), srModelData.aTransform.position ) * glm::rotate( glm::mat4( 1.0f ), glm::radians( 0.0f ), glm::vec3( 1.0f, 1.0f, 1.0f ) ), srModelData.aTransform.scale );
	
	ubo.view  = aView.viewMatrix;
	ubo.proj  = aView.GetProjection();

	void* data;
	vkMapMemory( DEVICE, srModelData.aUniformData.aMem.GetBuffer(  )[ sCurrentImage ], 0, sizeof( ubo ), 0, &data );
	memcpy( data, &ubo, sizeof( ubo ) );
	vkUnmapMemory( DEVICE, srModelData.aUniformData.aMem.GetBuffer(  )[ sCurrentImage ] );
}

void Renderer::InitModel( ModelData &srModelData, const String &srModelPath, const String &srTexturePath )
{
    	srModelData.Init(  );
	InitModelVertices( srModelPath, srModelData );
	aModels.push_back( &srModelData );
}

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
		UpdateUniformBuffers( imageIndex, *model );

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
	aView.viewMatrix = ToFirstPersonCameraTransformation(transform);
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
