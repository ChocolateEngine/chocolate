/*
descriptorcache.cpp ( Authored by p0lyh3dron )

Defines the descriptorcache declared in descriptorcache.h
*/
#include "../../../inc/core/renderer/descriptorcache.h"

/* Checks if a matching descriptor layout has already been allocated.  */
bool DescriptorCache::Exists(  )
{
	for ( const auto& layout : aLayouts )
	{ /* ... */ }
	return false;
}
/* Get the layout indexed by a call to Exists(  ).  */
VkDescriptorSetLayout DescriptorCache::GetLayout(  )
{
	return aLayoutReturn;
}
/* Adds an allocated layout to the list of layouts.  */
void DescriptorCache::AddLayout(  )
{
	/* ... */
}
