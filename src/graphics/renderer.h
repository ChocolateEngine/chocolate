/*
renderer.h ( Authored by p0lyh3dron )

Deckares the renderer class which is a higher
level abstraction for interfacing with
Vulkan.  
*/
#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include "core/core.h"
#include "system.h"
#include "graphics/renderertypes.h"
#include "graphics/imesh.h"
#include "graphics/sprite.h"
#include "gui/gui.h"
#include "materialsystem.h"
#include "debugprims/primcreator.h"
#include "types/databuffer.hh"

#include "allocator.h"

#include <vulkan/vulkan.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <array>

#include <optional>

class Renderer	//	Most of these objects are used to make new objects by initializing them with certain parameters, then creating and storing them
{
protected:
	typedef std::vector< VkImageView >		ImageViews;
	typedef std::vector< VkImage >			ImageSet;
	typedef std::vector< VkFramebuffer >		Framebuffers;
	typedef std::vector< VkCommandBuffer >		CommandBuffers;
	typedef std::vector< VkSemaphore >		SemaphoreList;
	typedef std::vector< VkFence >			FenceList;
	typedef std::vector< Model* >		ModelList;
	typedef std::vector< Sprite* >		SpriteList;
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
	bool                        aReinitSwapChain    = false;

	friend class MaterialSystem;

	/* A.  */
	void    InitCommandBuffers(  );
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
	ModelList       aModels;
	SpriteList      aSprites;

	DebugRenderer   aDbgDrawer;

	/* A.  */
	void    Init(  );
	/* A.  */
	void 	InitVulkan(  );
	/* Waaaaah!!!  */
	void	EnableImgui(  );
	/* A.  */
	bool 	LoadModel( Model* sModel, const String &srPath );
	/* A.  */
	bool 	LoadSprite( Sprite &srSprite, const String &srPath );
	/* A.  */
	void 	DrawFrame(  );
	/* Create a line and add it to drawing.  */
	void    CreateLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor );

	/* A.  */
	VkSampler       GetTextureSampler(  ) { return aTextureSampler; }

	/* Sets the view  */
	void    SetView( View& view );
	/* Get the window width and height  */
	void    GetWindowSize( uint32_t* width, uint32_t* height );
	/* A.  */
	SDL_Window    *GetWindow(  );

	/* A.  */
		Renderer(  );
	/* A.  */
		~Renderer(  );
};

extern Renderer* renderer;
