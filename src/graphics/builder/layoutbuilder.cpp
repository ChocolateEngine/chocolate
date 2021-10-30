/*
layoutbuilder.cpp ( Authored by p0lyh3dron )

*/
#include "layoutbuilder.h"
#include "../allocator.h"

#include <future>
/* Queues a descriptor set layout to be built.  */
void LayoutBuilder::Queue( VkDescriptorSetLayout *spLayout, Bindings sBindings )
{
	aQueue.push_back( { sBindings, spLayout } );
}
/* Asynchronously builds all descriptor set layouts in aQueue.  */
void LayoutBuilder::BuildLayouts( VkDescriptorSetLayout *spLayout )
{
	if ( spLayout )
		for ( auto&& layout : aQueue )
			if ( layout.apDestinationLayout == spLayout )
			{
				*spLayout = InitDescriptorSetLayout( layout.aBindings );
				return;
			}
	auto buildFunction = [ & ](  )
		{
			for ( auto&& layout : aQueue )
				*layout.apDestinationLayout = InitDescriptorSetLayout( layout.aBindings );
		};
	auto buildTask = std::async( std::launch::async, buildFunction );

	buildTask.wait(  );
}
