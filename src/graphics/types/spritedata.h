/*
spritedata.h ( Auhored by p0lyh3dron )

Declares the spritedata, a container that
stores all data related to a sprite.
*/
#pragma once

#include "../allocator.h"
#include "../initializers.h"
#include "types/transform.h"
#include "graphics/renderertypes.h"

class SpriteData
{
public:
	VkBuffer 		aVertexBuffer;
	VkBuffer		aIndexBuffer;
	VkDeviceMemory 		aVertexBufferMem;
	VkDeviceMemory		aIndexBufferMem;
	VkDescriptorSetLayout	aTextureLayout;
	VkPipelineLayout	aPipelineLayout;
	VkPipeline		aPipeline;

	uint32_t 		aVertexCount;
	uint32_t		aIndexCount;
	vertex_3d_t 		*apVertices;
	uint32_t 		*apIndices;
	bool 			aNoDraw = true;
        float			aWidth  = 0.5;
	float			aHeight = 0.5;
	glm::vec2		aPos;
	
	Transform		aTransform;
        TextureDescriptor	*apTexture;
	/* Allocates the model, and loads its textures etc.  */
	void		Init(  );
	/* Reinitialize data that is useless after the swapchain becomes outdated.  */
	void		Reinit(  );
	/* Binds the model data to the GPU to be rendered later.  */
	void 		Bind( VkCommandBuffer c, uint32_t i );
	/* Draws the model to the screen.  */
	void 		Draw( VkCommandBuffer c );
	/* Adds a material to the model.  */
	void		SetTexture( const std::string &srTexturePath, VkSampler sSampler );
	/* Default the model and set limits.  */
			SpriteData(  ) : apVertices( NULL ), apIndices( NULL ), aNoDraw( false ), aPos( { 0.5, 0.5 } ), aWidth( 0.5 ), aHeight( 0.5 ){  };
	/* Frees the memory used by objects outdated by a new swapchain state.  */
	void		FreeOldResources(  );
	/* Frees all memory used by model.  */
			~SpriteData(  );
};

