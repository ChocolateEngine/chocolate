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

class DescriptorCache
{
	typedef std::vector< VkDescriptorSetLayout >	AllocatedLayouts;
protected:
	AllocatedLayouts	aLayouts;
	VkDescriptorSetLayout	aLayoutReturn;
public:
	/* Checks if a matching descriptor layout has already been allocated.  */
	bool			Exists(  );
	/* Get the layout indexed by a call to Exists(  ).  */
	VkDescriptorSetLayout	GetLayout(  );
	/* Adds an allocated layout to the list of layouts.  */
	void			AddLayout(  );
};
