#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#define CH_FRAG_SHADER 1

#include "core.glsl"
#include "common_shadow.glsl"
#include "common_lighting.glsl"

#define VIEWPORT 0
#define MATERIAL 1

layout(push_constant) uniform Push
{
	uint aRenderable;
	uint aMaterial;
	uint aViewport;
	uint aDebugDraw;
} push;

// Material Info
layout(set = CH_DESC_SET_PER_SHADER, binding = 0) buffer readonly Buffer_Material
{
    int albedo;
    int ao;
    int emissive;
    int normalMap;

    float aoPower;
    float emissivePower;

	bool aAlphaTest; 
	bool useNormalMap; 
} materials[];

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 inPosition;
layout(location = 2) in vec3 inPositionWorld;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inNormalWorld;
layout(location = 5) in vec3 inTangent;
// layout(location = 6) in mat4 inTBN;

layout(location = 0) out vec4 outColor;

#define mat materials[ push.aMaterial ]
// #define mat materials[0]

#define texDiffuse  texSamplers[ mat.albedo ]
#define texAO       texSamplers[ mat.ao ]
#define texEmissive texSamplers[ mat.emissive ]


void main()
{
	// SurfaceDraw_t surface    = gSurfaceDraws[ push.aSurface ];
	Renderable_t renderable = gRenderables[ push.aRenderable ];

    // outColor = vec4( lightIntensity * vec3(texture(texDiffuse, fragTexCoord)), 1 );
    vec4 albedo = texture( texDiffuse, fragTexCoord );

	// TODO: Alpha Reference
	if ( mat.aAlphaTest && albedo.a < 0.5f )
		discard;

    // outColor = vec4( lightIntensity * vec3(texture(texDiffuse, fragTexCoord)), 1 );

	// outColor = albedo;
	// return;

	// Calculate normal in tangent space
	// vec3 vertNormal = normalize( inNormal );
	// vec3 T = inTangent;
	// vec3 B = cross(vertNormal, T);
	// mat3 TBN = mat3(T, B, vertNormal);
	// // vec3 tnorm = TBN * normalize(texture(samplerNormalMap, inUV).xyz * 2.0 - vec3(1.0));
	// vec3 tnorm = TBN * vec3(1.0, 1.0, 1.0);
	// // outNormal = vec4( tnorm, 1.0 );
	// // outNormal = vec4( inTangent, 1.0 );
	// // outNormal = vec4( inNormal, 1.0 );

	vec3 normalWorld = inNormalWorld;
	//if ( mat.useNormalMap )
	////{
	//	vec3 biNormal = cross( inNormal, inTangent );
//
	//	vec4 normalMap = texture( texSamplers[ mat.normalMap ], fragTexCoord );
    //	vec3 normalRaw = normalize( normalMap.rgb * 2.0 - 1.0 );
    //	normalWorld = normalize( mat3( inTangent, biNormal, inNormal ) * normalRaw );
	//	normalWorld = normalize( mat3( inMatrix ) * normalWorld );
	//}
	//else
	//{
    //	normalWorld = normalize( inNormalWorld );
	//}

	if ( push.aDebugDraw == 1 )
		albedo = vec4(1, 1, 1, 1);

	if ( push.aDebugDraw > 1 && push.aDebugDraw < 5 )
	{
		if ( push.aDebugDraw == 2 )
			outColor = vec4(inNormalWorld, 1);

		if ( push.aDebugDraw == 3 )
			outColor = vec4(inNormal, 1);

		if ( push.aDebugDraw == 4 )
			outColor = vec4(inTangent, 1);

		return;
	}

	outColor = vec4(0, 0, 0, albedo.a);
	
	outColor += AddLighting( albedo, inPositionWorld, normalWorld );

	// ----------------------------------------------------------------------------

	// add ambient occlusion (only one channel is needed here, so just use red)
    if ( mat.aoPower > 0.0 )
		outColor *= mix( 1, texture(texAO, fragTexCoord).r, mat.aoPower );

	// add emission
    if ( mat.emissivePower > 0.0 )
	    outColor.rgb += mix( vec3(0, 0, 0), texture(texEmissive, fragTexCoord).rgb, mat.emissivePower );
}

// look at this later for cubemaps:
// https://forum.processing.org/two/discussion/27871/use-samplercube-and-sampler2d-in-a-single-glsl-shader.html

