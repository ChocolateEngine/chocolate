#pragma once

// TODO: abstract this for the game code and other dlls so we don't need vulkan linked to everything
// only reason why it's in public and not closed off is because i need the vertex_3d_t stuff for the game code, along with the View class

#define GLM_ENABLE_EXPERIMENTAL

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <array>
#include <optional>
#include <vector>

#include "types/databuffer.hh"

#define KTX

#ifdef KTX
#include "ktx.h"
#include "ktxvulkan.h"
#endif

//#define MODEL_SET_PARAMETERS_TEMP( tImageView, uBuffers ) { { { VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, tImageView, srTextureSampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER } } }, { { { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uBuffers, sizeof( ubo_3d_t ) } } }

class QueueFamilyIndices
{
public:
	int 	aPresentFamily 	= -1;
	int	    aGraphicsFamily = -1;
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
        VkBuffer *apBuffer;
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
	alignas( 16 )glm::mat4 aMatrix;
	alignas( 16 )int       aTexIndex;
};

struct vertex_3d_t
{
	glm::vec3 pos{};
	glm::vec3 color{};
	glm::vec2 texCoord{};
	glm::vec3 normal{};
	
	bool operator==( const vertex_3d_t& other ) const
	{
		//return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
		return pos == other.pos && texCoord == other.texCoord && normal == other.normal;
	}

	static inline VkVertexInputBindingDescription GetBindingDesc()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof( vertex_3d_t );
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static inline std::array< VkVertexInputAttributeDescription, 4 > GetAttributeDesc()
	{
		std::array< VkVertexInputAttributeDescription, 4 >attributeDescriptions{  };
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof( vertex_3d_t, pos );

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof( vertex_3d_t, color );

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof( vertex_3d_t, texCoord );

		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[3].offset = offsetof( vertex_3d_t, normal );

		return attributeDescriptions;
	}
};

struct vertex_cube_3d_t
{
	glm::vec3 pos{};

	bool operator==( const vertex_cube_3d_t& other ) const
	{
		return pos == other.pos;
	}

	static inline VkVertexInputBindingDescription GetBindingDesc()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof( vertex_cube_3d_t );
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static inline std::array< VkVertexInputAttributeDescription, 1 > GetAttributeDesc()
	{
		std::array< VkVertexInputAttributeDescription, 1 >attributeDescriptions{  };
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof( vertex_cube_3d_t, pos );

		return attributeDescriptions;
	}
};

// TODO: have models made with the debug shader use this when i improve meshes
struct vertex_line_3d_t
{
	glm::vec3 pos{};
	glm::vec3 color{};
	
	bool operator==( const vertex_3d_t& other ) const
	{
		return pos == other.pos && color == other.color;
	}

	static inline VkVertexInputBindingDescription GetBindingDesc()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof( vertex_line_3d_t );
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static inline std::array< VkVertexInputAttributeDescription, 2 > GetAttributeDesc()
	{
		std::array< VkVertexInputAttributeDescription, 2 >attributeDescriptions{  };
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof( vertex_line_3d_t, pos );

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof( vertex_line_3d_t, color );

		return attributeDescriptions;
	}
};

struct vertex_2d_t
{
	glm::vec2 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	bool operator==( const vertex_2d_t& other ) const
	{
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}

	static inline VkVertexInputBindingDescription GetBindingDesc()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof( vertex_2d_t );
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static inline std::array< VkVertexInputAttributeDescription, 3 > GetAttributeDesc()
	{
		std::array< VkVertexInputAttributeDescription, 3 >attributeDescriptions{  };
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof( vertex_2d_t, pos );

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof( vertex_2d_t, color );

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof( vertex_2d_t, texCoord );

		return attributeDescriptions;
	}
};

namespace std	//	Black magic!! don't touch!!!!
{
	template<  > struct hash< vertex_3d_t >
	{
		size_t operator(  )( vertex_3d_t const& vertex ) const
		{
			return  ( ( hash< glm::vec3 >(  )( vertex.pos ) ^
               //    		( hash< glm::vec3 >(  )( vertex.color ) << 1 ) ) >> 1 ) ^
               //    		( hash< glm::vec3 >(  )( vertex.normal ) << 1 ) ) >> 1 ) ^
                   		( hash< glm::vec3 >(  )( vertex.normal ) << 1 ) ) ) ^
				( hash< glm::vec2 >(  )( vertex.texCoord ) << 1 );
		}
	};
	template<  > struct hash< vertex_cube_3d_t >
	{
		size_t operator(  )( vertex_cube_3d_t const& vertex ) const
		{
			return std::hash< glm::vec3 >{}( vertex.pos );
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

struct ubo_2d_t
{
	glm::vec2 extent;
};


class TextureDescriptor
{
public:
#ifdef KTX
	VkImage                         GetVkImage() { return texture.image; }
	VkDeviceMemory                  GetDeviceMemory() { return texture.deviceMemory; }
	uint32_t                        GetMipLevels() { return kTexture ? kTexture->numLevels : 0; }
#endif

	size_t                          aId;
	uint32_t                        aMipLevels;
	VkDeviceMemory                  aTextureImageMem;
	VkImage                         aTextureImage;
	VkImageView                     aTextureImageView;

#ifdef KTX
	// TEMP: just shove ktx data here for now
	ktxTexture* kTexture;
	ktxVulkanTexture texture;
#endif
};

class SampledImage {
public:
	VkDeviceMemory aMem;
	VkImage        aImg;
	VkImageView    aView;
};

// TODO: rename this stupid thing
using Texture = TextureDescriptor;

class UniformDescriptor
{	
public:
	UniformDescriptor() {}

	DataBuffer< VkBuffer >          aData;
	DataBuffer< VkDeviceMemory >    aMem;
	DataBuffer< VkDescriptorSet >   aSets;
};

// =================================================================
// View
// =================================================================

class View
{
public:

	View(int32_t x, int32_t y, uint32_t width, uint32_t height, float zNear, float zFar, float fieldOfView)
	{
		Set(x, y, width, height, zNear, zFar, fieldOfView);
	}

	void Set(int32_t x, int32_t y, uint32_t width, uint32_t height, float zNear, float zFar, float fieldOfView)
	{
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
		this->zNear = zNear;
		this->zFar = zFar;
		this->fieldOfView = fieldOfView;
		ComputeProjection();
	}

	void Set(View& view)
	{
		Set(view.x, view.y, view.width, view.height, view.zNear, view.zFar, view.fieldOfView);
		SetViewMatrix(view.viewMatrix);
	}

	void SetViewMatrix(const glm::mat4& viewMatrix)
	{
		this->viewMatrix = viewMatrix;
		this->projViewMatrix = projMatrix * viewMatrix;
	}

	const glm::mat4 &GetProjection() const { return projMatrix; }

	void ComputeProjection()
	{
		const float hAspect = (float)width / (float)height;
		const float vAspect = (float)height / (float)width;

		float V = 2.0f * atanf(tanf(glm::radians(fieldOfView) / 2.0f) * vAspect);

		projMatrix = glm::perspective(V, hAspect, zNear, zFar);

		projViewMatrix = projMatrix * viewMatrix;
	}

	int32_t x;
	int32_t y;
	uint32_t width;
	uint32_t height;

	float zNear;
	float zFar;
	float fieldOfView;

	glm::mat4 viewMatrix;
	glm::mat4 projMatrix;

	// view matrix times projection matrix
	glm::mat4 projViewMatrix;
};

