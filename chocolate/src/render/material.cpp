#include "graphics_int.h"
#include "render/irender.h"

#include "core/json5.h"
#include "core/string.h"

#include <unordered_set>
#include <variant>

#if __unix__
#include <limits.h>
#endif /* __unix__  */

extern IRender* render;

using MaterialVar2 = std::variant<
  ch_handle_t,
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
		ch_string name_alloc = ch_str_copy( name.data(), name.size() );
		apName               = name_alloc.data;
		aNameLen             = name_alloc.size;

	}

	MaterialVar( const std::string_view name, ch_handle_t data ) :
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
			ch_str_free( apName );
	}

	char*   apName;
	u32     aNameLen;
	EMatVar aType;
	
	// TODO: can this be smaller somehow? this takes up 16 bytes due to the vec4
	// which will only be used very, very rarely!!
	union
	{
		ch_handle_t    aDataTexture;
		float     aDataFloat;
		int       aDataInt;
		bool      aDataBool;
		glm::vec2 aDataVec2;
		glm::vec3 aDataVec3;
		glm::vec4 aDataVec4;
	};
	
	// inline void             SetTexture( ch_handle_t val )                      { aType = EMatVar_Texture; aDataTexture = val; }
	// inline void             SetFloat( float val )                         { aType = EMatVar_Float; aDataFloat = val; }
	// inline void             SetInt( int val )                             { aType = EMatVar_Int; aDataInt = val; }
	// inline void             SetVec2( const glm::vec2 &val )               { aType = EMatVar_Vec2; aDataVec2 = val; }
	// inline void             SetVec3( const glm::vec3 &val )               { aType = EMatVar_Vec3; aDataVec3 = val; }
	// inline void             SetVec4( const glm::vec4 &val )               { aType = EMatVar_Vec4; aDataVec4 = val; }
	
	inline void             SetVar( ch_handle_t val )                          { aType = EMatVar_Texture; aDataTexture = val; }
	inline void             SetVar( float val )                           { aType = EMatVar_Float; aDataFloat = val; }
	inline void             SetVar( int val )                             { aType = EMatVar_Int; aDataInt = val; }
	inline void             SetVar( bool val )                            { aType = EMatVar_Bool; aDataBool = val; }
	inline void             SetVar( const glm::vec2 &val )                { aType = EMatVar_Vec2; aDataVec2 = val; }
	inline void             SetVar( const glm::vec3 &val )                { aType = EMatVar_Vec3; aDataVec3 = val; }
	inline void             SetVar( const glm::vec4 &val )                { aType = EMatVar_Vec4; aDataVec4 = val; }

	inline ch_handle_t           GetTexture( ch_handle_t fallback = CH_INVALID_HANDLE ) { return ( aType == EMatVar_Texture ) ? aDataTexture : fallback; }
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


static ResourceList< MaterialData_t* >         gMaterials;
static std::unordered_map< ch_string, ch_handle_t > gMaterialPaths;
static std::unordered_map< ch_string, ch_handle_t > gMaterialNames;  // TODO: use string hashing for this?
static std::unordered_map< ch_handle_t, ch_handle_t >    gMaterialShaders;

static ch_handle_t                                  gInvalidMaterial;
static std::string                             gStrEmpty;


const char* Graphics::Mat_GetName( ch_handle_t shMat )
{
	for ( auto& [name, mat] : gMaterialNames )
	{
		if ( mat != shMat )
			continue;

		if ( name.data == nullptr )
			Log_Error( gLC_ClientGraphics, "Material Name is nullptr???\n" );

		return name.data;
	}

	return nullptr;
}


size_t Graphics::Mat_GetVarCount( ch_handle_t mat )
{
	MaterialData_t* data = nullptr;
	if ( !gMaterials.Get( mat, &data ) )
	{
		Log_Error( gLC_ClientGraphics, "Mat_SetVar: No Vars found, material must of been freed\n" );
		return 0;
	}

	return data->aVars.size();
}


EMatVar Graphics::Mat_GetVarType( ch_handle_t mat, size_t sIndex )
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


const char* Graphics::Mat_GetVarName( ch_handle_t mat, size_t sIndex )
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


ch_handle_t Graphics::Mat_GetShader( ch_handle_t mat )
{
	auto it = gMaterialShaders.find( mat );
	if ( it != gMaterialShaders.end() )
		return it->second;

	Log_Warn( gLC_ClientGraphics, "Mat_GetShader: No Material found in gMaterialShaders!\n" );
	return CH_INVALID_HANDLE;
}


void Graphics::Mat_SetShader( ch_handle_t mat, ch_handle_t shShader )
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


VertexFormat Graphics::Mat_GetVertexFormat( ch_handle_t mat )
{
	ch_handle_t shader = Mat_GetShader( mat );

	if ( shader == CH_INVALID_HANDLE )
		return VertexFormat_None;

	return Shader_GetVertexFormat( shader );
}


// Increments Reference Count for material
void Graphics::Mat_AddRef( ch_handle_t sMat )
{
	MaterialData_t* data = nullptr;
	if ( !gMaterials.Get( sMat, &data ) )
	{
		Log_Error( gLC_ClientGraphics, "Mat_AddRef: Material not found, material must of been freed or never loaded\n" );
		return;
	}

	Log_DevF( gLC_ClientGraphics, 3, "Incremented Ref Count for Material \"%d\" from %d to %d\n", sMat, data->aRefCount, data->aRefCount + 1 );
	data->aRefCount++;
}


// Decrements Reference Count for material - returns true if the material is deleted
bool Graphics::Mat_RemoveRef( ch_handle_t sMat )
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

	ch_string matName;

	for ( auto& [ name, mat ] : gMaterialNames )
	{
		if ( mat == sMat )
		{
			matName = name;
			break;
		}
	}

	if ( matName.size )
		Log_DevF( gLC_ClientGraphics, 3, "Decremented Ref Count for Material \"%s\" from %u to %u\n", matName.data, data->aRefCount, data->aRefCount - 1 );
	else
		Log_DevF( gLC_ClientGraphics, 3, "Decremented Ref Count for Material \"%zd\" from %u to %u\n", sMat, data->aRefCount, data->aRefCount - 1 );

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

	if ( matName.size )
	{
		// Log_DevF( gLC_ClientGraphics, 1, "Freeing Material \"%s\" - ch_handle_t \"%zd\"\n", matName.data(), sMat );
		Log_DevF( gLC_ClientGraphics, 1, "Freeing Material \"%s\"\n", matName.data );
		char* matNameChar = matName.data;
		gMaterialNames.erase( matName );
		ch_str_free( matNameChar );
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
void Mat_SetVarInternal( ch_handle_t mat, const std::string& name, const T& value )
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
	var.apName       = ch_str_copy( name.data(), name.size() ).data;

	var.SetVar( value );
}


void Graphics::Mat_SetVar( ch_handle_t mat, const std::string& name, ch_handle_t value )
{
	Mat_SetVarInternal( mat, name, value );
}

void Graphics::Mat_SetVar( ch_handle_t mat, const std::string& name, float value )              { Mat_SetVarInternal( mat, name, value ); }
void Graphics::Mat_SetVar( ch_handle_t mat, const std::string& name, int value )                { Mat_SetVarInternal( mat, name, value ); }
void Graphics::Mat_SetVar( ch_handle_t mat, const std::string& name, bool value )               { Mat_SetVarInternal( mat, name, value ); }
void Graphics::Mat_SetVar( ch_handle_t mat, const std::string& name, const glm::vec2& value )   { Mat_SetVarInternal( mat, name, value ); }
void Graphics::Mat_SetVar( ch_handle_t mat, const std::string& name, const glm::vec3& value )   { Mat_SetVarInternal( mat, name, value ); }
void Graphics::Mat_SetVar( ch_handle_t mat, const std::string& name, const glm::vec4& value )   { Mat_SetVarInternal( mat, name, value ); }


MaterialVar* Mat_GetVarInternal( ch_handle_t mat, std::string_view name )
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


MaterialVar* Mat_GetVarInternal( ch_handle_t mat, u32 sIndex )
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


int Graphics::Mat_GetTextureIndex( ch_handle_t mat, std::string_view name, ch_handle_t fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return render->GetTextureIndex( var ? var->GetTexture( fallback ) : fallback );
}


ch_handle_t Graphics::Mat_GetTexture( ch_handle_t mat, std::string_view name, ch_handle_t fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return var ? var->GetTexture( fallback ) : fallback;
}


// kinda dumb, but oh well
template <typename T>
T Mat_GetNumberValue( MaterialVar* var, T fallback )
{
	if ( !var )
		return fallback;

	switch ( var->aType )
	{
		case EMatVar_Float:
			return static_cast< T >( var->GetFloat( fallback ) );

		case EMatVar_Int:
			return static_cast< T >( var->GetInt( fallback ) );

		case EMatVar_Bool:
			return static_cast< T >( var->GetBool( fallback ) );

		default:
			Log_ErrorF( gLC_ClientGraphics, "Material Var \"%s\" Type is \"%d\", not Float, Int, or Bool\n", var->apName, var->aType );
			return fallback;
	}
}


float Graphics::Mat_GetFloat( ch_handle_t mat, std::string_view name, float fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return Mat_GetNumberValue( var, fallback );
}


int Graphics::Mat_GetInt( ch_handle_t mat, std::string_view name, int fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return Mat_GetNumberValue( var, fallback );
}


bool Graphics::Mat_GetBool( ch_handle_t mat, std::string_view name, bool fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return Mat_GetNumberValue( var, fallback );
}


const glm::vec2& Graphics::Mat_GetVec2( ch_handle_t mat, std::string_view name, const glm::vec2& fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return var ? var->GetVec2( fallback ) : fallback;
}


const glm::vec3& Graphics::Mat_GetVec3( ch_handle_t mat, std::string_view name, const glm::vec3& fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return var ? var->GetVec3( fallback ) : fallback;
}


const glm::vec4& Graphics::Mat_GetVec4( ch_handle_t mat, std::string_view name, const glm::vec4& fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, name );
	return var ? var->GetVec4( fallback ) : fallback;
}


// ---------------------------------------------------------------------------------------------------------


int Graphics::Mat_GetTextureIndex( ch_handle_t mat, u32 sIndex, ch_handle_t fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return render->GetTextureIndex( var ? var->GetTexture( fallback ) : fallback );
}


ch_handle_t Graphics::Mat_GetTexture( ch_handle_t mat, u32 sIndex, ch_handle_t fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetTexture( fallback ) : fallback;
}


float Graphics::Mat_GetFloat( ch_handle_t mat, u32 sIndex, float fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetFloat( fallback ) : fallback;
}


int Graphics::Mat_GetInt( ch_handle_t mat, u32 sIndex, int fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetInt( fallback ) : fallback;
}


bool Graphics::Mat_GetBool( ch_handle_t mat, u32 sIndex, bool fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetBool( fallback ) : fallback;
}


const glm::vec2& Graphics::Mat_GetVec2( ch_handle_t mat, u32 sIndex, const glm::vec2& fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetVec2( fallback ) : fallback;
}


const glm::vec3& Graphics::Mat_GetVec3( ch_handle_t mat, u32 sIndex, const glm::vec3& fallback )
{
	MaterialVar* var = Mat_GetVarInternal( mat, sIndex );
	return var ? var->GetVec3( fallback ) : fallback;
}


const glm::vec4& Graphics::Mat_GetVec4( ch_handle_t mat, u32 sIndex, const glm::vec4& fallback )
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
		switch ( cur.aObjects.apData[ ii ].aType )
		{
			default:
				Log_ErrorF( "Invalid Material Array Value Type: %s, only accepts Int, Double, True, or False\n", Json_TypeToStr( cur.aObjects.apData[ ii ].aType ) );
				break;

			case EJsonType_Int:
				value[ ii ] = cur.aObjects.apData[ ii ].aInt;
				break;

			case EJsonType_Double:
				value[ ii ] = cur.aObjects.apData[ ii ].aDouble;
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
bool Graphics_ParseMaterial( const ch_string& srName, const std::string& srPath, ch_handle_t& handle )
{
	PROF_SCOPE();

	ch_string fullPath;

	if ( srPath.ends_with( ".cmt" ) )
	{
		fullPath = FileSys_FindFile( srPath.data(), srPath.size() );
	}
	else
	{
		const char*    strings[]     = { srPath.data(), ".cmt" };
		const u64      lengths[]     = { srPath.size(), 4 };
		ch_string_auto path_with_ext = ch_str_join( 2, strings, lengths );
		fullPath                     = FileSys_FindFile( path_with_ext.data, path_with_ext.size );
	}

	if ( !fullPath.data )
	{
		Log_ErrorF( gLC_ClientGraphics, "Failed to Find Material: \"%s\"", srPath.c_str() );
		return false;
	}

	ch_string_auto data = FileSys_ReadFile( fullPath.data, fullPath.size );

	if ( !data.data )
		return false;

	JsonObject_t root;
	EJsonError   err = Json_Parse( &root, data.data );
	if ( err != EJsonError_None )
	{
		Log_ErrorF( gLC_ClientGraphics, "Error Parsing Material: %s\n", Json_ErrorToStr( err ) );
		return false;
	}

	ch_handle_t        shader = CH_INVALID_HANDLE;

	for ( size_t i = 0; i < root.aObjects.aCount; i++ )
	{
		JsonObject_t& cur = root.aObjects.apData[ i ];

		// dumb
		std::string   nameString( cur.name.data, cur.name.size );

		if ( cur.name.data && ch_str_equals( cur.name, "shader", 6 ) )
		{
			if ( shader != CH_INVALID_HANDLE )
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

			shader = gGraphics.GetShader( cur.aString.data );
			if ( shader == CH_INVALID_HANDLE )
			{
				Log_ErrorF( gLC_ClientGraphics, "Failed to find Material Shader: %s - \"%s\"\n", cur.aString.data, srPath.c_str() );
				Json_Free( &root );
				return false;
			}

			MaterialData_t* matData = nullptr;
			if ( handle == CH_INVALID_HANDLE )
			{
				matData                = new MaterialData_t;
				matData->aRefCount     = 1;
				handle                 = gMaterials.Add( matData );

				ch_string name         = ch_str_copy( srName.data, srName.size );
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
				// Sometimes the user can put numbers or bools in the texture path
				// so we need to check if it's a number or bool

				// Check if it's a number
				if ( cur.aString.data[ 0 ] >= '0' && cur.aString.data[ 0 ] <= '9' )
				{
					// Check if it's a float
					if ( strchr( cur.aString.data, '.' ) )
					{
						float value = 0.f;
						if ( ch_to_float( cur.aString.data, value ) )
						{
							float value = static_cast< float >( atof( cur.aString.data ) );
							gGraphics.Mat_SetVar( handle, nameString, value );
							break;
						}
					}

					// Check if it's an int
					long value = 0;
					if ( ch_to_long( cur.aString.data, value ) )
					{
						gGraphics.Mat_SetVar( handle, nameString, (int)value );
						break;
					}
				}

				// Check if it's a bool
				// TODO: use the convar bool alias list
				if ( ch_str_equals( cur.aString, "true", 4 ) )
				{
					gGraphics.Mat_SetVar( handle, nameString, true );
					break;
				}

				if ( ch_str_equals( cur.aString, "false", 5 ) )
				{
					gGraphics.Mat_SetVar( handle, nameString, false );
					break;
				}

				// This is indeed a texture
				TextureCreateData_t createData{};
				createData.aUsage  = EImageUsage_Sampled;
				createData.aFilter = EImageFilter_Linear;

				std::string texturePath( cur.aString.data, cur.aString.size );

				if ( FileSys_IsRelative( cur.aString.data, cur.aString.size ) )
				{
					if ( !texturePath.ends_with( ".ktx" ) )
						texturePath += ".ktx";

					std::string basePath = texturePath;

					if ( !FileSys_IsFile( texturePath.data(), texturePath.size() ) )
						texturePath = "models/" + basePath;

					if ( !FileSys_IsFile( texturePath.data(), texturePath.size() ) )
						texturePath = "materials/" + basePath;
				}

				ch_handle_t texture     = CH_INVALID_HANDLE;
				gGraphics.Mat_SetVar( handle, nameString, render->LoadTexture( texture, texturePath, createData ) );
				break;
			}

			case EJsonType_Int:
			{
				// integer is here is an int64_t
				if ( cur.aInt > INT_MAX )
				{
					Log_WarnF( gLC_ClientGraphics, "Overflowed Int Value for key \"%s\", clamping to INT_MAX - \"%s\"\n", cur.name.data, srPath.c_str() );
					gGraphics.Mat_SetVar( handle, nameString, INT_MAX );
					break;
				}
				else if ( cur.aInt < INT_MIN )
				{
					Log_WarnF( gLC_ClientGraphics, "Underflowed Int Value for key \"%s\", clamping to INT_MIN - \"%s\"\n", cur.name.data, srPath.c_str() );
					gGraphics.Mat_SetVar( handle, nameString, INT_MIN );
					break;
				}

				gGraphics.Mat_SetVar( handle, nameString, static_cast< int >( cur.aInt ) );
				break;
			}

			// double
			case EJsonType_Double:
			{
				gGraphics.Mat_SetVar( handle, nameString, static_cast< float >( cur.aDouble ) );
				break;
			}

			case EJsonType_True:
			{
				gGraphics.Mat_SetVar( handle, nameString, true );
				break;
			}

			case EJsonType_False:
			{
				gGraphics.Mat_SetVar( handle, nameString, false );
				break;
			}

			case EJsonType_Array:
			{
				switch ( cur.aObjects.aCount )
				{
					default:
					{
						Log_ErrorF( "Invalid Material Vector Type: Has %d elements, only accepts 2, 3, or 4 elements\n", cur.aObjects.aCount );
						break;
					}

					case 2:
					{
						glm::vec2 value = {};
						ParseMaterialVarArray( value, cur );
						gGraphics.Mat_SetVar( handle, cur.name.data, value );
						break;
					}

					case 3:
					{
						glm::vec3 value = {};
						ParseMaterialVarArray( value, cur );
						gGraphics.Mat_SetVar( handle, cur.name.data, value );
						break;
					}

					case 4:
					{
						glm::vec4 value = {};
						ParseMaterialVarArray( value, cur );
						gGraphics.Mat_SetVar( handle, cur.name.data, value );
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


static ch_string ParseMaterialNameFromPath( const char* path, s32 len )
{
	ch_string path_no_ext;

	if ( len < 0 )
		len = strlen( path );

	if ( len == 0 )
		return path_no_ext;

	path_no_ext.data = (char*)path;

	// Remove .cmt from the extension, no need for this
	if ( ch_str_ends_with( path, len, ".cmt", 4 ) )
	{
		path_no_ext.size = len - 4;
	}
	else
	{
		path_no_ext.size = len;
	}

	// Normalize Path Separators
	ch_string output = FileSys_CleanPath( path_no_ext.data, path_no_ext.size );

	return output;
}


ch_handle_t Graphics::LoadMaterial( const char* spPath, s32 sLen )
{
	ch_string name = ParseMaterialNameFromPath( spPath, sLen );

	auto nameIt = gMaterialNames.find( name );
	if ( nameIt != gMaterialNames.end() )
	{
		Log_DevF( gLC_ClientGraphics, 3, "Incrementing Ref Count for Material \"%s\" - \"%zd\"\n", spPath, nameIt->second );
		Mat_AddRef( nameIt->second );
		ch_str_free( name.data );
		return nameIt->second;
	}

	ch_handle_t handle = CH_INVALID_HANDLE;

	// lazy
	std::string tempPath( spPath, sLen );

	if ( !Graphics_ParseMaterial( name, tempPath, handle ) )
	{
		ch_str_free( name.data );
		return CH_INVALID_HANDLE;
	}

	Log_DevF( gLC_ClientGraphics, 1, "Loaded Material \"%s\"\n", name.data );

	ch_string path = ch_str_copy( spPath, sLen );

	gMaterialPaths[ path ] = handle;
	ch_str_free( name.data );
	return handle;
}


// Create a new material with a name and a shader
ch_handle_t Graphics::CreateMaterial( const std::string& srName, ch_handle_t shShader )
{
	ch_string name   = ParseMaterialNameFromPath( srName.data(), srName.size() );
	auto      nameIt = gMaterialNames.find( name );

	if ( nameIt != gMaterialNames.end() )
	{
		Log_DevF( gLC_ClientGraphics, 3, "Incrementing Ref Count for Material \"%s\" - \"%zd\"\n", srName.data(), nameIt->second );
		Mat_AddRef( nameIt->second );
		ch_str_free( name.data );
		return nameIt->second;
	}

	auto matData               = new MaterialData_t;
	matData->aRefCount         = 1;

	ch_handle_t handle              = gMaterials.Add( matData );

	gMaterialNames[ name ]     = handle;
	gMaterialShaders[ handle ] = shShader;

	Shader_AddMaterial( handle );

	return handle;
}


// Free a material
void Graphics::FreeMaterial( ch_handle_t shMaterial )
{
	Mat_RemoveRef( shMaterial );
}


// Find a material by name
// Name is a full path to the cmt file
// EXAMPLE: C:/chocolate/sidury/materials/dev/grid01.cmt
// NAME: dev/grid01
ch_handle_t Graphics::FindMaterial( const char* spName )
{
	ch_string name = ParseMaterialNameFromPath( spName, -1 );
	auto nameIt = gMaterialNames.find( name );

	ch_str_free( name.data );

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


ch_handle_t Graphics::GetMaterialByIndex( u32 sIndex )
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
const std::string& Graphics::GetMaterialPath( ch_handle_t sMaterial )
{
	if ( sMaterial == CH_INVALID_HANDLE )
		return "";

	for ( auto& [ path, handle ] : gMaterialPaths )
	{
		if ( sMaterial == handle )
		{
			std::string path_str( path.data, path.size );
			return path_str;
		}
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
		// get material data for ref count
		MaterialData_t* data = nullptr;

		if ( !gMaterials.Get( mat, &data ) )
		{
			Log_ErrorF( gLC_ClientGraphics, "Material \"%s\" - \"%zd\" not found in gMaterials\n", name.data, mat );
			continue;
		}

		Log_MsgF( gLC_ClientGraphics, "%zd - \"%s\" - %d Ref Count\n", i++, name.data, data->aRefCount );
	}
}

