
#include "../../../inc/core/renderer/renderer.h"

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

void renderer_c::init_commands
	(  )
{
	msg_s msg;
	msg.type = RENDERER_C;

	msg.msg = IMGUI_INITIALIZED;
	msg.func = [ & ]( std::vector< std::any > args ){ imGuiInitialized = true; };
	engineCommands.push_back( msg );
}

void renderer_c::init_vulkan
	(  )
{
	device.init_swap_chain( swapChain, swapChainImages, swapChainImageFormat, swapChainExtent );
	allocator.init_image_views( swapChainImages, swapChainImageViews, swapChainImageFormat );
	allocator.init_render_pass( renderPass, swapChainImageFormat );
	allocator.init_desc_set_layout( descSetLayout );
	allocator.init_graphics_pipeline< vertex_3d_t >( modelPipeline,
							 modelLayout,
							 swapChainExtent,
							 descSetLayout,
							 renderPass,
							 "materials/shaders/3dvert.spv",
							 "materials/shaders/3dfrag.spv" );
	allocator.init_graphics_pipeline< vertex_2d_t >( spritePipeline,
							 spriteLayout,
							 swapChainExtent,
							 descSetLayout,
							 renderPass,
							 "materials/shaders/2dvert.spv",
							 "materials/shaders/2dfrag.spv" );
	allocator.init_depth_resources( depthImage, depthImageMemory, depthImageView, swapChainExtent );
	allocator.init_frame_buffer( swapChainFramebuffers,
				     swapChainImageViews,
				     depthImageView,
				     renderPass,
				     swapChainExtent );
	device.init_texture_sampler( textureSampler, VK_SAMPLER_ADDRESS_MODE_REPEAT );
	allocator.init_desc_pool( descPool, { { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000 }, { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000 } } } );
	allocator.init_imgui_pool( device.window(  ), renderPass );
	 ;
	allocator.init_sync( imageAvailableSemaphores,
			     renderFinishedSemaphores,
			     inFlightFences,
			     imagesInFlight,
			     swapChainImages );
}

bool renderer_c::has_stencil_component
	( VkFormat fmt )
{
	return fmt == VK_FORMAT_D32_SFLOAT_S8_UINT || fmt == VK_FORMAT_D24_UNORM_S8_UINT;
}

void renderer_c::init_command_buffers
	(  )
{
	commandBuffers.resize( swapChainFramebuffers.size(  ) );
	VkCommandBufferAllocateInfo allocInfo{  };
	allocInfo.sType 		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool 		= device.c_pool(  );
	allocInfo.level 		= VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount	= ( uint32_t )commandBuffers.size(  );

	if ( vkAllocateCommandBuffers( device.dev(  ), &allocInfo, commandBuffers.data(  ) ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to allocate command buffers!" );
	}
	for ( int i = 0; i < commandBuffers.size(  ); i++ )
	{
		VkCommandBufferBeginInfo beginInfo{  };
		beginInfo.sType 		= VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags 		= 0; // Optional
		beginInfo.pInheritanceInfo 	= NULL; // Optional

		if ( vkBeginCommandBuffer( commandBuffers[ i ], &beginInfo ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to begin recording command buffer!" );
		}
		VkRenderPassBeginInfo renderPassInfo{  };
		renderPassInfo.sType 		 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass 	 = renderPass;
		renderPassInfo.framebuffer 	 = swapChainFramebuffers[ i ];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		std::array< VkClearValue, 2 > clearValues{  };
		clearValues[ 0 ].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		clearValues[ 1 ].depthStencil = { 1.0f, 0 };
			
		renderPassInfo.clearValueCount 	 = ( uint32_t )clearValues.size(  );
		renderPassInfo.pClearValues 	 = clearValues.data(  );

		vkCmdBeginRenderPass( commandBuffers[ i ], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
		
		vkCmdBindPipeline( commandBuffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, modelPipeline );

		for ( auto& model : *models )
		{
			model->bind( commandBuffers[ i ], modelLayout, i );
			model->draw( commandBuffers[ i ] );
		}
		vkCmdBindPipeline( commandBuffers[ i ], VK_PIPELINE_BIND_POINT_GRAPHICS, spritePipeline );
		for ( auto& sprite : *sprites )
		{
			if ( !sprite->noDraw )
			{
				sprite->bind( commandBuffers[ i ], spriteLayout, i );
				sprite->draw( commandBuffers[ i ] );
			}
		}

		if ( imGuiInitialized )
		{
			ImGui::Render(  );
			ImGui_ImplVulkan_RenderDrawData( ImGui::GetDrawData(  ), commandBuffers[ i ] );
		}
		
		vkCmdEndRenderPass( commandBuffers[ i ] );

		if ( vkEndCommandBuffer( commandBuffers[ i ] ) != VK_SUCCESS )
		{
			throw std::runtime_error( "Failed to record command buffer!" );
		}
	}
}

void renderer_c::load_obj
	( const std::string& objPath, model_data_t& model )
{
	tinyobj::attrib_t attrib;
	std::vector< tinyobj::shape_t > shapes;
	std::vector< tinyobj::material_t > materials;
	std::vector< vertex_3d_t > vertices;
	std::vector< uint32_t > indices;
	std::string warn, err;

	if ( !tinyobj::LoadObj( &attrib, &shapes, &materials, &warn, &err, objPath.c_str(  ) ) )
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
	model.vCount = ( uint32_t )vertices.size(  );
	model.iCount = ( uint32_t )indices.size(  );
	allocator.init_vertex_buffer( vertices, model.vBuffer, model.vBufferMem );
	allocator.init_index_buffer( indices, model.iBuffer, model.iBufferMem );
	printf( "%d vertices loaded!\n", vertices.size(  ) );
}
void renderer_c::load_gltf
	( const std::string& gltfPath, model_data_t& model )
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

void renderer_c::init_model_vertices
	( const std::string& modelPath, model_data_t& model )
{
	if ( modelPath.substr( modelPath.size(  ) - 4 ) == ".obj" )
	{
		load_obj( modelPath, model );
		return;
	}
	if ( modelPath.substr( modelPath.size(  ) - 4 ) == ".glb" )
	{
		load_gltf( modelPath, model );
		return;
	}
}

void renderer_c::init_sprite_vertices
	( const std::string& spritePath, sprite_data_t& sprite )
{
	std::vector< vertex_2d_t > vertices =
	{
		{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    		{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    		{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    		{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
	};
	std::vector< uint32_t > indices =
	{
		0, 1, 2, 2, 3, 0	
	};
	sprite.vCount = ( uint32_t )vertices.size(  );
	sprite.iCount = ( uint32_t )indices.size(  );
        allocator.init_vertex_buffer( vertices, sprite.vBuffer, sprite.vBufferMem );
	allocator.init_index_buffer( indices, sprite.iBuffer, sprite.iBufferMem );
}

void renderer_c::reinit_swap_chain
	(  )
{
	vkDeviceWaitIdle( device.dev(  ) );

	destroy_swap_chain(  );
	
	device.init_swap_chain( swapChain, swapChainImages, swapChainImageFormat, swapChainExtent );
	allocator.init_image_views( swapChainImages, swapChainImageViews, swapChainImageFormat );
	allocator.init_render_pass( renderPass, swapChainImageFormat );
	allocator.init_desc_set_layout( descSetLayout );
	allocator.init_graphics_pipeline< vertex_3d_t >( modelPipeline,
							 modelLayout,
							 swapChainExtent,
							 descSetLayout,
							 renderPass,
							 "materials/shaders/3dvert.spv",
							 "materials/shaders/3dfrag.spv" );
	allocator.init_graphics_pipeline< vertex_2d_t >( spritePipeline,
							 spriteLayout,
							 swapChainExtent,
							 descSetLayout,
							 renderPass,
							 "materials/shaders/2dvert.spv",
							 "materials/shaders/2dfrag.spv" );
	allocator.init_depth_resources( depthImage, depthImageMemory, depthImageView, swapChainExtent );
	allocator.init_frame_buffer( swapChainFramebuffers,
				     swapChainImageViews,
				     depthImageView,
				     renderPass,
				     swapChainExtent );
	for ( auto& model : *models )
	{
		allocator.init_uniform_buffers< ubo_3d_t >( model->uBuffers, model->uBuffersMem, swapChainImages );	//	Please fix this ubo_2d_t shit, I just want easy sprites
	}
	for ( auto& sprite : *sprites )
	{
		allocator.init_uniform_buffers< ubo_3d_t >( sprite->uBuffers, sprite->uBuffersMem, swapChainImages );	
	}
	allocator.init_desc_pool( descPool, { { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2000 }, { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2000 } } } );
	for ( auto& model : *models )
	{
		allocator.init_desc_sets< ubo_3d_t >( model->descSets,
						      model->uBuffers,
						      model->tImageView,
						      swapChainImages,
						      descSetLayout,
						      descPool,
						      textureSampler );	
	}
	for ( auto& sprite : *sprites )
	{
		allocator.init_desc_sets< ubo_2d_t >( sprite->descSets,
						      sprite->uBuffers,
						      sprite->tImageView,
						      swapChainImages,
						      descSetLayout,
						      descPool,
						      textureSampler );	
	}
	 ;
}

void renderer_c::destroy_swap_chain
	(  )
{
	vkDestroyImageView( device.dev(  ), depthImageView, nullptr );
	vkDestroyImage( device.dev(  ), depthImage, nullptr );
	vkFreeMemory( device.dev(  ), depthImageMemory, nullptr );
	for ( auto& model : *models )
	{
		for ( int i = 0; i < swapChainImages.size(  ); i++ )
		{
			vkDestroyBuffer( device.dev(  ), model->uBuffers[ i ], NULL );
			vkFreeMemory( device.dev(  ), model->uBuffersMem[ i ], NULL );
		}
	}
	for ( auto& sprite : *sprites  )
	{
		for ( int i = 0; i < swapChainImages.size(  ); i++ )
		{
			vkDestroyBuffer( device.dev(  ), sprite->uBuffers[ i ], NULL );
			vkFreeMemory( device.dev(  ), sprite->uBuffersMem[ i ], NULL );
		}
	}
	vkDestroyDescriptorPool( device.dev(  ), descPool, NULL );
	vkDestroyDescriptorSetLayout( device.dev(  ), descSetLayout, NULL );
        for ( auto framebuffer : swapChainFramebuffers )
	{
		vkDestroyFramebuffer( device.dev(  ), framebuffer, NULL );
        }
	vkFreeCommandBuffers( device.dev(  ), device.c_pool(  ), ( uint32_t )commandBuffers.size(  ), commandBuffers.data(  ) );

	vkDestroyPipeline( device.dev(  ), modelPipeline, NULL );
        vkDestroyPipelineLayout( device.dev(  ), modelLayout, NULL );
	vkDestroyPipeline( device.dev(  ), spritePipeline, NULL );
        vkDestroyPipelineLayout( device.dev(  ), spriteLayout, NULL );
        vkDestroyRenderPass( device.dev(  ), renderPass, NULL );

        for ( auto imageView : swapChainImageViews )
	{
		vkDestroyImageView( device.dev(  ), imageView, NULL );
        }

        vkDestroySwapchainKHR( device.dev(  ), swapChain, NULL );
}

template< typename T >
void renderer_c::destroy_renderable
	( T& renderable )
{
	vkDestroyImageView( device.dev(  ), renderable.tImageView, NULL );
	vkDestroyImage( device.dev(  ), renderable.tImage, NULL );
	vkFreeMemory( device.dev(  ), renderable.tImageMem, NULL );
	vkDestroyBuffer( device.dev(  ), renderable.iBuffer, NULL );
	vkFreeMemory( device.dev(  ), renderable.iBufferMem, NULL );
	vkDestroyBuffer( device.dev(  ), renderable.vBuffer, NULL );
	vkFreeMemory( device.dev(  ), renderable.vBufferMem, NULL );
}

void renderer_c::update_uniform_buffers
	( uint32_t currentImage, model_data_t& modelData )
{
	static auto startTime 	= std::chrono::high_resolution_clock::now(  );

	auto currentTime 	= std::chrono::high_resolution_clock::now();
	float time 		= std::chrono::duration< float, std::chrono::seconds::period >( currentTime - startTime ).count(  );

	ubo_3d_t ubo{  };
	ubo.model = glm::translate( glm::mat4( 1.0f ), glm::vec3( modelData.posX, modelData.posY, modelData.posZ ) ) * glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 45.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
	ubo.view  = glm::lookAt( glm::vec3( 0.1f, 10.0f, 25.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
	ubo.proj  = glm::perspective( glm::radians( 90.0f ), swapChainExtent.width / ( float ) swapChainExtent.height, 0.1f, 256.0f );

	ubo.proj[ 1 ][ 1 ] *= -1;

	void* data;
	vkMapMemory( device.dev(  ), modelData.uBuffersMem[ currentImage ], 0, sizeof( ubo ), 0, &data );
	memcpy( data, &ubo, sizeof( ubo ) );
	vkUnmapMemory( device.dev(  ), modelData.uBuffersMem[ currentImage ] );
}

void renderer_c::update_sprite_uniform_buffers
	( uint32_t currentImage, sprite_data_t& spriteData )
{
	ubo_3d_t ubo{  };
	ubo.model = glm::translate( glm::mat4( 1.0f ), glm::vec3( spriteData.posX, spriteData.posY, 0.0f ) ) * glm::rotate( glm::mat4( 1.0f ), glm::radians( 180.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
        ubo.view = glm::mat4( 1.0f );
	ubo.proj = glm::mat4( 1.0f );

	ubo.proj[ 1 ][ 1 ] *= -1;
	
	void* data;
	vkMapMemory( device.dev(  ), spriteData.uBuffersMem[ currentImage ], 0, sizeof( ubo ), 0, &data );	//	Validation vkMapMemory-size-00681 resolved... fix ubo sizes when 2d ubos work
	memcpy( data, &ubo, sizeof( ubo ) );
	vkUnmapMemory( device.dev(  ), spriteData.uBuffersMem[ currentImage ] );
}

void renderer_c::init_model
	( model_data_t& modelData, const std::string& modelPath, const std::string& texturePath )
{
	init_model_vertices( modelPath, modelData );
	allocator.init_texture_image( texturePath, modelData.tImage, modelData.tImageMem );
	allocator.init_texture_image_view( modelData.tImageView, modelData.tImage );
	allocator.init_uniform_buffers< ubo_3d_t >( modelData.uBuffers, modelData.uBuffersMem, swapChainImages );
	allocator.init_desc_sets< ubo_3d_t >( modelData.descSets,
					      modelData.uBuffers,
					      modelData.tImageView,
					      swapChainImages,
					      descSetLayout,
					      descPool,
					      textureSampler );
	models->push_back( &modelData );
	 ;
}

void renderer_c::init_sprite
	( sprite_data_t& spriteData, const std::string& spritePath )
{
	init_sprite_vertices( spritePath, spriteData );
	allocator.init_texture_image( spritePath, spriteData.tImage, spriteData.tImageMem );
	allocator.init_texture_image_view( spriteData.tImageView, spriteData.tImage );
	allocator.init_uniform_buffers< ubo_3d_t >( spriteData.uBuffers, spriteData.uBuffersMem, swapChainImages );	//	Fix ubo_2d_t
	allocator.init_desc_sets< ubo_2d_t >( spriteData.descSets,
					      spriteData.uBuffers,
					      spriteData.tImageView,
					      swapChainImages,
					      descSetLayout,
					      descPool,
					      textureSampler );
	sprites->push_back( &spriteData );
	 ;

}

void renderer_c::draw_frame
	(  )
{
	init_command_buffers(  );	//	Fucky wucky!!
	
	vkWaitForFences( device.dev(  ), 1, &inFlightFences[ currentFrame ], VK_TRUE, UINT64_MAX );
	
	uint32_t imageIndex;
	VkResult res =  vkAcquireNextImageKHR( device.dev(  ), swapChain, UINT64_MAX, imageAvailableSemaphores[ currentFrame ], VK_NULL_HANDLE, &imageIndex );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
	{
		reinit_swap_chain(  );
		return;
	}
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		throw std::runtime_error( "Failed ot acquire swap chain image!" );
	}
	if ( imagesInFlight[ imageIndex ] != VK_NULL_HANDLE )
	{
		vkWaitForFences( device.dev(  ), 1, &imagesInFlight[ imageIndex ], VK_TRUE, UINT64_MAX );
	}
	imagesInFlight[ imageIndex ] = inFlightFences[ currentFrame ];

	for ( auto& model : *models )
	{
		update_uniform_buffers( imageIndex, *model );
	}
	for ( auto& sprite : *sprites )
	{
		update_sprite_uniform_buffers( imageIndex, *sprite );
	}

	VkSubmitInfo submitInfo{  };
	submitInfo.sType 			= VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[  ] 		= { imageAvailableSemaphores[ currentFrame ] };
	VkPipelineStageFlags waitStages[  ] 	= { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount 		= 1;
	submitInfo.pWaitSemaphores 		= waitSemaphores;
	submitInfo.pWaitDstStageMask 		= waitStages;
	submitInfo.commandBufferCount 		= 1;
	submitInfo.pCommandBuffers 		= &commandBuffers[ imageIndex ];
	VkSemaphore signalSemaphores[  ] 	= { renderFinishedSemaphores[ currentFrame ] };
	submitInfo.signalSemaphoreCount 	= 1;
	submitInfo.pSignalSemaphores 		= signalSemaphores;

	vkResetFences( device.dev(  ), 1, &inFlightFences[ currentFrame ] );
	if ( vkQueueSubmit( device.g_queue(  ), 1, &submitInfo, inFlightFences[ currentFrame ] ) != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to submit draw command buffer!" );
	}
	
	VkPresentInfoKHR presentInfo{  };
	presentInfo.sType 			= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount 		= 1;
	presentInfo.pWaitSemaphores 		= signalSemaphores;

	VkSwapchainKHR swapChains[  ] 		= { swapChain };
	presentInfo.swapchainCount 		= 1;
	presentInfo.pSwapchains 		= swapChains;
	presentInfo.pImageIndices 		= &imageIndex;
	presentInfo.pResults 			= NULL; // Optional

	res = vkQueuePresentKHR( device.p_queue(  ), &presentInfo );

	if ( res == VK_ERROR_OUT_OF_DATE_KHR )
	{
		reinit_swap_chain(  );
	}
	else if ( res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR )
	{
		throw std::runtime_error( "Failed ot present swap chain image!" );
	}
	
	vkQueueWaitIdle( device.p_queue(  ) );

	currentFrame = ( currentFrame + 1 ) % MAX_FRAMES_PROCESSING;
	
	//SDL_Delay( 1000 / 144 );
}

void renderer_c::cleanup
	(  )
{
	destroy_swap_chain(  );
	vkDestroySampler( device.dev(  ), textureSampler, NULL );
	vkDestroyDescriptorSetLayout( device.dev(  ), descSetLayout, NULL );
	for ( auto& model : *models )
	{
		destroy_renderable< model_data_t >( *model );
	}
	for ( auto& sprite : *sprites )
	{
		destroy_renderable< sprite_data_t >( *sprite );
	}
	for ( int i = 0; i < MAX_FRAMES_PROCESSING; i++ )
	{
		vkDestroySemaphore( device.dev(  ), renderFinishedSemaphores[ i ], NULL );
		vkDestroySemaphore( device.dev(  ), imageAvailableSemaphores[ i ], NULL );
		vkDestroyFence( device.dev(  ), inFlightFences[ i ], NULL);
        }
}

renderer_c::renderer_c
	(  )
{
	systemType = RENDERER_C;
	allocator.dev = &device;
	init_commands(  );
}

void renderer_c::send_messages
	(  )
{
	msgs->add( GUI_C, ASSIGN_WIN, 0, { device.window(  ) } );
}

renderer_c::~renderer_c
	(  )
{
	cleanup(  );
}
