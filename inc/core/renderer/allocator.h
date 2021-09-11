/*
allocator.h ( Authored by p0lyh3dron )

Declares some of the allocator functions for
many of the Vk objects used in the renderer.  
*/
#pragma once

#define MAX_FRAMES_PROCESSING 2

#include "device.h"
#include "shadercache.h"
#include "descriptorcache.h"
#include "../../types/renderertypes.h"

#include <vector>
#include <functional>

#define NO_CULLING 1 << 0
#define NO_DEPTH   1 << 1

typedef std::vector< char >     ByteArray;

class Allocator
{
private:
	typedef std::string					String;
	typedef std::vector< std::function< void(  ) > > 	FunctionList;
	typedef std::vector< VkImage >			        ImageSet;
	typedef std::vector< uint32_t >				IndexSet;
	typedef std::vector< VkBuffer >				BufferSet;
	typedef std::vector< VkDeviceMemory >			MemorySet;
	typedef std::vector< VkDescriptorSet >			DescriptorSets;
	typedef std::vector< combined_image_info_t >		ImageInfoSets;
	typedef std::vector< combined_buffer_info_t >		BufferInfoSets;
	typedef std::vector< VkImageView >			ImageViews;
	typedef std::vector< VkDescriptorSetLayoutBinding >     DescSetLayouts;
	typedef std::vector< VkFramebuffer >			FrameBuffers;
	typedef std::vector< VkSemaphore >			SemaphoreList;
	typedef std::vector< VkFence >				FenceList;
	
	FunctionList 		aFreeQueue;
	ShaderCache		aShaderCache;
	DescriptorCache		aDescriptorCache;
	/* A.  */
	VkFormat 		FindDepthFormat(  );
	/* Reads a file into a byte array.  */
	ByteArray     		ReadFile( const String &srFilePath );
	/* Wraps bytecode into objects for the pipeline to use.  */
	VkShaderModule 		CreateShaderModule( const ByteArray &srCode );
	/* A.  */
	void 			TransitionImageLayout( VkImage sImage, VkImageLayout sOldLayout, VkImageLayout sNewLayout );

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
public:
        ImageSet 		*apSwapChainImages = NULL;
	VkRenderPass 		*apRenderPass = NULL;
        Device 			*apDevice;

	/* A.  */
	void 			InitAllocator( ImageSet &sSwapChainImages );
	/* A.  */
	void 			InitImageView( VkImageView &sImageView, VkImage sImage, VkFormat sFormat, VkImageAspectFlags sAspectFlags );
	/* A.  */
	void 			InitImage( VkImageCreateInfo sImageInfo, VkMemoryPropertyFlags sProperties, VkImage &srImage, VkDeviceMemory &srImageMemory );
	/* A.  */
	void 			InitTextureImage( const String &srImagePath, VkImage &srTImage, VkDeviceMemory &srTImageMem, float *spWidth, float *spHeight );
	/* A.  */
	void 			InitTextureImageView( VkImageView &srTImageView, VkImage sTImage );
	/* A.  */
	TextureDescriptor       InitTexture( const String &srImagePath, VkDescriptorSetLayout sLayout, VkDescriptorPool sPool, VkSampler sSampler );
	/* A.  */
	template< typename T >
		void 	        InitTexBuffer( const std::vector< T > &srData, VkBuffer &srBuffer, VkDeviceMemory &srBufferMem, VkBufferUsageFlags sUsage );
	/* A.  */
        void 			InitUniformBuffers( VkBuffer *&sprUBuffers, VkDeviceMemory *&sprUBuffersMem );
	/* A.  */
	void 			InitDescriptorSets( VkDescriptorSet *&sprDescSets, VkDescriptorSetLayout &srDescSetLayout,
						    VkDescriptorPool &srDescPool, ImageInfoSets sDescImageInfos, BufferInfoSets sDescBufferInfos );
        /* Initializes the texture, uniform values, and pipeline for the model.  */
	void			InitModelResources(  );
	/* A.  */
	void 			InitImageViews( ImageViews &srSwapChainImageViews );
	/* A.  */
	void 			InitRenderPass( VkRenderPass &srRenderPass );
	/* A.  */
        VkDescriptorSetLayout   InitDescriptorSetLayout( DescSetLayouts sBindings );
	/* Creates graphics pipeline layouts using the specified descriptor set layouts.  */
	VkPipelineLayout        InitPipelineLayouts( VkDescriptorSetLayout *spSetLayouts, uint32_t setLayoutsCount );
	/* A.  */
	template< typename T >
        void 			InitGraphicsPipeline( VkPipeline &srPipeline, VkPipelineLayout &srLayout, VkDescriptorSetLayout &srDescSetLayout, const String &srVertShader,
						      const String &srFragShader, int sFlags );
	/* A.  */
	void 			InitDepthResources( VkImage &srDepthImage, VkDeviceMemory &srDepthImageMemory, VkImageView &srDepthImageView );
	/* A.  */
	void 			InitFrameBuffer( FrameBuffers &srSwapChainFramebuffers, ImageViews &srSwapChainImageViews,
						 VkImageView &srDepthImageView );
	/* A.  */
	void 			InitDescPool( VkDescriptorPool &srDescPool, std::vector< VkDescriptorPoolSize > sPoolSizes );
	/* A.  */
	void 			InitImguiPool( SDL_Window *spWindow );
	/* A.  */
	void 			InitSync( SemaphoreList &srImageAvailableSemaphores, SemaphoreList &srRenderFinishedSemaphores,
					  FenceList &srInFlightFences, FenceList &srImagesInFlight );
	/* A.  */
	void 			FreeResources(  );
	/* A.  */
				~Allocator(  );
};
