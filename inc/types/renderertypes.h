#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <array>
#include <optional>
#include <vector>

class QueueFamilyIndices
{
private:
	int 	aPresentFamily 	= -1;
	int	aGraphicsFamily = -1;
public:
	/* Function that returns true if there is a valid queue family available.  */
	bool    Complete(  ){ return ( aPresentFamily > -1 ) && ( aGraphicsFamily > -1 ); }
};

class SwapChainSupportInfo
{
private:
	typedef std::vector< VkSurfaceFormatKHR > 	SurfaceFormats;
	typedef std::vector< VkPresentModeKHR >		PresentModes;
public:
	VkSurfaceCapabilitiesKHR        aCapabilities;
        SurfaceFormats 			aFormats;
	PresentModes 			aPresentModes;
};

struct combined_buffer_info_t
{
	VkDescriptorType type;
	std::vector< VkBuffer >& buffer;
	unsigned int range;
};

struct combined_image_info_t
{
        VkImageLayout imageLayout;
	VkImageView imageView;
	VkSampler textureSampler;
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

namespace std	//	Black magic!! don't touch!!!!
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

class SpriteData
{
	typedef std::vector< VkDescriptorSet > DescriptorSetList;
	
	VkBuffer 		aVertexBuffer;
	VkBuffer		aIndexBuffer;
	VkDeviceMemory 		aVertexBufferMem;
        VkDeviceMemory		aIndexBufferMem;
	VkDeviceMemory		aTextureImageMem;
	VkImage 		aTextureImage;
	VkImageView 		aTextureImageView;
	DescriptorSetList 	aDescriptorSets;
	uint32_t 		aVertexCount;
	uint32_t		aIndexCount;
public:
	bool 			aNoDraw = false;
	float 			aPosX 	= 0.0f;
	float			aPosY 	= 0.0f;
	float 			aWidth 	= 0.5f;
	float			aHeight = 0.5f;

	/* Binds the sprite data to the GPU to be submitted for rendering.  */
	void 		Bind( VkCommandBuffer c, VkPipelineLayout p, uint32_t i )
		{
			VkBuffer 	vBuffers[  ]    = { aVertexBuffer };
			VkDeviceSize 	offsets[  ] 	= { 0 };
		        vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, p, 0, 1, &aDescriptorSets[ i ], 0, NULL );
			vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
			vkCmdBindIndexBuffer( c, aIndexBuffer, 0, VK_INDEX_TYPE_UINT32 );
		}
	/* Draw the sprite to the screen.  */
	void 		Draw( VkCommandBuffer c ){ vkCmdDrawIndexed( c, aIndexCount, 1, 0, 0, 0 ); }
};

struct ModelData
{
	typedef std::vector< VkBuffer >		BufferSet;
	typedef std::vector< VkDeviceMemory >	MemorySet;
	typedef std::vector< VkDescriptorSet > 	DescriptorSetList;
	
        VkBuffer 		aVertexBuffer;
	VkBuffer		aIndexBuffer;
	VkDeviceMemory 		aVertexBufferMem;
        VkDeviceMemory		aIndexBufferMem;
	VkDeviceMemory		aTextureImageMem;
	VkImage 		aTextureImage;
	VkImageView 		aTextureImageView;
	BufferSet		aUniformBuffers;
	MemorySet		aUniformBuffersMem;
	DescriptorSetList 	aDescriptorSets;
	uint32_t 		aVertexCount;
	uint32_t		aIndexCount;
public:
	bool 			aNoDraw = false;
	float 			aPosX 	= 0.0f;
	float			aPosY 	= 0.0f;
	float			aPosZ	= 0.0f;
	float 			aWidth 	= 0.5f;
	float			aHeight = 0.5f;

	/* Binds the model data to the GPU to be rendered later.  */
	void 		Bind( VkCommandBuffer c, VkPipelineLayout p, uint32_t i )
		{
			VkBuffer 	vBuffers[  ] 	= { aVertexBuffer };
			VkDeviceSize 	offsets[  ] 	= { 0 };
			vkCmdBindVertexBuffers( c, 0, 1, vBuffers, offsets );
			vkCmdBindIndexBuffer( c, aIndexBuffer, 0, VK_INDEX_TYPE_UINT32 );
			vkCmdBindDescriptorSets( c, VK_PIPELINE_BIND_POINT_GRAPHICS, p, 0, 1, &aDescriptorSets[ i ], 0, NULL );
		}
	/* Draws the model to the screen.  */
	void 		Draw( VkCommandBuffer c ){ vkCmdDrawIndexed( c, aIndexCount, 1, 0, 0, 0 ); }
};

