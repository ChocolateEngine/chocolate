/*
 *	primvtx.hh
 *
 *	Authored by Karl "p0ly_" Kreuze on January 22, 2022
 *
 *	Declares the primitive vertex class, a vertex optimized
 *	for use in drawing primitives in the Vulkan API.
 */
#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

class PrimVtx {
	using Attributes = std::array< VkVertexInputAttributeDescription, 2 >;
public:
	glm::vec3 aPos;
	glm::vec3 aColor;
	/* Returns information about how to read vertices in the shader.  */
	static VkVertexInputBindingDescription GetBindings() {
		VkVertexInputBindingDescription b;

		b.binding   = 0;
		b.stride    = sizeof( PrimVtx );
		b.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return b;
	}
	/* Returns the binding information about the vertex.  */
	static Attributes GetAttribute() {
		Attributes a{};

		a[ 0 ].binding  = 0;
		a[ 0 ].location = 0;
		a[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
		a[ 0 ].offset   = offsetof( PrimVtx, aPos );

		a[ 1 ].binding  = 0;
		a[ 1 ].location = 1;
		a[ 1 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
		a[ 1 ].offset   = offsetof( PrimVtx, aColor );

		return a;
	}
};
