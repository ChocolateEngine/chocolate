#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "core.glsl"

layout(push_constant) uniform Push
{
	int  aAlbedo;
	uint aRenderable;
	uint aViewport;
	mat4 aModel;
} push;

// layout(location = 0) in vec3 inPosition;
// layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 outTexCoord;

void main()
{
	// gl_Position = gViewport[push.aViewInfo].aProjView * push.aModel * vec4(inPosition, 1.0);
	// outTexCoord = inTexCoord;

	Renderable_t renderable = gRenderables[ push.aRenderable ];
	uint         vertIndex  = gl_VertexIndex;

	// is this renderable using an index buffer?
	if ( renderable.aIndexBuffer != CH_INVALID_BUFFER )
		vertIndex = gIndexBuffers[ renderable.aIndexBuffer ].aIndex[ gl_VertexIndex ];

	VertexData_t vert = gVertexBuffers[ renderable.aVertexBuffer ].aVert[ vertIndex ];

	mat4 inMatrix = gModelMatrices[ push.aRenderable ];
	vec3 inPos    = vert.aPosNormX.xyz;
	// vec3 inNorm   = vec3(vert.aPosNormX.w, vert.aNormYZ_UV.xy);
	vec2 inUV     = vert.aNormYZ_UV.zw;
	vec4 inColor  = vert.aColor;

	gl_Position = gViewports[ push.aViewport ].aProjView * inMatrix * vec4(inPos, 1.0);
	outTexCoord = inUV;
}

