/* 
layoutbuilder.h ( Authored by p0lyh3dron )

Declares the layoutbuilder class which will
asynchronously build descriptor set layouts.
*/
#pragma once

#include <vector>
#include <vulkan/vulkan.hpp>

typedef std::vector< VkDescriptorSetLayoutBinding >   Bindings;

class QueuedLayout
{
public:
        Bindings		aBindings;
	VkDescriptorSetLayout	*apDestinationLayout;
};

class LayoutBuilder
{
	typedef std::vector< QueuedLayout >	LayoutQueue;
protected:
	LayoutQueue	aQueue;
public:
	/* Queues a descriptor set layout to be built.  */
	void	Queue( VkDescriptorSetLayout *spLayout, Bindings sBindings );
	/* Asynchronously builds all descriptor set layouts in aQueue.  */
	void	BuildLayouts( VkDescriptorSetLayout *spSingleLayout = NULL );
};
