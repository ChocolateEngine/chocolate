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

