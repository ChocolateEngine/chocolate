#include <unordered_set>

#include "../main_window/main.h"
#include "file_picker.h"
#include "imgui/imgui.h"
#include "material_editor.h"


CONVAR_INT( matedit_texture_size, 64, CVARF_ARCHIVE, "Size of textures drawn in the image preview" );


int gOffsetX = 0;
int gOffsetY = 0;


struct MaterialEntry
{
	std::string name;
	std::string path;
	ch_handle_t mat = CH_INVALID_HANDLE;
	// std::unordered_map< u32, TextureParameterData > textureData;
};


struct MaterialEditorData
{
	std::vector< MaterialEntry >                   materialList;

	ch_handle_t                                    mat           = CH_INVALID_HANDLE;

	bool                                           drawNewDialog = false;
	std::string                                    newMatName    = "";

	std::string                                    newVarName    = "";

	std::unordered_map< ch_handle_t, ImTextureID > imguiTextures;

	FilePickerData_t                               textureBrowser;
	u32                                            textureBrowserVar  = UINT32_MAX;
	const char*                                    textureBrowserName = nullptr;
};


static MaterialEditorData gMatEditor;


static int                StringTextInput( ImGuiInputTextCallbackData* data )
{
	std::string* string = (std::string*)data->UserData;
	string->resize( data->BufTextLen );

	return 0;
}


void Editor_DrawTextureInfo( TextureInfo_t& info )
{
	ImGui::Text( "Name: %s", info.name.size ? info.name.data : "UNNAMED" );

	if ( info.aPath.size )
		ImGui::Text( info.aPath.data );

	ImGui::Text( "%d x %d - %.6f MB", info.aSize.x, info.aSize.y, ch_bytes_to_mb( info.aMemoryUsage ) );
	ImGui::Text( "Format: TODO" );
	ImGui::Text( "Mip Levels: TODO" );
	ImGui::Text( "GPU Index: %d", info.aGpuIndex );
	ImGui::Text( "Ref Count: %d", info.aRefCount );
}


void MaterialEditor_DrawNewDialog()
{
	gMatEditor.newMatName.reserve( 128 );

	if ( ImGui::InputText( "Name", gMatEditor.newMatName.data(), 128, ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_EnterReturnsTrue, StringTextInput, &gMatEditor.newMatName ) )
	{
		ch_handle_t    shader    = graphics->GetShader( "basic_3d" );
		ch_handle_t    mat       = graphics->CreateMaterial( gMatEditor.newMatName, shader );

		MaterialEntry& matEntry  = gMatEditor.materialList.emplace_back();
		matEntry.mat             = mat;
		matEntry.name            = gMatEditor.newMatName;

		gMatEditor.drawNewDialog = false;
		gMatEditor.newMatName.clear();

		MaterialEditor_SetActiveMaterial( mat );
	}
}


void MaterialEditor_DrawMenuBar()
{
	if ( ImGui::BeginMainMenuBar() )
	{
		if ( ImGui::BeginMenu( "File" ) )
		{
			if ( ImGui::MenuItem( "New" ) )
			{
				gMatEditor.drawNewDialog = true;
			}

			if ( ImGui::MenuItem( "Load" ) )
			{
			}

			if ( ImGui::MenuItem( "Save" ) )
			{
			}

			if ( ImGui::MenuItem( "Save As" ) )
			{
			}

			if ( ImGui::MenuItem( "Close" ) )
			{
			}

			ImGui::Separator();

			if ( ImGui::BeginMenu( "Open Recent" ) )
			{
				ImGui::EndMenu();
			}

			ImGui::Separator();

			if ( ImGui::MenuItem( "Quit" ) )
			{
				gShowQuitConfirmation = true;
			}

			ImGui::EndMenu();
		}

		if ( ImGui::BeginMenu( "Edit" ) )
		{
			if ( ImGui::MenuItem( "Settings" ) )
			{
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
}


float gMaterialListWidth    = 0;
float gMaterialPreviewWidth = 0;
float gMaterialDataWidth    = 0;


void  MaterialEditor_DrawOpenMaterials()
{
	float  borderSize = ImGui::GetStyle().ChildBorderSize * 2;
	float  spacingX   = ImGui::GetStyle().ItemSpacing.y * 6;
	float  spacingY   = ImGui::GetStyle().ItemSpacing.y * 4;
	float  padding    = ImGui::GetStyle().FramePadding.y;

	ImVec2 textSize   = ImGui::CalcTextSize( "Material List" );

	ImGui::SetNextWindowSizeConstraints(
	  { textSize.x + spacingY, (float)( gHeight - spacingY ) },
	  { ( (float)gWidth / 2.f ) - spacingX, (float)( gHeight - spacingY ) } );

	if ( !ImGui::BeginChild( "Material List", {}, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX ) )
	{
		ImGui::EndChild();
		return;
	}

	ImGui::Text( "Material List" );
	ImGui::Separator();

	for ( MaterialEntry& matEntry : gMatEditor.materialList )
	{
		bool selected = matEntry.mat == gMatEditor.mat;
		if ( ImGui::Selectable( matEntry.name.data(), &selected ) )
		{
			MaterialEditor_SetActiveMaterial( matEntry.mat );
		}
	}

	gMaterialListWidth = ImGui::GetWindowSize().x;

	ImGui::EndChild();
}


void MaterialEditor_DrawMaterialPreview()
{
	float  borderSize = ImGui::GetStyle().ChildBorderSize * 2;
	float  spacingX   = ImGui::GetStyle().ItemSpacing.y * 6;
	float  spacingY   = ImGui::GetStyle().ItemSpacing.y * 4;
	float  padding    = ImGui::GetStyle().FramePadding.y;

	ImVec2 textSize   = ImGui::CalcTextSize( "Material Preview" );

	// ImGui::SetNextWindowSizeConstraints(
	// 	{ textSize.x + spacingY, (float)( (gHeight)-gMainMenuBarHeight - spacingY ) },
	// 	{ ( (float)gWidth / 2.f ) - spacingX, (float)( (gHeight)-gMainMenuBarHeight - spacingY ) }
	// );

	ImGui::SetNextWindowSizeConstraints(
	  { 64, (float)( (gHeight)-spacingY ) },
	  { ( (float)gWidth ) - ( gMaterialDataWidth + gMaterialListWidth ), (float)( (gHeight)-spacingY ) } );

	static ImVec2 lastSize = ImGui::GetWindowSize();

	ImGui::SetNextWindowPos( { (float)gWidth - ( lastSize.x + ImGui::GetStyle().ItemSpacing.x ), (float)gOffsetY + ( ImGui::GetStyle().ItemSpacing.y * 2 ) } );

	// if ( !ImGui::BeginChild( "Material Preview", {}, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX ) )
	if ( !ImGui::BeginChild( "Material Preview", {}, ImGuiChildFlags_Border ) )
	{
		ImGui::EndChild();
		return;
	}

	ImGui::Text( "Material Preview" );
	ImGui::Separator();

	static ImTextureID missingTex = render->AddTextureToImGui( CH_INVALID_HANDLE );

	ImVec2             size       = ImGui::GetWindowSize();
	lastSize                      = size;
	gMaterialPreviewWidth         = size.x;

	float imageSize               = 96.f;

	ImGui::Image( missingTex, { size.x, size.x } );

	ImGui::Text( "TODO" );

	ImGui::EndChild();
}


// TODO: store extra material data in the cmt file for this editor
// like source texture path, and texture compile options
void MaterialEditor_DrawMaterialData()
{
	ImGui::SetNextWindowSizeConstraints(
	  { 0.f, 0.f },
	  { ( (float)gWidth ) - ( gMaterialListWidth + 64 ), (float)( gHeight ) } );

	if ( !ImGui::BeginChild( "Material Data", {}, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX ) )
	{
		gMaterialDataWidth = ImGui::GetWindowSize().x;
		ImGui::EndChild();
		return;
	}

	gMaterialDataWidth        = ImGui::GetWindowSize().x;

	const char* matPath       = graphics->Mat_GetName( gMatEditor.mat );
	ch_handle_t oldShader     = graphics->Mat_GetShader( gMatEditor.mat );
	const char* oldShaderName = graphics->GetShaderName( oldShader );

	ImGui::Text( matPath );

	// Draw Shader Dropdown
	if ( ImGui::BeginCombo( "Shader", oldShaderName ? oldShaderName : "" ) )
	{
		u32 shaderCount = graphics->GetGraphicsShaderCount();

		for ( u32 i = 0; i < shaderCount; i++ )
		{
			ch_handle_t shader     = graphics->GetGraphicsShaderByIndex( i );
			const char* shaderName = graphics->GetShaderName( shader );

			if ( ImGui::Selectable( shaderName ) )
			{
				graphics->Mat_SetShader( gMatEditor.mat, shader );
			}
		}

		ImGui::EndCombo();
	}

	ch_handle_t shader     = graphics->Mat_GetShader( gMatEditor.mat );
	const char* shaderName = graphics->GetShaderName( shader );

	ImGui::Separator();

	// Get Shader Variable Data
	u32 shaderVarCount = graphics->GetShaderVarCount( shader );

	if ( !shaderVarCount )
	{
		ImGui::Text( "Shader has no material properties" );
		ImGui::EndChild();
		return;
	}

	size_t                 varCount   = graphics->Mat_GetVarCount( gMatEditor.mat );
	ShaderMaterialVarDesc* shaderVars = graphics->GetShaderVars( shader );

	if ( !shaderVars )
	{
		ImGui::Text( "FAILED TO GET SHADER VARS!!!!" );
		ImGui::EndChild();
		return;
	}

	// Shader Booleans
	ImGui::Text( "Booleans" );

	for ( u32 varI = 0; varI < shaderVarCount; varI++ )
	{
		if ( shaderVars[ varI ].type != EMatVar_Bool )
			continue;

		bool value = graphics->Mat_GetBool( gMatEditor.mat, shaderVars[ varI ].name );
		bool out   = value;

		if ( ImGui::BeginChild( shaderVars[ varI ].name, {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY ) )
		{
			if ( ImGui::Checkbox( shaderVars[ varI ].name, &out ) )
			{
				graphics->Mat_SetVar( gMatEditor.mat, shaderVars[ varI ].name, out );
			}

			if ( !shaderVars[ varI ].desc )
				continue;

			ImGui::Text( shaderVars[ varI ].desc );
		}

		ImGui::EndChild();
	}

	ImGui::Spacing();

	u32 imId = 1;

	// maybe make a section for textures only?

	ImGui::TextUnformatted( "Textures" );

	for ( u32 varI = 0; varI < shaderVarCount; varI++ )
	{
		ShaderMaterialVarDesc& shaderVar = shaderVars[ varI ];

		if ( shaderVar.type != EMatVar_Texture )
			continue;

		if ( ImGui::BeginChild( shaderVar.name, {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY ) )
		{
			ch_handle_t texHandle = graphics->Mat_GetTexture( gMatEditor.mat, shaderVar.name );

			float       size      = std::clamp( matedit_texture_size, 2, 8192 );

			ImVec2      texSize   = { size, size };

			auto        texIt     = gMatEditor.imguiTextures.find( texHandle );
			ImTextureID imTex     = 0;

			if ( texIt == gMatEditor.imguiTextures.end() )
			{
				imTex                                 = render->AddTextureToImGui( texHandle );
				gMatEditor.imguiTextures[ texHandle ] = imTex;
			}
			else
			{
				imTex = texIt->second;
			}

			// if ( ImGui::BeginChild( shaderVar.name, {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY ) )
			{
				if ( ImGui::BeginChild( imId++, {}, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY ) )
				{
					ImGui::Image( imTex, texSize );
				}

				ImGui::EndChild();

				ImGui::SameLine();

				// ImGui::SetNextWindowSizeConstraints( { 50, 50 }, { 100000, 100000 } );

				if ( ImGui::BeginChild( imId++, {}, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY ) )
				{
					ImGui::TextUnformatted( shaderVar.desc ? shaderVar.desc : shaderVar.name );

					if ( ImGui::Button( "Select" ) || ( gMatEditor.textureBrowser.open && gMatEditor.textureBrowserVar == varI ) )
					{
						gMatEditor.textureBrowserName       = shaderVar.name;
						gMatEditor.textureBrowserVar        = varI;
						gMatEditor.textureBrowser.open      = true;
						gMatEditor.textureBrowser.filterExt = { ".ktx" };
					}

					ImGui::SameLine();

					bool resetTexture = false;
					if ( ImGui::Button( "Reset" ) )
					{
						resetTexture = true;
					}

					// TODO: show texture import options?
					// Mainly an option to have texture filtering on or off

					TextureInfo_t texInfo = render->GetTextureInfo( texHandle );

					ImGui::Text( texInfo.name.data ? texInfo.name.data : texInfo.aPath.data );

					if ( ImGui::IsItemHovered() )
					{
						if ( ImGui::BeginTooltip() )
							Editor_DrawTextureInfo( texInfo );

						ImGui::EndTooltip();
					}

					// shaderVar.name should be the same memory as textureBrowserName for now
					if ( gMatEditor.textureBrowser.output == EFilePickerReturn_SelectedItems && shaderVar.name == gMatEditor.textureBrowserName )
					{
						gMatEditor.textureBrowser.output = EFilePickerReturn_None;

						if ( texHandle && texHandle != shaderVar.defaultTextureHandle )
						{
							render->FreeTextureFromImGui( texHandle );
							render->FreeTexture( texHandle );
							gMatEditor.imguiTextures.erase( texHandle );
						}

						TextureCreateData_t createInfo{};
						createInfo.aUsage      = EImageUsage_Sampled;

						ch_handle_t newTexture = CH_INVALID_HANDLE;
						render->LoadTexture( newTexture, gMatEditor.textureBrowser.selectedItems[ 0 ], createInfo );
						graphics->Mat_SetVar( gMatEditor.mat, shaderVar.name, newTexture );

						texHandle                    = newTexture;

						gMatEditor.textureBrowserVar = UINT32_MAX;
					}

					if ( resetTexture )
					{
						if ( texHandle && texHandle != shaderVar.defaultTextureHandle )
						{
							render->FreeTextureFromImGui( texHandle );
							render->FreeTexture( texHandle );
							gMatEditor.imguiTextures.erase( texHandle );
						}

						graphics->Mat_SetVar( gMatEditor.mat, shaderVar.name, shaderVar.defaultTextureHandle );
					}
				}

				ImGui::EndChild();
			}
		}

		ImGui::EndChild();
	}

	ImGui::TextUnformatted( "Variables" );

	// Shader Variables
	for ( u32 varI = 0; varI < shaderVarCount; varI++ )
	{
		ShaderMaterialVarDesc& shaderVar = shaderVars[ varI ];

		if ( shaderVar.type == EMatVar_Invalid || shaderVar.type == EMatVar_Bool || shaderVar.type == EMatVar_Texture )
			continue;

		if ( ImGui::BeginChild( shaderVar.name, {}, ImGuiChildFlags_Border | ImGuiChildFlags_AutoResizeY ) )
		{
			ImGui::TextUnformatted( shaderVar.desc ? shaderVar.desc : shaderVar.name );

			switch ( shaderVar.type )
			{
				default:
				case EMatVar_Invalid:
				case EMatVar_Bool:
				case EMatVar_Texture:
					continue;

				case EMatVar_Float:
				{
					float value = graphics->Mat_GetFloat( gMatEditor.mat, shaderVar.name );

					if ( ImGui::DragFloat( shaderVar.name, &value, 0.05f ) )
					{
						graphics->Mat_SetVar( gMatEditor.mat, shaderVar.name, value );
					}

					break;
				}

				case EMatVar_Int:
				{
					int value = graphics->Mat_GetInt( gMatEditor.mat, shaderVar.name );

					if ( ImGui::DragInt( shaderVar.name, &value ) )
					{
						graphics->Mat_SetVar( gMatEditor.mat, shaderVar.name, value );
					}

					break;
				}

				case EMatVar_Vec2:
				{
					glm::vec2 value = graphics->Mat_GetVec2( gMatEditor.mat, shaderVar.name );

					if ( ImGui::DragScalarN( shaderVar.name, ImGuiDataType_Float, &value.x, 2, 0.25f, nullptr, nullptr, nullptr, 1.f ) )
					{
						graphics->Mat_SetVar( gMatEditor.mat, shaderVar.name, value );
					}

					break;
				}

				case EMatVar_Vec3:
				{
					glm::vec3 value = graphics->Mat_GetVec3( gMatEditor.mat, shaderVar.name );

					if ( ImGui::DragScalarN( shaderVar.name, ImGuiDataType_Float, &value.x, 3, 0.25f, nullptr, nullptr, nullptr, 1.f ) )
					{
						graphics->Mat_SetVar( gMatEditor.mat, shaderVar.name, value );
					}

					break;
				}

				case EMatVar_Vec4:
				{
					glm::vec4 value = graphics->Mat_GetVec4( gMatEditor.mat, shaderVar.name );

					if ( ImGui::DragScalarN( shaderVar.name, ImGuiDataType_Float, &value.x, 4, 0.25f, nullptr, nullptr, nullptr, 1.f ) )
					{
						graphics->Mat_SetVar( gMatEditor.mat, shaderVar.name, value );
					}

					break;
				}
			}
		}

		ImGui::EndChild();
	}

	ImGui::EndChild();
}


void MaterialEditor_Draw( glm::uvec2 sOffset )
{
	render->GetSurfaceSize( gGraphicsWindow, gWidth, gHeight );

	MaterialEditor_DrawMenuBar();
	gMainMenuBarHeight = ImGui::GetItemRectSize().y;

	gOffsetY           = sOffset.y + gMainMenuBarHeight;

	gWidth -= sOffset.x;
	gHeight -= gOffsetY;

	if ( gMatEditor.textureBrowser.open )
	{
		FilePicker_Draw( gMatEditor.textureBrowser, gMatEditor.textureBrowserName );
		return;
	}

	if ( gMatEditor.drawNewDialog )
	{
		MaterialEditor_DrawNewDialog();
	}

	MaterialEditor_DrawOpenMaterials();

	ImGui::SameLine();

	if ( gMatEditor.mat == CH_INVALID_HANDLE )
	{
		ImGui::Text( "No Material Selected" );
		return;
	}

	MaterialEditor_DrawMaterialData();
	ImGui::SameLine();
	MaterialEditor_DrawMaterialPreview();
}


void MaterialEditor_CloseMaterial( ch_handle_t mat )
{
	MaterialEditor_FreeImGuiTextures();
}


void MaterialEditor_FreeMaterial( ch_handle_t mat )
{
	MaterialEditor_CloseMaterial( mat );
	graphics->FreeMaterial( mat );
}


bool MaterialEditor_LoadMaterial( const std::string& path )
{
	ch_handle_t mat = graphics->LoadMaterial( path.data(), path.size() );

	if ( mat == CH_INVALID_HANDLE )
		return false;

	// Is this already loaded?
	for ( MaterialEntry& matEntry : gMatEditor.materialList )
	{
		if ( matEntry.mat == mat )
		{
			// decrement the ref count
			graphics->FreeMaterial( mat );
			MaterialEditor_SetActiveMaterial( matEntry.mat );
			return true;
		}
	}

	ch_string_auto fileNameNoExt = FileSys_GetFileNameNoExt( path.data(), path.size() );

	MaterialEntry& matEntry      = gMatEditor.materialList.emplace_back();
	matEntry.mat                 = mat;
	matEntry.path                = path;
	matEntry.name                = std::string( fileNameNoExt.data, fileNameNoExt.size );

	MaterialEditor_SetActiveMaterial( mat );

	return true;
}


void MaterialEditor_SetActiveMaterial( ch_handle_t sMat )
{
	if ( gMatEditor.mat )
	{
		MaterialEditor_CloseMaterial( gMatEditor.mat );
	}

	gMatEditor.mat = sMat;
}


ch_handle_t MaterialEditor_GetActiveMaterial()
{
	return gMatEditor.mat;
}


void MaterialEditor_FreeImGuiTextures()
{
	for ( auto& [ texture, imTexture ] : gMatEditor.imguiTextures )
	{
		render->FreeTextureFromImGui( texture );
	}

	gMatEditor.imguiTextures.clear();
}


void MaterialEditor_Init()
{
	MaterialEditor_Close();

	gMatEditor.textureBrowser.filterExt.clear();
	gMatEditor.textureBrowser.filterExt.push_back( ".ktx" );
	gMatEditor.textureBrowser.open        = false;
	gMatEditor.textureBrowser.childWindow = true;
	gMatEditor.textureBrowserVar          = UINT32_MAX;
	gMatEditor.mat                        = CH_INVALID_HANDLE;
}


void MaterialEditor_Close()
{
	for ( MaterialEntry& matEntry : gMatEditor.materialList )
	{
		graphics->FreeMaterial( matEntry.mat );
	}

	gMatEditor.materialList.clear();

	MaterialEditor_FreeImGuiTextures();
}
