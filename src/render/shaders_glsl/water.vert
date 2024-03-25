#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#define CH_VERT_SHADER 1

#include "core.glsl"


layout(push_constant) uniform Push
{
    vec3 aColor;
	uint aRenderable;
	uint aViewport;
} push;


// layout(location = 0) in vec3 inPosition;
// layout(location = 1) in vec3 inNormal;
// layout(location = 2) in vec2 inTexCoord;
// layout(location = 3) in vec4 inColor;


//layout(location = 0) out vec2 fragTexCoord;
//layout(location = 1) out vec3 outPosition;
// layout(location = 3) out float lightIntensity;

layout(location = 0) out vec3 outPositionWorld;
layout(location = 1) out vec3 outNormalWorld;

void main()
{
	// TODO: for morph targets:
	// https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-4-animation-dawn-demo
	// float4 position = (1.0f - interp) * vertexIn.prevPositionKey + interp * vertexIn.nextPositionKey;

	// // vertexIn.positionDiffN = position morph target N - neutralPosition
	// float4 position = neutralPosition;
	// position += weight0 * vertexIn.positionDiff0;
	// position += weight1 * vertexIn.positionDiff1;
	// position += weight2 * vertexIn.positionDiff2;
	// etc.

	// or, for each blend shape
	// for ( int i = 0; i < ubo.morphCount; i++ )

	Renderable_t renderable = gRenderables[ push.aRenderable ];
	uint         vertIndex  = gl_VertexIndex;

	// is this renderable using an index buffer?
	if ( renderable.aIndexBuffer != 4294967295 )
		vertIndex = gIndexBuffers[ renderable.aIndexBuffer ].aIndex[ gl_VertexIndex ];

	VertexData_t vert = gVertexBuffers[ renderable.aVertexBuffer ].aVert[ vertIndex ];

	mat4 inMatrix = gModelMatrices[ push.aRenderable ];
	vec3 inPos    = vert.aPosNormX.xyz;
	vec3 inNorm   = vec3(vert.aPosNormX.w, vert.aNormYZ_UV.xy);
	vec2 inUV     = vert.aNormYZ_UV.zw;

	//outPosition = inPos;

	outPositionWorld = (inMatrix * vec4(inPos, 1.0)).rgb;

	gl_Position = gViewports[ push.aViewport ].aProjView * inMatrix * vec4(inPos, 1.0);
	// gl_Position = projView[push.projView].projView * vec4(inPosition, 1.0);

	vec3 normalWorldSpace = normalize(mat3(inMatrix) * inNorm);

	if ( isnan( normalWorldSpace.x ) )
	{
		normalWorldSpace = vec3(0, 0, 0);
	}

	outNormalWorld = normalWorldSpace;

	// lightIntensity = max(dot(normalWorldSpace, DIRECTION_TO_LIGHT), 0.15);

	//fragTexCoord = inUV;
}

