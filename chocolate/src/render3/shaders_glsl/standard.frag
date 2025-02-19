#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

// Allows the shader compiler to know how to handle buffer references
#extension GL_EXT_buffer_reference : require

// shader input
layout (location = 0) in vec3 in_color;
layout (location = 1) in vec2 in_uv;

// output write
layout (location = 0) out vec4 out_frag_color;

// test textures
#define CH_BINDING_TEXTURES 0

layout(set = 0, binding = CH_BINDING_TEXTURES) uniform sampler2D[]       g_tex;
// layout(set = 0, binding = CH_BINDING_TEXTURES) uniform sampler2DArray[]  g_tex_array;
// layout(set = 0, binding = CH_BINDING_TEXTURES) uniform sampler2DShadow[] g_tex_shadow;
// layout(set = 0, binding = CH_BINDING_TEXTURES) uniform samplerCube[]     g_tex_cube;

struct vertex_t
{
	vec3  pos;
	float uv_x;
	vec3  normal;
	float uv_y;
	vec4  color;
};

// "buffer_reference" means that the shader knows this is used from a buffer address
layout( buffer_reference, std430 ) readonly buffer buffer_vertex
{ 
	vertex_t vertices[];
};

layout( push_constant ) uniform constants
{
	mat4          proj_view_matrix;
	buffer_vertex vertex_address;  // u64 handle
	int           diffuse;
} push;


void main() 
{
	out_frag_color = texture( g_tex[ push.diffuse ], in_uv );
	// out_frag_color = vec4( in_color, 1.0f );
}

