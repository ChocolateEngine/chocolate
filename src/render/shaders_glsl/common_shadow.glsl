#ifndef CH_COMMON_SHADOW_GLSL
#define CH_COMMON_SHADOW_GLSL

//! Sampler array containing the shadowmaps in the scene.
layout(set = 0, binding = 0) uniform sampler2DShadow[] texShadowMaps;

//! 5x5 Gaussian Kernel.
/*!
    Only contains the values for positive coordinates.
*/
const float gGaussian5x5[3][3] = {
	{ 41.0 / 273, 26.0 / 273, 7.0 / 273 },
	{ 26.0 / 273, 16.0 / 273, 4.0 / 273 },
	{  7.0 / 273,  4.0 / 273, 1.0 / 273 }
};

//! Samples a shadow at a given point.
/*!
    Uses 5x5 Gaussian PCF.

    \param shadowMapID ID of the shadow map to sample.
    \param shadowCoord fragment position in shadow clip space.
    \return shadow term.
*/
float SampleShadowMapPCF( int shadowMapID, vec3 shadowCoord )
{
	ivec2 shadowMapSize = textureSize( texShadowMaps[ shadowMapID ], 0 );
	vec2 delta = 1 / vec2( shadowMapSize );

	float shadow = 0;

	for (int i = -2; i <= 2; i++)
	{
		for (int j = -2; j <= 2; j++)
		{
			vec3 offset = vec3( i * delta.x, j * delta.y, 0 );
			shadow += texture( texShadowMaps[ shadowMapID ], shadowCoord + offset ) * gGaussian5x5[ abs( i ) ][ abs( j ) ];
		}
	}

	return shadow;
}

#endif  // CH_COMMON_SHADOW_GLSL
