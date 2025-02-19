#include "render.h"


// TODO: upload textures with vma


struct vk_sampler_settings_t
{
	VkFilter             filter;
	VkSamplerAddressMode address_mode;
	VkBool32             depth_compare;
};


inline bool operator==( const vk_sampler_settings_t& a, const vk_sampler_settings_t& b )
{
	if ( a.filter != b.filter )
		return false;

	if ( a.address_mode != b.address_mode )
		return false;

	if ( a.depth_compare != b.depth_compare )
		return false;

	return true;
}


// fun stuff
namespace std
{
template<>
struct hash< vk_sampler_settings_t >
{
	size_t operator()( vk_sampler_settings_t const& settings ) const
	{
		size_t value = ( hash< VkFilter >()( settings.filter ) );
		value ^= ( hash< VkSamplerAddressMode >()( settings.address_mode ) );
		value ^= ( hash< VkBool32 >()( settings.depth_compare ) );
		return value;
	}
};
}


ch_handle_ref_list_32< r_texture_h, vk_texture_t >     g_textures;
std::unordered_map< ch_string, r_texture_h >           g_texture_map;
u32                                                    g_texture_count;

// hash of the texture sampler options
std::unordered_map< vk_sampler_settings_t, VkSampler > g_samplers;

void                                                   vk_sampler_recreate();


 // a few gpu's out there have a max of 255 mipmap levels
// https://vulkan.gpuinfo.org/displaydevicelimit.php?name=maxSamplerLodBias
#define MAX_LOD_BIAS 255 


CONVAR_RANGE_INT_NAME_CMD( vk_miplod_min, "vk.texture.mipmap.lod.min", 0, 0, MAX_LOD_BIAS, 0, "Minimum Mipmap Level" )
{
	vk_sampler_recreate();
}


CONVAR_RANGE_INT_NAME_CMD( vk_miplod_max, "vk.texture.mipmap.lod.max", MAX_LOD_BIAS, 0, MAX_LOD_BIAS, 0, "Maximum Mipmap Level" )
{
	vk_sampler_recreate();
}


CONVAR_BOOL_NAME_CMD( vk_linear_mipmaps, "vk.texture.mipmap.linear", 1, CVARF_ARCHIVE, "Enable or Disable Linear Mipmaps" )
{
	vk_sampler_recreate();
}


CONVAR_RANGE_INT_NAME_CMD( vk_anisotropy, "vk.texture.anisotropy", 16, 0, 16, CVARF_ARCHIVE, "Anisotropic Filtering Setting" )
{
	vk_sampler_recreate();
}


CONVAR_RANGE_INT_NAME_CMD( vk_texture_filtering, "vk.texture.filtering", 1, 0, 2, CVARF_ARCHIVE,
	"Enable or Disable Linear Texture Filtering, 0 for off, 1 for default settings, 2 for forced on" )
{
	vk_sampler_recreate();
}


static void texture_free_data( vk_texture_t* texture )
{
	vk_queue_wait_graphics();

	if ( texture->view )
		vkDestroyImageView( g_vk_device, texture->view, nullptr );

	if ( texture->image )
		vkDestroyImage( g_vk_device, texture->image, nullptr );

	if ( texture->memory )
		vkFreeMemory( g_vk_device, texture->memory, nullptr );
}


r_texture_h texture_load( r_texture_h& handle, const char* path, vk_texture_load_info_t& load_info )
{
	if ( !path )
		return {};

	ch_string full_path;

	if ( ch_str_ends_with( path, ".ktx", 4 ) )
	{
		full_path = FileSys_FindFile( path );
	}
	else
	{
		char new_path[ 512 ]{};
		strcat( new_path, path );
		strcat( new_path, ".ktx" );
		full_path = FileSys_FindFile( new_path );
	}

	if ( !full_path.data )
	{
		Log_ErrorF( gLC_Render, "Failed to find texture: \"%s\"\n", path );
		return {};
	}

	// Check if we've loaded this already
	auto it = g_texture_map.find( full_path );
	if ( it != g_texture_map.end() )
	{
		handle = it->second;
		g_textures.ref_increment( handle );
		ch_str_free( full_path );
		return handle;
	}

	// allocate a new handle
	vk_texture_t* texture = nullptr;

	// Check for handle re-use
	if ( g_textures.handle_valid( handle ) )
	{
		// TODO: handle imgui textures
		texture = g_textures.get( handle );
		texture_free_data( texture );
		memset( &texture, 0, sizeof( vk_texture_t ) );
	}
	else if ( !g_textures.create( handle, &texture ) )
	{
		Log_Error( gLC_Render, "texture_load: Failed to allocate texture handle!\n" );
		handle = {};
		return {};
	}

	// load the texture
	if ( !ktx_load( full_path.data, texture ) )
	{
		g_textures.ref_decrement( handle );
		Log_ErrorF( gLC_Render, "texture_load: Failed to load texture: \"%s\"\n", full_path.data );

		handle                     = {};
		g_texture_map[ full_path ] = handle;
		return {};
	}

	g_texture_map[ full_path ] = handle;
	g_texture_count++;
	return handle;
}


void texture_free( r_texture_h& handle )
{
	vk_texture_t* texture = g_textures.get( handle );

	if ( !texture )
		return;
	
	if ( g_textures.ref_decrement_delay( handle ) > 0 )
		return;

	texture_free_data( texture );
	g_textures.ref_decrement_finish( handle );
	g_texture_count--;
}


bool texture_create_missing()
{
	return true;
}


int vk_device_get_anisotropic_filter_level()
{
	return glm::min( (float)vk_anisotropy, g_vk_device_properties.limits.maxSamplerAnisotropy );
}


// TODO: use a hash map for caches
VkSampler vk_sampler_get( VkFilter filter, VkSamplerAddressMode address_mode, VkBool32 depth_compare )
{
	VkSamplerCreateInfo samplerInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

	if ( vk_texture_filtering == 2 )
	{
		// Force Linear Filtering
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
	}
	else if ( vk_texture_filtering )
	{
		samplerInfo.magFilter = filter;
		samplerInfo.minFilter = filter;
	}
	else
	{
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
	}

	// generate a hash for these
	vk_sampler_settings_t settings{ samplerInfo.minFilter, address_mode, depth_compare };

	auto it = g_samplers.find( settings );
	if ( it != g_samplers.end() )
		return it->second;

	// This Sampler has not been created yet, so create a new texture sampler
	if ( g_samplers.size() == g_vk_device_properties.limits.maxSamplerAllocationCount )
	{
		Log_ErrorF( "Out of Texture Samplers!\n" );

		// just get the first texture sampler
		for ( auto& [ _settings, sampler ] : g_samplers )
			return sampler;

		Log_FatalF( "No Texture Samplers found, yet we are at the maxSamplerAllocationCount of %d?\n", g_vk_device_properties.limits.maxSamplerAllocationCount );
		return VK_NULL_HANDLE;
	}

	samplerInfo.addressModeU = address_mode;
	samplerInfo.addressModeV = address_mode;
	samplerInfo.addressModeW = address_mode;

	// make sure this device has anisotropic filtering and the user requests it
	if ( vk_anisotropy > 1 && g_vk_device_properties.limits.maxSamplerAnisotropy > 1 )
	{
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy    = vk_device_get_anisotropic_filter_level();
	}
	else
	{
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.maxAnisotropy    = 0;
	}

	samplerInfo.mipmapMode              = vk_linear_mipmaps ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
	samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable           = depth_compare;
	samplerInfo.compareOp               = VK_COMPARE_OP_LESS;
	samplerInfo.mipLodBias              = 0.f;
	samplerInfo.maxLod                  = std::max( 0, vk_miplod_max );
	samplerInfo.minLod                  = std::min( (float)std::max( 0, vk_miplod_min ), samplerInfo.maxLod );

	VkSampler& sampler                  = g_samplers[ settings ];

	vk_check( vkCreateSampler( g_vk_device, &samplerInfo, NULL, &sampler ), "Failed to create sampler!" );

	Log_DevF( gLC_Render, 1, "Created New Texture Sampler (%d / %d Max Samplers)\n", g_samplers.size(), g_vk_device_properties.limits.maxSamplerAllocationCount );

	return sampler;
}


void vk_sampler_destroy_all()
{
	for ( auto& [ settings, sampler ] : g_samplers )
		vkDestroySampler( g_vk_device, sampler, nullptr );

	g_samplers.clear();
	Log_DevF( gLC_Render, 1, "Cleared Texture Samplers\n" );
}


void vk_sampler_recreate()
{
	// is the renderer running?
	if ( !g_vk_instance )
		return;

	vk_queue_wait_graphics();
	vk_sampler_destroy_all();
}

