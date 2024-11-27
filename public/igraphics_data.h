#pragma once

//
// Handle's all Graphics Data, such as Textures, Materials, and Models
// Does not upload anything to the GPU, that is the job of the Renderer
// This is just a data manager, useful for servers
//


#include "core/core.h"
#include "system.h"


// ------------------------------------------------------------------------------------


enum class GraphicsFmt
{
	INVALID,

	// -------------------------
	// 8 bit depth

	R8_SRGB,
	R8_SINT,
	R8_UINT,

	RG88_SRGB,
	RG88_SINT,
	RG88_UINT,

	RGB888_SRGB,
	RGB888_SINT,
	RGB888_UINT,

	RGBA8888_SRGB,
	RGBA8888_SNORM,
	RGBA8888_UNORM,

	// -------------------------

	BGRA8888_SRGB,
	BGRA8888_SNORM,
	BGRA8888_UNORM,

	// -------------------------
	// 16 bit depth

	R16_SFLOAT,
	R16_SINT,
	R16_UINT,

	RG1616_SFLOAT,
	RG1616_SINT,
	RG1616_UINT,

	RGB161616_SFLOAT,
	RGB161616_SINT,
	RGB161616_UINT,

	RGBA16161616_SFLOAT,
	RGBA16161616_SINT,
	RGBA16161616_UINT,

	// -------------------------
	// 32 bit depth

	R32_SFLOAT,
	R32_SINT,
	R32_UINT,

	RG3232_SFLOAT,
	RG3232_SINT,
	RG3232_UINT,

	RGB323232_SFLOAT,
	RGB323232_SINT,
	RGB323232_UINT,

	RGBA32323232_SFLOAT,
	RGBA32323232_SINT,
	RGBA32323232_UINT,

	// -------------------------
	// GPU Compressed Formats

	BC1_RGB_UNORM_BLOCK,
	BC1_RGB_SRGB_BLOCK,
	BC1_RGBA_UNORM_BLOCK,
	BC1_RGBA_SRGB_BLOCK,

	BC2_UNORM_BLOCK,
	BC2_SRGB_BLOCK,

	BC3_UNORM_BLOCK,
	BC3_SRGB_BLOCK,

	BC4_UNORM_BLOCK,
	BC4_SNORM_BLOCK,

	BC5_UNORM_BLOCK,
	BC5_SNORM_BLOCK,

	BC6H_UFLOAT_BLOCK,
	BC6H_SFLOAT_BLOCK,

	BC7_UNORM_BLOCK,
	BC7_SRGB_BLOCK,

	// -------------------------
	// Other Formats

	D16_UNORM,
	D32_SFLOAT,
	D32_SFLOAT_S8_UINT,
};


constexpr glm::vec4 gFrustumFaceData[ 8u ] = {
	// Near Face
	{ 1, 1, -1, 1.f },
	{ -1, 1, -1, 1.f },
	{ 1, -1, -1, 1.f },
	{ -1, -1, -1, 1.f },

	// Far Face
	{ 1, 1, 1, 1.f },
	{ -1, 1, 1, 1.f },
	{ 1, -1, 1, 1.f },
	{ -1, -1, 1, 1.f },
};


enum EFrustum
{
	EFrustum_Top,
	EFrustum_Bottom,
	EFrustum_Right,
	EFrustum_Left,
	EFrustum_Near,
	EFrustum_Far,
	EFrustum_Count,
};


// ------------------------------------------------------------------------------------


struct texture_load_info_t
{

};


struct material_create_info_t
{
};


struct texture_data_t
{
	GraphicsFmt format;
	glm::vec3   size;
	u32         mip_levels;
	u32         array_layers;
	u32         samples;
	u32         data_size;
	u8*         data;
};


struct frustum_t
{
	glm::vec4 planes[ EFrustum_Count ];
	glm::vec3 points[ 8 ];  // 4 Positions for the near plane corners, last 4 are the far plane corner positions

	// https://iquilezles.org/articles/frustumcorrect/
	bool      is_box_visible( const glm::vec3& sMin, const glm::vec3& sMax ) const
	{
		PROF_SCOPE();

		// Check Box Outside/Inside of Frustum
		for ( int i = 0; i < EFrustum_Count; i++ )
		{
			if ( ( glm::dot( planes[ i ], glm::vec4( sMin.x, sMin.y, sMin.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMax.x, sMin.y, sMin.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMin.x, sMax.y, sMin.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMax.x, sMax.y, sMin.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMin.x, sMin.y, sMax.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMax.x, sMin.y, sMax.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMin.x, sMax.y, sMax.z, 1.0f ) ) < 0.0 ) &&
			     ( glm::dot( planes[ i ], glm::vec4( sMax.x, sMax.y, sMax.z, 1.0f ) ) < 0.0 ) )
			{
				return false;
			}
		}

		// Check Frustum Outside/Inside Box
		for ( int j = 0; j < 3; j++ )
		{
			int out = 0;
			for ( int i = 0; i < 8; i++ )
			{
				if ( points[ i ][ j ] > sMax[ j ] )
					out++;
			}

			if ( out == 8 )
				return false;

			out = 0;
			for ( int i = 0; i < 8; i++ )
			{
				if ( points[ i ][ j ] < sMin[ j ] )
					out++;
			}

			if ( out == 8 )
				return false;
		}

		return true;
	}

	void get_aabb( glm::vec3& srMin, glm::vec3& srMax ) const
	{
		srMin = points[ 0 ];
		srMax = points[ 0 ];

		for ( int i = 1; i < 8; i++ )
		{
			srMin = glm::min( srMin, points[ i ] );
			srMax = glm::max( srMax, points[ i ] );
		}
	}

	bool is_visible( const frustum_t& other )
	{
		// Just do a aabb test
		glm::vec3 min;
		glm::vec3 max;

		other.get_aabb( min, max );

		return is_box_visible( min, max );
	}
};


struct model_material_t
{
	u32        vertex_offset;
	u32        vertex_count;

	u32        index_offset;
	u32        index_count;

	ch_handle_t material;
};


struct model_t
{
	u32               ref_count;

	

	model_material_t* materials;
	u32               material_count;  // could this be u16? is there ever a need for more than 65535 materials on a model?
};


// ------------------------------------------------------------------------------------


class IGraphicsData : public ISystem
{
   public:
	// --------------------------------------------------------------------------------------------
	// General
	// --------------------------------------------------------------------------------------------

	// Creates a new frustum
	virtual frustum_t      frustum_create( const glm::mat4& proj_view )                                                                     = 0;
	
	// --------------------------------------------------------------------------------------------
	// Textures
	// --------------------------------------------------------------------------------------------

	// Loads a texture from disk
	virtual ch_handle_t*    texture_load( const char** paths, const texture_load_info_t* create_infos, size_t count = 1 )                = 0;
	virtual ch_handle_t*    texture_load( const char** paths, s64* pathLens, const texture_load_info_t* create_infos, size_t count = 1 ) = 0;
	//virtual ch_handle_t     texture_load( ch_string* paths, const texture_load_info_t* create_infos, size_t count = 1 )   = 0;
	virtual ch_handle_t     texture_load( const char* path, s64 pathLen, const texture_load_info_t& create_info )                        = 0;

	virtual void           texture_free( ch_handle_t* texture, size_t count = 1 )                                                            = 0;
	virtual texture_data_t texture_get_data( ch_handle_t texture )                                                                           = 0;

	// --------------------------------------------------------------------------------------------
	// Materials
	// --------------------------------------------------------------------------------------------
	
	// --------------------------------------------------------------------------------------------
	// Models
	// --------------------------------------------------------------------------------------------
	
	// --------------------------------------------------------------------------------------------
	// Model Building - useful for importing custom models, like .obj or .gltf files
	// --------------------------------------------------------------------------------------------
};


#define CH_GRAPHICS_DATA         "ChocolateGraphicsData"
#define CH_GRAPHICS_DATA_VERSION 1

