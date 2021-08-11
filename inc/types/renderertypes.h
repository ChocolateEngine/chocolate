#ifndef RENDERERTYPES_H
#define RENDERERTYPES_H

#define GLM_ENABLE_EXPERIMENTAL

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <array>
#include <optional>
#include <vector>

struct queue_family_indices_t
{
	int presentFamily = -1, graphicsFamily = -1; // make this not use optional pls
	bool complete (  ) { return ( presentFamily > -1 ) && ( graphicsFamily > -1 ); }
};

struct swap_chain_support_info_t
{
	VkSurfaceCapabilitiesKHR 		c;	//	capabilities
	std::vector< VkSurfaceFormatKHR > 	f;	//	formats
	std::vector< VkPresentModeKHR > 	p;	//	present modes
};

struct combined_buffer_info_t
{
	VkDescriptorType type;
	std::vector< VkBuffer >& buffer;
	unsigned int range;
};

struct combined_image_info_t
{
	VkDescriptorImageInfo imageInfo;
	VkDescriptorType type;
};

struct desc_set_layout_t
{
	VkDescriptorType descriptorType;
	unsigned int descriptorCount;
	VkShaderStageFlags stageFlags;
	const VkSampler* pImmutableSamplers;
};

struct push_constant_t
{
	glm::vec2 scale, translate;
};

struct vertex_3d_t
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription get_binding_desc
		(  )
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof( vertex_3d_t );
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array< VkVertexInputAttributeDescription, 3 > get_attribute_desc
		(  )
	{
		std::array< VkVertexInputAttributeDescription, 3 >attributeDescriptions{  };
		attributeDescriptions[ 0 ].binding  = 0;
		attributeDescriptions[ 0 ].location = 0;
		attributeDescriptions[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[ 0 ].offset   = offsetof( vertex_3d_t, pos );

		attributeDescriptions[ 1 ].binding  = 0;
		attributeDescriptions[ 1 ].location = 1;
		attributeDescriptions[ 1 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[ 1 ].offset   = offsetof( vertex_3d_t, color );

		attributeDescriptions[ 2 ].binding  = 0;
		attributeDescriptions[ 2 ].location = 2;
		attributeDescriptions[ 2 ].format   = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[ 2 ].offset   = offsetof( vertex_3d_t, texCoord );

		return attributeDescriptions;
	}
	bool operator==( const vertex_3d_t& other ) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

struct vertex_2d_t
{
	glm::vec2 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription get_binding_desc
		(  )
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof( vertex_2d_t );
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array< VkVertexInputAttributeDescription, 3 > get_attribute_desc
		(  )
	{
		std::array< VkVertexInputAttributeDescription, 3 >attributeDescriptions{  };
		attributeDescriptions[ 0 ].binding  = 0;
		attributeDescriptions[ 0 ].location = 0;
		attributeDescriptions[ 0 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[ 0 ].offset   = offsetof( vertex_2d_t, pos );

		attributeDescriptions[ 1 ].binding  = 0;
		attributeDescriptions[ 1 ].location = 1;
		attributeDescriptions[ 1 ].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[ 1 ].offset   = offsetof( vertex_2d_t, color );

		attributeDescriptions[ 2 ].binding  = 0;
		attributeDescriptions[ 2 ].location = 2;
		attributeDescriptions[ 2 ].format   = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[ 2 ].offset   = offsetof( vertex_2d_t, texCoord );

		return attributeDescriptions;
	}
	bool operator==( const vertex_2d_t& other ) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std
{
	template<  > struct hash< vertex_3d_t >
	{
		size_t operator(  )( vertex_3d_t const& vertex ) const
		{
			return  ( ( hash< glm::vec3 >(  )( vertex.pos ) ^
                   		( hash< glm::vec3 >(  )( vertex.color ) << 1 ) ) >> 1 ) ^
				( hash< glm::vec2 >(  )( vertex.texCoord ) << 1 );
		}
	};
	template<  > struct hash< vertex_2d_t >
	{
		size_t operator(  )( vertex_2d_t const& vertex ) const
		{
			return  ( ( hash< glm::vec2 >(  )( vertex.pos ) ^
                   		( hash< glm::vec3 >(  )( vertex.color ) << 1 ) ) >> 1 ) ^
				( hash< glm::vec2 >(  )( vertex.texCoord ) << 1 );
		}
	};
}

struct ubo_3d_t
{
	glm::mat4 model, view, proj;
};

struct ubo_2d_t
{
	glm::vec2 extent;
};

struct sprite_data_t
{
	VkBuffer vBuffer, iBuffer;
	VkDeviceMemory vBufferMem, iBufferMem, tImageMem;
	VkImage tImage;
	VkImageView tImageView;
	std::vector< VkBuffer > uBuffers;
	std::vector< VkDeviceMemory > uBuffersMem;
	std::vector< VkDescriptorSet > descSets;
	uint32_t vCount, iCount;
	bool noDraw = false;

	float posX, posY;
	void bind
		( VkCommandBuffer c, VkPipelineLayout p, uint32_t i )
		{
			VkBuffer vBuffers[  ] 		= { vBuffer };
			VkDeviceSize offsets[  ] 	= { 0 };
			vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
			vkCmdBindIndexBuffer( c, iBuffer, 0, VK_INDEX_TYPE_UINT32 );
			vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, p, 0, 1, &descSets[ i ], 0, NULL );
		}
	void draw
		( VkCommandBuffer c )
		{
			vkCmdDrawIndexed( c, iCount, 1, 0, 0, 0 );
		}
};

struct model_data_t
{
	VkBuffer vBuffer, iBuffer;
	VkDeviceMemory vBufferMem, iBufferMem, tImageMem;
	VkImage tImage;
	VkImageView tImageView;
	std::vector< VkBuffer > uBuffers;
	std::vector< VkDeviceMemory > uBuffersMem;
	std::vector< VkDescriptorSet > descSets;
	uint32_t vCount, iCount;

	float posX, posY, posZ;

	void bind
		( VkCommandBuffer c, VkPipelineLayout p, uint32_t i )
		{
			VkBuffer vBuffers[  ] 		= { vBuffer };
			VkDeviceSize offsets[  ] 	= { 0 };
			vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
			vkCmdBindIndexBuffer( c, iBuffer, 0, VK_INDEX_TYPE_UINT32 );
			vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, p, 0, 1, &descSets[ i ], 0, NULL );
		}
	void draw
		( VkCommandBuffer c )
		{
			vkCmdDrawIndexed( c, iCount, 1, 0, 0, 0 );
		}
};

#endif
