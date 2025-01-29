#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#define CH_VERT_SHADER 1

#include "core.glsl"


layout(push_constant) uniform Push
{
	uint aRenderable;
	uint aMaterial;
	uint aViewport;
	uint aDebugDraw;
} push;


// layout(location = 0) in vec3 inPosition;
// layout(location = 1) in vec3 inNormal;
// layout(location = 2) in vec2 inTexCoord;
// layout(location = 3) in vec4 inColor;


layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 outPosition;
layout(location = 2) out vec3 outPositionWorld;
layout(location = 3) out vec3 outNormal;
layout(location = 4) out vec3 outNormalWorld;
layout(location = 5) out vec3 outTangent;
// layout(location = 6) out mat4 outTBN;
// layout(location = 3) out float lightIntensity;

void main()
{
	Renderable_t renderable = gRenderables[ push.aRenderable ];
	uint         vertIndex  = gl_VertexIndex;

	// is this renderable using an index buffer?
	if ( renderable.aIndexBuffer != CH_INVALID_BUFFER )
		vertIndex = gIndexBuffers[ renderable.aIndexBuffer ].aIndex[ gl_VertexIndex ];

	VertexData_t vert = gVertexBuffers[ renderable.aVertexBuffer ].aVert[ vertIndex ];

	mat4 inMatrix = gModelMatrices[ push.aRenderable ];
	vec3 inPos    = vert.aPosNormX.xyz;
	vec3 inNorm   = vec3(vert.aPosNormX.w, vert.aNormYZ_UV.xy);
	vec2 inUV     = vert.aNormYZ_UV.zw;

	outPosition = inPos;

	outPositionWorld = (inMatrix * vec4(outPosition, 1.0)).rgb;
	// outMatrix = inMatrix;

	gl_Position = gViewports[ push.aViewport ].aProjView * inMatrix * vec4(inPos, 1.0);
	// gl_Position = projView[push.projView].projView * vec4(inPosition, 1.0);

	vec3 normalWorldSpace = normalize(mat3(inMatrix) * inNorm);

	if ( isnan( normalWorldSpace.x ) )
	{
		normalWorldSpace = vec3(0, 0, 0);
	}

	outNormal = inNorm;
	outNormalWorld = normalWorldSpace;

	// lightIntensity = max(dot(normalWorldSpace, DIRECTION_TO_LIGHT), 0.15);

	fragTexCoord = inUV;

    vec3 c1 = cross( inNorm, vec3(0.0, 0.0, 1.0) );
    vec3 c2 = cross( inNorm, vec3(0.0, 1.0, 0.0) );

    if (length(c1) > length(c2))
    {
        outTangent = c1;
    }
    else
    {
        outTangent = c2;
    }

    outTangent = normalize(outTangent);

	if ( isnan( outTangent.x ) )
	{
		outTangent = vec3(0, 0, 0);
	}


	//vec3 T = normalize(vec3(normalMat * ModelSpaceTangent));
	//vec3 B = normalize(vec3(normalMat * ModelSpaceBiTangent));
	//vec3 N = normalize(vec3(normalMat * ModelSpaceNormal));
	//mat3 tbn = mat3(T, B, N);
	//outTBN = tbn;
}

