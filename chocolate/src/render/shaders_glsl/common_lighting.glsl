#ifndef CH_COMMON_LIGHTING_GLSL
#define CH_COMMON_LIGHTING_GLSL

#define CH_FRAG_SHADER 1

#include "core.glsl"
#include "common_shadow.glsl"

const mat4 gBiasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

const float gAmbient = 0.0f;

float LinearizeDepth( float sNearZ, float sFarZ, float sDepth )
{
	float n = sNearZ;
	float f = sFarZ;
	float z = sDepth;
	return (2.0 * n) / (f + n - z * (f - n));
}


float GetLuminance( vec3 color )
{
	return dot( color, vec3( 0.2125f, 0.7154f, 0.0721f ) );
}


float GetActualLuminance( vec3 color )
{
    return dot( color, vec3(1.0 / 3.0) );
}


vec4 AddLighting( vec4 albedo, vec3 inPositionWorld, vec3 inNormalWorld )
{
	vec4 outColor = vec4( 0.f, 0.f, 0.f, 0.f );

	if ( gCore.aNumLights[ CH_LIGHT_TYPE_WORLD ] == 0 )
		outColor = albedo;

    #if 01
	// ----------------------------------------------------------------------------
	// Add World Lights

	// for ( int i = 0; i < gCore.aNumLightWorld; i++ )
	for ( int i = 0; i < gCore.aNumLights[ CH_LIGHT_TYPE_WORLD ]; i++ )
	{
		if ( gCore.aLightWorld[ i ].aColor.w == 0.f )
			continue;

		// Diffuse part
		float intensity = max( dot( inNormalWorld, gCore.aLightWorld[ i ].aDir ), 0.f );

		vec3 diff;

		// if ( push.dbgShowDiffuse )
		//	outColor.rgb += aLightWorld[ i ].aColor * vec3( max( intensity, 0.15 ) );
		// else
			diff = gCore.aLightWorld[ i ].aColor.rgb * gCore.aLightWorld[ i ].aColor.a * max( intensity, 0.15 ) * albedo.rgb;

		// shadow
		// if ( gLightsWorld[ i ].aShadow != -1 )
		// {
		// 	mat4 depthBiasMVP = gBiasMat * gLightsWorld[ i ].aProjView;
		// 	// mat4 depthBiasMVP = gBiasMat * gViewport[ 0 ].aProjView;
		// 	vec4 shadowCoord = depthBiasMVP * vec4( inPositionWorld, 1.0 );
// 
		// 	// float shadow = SampleShadowMapPCF( gLightsWorld[ i ].aShadow, shadowCoord.xyz / shadowCoord.w );
		// 	float shadow = SampleShadowMapPCF( 0, shadowCoord.xyz / shadowCoord.w );
		// 	diff *= shadow;
		// 	// diff = shadow;
		// }

		outColor.rgb += diff;
	}
#endif

#if 1
	// ----------------------------------------------------------------------------
	// Add Point Lights

	for ( int i = 0; i < gCore.aNumLights[ CH_LIGHT_TYPE_POINT ]; i++ )
	{
		if ( gCore.aLightPoint[ i ].aColor.w == 0.f )
			continue;

		// Vector to light
		vec3 lightDir = gCore.aLightPoint[ i ].aPos - inPositionWorld;

		// Distance from light to fragment position
		float dist = length( lightDir );

		//if(dist < ubo.lights[i].radius)
		{
			lightDir = normalize(lightDir);

			// Attenuation
			float atten = gCore.aLightPoint[ i ].aRadius / (pow(dist, 2.0) + 1.0);

			// Diffuse part
			vec3 vertNormal = normalize( inNormalWorld );
			float NdotL = max( 0.0, dot(vertNormal, lightDir) );
			vec3 diff = gCore.aLightPoint[ i ].aColor.rgb * gCore.aLightPoint[ i ].aColor.a * albedo.rgb * NdotL * atten;
			// vec3 diff = gCore.aLightPoint[ i ].aColor.rgb * gCore.aLightPoint[ i ].aColor.a * NdotL * atten;

			// vec3 lightColor = gCore.aLightPoint[ i ].aColor.rgb;
			// float albedoLuminance = GetLuminance( albedo.rgb );
			// lightColor = mix( lightColor, albedo.rgb * lightColor, albedoLuminance );
// 
			// vec3 diff = lightColor * gCore.aLightPoint[ i ].aColor.a * NdotL * atten;

			outColor.rgb += diff;
		}
	}

	// ----------------------------------------------------------------------------
	// Add Cone Lights

	#define CONSTANT 1
	#define LINEAR 1
	#define QUADRATIC 1

	for ( int i = 0; i < gCore.aNumLights[ CH_LIGHT_TYPE_CONE ]; i++ )
	{
		if ( gCore.aLightCone[ i ].aColor.w == 0.f )
			continue;

		// Vector to light
		vec3 lightDir = gCore.aLightCone[ i ].aPos - inPositionWorld;
		lightDir = normalize(lightDir);

		// Distance from light to fragment position
		float dist = length( lightDir );

		float theta = dot( lightDir, normalize( gCore.aLightCone[ i ].aDir.xyz ));

		// Inner Cone FOV - Outer Cone FOV
		float epsilon   = gCore.aLightCone[ i ].aFov.x - gCore.aLightCone[ i ].aFov.y;
		// float intensity = clamp( (theta - gCore.aLightCone[ i ].aFov.y) / epsilon, 0.0, 1.0 );
		float intensity = clamp( (gCore.aLightCone[ i ].aFov.y - theta) / epsilon, 0.0, 1.0 );

		// attenuation
		float atten = 1.0 / (pow(dist, 1.0) + 1.0);
		// float atten = 1.0 / (CONSTANT + LINEAR * dist + QUADRATIC * (dist * dist));
		// float atten = (CONSTANT + LINEAR * dist + QUADRATIC * (dist * dist));

		// vec3 diff = gCore.aLightCone[ i ].aColor.rgb * gCore.aLightCone[ i ].aColor.a * albedo.rgb * intensity * atten;
		// vec3 diff = gCore.aLightCone[ i ].aColor.rgb * gCore.aLightCone[ i ].aColor.a * intensity * atten;

		vec3 lightColor = gCore.aLightCone[ i ].aColor.rgb;
		float albedoLuminance = max( 0.4f, GetLuminance( albedo.rgb ) );
		lightColor = mix( lightColor, albedo.rgb * lightColor, albedoLuminance );

		vec3 diff = albedo.rgb * lightColor * gCore.aLightCone[ i ].aColor.a * intensity * atten;

		// shadow
		if ( gCore.aLightCone[ i ].aShadow != -1 )
		{
			mat4 depthBiasMVP = gBiasMat * gCore.aLightCone[ i ].aProjView;
			vec4 shadowCoord = depthBiasMVP * vec4( inPositionWorld, 1.0 );
		
			// TODO: this could go out of bounds and lose the device
			// maybe add a check here to make sure we don't go out of bounds?
			float shadow = SampleShadowMapPCF( gCore.aLightCone[ i ].aShadow, shadowCoord.xyz / shadowCoord.w );
			diff *= shadow;
		}

		outColor.rgb += diff;
	}
#endif

	return outColor;
}

#endif