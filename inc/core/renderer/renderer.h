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
#include "../../types/modeldata.h"
#include "../../types/spritedata.h"
#include "../gui.h"
#include "materialsystem.h"

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

	ImageViews                  aSwapChainImageViews;       // View into an image, describing which part to access, one needed for each image
	Framebuffers                aSwapChainFramebuffers;		//  
	CommandBuffers              aCommandBuffers;	        // Send commands to these to be executed later, better for concurrency, so many are nice
	VkDescriptorPool            aDescPool;
	VkSampler                   aTextureSampler;
	VkImage                     aDepthImage;
	VkDeviceMemory              aDepthImageMemory;
	VkImageView                 aDepthImageView;
	VkImage                     aColorImage;
	VkDeviceMemory              aColorImageMemory;
	VkImageView                 aColorImageView;
	SemaphoreList               aImageAvailableSemaphores;
	SemaphoreList               aRenderFinishedSemaphores;
	FenceList                   aInFlightFences;
	FenceList                   aImagesInFlight;
	int                         aCurrentFrame       = 0;
	bool                        aImGuiInitialized   = false;

	MaterialSystem*             apMaterialSystem;
	friend class MaterialSystem;  // maybe temporary?

	/* A.  */
	void    InitCommands(  );
	/* Waaaaah!!!  */
	void	EnableImgui(  );
	/* A.  */
	void    InitCommandBuffers(  );
	/* Loads all shaders that will be used in rendering.  */
	void	InitShaders(  );
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
	void 	Cleanup(  );
public:
	enum class	Commands{ NONE = 0, IMGUI_INITIALIZED, SET_VIEW, GET_WINDOW_SIZE };
	
	View            aView;
	ModelDataList   aModels;
	SpriteDataList  aSprites;

	/* A.  */
	void    Init(  );
	/* A.  */
	void 	InitVulkan(  );
	/* A.  */
	void 	InitModel( ModelData &srModelData, const String &srModelPath, const String &srTexturePath );
	/* A.  */
	void 	InitSprite( SpriteData &srSpriteData, const String &srSpritePath );
	/* A.  */
	void 	DrawFrame(  );

	/* Sets the view  */
	void    SetView( View& view );
	/* Get the window width and height  */
	void    GetWindowSize( uint32_t* width, uint32_t* height );
	/* A.  */
	SDL_Window    *GetWindow(  );

	/* A.  */
		Renderer(  );
	/* A.  */
	void 	SendMessages(  );
	/* A.  */
		~Renderer(  );
};
