/*
descriptorcache.h ( Authored by p0lyh3dron )

Declares the descriptor cache, which aims
to save on resources when new models require 
new layouts for materials.
*/
#pragma once

#include <vulkan/vulkan.hpp>
#include <map>
#include <string>
#include <unordered_map>

class LayoutInfo
{
public:
	VkDescriptorType	aType;
	VkShaderStageFlags	aStageFlags;
};

typedef std::vector< LayoutInfo >	LayoutInfoList;

class AllocatedLayout
{
public:
	LayoutInfoList		aList;
	VkDescriptorSetLayout	aLayout;
};

class DescriptorCache
{
	typedef std::vector< AllocatedLayout >     AllocatedLayouts;
protected:
	AllocatedLayouts	aLayouts{  };
	VkDescriptorSetLayout	aLayoutReturn;
public:
	/* Checks if a matching descriptor layout has already been allocated.  */
	bool			Exists( LayoutInfoList sLayouts );
	/* Get the layout indexed by a call to Exists(  ).  */
	VkDescriptorSetLayout	GetLayout(  );
	/* Adds an allocated layout to the list of layouts.  */
	void			AddLayout( VkDescriptorSetLayout sLayout, LayoutInfoList sLayouts );
};
