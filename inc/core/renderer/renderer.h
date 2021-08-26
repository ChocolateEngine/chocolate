/*
renderer.h ( Authored by p0lyh3dron )

Deckares the renderer class which is a higher
level abstraction for interfacing with
Vulkan.  
*/
#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "../../shared/system.h"
#include "../../types/enums.h"
#include "../../types/renderertypes.h"
#include "../gui.h"

#include "allocator.h"

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <array>

#include <optional>

class Renderer : public BaseSystem	//	Most of these objects are used to make new objects by initializing them with certain parameters, then creating and storing them
{
protected:
	typedef std::vector< VkImageView >		ImageViews;
	typedef std::vector< VkImage >			ImageSet;
	typedef std::vector< VkFramebuffer >		Framebuffers;
	typedef std::vector< VkCommandBuffer >		CommandBuffers;
	typedef std::vector< VkSemaphore >		SemaphoreList;
	typedef std::vector< VkFence >			FenceList;
	typedef std::vector< ModelData* >		ModelDataList;
	typedef std::vector< SpriteData* >		SpriteDataList;
	typedef std::string				String;
	
        Device 				aDevice;
        Allocator 			aAllocator;
	VkSwapchainKHR 			aSwapChain;		        //	Queue for stuff to be rendered, does some processing before drawn to screen
        ImageViews 			aSwapChainImageViews;	        //	View into an image, describing which part to access, one needed for each image
        ImageSet 			aSwapChainImages;	        //	Stores images to be rendered, can have many
        Framebuffers 			aSwapChainFramebuffers;		//	
        CommandBuffers 			aCommandBuffers;	        //	Send commands to these to be executed later, better for concurrency, so many are nice
	VkFormat 			aSwapChainImageFormat;
	VkExtent2D 			aSwapChainExtent;
	VkRenderPass 			aRenderPass;		        //	Stores framebuffer attachments that will be used for rendering
	VkDescriptorSetLayout 		aModelSetLayout;
	VkDescriptorSetLayout 		aSpriteSetLayout;
	VkDescriptorPool 		aDescPool;
	VkImageView 			aTextureImageView;
	VkSampler 			aTextureSampler;
	VkImage 			aDepthImage;
	VkDeviceMemory 			aDepthImageMemory;
	VkImageView 			aDepthImageView;
	VkPipelineLayout 		aModelLayout;
	VkPipelineLayout 		aSpriteLayout;
	VkPipeline 			aModelPipeline;		        //	Colossal object that handles most stages of rendering. Seems to need multiple to render multiple things.
	VkPipeline 			aSpritePipeline;
        SemaphoreList 			aImageAvailableSemaphores;
	SemaphoreList 			aRenderFinishedSemaphores;
        FenceList 			aInFlightFences;
	FenceList 			aImagesInFlight;
        int 				aCurrentFrame 		= 0;
        bool 				aImGuiInitialized 	= false;

	/* A.  */
	void    InitCommands(  );
	/* A.  */
	void    InitCommandBuffers(  );
	/* A.  */
	void    LoadObj( const String &srObjPath, ModelData &srModel );
	/* A.  */
	void    LoadGltf( const String &srGltfPath, ModelData &srModel );
	/* A.  */
	void    InitModelVertices( const String &srModelPath, ModelData &srModel );
	/* A.  */
	void    InitSpriteVertices( const String &srSpritePath, SpriteData &srSprite );
	/* A.  */
	void    ReinitSwapChain(  );
	/* A.  */
	void    DestroySwapChain(  );
	/* A.  */
	bool    HasStencilComponent( VkFormat sFmt );
	/* A.  */
	template< typename T >
	void    DestroyRenderable( T &srRenderable );
	/* A.  */
	void    UpdateUniformBuffers( uint32_t sCurrentImage, ModelData &srModelData );
	/* A.  */
	void 	Cleanup(  );
public:
        ModelDataList 	aModels;
        SpriteDataList 	aSprites;
	/* A.  */
	void 	InitVulkan(  );
	/* A.  */
	void 	InitModel( ModelData &srModelData, const String &srModelPath, const String &srTexturePath );
	/* A.  */
	void 	InitSprite( SpriteData &srSpriteData, const String &srSpritePath );
	/* A.  */
	void 	DrawFrame(  );
	/* A.  */
		Renderer(  );
	/* A.  */
	void 	SendMessages(  );
	/* A.  */
		~Renderer(  );
};
