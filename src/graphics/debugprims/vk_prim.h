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
	glm::mat4 aTransform;
};

class Primitive {
protected:
	VkBuffer       aVBuf;
	VkDeviceMemory aVMem;
	uint32_t       aVtxCnt;
public:
	VkBuffer       &GetBuffer() { return aVBuf; }
	VkDeviceMemory &GetMem()    { return aVMem; }
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
        Line  *InitLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor );
	/* Removes a line from the debug draw list.  */
	void   RemoveLine( Line *spLine );
	/* Creates a new pipeline for rendering primitives.  */
	void   InitPrimPipeline();
	/* Initializes the general stuff required for rendering.  */
	void   Init();
	      ~VulkanPrimitiveMaterials();
};
