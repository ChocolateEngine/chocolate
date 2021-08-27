/*
renderer.cpp ( Authored by p0lyh3dron )

Defines the methods declared in renderer.h,
creating a vulkan interface with which to
draw to the screen.
 */
#include "../../../inc/core/renderer/renderer.h"
#include "../../../inc/core/renderer/initializers.h"

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

#define MODEL_SET_LAYOUT_PARAMETERS { { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, NULL }, { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL } } }

#define SPRITE_SET_LAYOUT_PARAMETERS { { { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, NULL, } } }

#define MODEL_SET_PARAMETERS( tImageView, uBuffers ) { { { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tImageView, aTextureSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER } } }, { { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uBuffers, sizeof( ubo_3d_t ) } } }

#define SPRITE_SET_PARAMETERS( tImageView ) { { { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tImageView, aTextureSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER } } }

void Renderer::InitCommands(  )
{
        apCommandManager->Add( Renderer::Commands::IMGUI_INITIALIZED, Command<  >( std::bind( &Renderer::EnableImgui, this ) ) );
}

void Renderer::EnableImgui(  )
{
	aImGuiInitialized = true;
}

void Renderer::InitVulkan(  )
{
	aDevice.InitSwapChain( aSwapChain, aSwapChainImages, aSwapChainImageFormat, aSwapChainExtent );
	aAllocator.apSwapChainImages = &aSwapChainImages;
	aAllocator.InitImageViews( aSwapChainImageViews, aSwapChainImageFormat );
	aAllocator.InitRenderPass( aRenderPass, aSwapChainImageFormat );
	aAllocator.apRenderPass = &aRenderPass;
	aAllocator.InitDescriptorSetLayout( aModelSetLayout, MODEL_SET_LAYOUT_PARAMETERS );
	aAllocator.InitDescriptorSetLayout( aSpriteSetLayout, SPRITE_SET_LAYOUT_PARAMETERS );
	aAllocator.InitGraphicsPipeline< vertex_3d_t >( aModelPipeline, aModelLayout, aSwapChainExtent, aModelSetLayout, "materials/shaders/3dvert.spv",
							  "materials/shaders/3dfrag.spv", 0 );
	aAllocator.InitGraphicsPipeline< vertex_2d_t >( aSpritePipeline, aSpriteLayout, aSwapChainExtent, aSpriteSetLayout, "materials/shaders/2dvert.spv",
							  "materials/shaders/2dfrag.spv", NO_CULLING | NO_DEPTH );
	aAllocator.InitDepthResources( aDepthImage, aDepthImageMemory, aDepthImageView, aSwapChainExtent );
	aAllocator.InitFrameBuffer( aSwapChainFramebuffers, aSwapChainImageViews, aDepthImageView, aSwapChainExtent );
	aDevice.InitTextureSampler( aTextureSampler, VK_SAMPLER_ADDRESS_MODE_REPEAT );
	aAllocator.InitDescPool( aDescPool, { { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000 }, { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000 } } } );
	aAllocator.InitImguiPool( aDevice.GetWindow(  ) );
	aAllocator.InitSync( aImageAvailableSemaphores, aRenderFinishedSemaphores, aInFlightFences, aImagesInFlight );
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
	allocInfo.commandPool 		= aDevice.GetCommandPool(  );
	allocInfo.level 		= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount	= ( uint32_t )aCommandBuffers.size(  );

	if ( vkAllocateCommandBuffers( aDevice.GetDevice(  ), &allocInfo, aCommandBuffers.data(  ) ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to allocate command buffers!" );
	}
	for ( int i = 0; i < aCommandBuffers.size(  ); i++ )
	{
		VkCommandBufferBeginInfo beginInfo{  };
		beginInfo.sType 		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags 		= 0; // Optional
		beginInfo.pInheritanceInfo 	= NULL; // Optional

		if ( vkBeginCommandBuffer( aCommandBuffers[ i ], &beginInfo ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to begin recording command buffer!" );
		}
		VkRenderPassBeginInfo renderPassInfo{  };
		renderPassInfo.sType 		 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass 	 = aRenderPass;
		renderPassInfo.framebuffer 	 = aSwapChainFramebuffers[ i ];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = aSwapChainExtent;

		std::array< VkClearValue, 2 > clearValues{  };
		clearValues[ 0 ].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[ 1 ].depthStencil = { 1.0f, 0 };
			
		renderPassInfo.clearValueCount 	 = ( uint32_t )clearValues.size(  );
		renderPassInfo.pClearValues 	 = clearValues.data(  );

		vkCmdBeginRenderPass( aCommandBuffers[ i ], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
		
		vkCmdBindPipeline( aCommandBuffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, aModelPipeline );

		for ( auto& model : aModels )
		{
			model->Bind( aCommandBuffers[ i ], aModelLayout, i );
			model->Draw( aCommandBuffers[ i ] );
		}
		vkCmdBindPipeline( aCommandBuffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, aSpritePipeline );
		for ( auto& sprite : aSprites )
		{
			if ( !sprite->aNoDraw )
			{
				sprite->Bind( aCommandBuffers[ i ], aSpriteLayout, i );
				push_constant_t push{  };
				push.scale	= { 1, 1 };
				push.translate  = { sprite->aPosX, sprite->aPosY };

				vkCmdPushConstants
					(
						aCommandBuffers[ i ],
						aSpriteLayout,
						VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						0,
						sizeof( push_constant_t ),
						&push
						);
				sprite->Draw( aCommandBuffers[ i ] );
			}
		}
		if ( aImGuiInitialized )
		{
			ImGui::Render(  );
			ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(  ), aCommandBuffers[ i ] );
		}
		
		vkCmdEndRenderPass( aCommandBuffers[ i ] );

		if ( vkEndCommandBuffer( aCommandBuffers[ i ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to record command buffer!" );
		}
	}
}

void Renderer::LoadObj( const String &srObjPath, ModelData &srModel )
{
	tinyobj::attrib_t attrib;
	std::vector< tinyobj::shape_t > shapes;
	std::vector< tinyobj::material_t > materials;
	std::vector< vertex_3d_t > vertices;
	std::vector< uint32_t > indices;
	std::string warn, err;

	if ( !tinyobj::LoadObj( &attrib, &shapes, &materials, &warn, &err, srObjPath.c_str(  ) ) )
	{
		throw std::runtime_error( warn + err );
	}

	std::unordered_map< vertex_3d_t, uint32_t  >uniqueVertices{  };
	
	for ( const auto& shape : shapes )
	{
		for ( const auto& index : shape.mesh.indices )
		{
			vertex_3d_t vertex{  };

			vertex.pos = {
				attrib.vertices[ 3 * index.vertex_index + 0 ],
				attrib.vertices[ 3 * index.vertex_index + 1 ],
				attrib.vertices[ 3 * index.vertex_index + 2 ]
			};

			vertex.texCoord = {
				attrib.texcoords[ 2 * index.texcoord_index + 0 ],
				1.0f - attrib.texcoords[ 2 * index.texcoord_index + 1 ]
			};

			vertex.color = { 1.0f, 1.0f, 1.0f };
			if ( uniqueVertices.count( vertex ) == 0 )
			{
				uniqueVertices[ vertex ] = ( uint32_t )vertices.size(  );
				vertices.push_back( vertex );
			}
			
			indices.push_back( uniqueVertices[ vertex ] );
		}
	}
	srModel.aVertexCount 	= ( uint32_t )vertices.size(  );
	srModel.aIndexCount 	= ( uint32_t )indices.size(  );
	aAllocator.InitTexBuffer( vertices, srModel.aVertexBuffer, srModel.aVertexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
	aAllocator.InitTexBuffer( indices, srModel.aIndexBuffer, srModel.aIndexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
}

void Renderer::LoadGltf( const String &srGltfPath, ModelData &srModel )
{
	/*tinygltf::Model glModel;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	tinygltf::Accessor& accessor;
	tinygltf::BufferView& bufferView;
	tinygltf::Buffer& buffer;
	float* positions;
	std::vector< vertex_3d_t > vertices;
	std::vector< uint32_t > indices;

	if ( !loader.LoadASCIIFromFile( &glModel, &err, &warn, gltfPath.c_str(  ) ) )
	{
		fprintf( stderr, "Failed to parse GLTF: %s", gltfPath.c_str(  ) );
	}
	if ( !warn.empty(  ) )
	{
		printf( "Warning: %s", warn.c_str(  ) );
	}
	if ( !err.empty(  ) )
	{
		printf( "Error: %s", err.c_str(  ) );
	}

	accessor = glModel.accessors[ primitive.attributes[ "POSITION" ] ];
	bufferView = glModel.bufferViews[ accessor.bufferView ];
	buffer = glModel.buffers[ bufferView.buffer ];
	positions = ( float* )( &buffer.data[ bufferView.byteOffset + accessor.byteOffset ] );

	std::unordered_map< vertex_3d_t, uint32_t > uniqueVertices{  };
	
	for ( int i = 0; i < accessor.count; ++i )
	{
		vertex_3d_t vertex{  };

		vertex.pos = {
			positions[ i * 3 + 0 ];
			positions[ i * 3 + 1 ];
			positions[ i * 3 + 2 ];
		};
		}*/
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
        aAllocator.InitTexBuffer( vertices, srSprite.aVertexBuffer, srSprite.aVertexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
	aAllocator.InitTexBuffer( indices, srSprite.aIndexBuffer, srSprite.aIndexBufferMem, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT );
}

void Renderer::ReinitSwapChain(  )
{
	int width = 0, height = 0;
	SDL_Vulkan_GetDrawableSize( aDevice.GetWindow(  ), &width, &height );
	for ( ; width == 0 || height == 0; )
	{
		SDL_Vulkan_GetDrawableSize( aDevice.GetWindow(  ), &width, &height );
	}
	aDevice.SetResolution( width, height );
	vkDeviceWaitIdle( aDevice.GetDevice(  ) );

	DestroySwapChain(  );

	aDevice.InitSwapChain( aSwapChain, aSwapChainImages, aSwapChainImageFormat, aSwapChainExtent );
	aAllocator.apSwapChainImages = &aSwapChainImages;
	aAllocator.InitImageViews( aSwapChainImageViews, aSwapChainImageFormat );
	aAllocator.InitRenderPass( aRenderPass, aSwapChainImageFormat );
	aAllocator.apRenderPass = &aRenderPass;
	aAllocator.InitDescriptorSetLayout( aModelSetLayout, MODEL_SET_LAYOUT_PARAMETERS );
	aAllocator.InitDescriptorSetLayout( aSpriteSetLayout, SPRITE_SET_LAYOUT_PARAMETERS );
	aAllocator.InitGraphicsPipeline< vertex_3d_t >( aModelPipeline, aModelLayout, aSwapChainExtent, aModelSetLayout, "materials/shaders/3dvert.spv",
							  "materials/shaders/3dfrag.spv", 0 );
	aAllocator.InitGraphicsPipeline< vertex_2d_t >( aSpritePipeline, aSpriteLayout, aSwapChainExtent, aSpriteSetLayout, "materials/shaders/2dvert.spv",
							  "materials/shaders/2dfrag.spv", NO_CULLING | NO_DEPTH );
	aAllocator.InitDepthResources( aDepthImage, aDepthImageMemory, aDepthImageView, aSwapChainExtent );
	aAllocator.InitFrameBuffer( aSwapChainFramebuffers, aSwapChainImageViews, aDepthImageView, aSwapChainExtent );
	
	for ( auto& model : aModels )
	{
		aAllocator.InitUniformBuffers( model->aUniformBuffers, model->aUniformBuffersMem );	//	Please fix this ubo_2d_t shit, I just want easy sprites
	}
        aAllocator.InitDescPool( aDescPool, { { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000 }, { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000 } } } );
	for ( auto& model : aModels )
	{
		aAllocator.InitDescriptorSets( model->aDescriptorSets, aModelSetLayout, aDescPool, MODEL_SET_PARAMETERS( model->aTextureImageView, model->aUniformBuffers ) );	
	}
	for ( auto& sprite : aSprites )
	{	        
		aAllocator.InitDescriptorSets( sprite->aDescriptorSets, aSpriteSetLayout, aDescPool, SPRITE_SET_PARAMETERS( sprite->aTextureImageView ) );	
	}
}

void Renderer::DestroySwapChain(  )
{
	vkDestroyImageView( aDevice.GetDevice(  ), aDepthImageView, nullptr );
	vkDestroyImage( aDevice.GetDevice(  ), aDepthImage, nullptr );
	vkFreeMemory( aDevice.GetDevice(  ), aDepthImageMemory, nullptr );
	for ( auto& model : aModels )
	{
		for ( int i = 0; i < aSwapChainImages.size(  ); i++ )
		{
			vkDestroyBuffer( aDevice.GetDevice(  ), model->aUniformBuffers[ i ], NULL );
			vkFreeMemory( aDevice.GetDevice(  ), model->aUniformBuffersMem[ i ], NULL );
		}
        }
	for ( auto& sprite : aSprites  )
	{

	}
	vkDestroyDescriptorPool( aDevice.GetDevice(  ), aDescPool, NULL );
	vkDestroyDescriptorSetLayout( aDevice.GetDevice(  ), aSpriteSetLayout, NULL );
	vkDestroyDescriptorSetLayout( aDevice.GetDevice(  ), aModelSetLayout, NULL );
        for ( auto framebuffer : aSwapChainFramebuffers )
	{
		vkDestroyFramebuffer( aDevice.GetDevice(  ), framebuffer, NULL );
        }
	vkFreeCommandBuffers( aDevice.GetDevice(  ), aDevice.GetCommandPool(  ), ( uint32_t )aCommandBuffers.size(  ), aCommandBuffers.data(  ) );

	vkDestroyPipeline( aDevice.GetDevice(  ), aModelPipeline, NULL );
        vkDestroyPipelineLayout( aDevice.GetDevice(  ), aModelLayout, NULL );
	vkDestroyPipeline( aDevice.GetDevice(  ), aSpritePipeline, NULL );
        vkDestroyPipelineLayout( aDevice.GetDevice(  ), aSpriteLayout, NULL );
        vkDestroyRenderPass( aDevice.GetDevice(  ), aRenderPass, NULL );

        for ( auto imageView : aSwapChainImageViews )
	{
		vkDestroyImageView( aDevice.GetDevice(  ), imageView, NULL );
        }

        vkDestroySwapchainKHR( aDevice.GetDevice(  ), aSwapChain, NULL );
}

template< typename T >
void Renderer::DestroyRenderable( T &srRenderable )
{
	vkDestroyImageView( aDevice.GetDevice(  ), srRenderable.aTextureImageView, NULL );
	vkDestroyImage( aDevice.GetDevice(  ), srRenderable.aTextureImage, NULL );
	vkFreeMemory( aDevice.GetDevice(  ), srRenderable.aTextureImageMem, NULL );
	vkDestroyBuffer( aDevice.GetDevice(  ), srRenderable.aIndexBuffer, NULL );
	vkFreeMemory( aDevice.GetDevice(  ), srRenderable.aIndexBufferMem, NULL );
	vkDestroyBuffer( aDevice.GetDevice(  ), srRenderable.aVertexBuffer, NULL );
	vkFreeMemory( aDevice.GetDevice(  ), srRenderable.aVertexBufferMem, NULL );
}

void Renderer::UpdateUniformBuffers( uint32_t sCurrentImage, ModelData &srModelData )
{
	static auto startTime 	= std::chrono::high_resolution_clock::now(  );

	auto currentTime 	= std::chrono::high_resolution_clock::now();
	float time 		= std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count(  );

	ubo_3d_t ubo{  };
	ubo.model = glm::translate( glm::mat4( 1.0f ), glm::vec3( srModelData.aPosX, srModelData.aPosY, srModelData.aPosZ ) ) * glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 45.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
	ubo.view  = glm::lookAt( glm::vec3( 0.1f, 10.0f, 25.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
	ubo.proj  = glm::perspective( glm::radians( 90.0f ), aSwapChainExtent.width / ( float ) aSwapChainExtent.height, 0.1f, 256.0f );

	ubo.proj[ 1 ][ 1 ] *= -1;

	void* data;
	vkMapMemory( aDevice.GetDevice(  ), srModelData.aUniformBuffersMem[ sCurrentImage ], 0, sizeof( ubo ), 0, &data );
	memcpy( data, &ubo, sizeof( ubo ) );
	vkUnmapMemory( aDevice.GetDevice(  ), srModelData.aUniformBuffersMem[ sCurrentImage ] );
}

void Renderer::InitModel( ModelData &srModelData, const String &srModelPath, const String &srTexturePath )
{
        InitModelVertices( srModelPath, srModelData );
	aAllocator.InitTextureImage( srTexturePath, srModelData.aTextureImage, srModelData.aTextureImageMem, NULL, NULL );
	aAllocator.InitTextureImageView( srModelData.aTextureImageView, srModelData.aTextureImage );
	aAllocator.InitUniformBuffers( srModelData.aUniformBuffers, srModelData.aUniformBuffersMem );
	aAllocator.InitDescriptorSets( srModelData.aDescriptorSets, aModelSetLayout, aDescPool, MODEL_SET_PARAMETERS( srModelData.aTextureImageView, srModelData.aUniformBuffers ) );
	aModels.push_back( &srModelData );
}

void Renderer::InitSprite( SpriteData &srSpriteData, const String &srSpritePath )
{
	aAllocator.InitTextureImage( srSpritePath, srSpriteData.aTextureImage, srSpriteData.aTextureImageMem, NULL, NULL );
	aAllocator.InitTextureImageView( srSpriteData.aTextureImageView, srSpriteData.aTextureImage );
	InitSpriteVertices( srSpritePath, srSpriteData );
	aAllocator.InitDescriptorSets( srSpriteData.aDescriptorSets, aSpriteSetLayout, aDescPool, SPRITE_SET_PARAMETERS( srSpriteData.aTextureImageView ) );
	aSprites.push_back( &srSpriteData );

}

void Renderer::DrawFrame(  )
{
        InitCommandBuffers(  );	//	Fucky wucky!!
	
	vkWaitForFences( aDevice.GetDevice(  ), 1, &aInFlightFences[ aCurrentFrame ], VK_TRUE, UINT64_MAX );
	
	uint32_t imageIndex;
	VkResult res =  vkAcquireNextImageKHR( aDevice.GetDevice(  ), aSwapChain, UINT64_MAX, aImageAvailableSemaphores[ aCurrentFrame ], VK_NULL_HANDLE, &imageIndex );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
	{
		ReinitSwapChain(  );
		return;
	}
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		throw std::runtime_error( "Failed ot acquire swap chain image!" );
	}
	if ( aImagesInFlight[ imageIndex ] != VK_NULL_HANDLE )
	{
		vkWaitForFences( aDevice.GetDevice(  ), 1, &aImagesInFlight[ imageIndex ], VK_TRUE, UINT64_MAX );
	}
	aImagesInFlight[ imageIndex ] = aInFlightFences[ aCurrentFrame ];

	for ( auto& model : aModels )
	{
	        UpdateUniformBuffers( imageIndex, *model );
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

	vkResetFences( aDevice.GetDevice(  ), 1, &aInFlightFences[ aCurrentFrame ] );
	if ( vkQueueSubmit( aDevice.GetGraphicsQueue(  ), 1, &submitInfo, aInFlightFences[ aCurrentFrame ] ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to submit draw command buffer!" );
	}
	
	VkPresentInfoKHR presentInfo{  };
	presentInfo.sType 			= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount 		= 1;
	presentInfo.pWaitSemaphores 		= signalSemaphores;

	VkSwapchainKHR swapChains[  ] 		= { aSwapChain };
	presentInfo.swapchainCount 		= 1;
	presentInfo.pSwapchains 		= swapChains;
	presentInfo.pImageIndices 		= &imageIndex;
	presentInfo.pResults 			= NULL; // Optional

	res = vkQueuePresentKHR( aDevice.GetPresentQueue(  ), &presentInfo );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
	{
		ReinitSwapChain(  );
	}
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		throw std::runtime_error( "Failed ot present swap chain image!" );
	}
	
	vkQueueWaitIdle( aDevice.GetPresentQueue(  ) );

	aCurrentFrame = ( aCurrentFrame + 1 ) % MAX_FRAMES_PROCESSING;
	
	//SDL_Delay( 1000 / 144 );
}

void Renderer::Cleanup(  )
{
        DestroySwapChain(  );
	vkDestroySampler( aDevice.GetDevice(  ), aTextureSampler, NULL );
	vkDestroyDescriptorSetLayout( aDevice.GetDevice(  ), aModelSetLayout, NULL );
	vkDestroyDescriptorSetLayout( aDevice.GetDevice(  ), aSpriteSetLayout, NULL );
	for ( auto& model : aModels )
	{
	        DestroyRenderable< ModelData >( *model );
	}
	for ( auto& sprite : aSprites )
	{
	        DestroyRenderable< SpriteData >( *sprite );
	}
	for ( int i = 0; i < MAX_FRAMES_PROCESSING; i++ )
	{
		vkDestroySemaphore( aDevice.GetDevice(  ), aRenderFinishedSemaphores[ i ], NULL );
		vkDestroySemaphore( aDevice.GetDevice(  ), aImageAvailableSemaphores[ i ], NULL );
		vkDestroyFence( aDevice.GetDevice(  ), aInFlightFences[ i ], NULL);
        }
}

Renderer::Renderer(  ) : BaseSystem(  )
{
	aSystemType = RENDERER_C;
	aAllocator.apDevice = &aDevice;
}

void Renderer::SendMessages(  )
{
	apCommandManager->Execute( GuiSystem::Commands::ASSIGN_WINDOW, aDevice.GetWindow(  ) );
}

Renderer::~Renderer
	(  )
{
	Cleanup(  );
}
