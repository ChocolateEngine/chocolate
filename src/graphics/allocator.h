/*
allocator.h ( Authored by p0lyh3dron )

Declares some of the allocator functions for
many of the Vk objects used in the renderer.  
*/
#pragma once

#define MAX_FRAMES_PROCESSING 2

#include "device.h"
#include "cache/shadercache.h"
#include "cache/descriptorcache.h"
#include "builder/pipelinebuilder.h"
#include "builder/layoutbuilder.h"

#include "graphics/renderertypes.h"
#include "initializers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <array>
#include <string>
#include <fstream>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_vulkan.h"

#define PSWAPCHAIN 	gpDevice->GetSwapChain(  )
#define DEVICE		gpDevice->GetDevice(  )

#include <vector>
#include <functional>

#define NO_CULLING 1 << 0
#define NO_DEPTH   1 << 1

typedef std::vector< char >     		        ByteArray;
typedef std::string				        String;
typedef std::vector< std::function< void(  ) > >        FunctionList;
typedef std::vector< VkImage >			        ImageSet;
typedef std::vector< uint32_t >			        IndexSet;
typedef std::vector< VkBuffer >			        BufferSet;
typedef std::vector< VkDeviceMemory >		        MemorySet;
typedef std::vector< VkDescriptorSet >		        DescriptorSets;
typedef std::vector< combined_image_info_t >	        ImageInfoSets;
typedef std::vector< combined_buffer_info_t >		BufferInfoSets;
typedef std::vector< VkImageView >			ImageViews;
typedef std::vector< VkDescriptorSetLayoutBinding >	DescSetLayouts;
typedef std::vector< VkFramebuffer >			FrameBuffers;
typedef std::vector< VkSemaphore >			SemaphoreList;
typedef std::vector< VkFence >				FenceList;

extern FunctionList 			gFreeQueue;
extern ShaderCache			gShaderCache;
extern DescriptorCache			gDescriptorCache;
extern PipelineBuilder			gPipelineBuilder;
extern LayoutBuilder			gLayoutBuilder;

extern Device 				*gpDevice;
extern ImageSet 			*gpSwapChainImages;
extern VkRenderPass 			*gpRenderPass;
extern VkDescriptorPool			*gpPool;

/* A.  */
VkFormat 		FindDepthFormat(  );
/* Wraps bytecode into objects for the pipeline to use.  */
VkShaderModule 		CreateShaderModule( const ByteArray &srCode );
/* A.  */
void 			TransitionImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout, uint32_t sMipLevels );
/* Creates a buffer and maps the memory.  */
void 			InitBuffer( VkDeviceSize sSize, VkBufferUsageFlags sUsage, VkMemoryPropertyFlags sProperties,
				    VkBuffer &srBuffer, VkDeviceMemory &srBufferMemory );
/* Initializes a texture, view, and appropriate descriptors.  */
void			InitTexture(  );
/* Copies the memory into the GPU.  */
void 			MapMemory( VkDeviceMemory sBufferMemory, VkDeviceSize sSize, const void *spData );
/* Copies the contents of a buffer into another buffer.  */
void 			CopyBuffer( VkBuffer sSrc, VkBuffer sDst, VkDeviceSize sSize );
/* A.  */
void 			CopyBufferToImage( VkBuffer sBuffer, VkImage sImage, uint32_t sWidth, uint32_t sHeight );
/* Immediately submits a command to a command buffer.  */
void 			Submit( const auto &&sFunction );
/* A.  */
void 			InitAllocator( ImageSet &sSwapChainImages );
/* A.  */
void 			InitImageView( VkImageView &sImageView, VkImageViewCreateInfo &srInfo );
/* A.  */
void 			InitImage( VkImageCreateInfo sImageInfo, VkMemoryPropertyFlags sProperties, VkImage &srImage, VkDeviceMemory &srImageMemory );
/* A.  */
void 			InitTextureImageView( VkImageView &srTImageView, VkImage sTImage, uint32_t sMipLevels );
/* Creates the mip maps from a given VkImage.  */
void			GenerateMipMaps( VkImage sImage, VkFormat sFormat, uint32_t sWidth, uint32_t sHeight, uint32_t sMipLevels );
/* A.  */
TextureDescriptor       *InitTexture( const String &srImagePath, VkDescriptorSetLayout sLayout, VkDescriptorPool sPool, VkSampler sSampler,
				      float *spWidth = NULL, float *spHeight = NULL );
TextureDescriptor       *InitTexture( const String &srImagePath, VkDescriptorSetLayout sLayout, VkDescriptorPool sPool, VkSampler sSampler, int b );

// BufferSize is sizeof(element) * count
void 	        	InitRenderableBuffer( void* srData, size_t sBufferSize, VkBuffer &srBuffer, VkDeviceMemory &srBufferMem, VkBufferUsageFlags sUsage );

/* A.  */
void 			InitUniformBuffers( DataBuffer< VkBuffer > &srUBuffers, DataBuffer< VkDeviceMemory > &srUBuffersMem, size_t uboSize );
/* A.  */
void 			InitDescriptorSets( DataBuffer< VkDescriptorSet > &srDescSets, VkDescriptorSetLayout &srDescSetLayout,
					    ImageInfoSets sDescImageInfos, BufferInfoSets sDescBufferInfos );

void            AllocImageSets( std::vector< VkDescriptorSet > &srSets, VkDescriptorSetLayout &srLayout, VkDescriptorPool &srPool );

void            UpdateImageSets( std::vector< VkDescriptorSet > &srSets, std::vector< TextureDescriptor* > &srImages, VkSampler &srSampler );

/* Initializes the uniform data, such as uniform buffers.  */
void			InitUniformData( UniformDescriptor &srDescriptor, VkDescriptorSetLayout sLayout, unsigned int uboSize );
/* A.  */
void 			InitImageViews( ImageViews &srSwapChainImageViews );
/* A.  */
void 			InitRenderPass(  );
/* A.  */
VkDescriptorSetLayout   InitDescriptorSetLayout( DescSetLayouts sBindings );
/* A.  */
VkDescriptorSetLayout   InitDescriptorSetLayout( VkDescriptorSetLayoutBinding sBinding );
/* Creates graphics pipeline layouts using the specified descriptor set layouts.  */
VkPipelineLayout        InitPipelineLayouts( VkDescriptorSetLayout *spSetLayouts, uint32_t setLayoutsCount, uint32_t sPushExtent );
/* A.  */
void 			InitDepthResources( VkImage &srDepthImage, VkDeviceMemory &srDepthImageMemory, VkImageView &srDepthImageView );
/* Initializes the multisampling image.  */
void			InitColorResources( VkImage &srColorImage, VkDeviceMemory &srColorImageMemory, VkImageView &srColorImageView );
/* A.  */
void 			InitFrameBuffer( FrameBuffers &srSwapChainFramebuffers, ImageViews &srSwapChainImageViews,
					 VkImageView &srDepthImageView, VkImageView &srColorImageView );
/* A.  */
void 			InitDescPool( std::vector< VkDescriptorPoolSize > sPoolSizes );
/* A.  */
void 			InitImguiPool( SDL_Window *spWindow );
/* A.  */
void 			InitSync( SemaphoreList &srImageAvailableSemaphores, SemaphoreList &srRenderFinishedSemaphores,
				  FenceList &srInFlightFences, FenceList &srImagesInFlight );
/* A.  */
void 			FreeResources(  );
