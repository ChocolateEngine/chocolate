#include "graphics_int.h"
#include "render/irender.h"

#include "core/json5.h"
#include "core/string.hpp"

#include <unordered_set>
#include <variant>

#if __unix__
#include <limits.h>
#endif /* __unix__  */

extern IRender* render;

using MaterialVar2 = std::variant<
  ChHandle_t,
  float,
  int,
  glm::vec2,
  glm::vec3,
  glm::vec4 >;

struct MaterialVar
{
	MaterialVar( const std::string_view name, EMatVar type ) :
		aNameLen( name.size() ), aType( type )
	{
		apName = Util_AllocString( name.data(), name.size() );
	}

	MaterialVar( const std::string_view name, Handle data ) :
		MaterialVar( name, EMatVar_Texture )
	{ aDataTexture = data; }

	MaterialVar( const std::string_view name, float data ) :
		MaterialVar( name, EMatVar_Float )
	{ aDataFloat = data; }

	MaterialVar( const std::string_view name, int data ) :
		MaterialVar( name, EMatVar_Int )
	{ aDataInt = data; }

	MaterialVar( const std::string_view name, const glm::vec2& data ) :
		MaterialVar( name, EMatVar_Vec2 )
	{ aDataVec2 = data; }

	MaterialVar( const std::string_view name, const glm::vec3& data ) :
		MaterialVar( name, EMatVar_Vec3 )
	{ aDataVec3 = data; }

	MaterialVar( const std::string_view name, const glm::vec4& data ) :
		MaterialVar( name, EMatVar_Vec4 )
	{ aDataVec4 = data; }

	~MaterialVar()
	{
		if ( apName )
			Util_FreeString( apName );
	}

	char*   apName;
	u32     aNameLen;
	EMatVar aType;
	
	// TODO: can this be smaller somehow? this takes up 16 bytes due to the vec4
	// which will only be used very, very rarely!!
	union
	{
		Handle    aDataTexture;
		float     aDataFloat;
		int       aDataInt;
		bool      aDataBool;
		glm::vec2 aDataVec2;
		glm::vec3 aDataVec3;
		glm::vec4 aDataVec4;
	};
	
	// inline void             SetTexture( Handle val )                      { aType = EMatVar_Texture; aDataTexture = val; }
	// inline void             SetFloat( float val )                         { aType = EMatVar_Float; aDataFloat = val; }
	// inline void             SetInt( int val )                             { aType = EMatVar_Int; aDataInt = val; }
	// inline void             SetVec2( const glm::vec2 &val )               { aType = EMatVar_Vec2; aDataVec2 = val; }
	// inline void             SetVec3( const glm::vec3 &val )               { aType = EMatVar_Vec3; aDataVec3 = val; }
	// inline void             SetVec4( const glm::vec4 &val )               { aType = EMatVar_Vec4; aDataVec4 = val; }
	
	inline void             SetVar( Handle val )                          { aType = EMatVar_Texture; aDataTexture = val; }
	inline void             SetVar( float val )                           { aType = EMatVar_Float; aDataFloat = val; }
	inline void             SetVar( int val )                             { aType = EMatVar_Int; aDataInt = val; }
	inline void             SetVar( bool val )                            { aType = EMatVar_Bool; aDataBool = val; }
	inline void             SetVar( const glm::vec2 &val )                { aType = EMatVar_Vec2; aDataVec2 = val; }
	inline void             SetVar( const glm::vec3 &val )                { aType = EMatVar_Vec3; aDataVec3 = val; }
	inline void             SetVar( const glm::vec4 &val )                { aType = EMatVar_Vec4; aDataVec4 = val; }

	inline Handle           GetTexture( Handle fallback = InvalidHandle ) { return ( aType == EMatVar_Texture ) ? aDataTexture : fallback; }
	inline const float&     GetFloat( float fallback = 0.f )              { return ( aType == EMatVar_Float ) ? aDataFloat : fallback; }
	inline const int&       GetInt( int fallback = 0 )                    { return ( aType == EMatVar_Int ) ? aDataInt : fallback; }
	inline bool             GetBool( bool fallback = false )              { return ( aType == EMatVar_Bool ) ? aDataBool : fallback; }
	inline const glm::vec2& GetVec2( const glm::vec2 &fallback )          { return ( aType == EMatVar_Vec2 ) ? aDataVec2 : fallback; }
	inline const glm::vec3& GetVec3( const glm::vec3 &fallback )          { return ( aType == EMatVar_Vec3 ) ? aDataVec3 : fallback; }
	inline const glm::vec4& GetVec4( const glm::vec4 &fallback )          { return ( aType == EMatVar_Vec4 ) ? aDataVec4 : fallback; }
};


struct MaterialData_t
{
	// std::vector< MaterialVar > aVars;
	ChVector< MaterialVar > aVars;
	u32                     aRefCount = 0;
};


static ResourceList< MaterialData_t* >                gMaterials;
static std::unordered_map< std::string, Handle >      gMaterialPaths;
static std::unordered_map< std::string_view, Handle > gMaterialNames;  // TODO: use string hashing for this?
static std::unordered_map< Handle, Handle >           gMaterialShaders;

static Handle                                         gInvalidMaterial;
static std::string                                    gStrEmpty;


const char* Graphics::Mat_GetName( Handle shMat )
{
	for ( auto& [name, mat] : gMaterialNames )
	{
		if ( mat != shMat )
			continue;

		if ( name.data() == nullptr )
			Log_Error( gLC_ClientGraphics, "Material Name is nullptr???\n" );

		return name.data();
	}

	return nullptr;
}


size_t Graphics::Mat_GetVarCount( Handle mat )
{
	MaterialData_t* data = nullptr;
	if ( !gMaterials.Get( mat, &data ) )
	{
		Log_Error( gLC_ClientGraphics, "Mat_SetVar: No Vars found, material must of been freed\n" );
		return 0;
	}

	return data->aVars.size();
}


EMatVar Graphics::Mat_GetVarType( Handle mat, size_t sIndex )
{
	MaterialData_t* data = nullptr;
	if ( !gMaterials.Get( mat, &data ) )
	{
		Log_Error( gLC_ClientGraphics, "Mat_GetVarType: No Vars found, material must of been freed\n" );
		return EMatVar_Invalid;
	}

	if ( sIndex >= data->aVars.size() )
	{
		Log_ErrorF( gLC_ClientGraphics, "Mat_GetVarType: Index out of bounds (index: %zu - size: %zd)\n", sIndex, data->aVars.size() );
		return EMatVar_Invalid;
	}

	return data->aVars[ sIndex ].aType;
}


const char* Graphics::Mat_GetVarName( Handle mat, size_t sIndex )
{
	MaterialData_t* data = nullptr;
	if ( !gMaterials.Get( mat, &data ) )
	{
		Log_Error( gLC_ClientGraphics, "Mat_GetVarType: No Vars found, material must of been freed\n" );
		return "";
	}

	if ( sIndex >= data->aVars.size() )
	{
		Log_ErrorF( gLC_ClientGraphics, "Mat_GetVarType: Index out of bounds (index: %zu - size: %zd)\n", sIndex, data->aVars.size() );
		return "";
	}

	return data->aVars[ sIndex ].apName;
}


Handle Graphics::Mat_GetShader( Handle mat )
{
	auto it = gMaterialShaders.find( mat );
	if ( it != gMaterialShaders.end() )
		return it->second;

	Log_Warn( gLC_ClientGraphics, "Mat_GetShader: No Material found in gMaterialShaders!\n" );
	return InvalidHandle;
}


void Graphics::Mat_SetShader( Handle mat, Handle shShader )
{
	auto it = gMaterialShaders.find( mat );
	if ( it != gMaterialShaders.end() )
	{
		Shader_RemoveMaterial( mat );

		it->second = shShader;

		// Tell Shader System we changed shader
		Shader_AddMaterial( mat );
	}
	else
	{
		Log_Warn( gLC_ClientGraphics, "Mat_SetShader: No Material found in gMaterialShaders!\n" );
	}
}


VertexFormat Graphics::Mat_GetVertexFormat( Handle mat )
{
	Handle shader = Mat_GetShader( mat );

	if ( shader == InvalidHandle )
		return VertexFormat_None;

	return Shader_GetVertexFormat( shader );
}


// Increments Reference Count for material
void Graphics::Mat_AddRef( ChHandle_t sMat )
{
	MaterialData_t* data = nullptr;
	if ( !gMaterials.Get( sMat, &data ) )
	{
		Log_Error( gLC_ClientGraphics, "Mat_AddRef: Material not found, material must of been freed or never loaded\n" );
		return;
	}

	Log_DevF( gLC_ClientGraphics, 2, "Incremented Ref Count for Material \"%zd\" from %u to %u\n", data->aRefCount, data->aRefCount + 1 );
	data->aRefCount++;
}


// Decrements Reference Count for material - returns true if the material is deleted
bool Graphics::Mat_RemoveRef( ChHandle_t sMat )
{
	MaterialData_t* data = nullptr;
	if ( !gMaterials.Get( sMat, &data ) )
	{
		Log_Error( gLC_ClientGraphics, "Mat_RemoveRef: Material not found, material must of been freed or never loaded\n" );
		return true;  // we can't find it, so technically it's deleted or never existed, so return true
	}

	// TODO: when you implement a new resource system
	// have that implement reference counting itself
	CH_ASSERT( data->aRefCount != 0 );

	std::string_view matName;

	for ( auto& [ name, mat ] : gMaterialNames )
	{
		if ( mat == sMat )
		{
			matName = name;
			break;
		}
	}

	if ( matName.size() )
		Log_DevF( gLC_ClientGraphics, 2, "Decremented Ref Count for Material \"%s\" from %u to %u\n", matName.data(), data->aRefCount, data->aRefCount - 1 );
	else
		Log_DevF( gLC_ClientGraphics, 2, "Decremented Ref Count for Material \"%zd\" from %u to %u\n", sMat, data->aRefCount, data->aRefCount - 1 );

	data->aRefCount--;

	if ( data->aRefCount != 0 )
		return false;

	// free textures in material vars
	for ( MaterialVar& var : data->aVars )
	{
		if ( var.aType != EMatVar_Texture )
			continue;

		render->FreeTexture( var.aDataTexture );
	}

	Shader_RemoveMaterial( sMat );

	delete data;
	gMaterials.Remove( sMat );

	if ( matName.size() )
	{
		// Log_DevF( gLC_ClientGraphics, 1, "Freeing Material \"%s\" - Handle \"%zd\"\n", matName.data(), sMat );
		Log_DevF( gLC_ClientGraphics, 1, "Freeing Material \"%s\"\n", matName.data() );
		char* matNameChar = (char*)matName.data();
		gMaterialNames.erase( matName );
		Util_FreeString( matNameChar );
	}
	else
	{
		Log_DevF( gLC_ClientGraphics, 1, "Freeing Material \"%zd\"\n", sMat );
	}

	// make sure it's not in the dirty materials list
	if ( gGraphicsData.aDirtyMaterials.contains( sMat ) )
		gGraphicsData.aDirtyMaterials.erase( sMat );

	return true;
}


template <typename T>
void Mat_SetVarInternal( Handle mat, const std::string& name, const T& value )
{
	PROF_SCOPE();

	MaterialData_t* data = nullptr;
	if ( !gMaterials.Get( mat, &data ) )
	{
		Log_Error( gLC_ClientGraphics, "Mat_SetVar: No Vars found, material must of been freed\n" );
		return;
	}

	gGraphicsData.aDirtyMaterials.emplace( mat );

	for ( MaterialVar& var : data->aVars )
	{
		if ( name.size() != var.aNameLen )
			continue;

		if ( var.apName == name )
		{
			var.SetVar( value );
			return;
		}
	}

	// data->aVars.emplace_back( name, value );
	MaterialVar& var = data->aVars.emplace_back( true );

	var.aNameLen     = name.size();
	var.apName       = Util_AllocString( name.data(), name.size() );

	var.SetVar( value );
}


void Graphics::Mat_SetVar( Handle mat, const std::string& name, Handle value )
{
	Mat_SetVarInternal( mat, name, value );
}

void Graphics::Mat_SetVar( Handle mat, const std::string& name, float value )              { Mat_SetVarInternal( mat, name, value ); }
void Graphics::Mat_SetVar( Handle mat, const std::string& name, int value )                { Mat_SetVarInternal( mat, name, value ); }
void Graphics::Mat_SetVar( Handle mat, const std::string& name, bool value )               { Mat_SetVarInternal( mat, name, value ); }
void Graphics::Mat_SetVar( Handle mat, const std::string& name, const glm::vec2& value )   { Mat_SetVarInternal( mat, name, value ); }
void Graphics::Mat_SetVar( Handle mat, const std::string& name, const glm::vec3& value )   { Mat_SetVarInternal( mat, name, value ); }
void Graphics::Mat_SetVar( Handle mat, const std::string& name, const glm::vec4& value )   { Mat_SetVarInternal( mat, name, value ); }


MaterialVar* Mat_GetVarInternal( Handle mat, std::string_view name )
{
	PROF_SCOPE();

	MaterialData_t* data = nullptr;
	if ( !gMaterials.Get( mat, &data ) )
	{
		Log_ErrorF( gLC_ClientGraphics, "%s : No Vars found, material must of been freed\n", CH_FUNC_NAME_CLASS );
		return nullptr;
	}

	for ( MaterialVar& var : data->aVars )
	{
		if ( name.size() != var.aNameLen )
			continue;

		if ( var.apName == name )
			return &var;
	}

	return nullptr;
}


MaterialVar* Mat_GetVarInternal( Handle mat, u32 sIndex )
{
	PROF_SCOPE();

	MaterialData_t* data = nullptr;
	if ( !gMaterials.Get( mat, &data ) )
	{
		Log_ErrorF( gLC_ClientGraphics, "%s : No Vars found, material must of been freed\n",  CH_FUNC_NAME_CLASS );
		return nullptr;
	}

	if ( sIndex >= data->aVars.size() )
	{
		Log_ErrorF( gLC_ClientGraphics, "%s, : Index out of bounds (index: %zu - size: %zd)\n", CH_FUNC_NAME_CLASS, sIndex, data->aVars.size() );
		return nullptr;
	}

	return &data->aVars[ sIndex ];
}


int Graphics::Mat_GetTextureIndex( Handle mat, std::string_view name, Handle fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return render->GetTextureIndex( var ? var->GetTexture( fallback ) : fallback );
}


Handle Graphics::Mat_GetTexture( Handle mat, std::string_view name, Handle fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return var ? var->GetTexture( fallback ) : fallback;
}


float Graphics::Mat_GetFloat( Handle mat, std::string_view name, float fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return var ? var->GetFloat( fallback ) : fallback;
}


int Graphics::Mat_GetInt( Handle mat, std::string_view name, int fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return var ? var->GetInt( fallback ) : fallback;
}


bool Graphics::Mat_GetBool( Handle mat, std::string_view name, bool fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return var ? var->GetBool( fallback ) : fallback;
}


const glm::vec2& Graphics::Mat_GetVec2( Handle mat, std::string_view name, const glm::vec2& fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return var ? var->GetVec2( fallback ) : fallback;
}


const glm::vec3& Graphics::Mat_GetVec3( Handle mat, std::string_view name, const glm::vec3& fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return var ? var->GetVec3( fallback ) : fallback;
}


const glm::vec4& Graphics::Mat_GetVec4( Handle mat, std::string_view name, const glm::vec4& fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return var ? var->GetVec4( fallback ) : fallback;
}


// ---------------------------------------------------------------------------------------------------------


int Graphics::Mat_GetTextureIndex( Handle mat, u32 sIndex, Handle fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return render->GetTextureIndex( var ? var->GetTexture( fallback ) : fallback );
}


Handle Graphics::Mat_GetTexture( Handle mat, u32 sIndex, Handle fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetTexture( fallback ) : fallback;
}


float Graphics::Mat_GetFloat( Handle mat, u32 sIndex, float fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetFloat( fallback ) : fallback;
}


int Graphics::Mat_GetInt( Handle mat, u32 sIndex, int fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetInt( fallback ) : fallback;
}


bool Graphics::Mat_GetBool( Handle mat, u32 sIndex, bool fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetBool( fallback ) : fallback;
}


const glm::vec2& Graphics::Mat_GetVec2( Handle mat, u32 sIndex, const glm::vec2& fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetVec2( fallback ) : fallback;
}


const glm::vec3& Graphics::Mat_GetVec3( Handle mat, u32 sIndex, const glm::vec3& fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetVec3( fallback ) : fallback;
}


const glm::vec4& Graphics::Mat_GetVec4( Handle mat, u32 sIndex, const glm::vec4& fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetVec4( fallback ) : fallback;
}


// ---------------------------------------------------------------------------------------------------------


template< typename VEC >
static void ParseMaterialVarArray( VEC& value, JsonObject_t& cur )
{
	for ( int ii = 0; ii < value.length(); ii++ )
	{
		switch ( cur.aObjects[ ii ].aType )
		{
			default:
				Log_ErrorF( "Invalid Material Array Value Type: %s, only accepts Int, Double, True, or False\n", Json_TypeToStr( cur.aObjects[ ii ].aType ) );
				break;

			case EJsonType_Int:
				value[ ii ] = cur.aObjects[ ii ].aInt;
				break;

			case EJsonType_Double:
				value[ ii ] = cur.aObjects[ ii ].aDouble;
				break;

			case EJsonType_True:
				value[ ii ] = 1;
				break;

			case EJsonType_False:
				value[ ii ] = 0;
				break;
		}
	}
}


// Used in normal material loading, and eventually, live material reloading
bool Graphics_ParseMaterial( const std::string& srName, const std::string& srPath, Handle& handle )
{
	PROF_SCOPE();

	std::string fullPath;

	if ( srPath.ends_with( ".cmt" ) )
		fullPath = FileSys_FindFile( srPath );
	else
		fullPath = FileSys_FindFile( srPath + ".cmt" );

	if ( fullPath.empty() )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to Find Material: \"%s\"", srPath.c_str() );
		return false;
	}

	std::vector< char > data = FileSys_ReadFile( fullPath );

	if ( data.empty() )
		return false;

	JsonObject_t root;
	EJsonError   err = Json_Parse( &root, data.data() );

	if ( err != EJsonError_None )
	{
		Log_ErrorF( gLC_ClientGraphics, "Error Parsing Material: %s\n", Json_ErrorToStr( err ) );
		return false;
	}

	Handle        shader = InvalidHandle;

	for ( size_t i = 0; i < root.aObjects.size(); i++ )
	{
		JsonObject_t& cur = root.aObjects[ i ];

		if ( cur.apName && strcmp( cur.apName, "shader" ) == 0 )
		{
			if ( shader != InvalidHandle )
			{
				Log_WarnF( gLC_ClientGraphics, "Shader is specified multiple times in material \"%s\"", srPath.c_str() );
				continue;
			}

			if ( cur.aType != EJsonType_String )
			{
				Log_WarnF( gLC_ClientGraphics, "Shader value is not a string!: \"%s\"", srPath.c_str() );
				Json_Free( &root );
				return false;
			}

			shader = gGraphics.GetShader( cur.apString );
			if ( shader == InvalidHandle )
			{
				Log_ErrorF( gLC_ClientGraphics, "Failed to find Material Shader: %s - \"%s\"\n", cur.apString, srPath.c_str() );
				Json_Free( &root );
				return false;
			}

			MaterialData_t* matData = nullptr;
			if ( handle == InvalidHandle )
			{
				matData            = new MaterialData_t;
				matData->aRefCount = 1;
				handle             = gMaterials.Add( matData );

				char* name         = Util_AllocString( srName.data(), srName.size() );
				gMaterialNames[ name ] = handle;
			}
			else
			{
				if ( !gMaterials.Get( handle, &matData ) )
				{
					Log_WarnF( gLC_ClientGraphics, "Failed to find Material Data while updating: \"%s\"\n", srPath.c_str() );
					Json_Free( &root );
					return false;
				}

				Shader_RemoveMaterial( handle );
			}

			gMaterialShaders[ handle ] = shader;
			Shader_AddMaterial( handle );

			continue;
		}

		switch ( cur.aType )
		{
			default:
			{
				Log_WarnF( gLC_ClientGraphics, "Unused Value Type: %d - \"%s\"\n", cur.aType, srPath.c_str() );
				break;
			}

			// Texture Path
			case EJsonType_String:
			{
				TextureCreateData_t createData{};
				createData.aUsage  = EImageUsage_Sampled;
				createData.aFilter = EImageFilter_Linear;

				std::string texturePath = cur.apString;

				if ( FileSys_IsRelative( cur.apString ) )
				{
					if ( !texturePath.ends_with( ".ktx" ) )
						texturePath += ".ktx";

					std::string basePath = texturePath;

					if ( !FileSys_IsFile( texturePath ) )
						texturePath = "models/" + basePath;

					if ( !FileSys_IsFile( texturePath ) )
						texturePath = "materials/" + basePath;
				}

				Handle texture     = InvalidHandle;
				gGraphics.Mat_SetVar( handle, cur.apName, render->LoadTexture( texture, texturePath, createData ) );
				break;
			}

			case EJsonType_Int:
			{
				// integer is here is an int64_t
				if ( cur.aInt > INT_MAX )
				{
					Log_WarnF( gLC_ClientGraphics, "Overflowed Int Value for key \"%s\", clamping to INT_MAX - \"%s\"\n", cur.apName, srPath.c_str() );
					gGraphics.Mat_SetVar( handle, cur.apName, INT_MAX );
					break;
				}
				else if ( cur.aInt < INT_MIN )
				{
					Log_WarnF( gLC_ClientGraphics, "Underflowed Int Value for key \"%s\", clamping to INT_MIN - \"%s\"\n", cur.apName, srPath.c_str() );
					gGraphics.Mat_SetVar( handle, cur.apName, INT_MIN );
					break;
				}

				gGraphics.Mat_SetVar( handle, cur.apName, static_cast< int >( cur.aInt ) );
				break;
			}

			// double
			case EJsonType_Double:
			{
				gGraphics.Mat_SetVar( handle, cur.apName, static_cast< float >( cur.aDouble ) );
				break;
			}

			case EJsonType_True:
			{
				gGraphics.Mat_SetVar( handle, cur.apName, true );
				break;
			}

			case EJsonType_False:
			{
				gGraphics.Mat_SetVar( handle, cur.apName, false );
				break;
			}

			case EJsonType_Array:
			{
				switch ( cur.aObjects.size() )
				{
					default:
					{
						Log_ErrorF( "Invalid Material Vector Type: Has %d elements, only accepts 2, 3, or 4 elements\n", cur.aObjects.size() );
						break;
					}

					case 2:
					{
						glm::vec2 value = {};
						ParseMaterialVarArray( value, cur );
						gGraphics.Mat_SetVar( handle, cur.apName, value );
						break;
					}

					case 3:
					{
						glm::vec3 value = {};
						ParseMaterialVarArray( value, cur );
						gGraphics.Mat_SetVar( handle, cur.apName, value );
						break;
					}

					case 4:
					{
						glm::vec4 value = {};
						ParseMaterialVarArray( value, cur );
						gGraphics.Mat_SetVar( handle, cur.apName, value );
						break;
					}
				}

				break;
			}
		}
	}

	Json_Free( &root );

	return true;
}


static std::string ParseMaterialNameFromPath( const std::string& path )
{
	std::string output;

	// Remove .cmt from the extension, no need for this
	if ( path.ends_with( ".cmt" ) )
	{
		output = path.substr( 0, path.size() - 4 );
	}
	else
	{
		output = path;
	}

	return output;
}


Handle Graphics::LoadMaterial( const std::string& srPath )
{
	std::string name = ParseMaterialNameFromPath( srPath );

	auto nameIt = gMaterialNames.find( name.c_str() );
	if ( nameIt != gMaterialNames.end() )
	{
		Log_DevF( gLC_ClientGraphics, 2, "Incrementing Ref Count for Material \"%s\" - \"%zd\"\n", srPath.c_str(), nameIt->second );
		Mat_AddRef( nameIt->second );
		return nameIt->second;
	}

	Handle handle = InvalidHandle;
	if ( !Graphics_ParseMaterial( name, srPath, handle ) )
		return InvalidHandle;

	Log_DevF( gLC_ClientGraphics, 1, "Loaded Material \"%s\"\n", srPath.c_str() );
	gMaterialPaths[ srPath ] = handle;
	return handle;
}


// Create a new material with a name and a shader
Handle Graphics::CreateMaterial( const std::string& srName, Handle shShader )
{
	auto nameIt = gMaterialNames.find( srName.c_str() );
	if ( nameIt != gMaterialNames.end() )
	{
		Log_DevF( gLC_ClientGraphics, 2, "Incrementing Ref Count for Material \"%s\" - \"%zd\"\n", srName.c_str(), nameIt->second );
		Mat_AddRef( nameIt->second );
		return nameIt->second;
	}

	auto matData       = new MaterialData_t;
	matData->aRefCount = 1;

	Handle handle      = gMaterials.Add( matData );
	char*  name        = Util_AllocString( srName.data(), srName.size() );

	gMaterialNames[ name ]     = handle;
	gMaterialShaders[ handle ] = shShader;

	Shader_AddMaterial( handle );

	return handle;
}


// Free a material
void Graphics::FreeMaterial( ChHandle_t shMaterial )
{
	Mat_RemoveRef( shMaterial );
}


// Find a material by name
// Name is a full path to the cmt file
// EXAMPLE: C:/chocolate/sidury/materials/dev/grid01.cmt
// NAME: dev/grid01
ChHandle_t Graphics::FindMaterial( const char* spName )
{
	std::string name = ParseMaterialNameFromPath( spName );

	auto nameIt = gMaterialNames.find( name );
	if ( nameIt != gMaterialNames.end() )
	{
		// TODO: should this really add a reference to this?
		Mat_AddRef( nameIt->second );
		return nameIt->second;
	}

	return CH_INVALID_HANDLE;
}


// Get the total amount of materials created
u32 Graphics::GetMaterialCount()
{
	return gMaterials.size();
}


ChHandle_t Graphics::GetMaterialByIndex( u32 sIndex )
{
	return gMaterials.GetHandleByIndex( sIndex );

	//if ( gMaterials.aHandles.size() >= sIndex )
	//{
	//	Log_ErrorF( "Invalid Material Index: %d, only %d Materials loaded\n", sIndex, gMaterials.size() );
	//	return CH_INVALID_HANDLE;
	//}
	//
	//return gMaterials.aHandles[ sIndex ];
}


// Get the path to the material
const std::string& Graphics::GetMaterialPath( Handle sMaterial )
{
	if ( sMaterial == CH_INVALID_HANDLE )
		return "";

	for ( auto& [ path, handle ] : gMaterialPaths )
	{
		if ( sMaterial == handle )
			return path;
	}

	return "";
}


// Tell all materials to rebuild
void Graphics::SetAllMaterialsDirty()
{
	if ( gMaterialShaders.size() == gGraphicsData.aDirtyMaterials.size() )
		return;

	for ( const auto& [ mat, shader ] : gMaterialShaders )
		gGraphicsData.aDirtyMaterials.emplace( mat );
}


CONCMD( r_mark_all_materials_dirty )
{
	gGraphics.SetAllMaterialsDirty();
}


CONCMD( r_dump_materials )
{
	Log_MsgF( gLC_ClientGraphics, "%d Materials Loaded\n", gMaterialNames.size() );

	u32 i = 0;
	for ( auto& [ name, mat ] : gMaterialNames )
	{
		Log_MsgF( gLC_ClientGraphics, "%zd - \"%s\"\n", i++, name.data() );
	}
}



// ---------------------------------------------------------------------------------------------------------
// Resource System Functions


bool _Graphics_LoadMaterial( ChHandle_t& item, const fs::path& srPath )
{
	return false;
}


bool _Graphics_CreateMaterial( ChHandle_t& item, const fs::path& srInternalPath, void* spData )
{
	return false;
}


void _Graphics_FreeMaterial( ChHandle_t item )
{
}

