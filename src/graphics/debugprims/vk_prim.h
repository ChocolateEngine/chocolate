/*
 *	vk_prim.h
 *
 *	Authored by "p0ly_" on January 20, 2022
 *
 *	Declares the vulkan things required for debug rendering.
 *      
 *	
 */
#pragma once

#include "graphics/primvtx.hh"
#include "graphics/renderertypes.h"

class PrimPushConstant {
public:
	alignas( 16 ) glm::mat4 aTransform;
        alignas( 16 ) glm::vec3 a;
	alignas( 16 ) glm::vec3 b;
	alignas( 16 ) glm::vec3 aColor;
};

class Primitive {
protected:
public:
	PrimPushConstant aPush;
	void            Draw( VkCommandBuffer c );
	               ~Primitive();
};

class Line : public Primitive {
public:
	void Init( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor );
};

class VulkanPrimitiveMaterials {
	using Sets       = std::vector< VkDescriptorSet >;
	using Primitives = std::vector< Primitive* >;
protected:
	VkPipelineLayout      aPipeLayout;
	VkPipeline            aPipe;
	Primitives            aPrimitives;
public:
	/* Update function to draw all loaded primitives.  */
	void   DrawPrimitives( VkCommandBuffer c, View v );
	/* Creates a new line.  */
        void   InitLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor );
        /* Clears the lines that were queued.  */
	void   RemoveLines();
	/* Creates a new pipeline for rendering primitives.  */
	void   InitPrimPipeline();
	/* Initializes the general stuff required for rendering.  */
	void   Init();
	      ~VulkanPrimitiveMaterials();
};
