/*
descriptorcache.cpp ( Authored by p0lyh3dron )

Defines the descriptorcache declared in descriptorcache.h
*/
#include "descriptorcache.h"

/* Checks if a matching descriptor layout has already been allocated.  */
bool DescriptorCache::Exists( LayoutInfoList sInfo )
{
	uint32_t	binding;
        for ( auto&& layout : aLayouts )
		if ( layout.aList.size(  ) == sInfo.size(  ) )
		{
			for ( uint32_t i = 0; i < layout.aList.size(  ); ++i )
				if ( layout.aList[ i ].aType 		== sInfo[ i ].aType &&
				     layout.aList[ i ].aStageFlags 	== sInfo[ i ].aStageFlags )
				{
					if ( i + 1 == layout.aList.size(  ) )
						return aLayoutReturn = layout.aLayout;
				}
				else
					break;
	        }
	return false;
}
/* Get the layout indexed by a call to Exists(  ).  */
VkDescriptorSetLayout DescriptorCache::GetLayout(  )
{
	return aLayoutReturn;
}
/* Adds an allocated layout to the list of layouts.  */
void DescriptorCache::AddLayout( VkDescriptorSetLayout sLayout, LayoutInfoList sLayouts )
{
        aLayouts.push_back( { sLayouts, sLayout } );
}
