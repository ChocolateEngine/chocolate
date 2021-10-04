/*
shaders.h ( Authored by p0lyhdron )

Defines shader effect stuff.
*/
#include "../../../inc/core/renderer/shaders.h"

void Basic3D::Init(  )
{
	aModules.Allocate( 2 );
	aModules[ 0 ] = CreateShaderModule( ReadFile( pVShader ) );
	aModules[ 1 ] = CreateShaderModule( ReadFile( pFShader ) );
	aSetLayouts.Allocate( 2 );
	gLayoutBuilder.Queue( &aSetLayouts[ 0 ], { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
									     VK_SHADER_STAGE_FRAGMENT_BIT, NULL ) } } );
	gLayoutBuilder.Queue( &aSetLayouts[ 1 ], { { DescriptorLayoutBinding( VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
									    VK_SHADER_STAGE_VERTEX_BIT, NULL ) } } );
	gPipelineBuilder.Queue( &aPipeline, &aPipelineLayout, &aSetLayouts, pVShader, pFShader, 0 );
}
