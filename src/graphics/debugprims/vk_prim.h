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

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

class PrimVtx {
	using Attributes = std::array< VkVertexInputAttributeDescription, 2 >;
public:
	glm::vec3 aPos;
	glm::vec3 aColor;

	static VkVertexInputBindingDescription GetBindings() {
		VkVertexInputBindingDescription b;

		b.binding   = 0;
		b.stride    = sizeof( PrimVtx );
		b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return b;
	}

	static Attributes GetAttribute() {
	        Attributes a{};

		a[ 0 ].binding  = 0;
		a[ 0 ].location = 0;
		a[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
		a[ 0 ].offset   = offsetof( PrimVtx, aPos );

	        a[ 0 ].binding  = 0;
		a[ 0 ].location = 1;
		a[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
		a[ 0 ].offset   = offsetof( PrimVtx, aColor );

		return a;
	}
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
	void DrawPrimitives( VkCommandBuffer c );
	/* Creates a new line.  */
	void InitLine( glm::vec3 sX, glm::vec3 sY, glm::vec3 sColor );
	/* Creates a new pipeline for rendering primitives.  */
	void InitPrimPipeline();
	/* Initializes the general stuff required for rendering.  */
	void Init();
	    ~VulkanPrimitiveMaterials();
};
