#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

#include "core.glsl"

layout(push_constant) uniform SelectParams {
	uint aRenderable;
	uint aViewport;
	uint aVertexCount;
	uint aDiffuse;
	vec4 aColor;
	vec2 aCursorPos;
}
push;

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
	vec2 inUV     = vert.aNormYZ_UV.zw;

	gl_Position = gViewports[ push.aViewport ].aProjView * inMatrix * vec4(inPos, 1.0);
	outTexCoord = inUV;
}

