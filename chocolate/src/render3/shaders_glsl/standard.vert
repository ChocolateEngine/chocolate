#version 450

// Allows the shader compiler to know how to handle buffer references
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec2 out_uv;


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


//push constants block
layout( push_constant ) uniform constants
{	
//	mat4          world_matrix;
//	mat4          view_matrix;
//	mat4          proj_matrix;
	mat4          proj_view_matrix;
	buffer_vertex vertex_address;  // u64 handle
} push;


// layout(location = 0) in vec3  pos;
// layout(location = 1) in float uv_x;
// layout(location = 2) in vec3  normal;
// layout(location = 3) in float uv_y;
// layout(location = 4) in vec4  color;


void main() 
{
	// load vertex data from device address
	vertex_t v  = push.vertex_address.vertices[ gl_VertexIndex ];

	// output data
	// gl_Position = push.view_proj_matrix * push.world_matrix * vec4( v.pos, 1.0f );
	//gl_Position = push.view_matrix * push.proj_matrix * vec4( pos.xyz, 1.0f );
	//gl_Position = push.proj_matrix * push.view_matrix * vec4( v.pos.xyz, 1.0f );
	gl_Position = push.proj_view_matrix * vec4( v.pos.xyz, 1.0f );

	// out_color = vec3( 1, 1, 1 );
	out_color   = v.color.xyz;
	// out_uv.x    = v.uv_x;
	// out_uv.y    = v.uv_y;
}

