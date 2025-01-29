#include "entity_editor.h"
#include "entity.h"
#include "main.h"
#include "file_picker.h"
#include "skybox.h"
// #include "importer.h"
#include "inputsystem.h"
#include "gizmos.h"

#include "imgui/imgui.h"

#include <unordered_set>


FilePickerData_t      gModelBrowserData{};
FilePickerData_t      gImporterFilePicker{};
FilePickerData_t      gSkyboxFilePicker{};

// keeping in case i want to have the asset browser here as well
glm::ivec2            gAssetBrowserSize{};
float                 gAssetBrowserOffset         = 0.f;


constexpr const char* CH_EDITOR_MODEL_LIGHT_CONE  = "dev/light_cone.glb";
constexpr const char* CH_EDITOR_MODEL_LIGHT_POINT = "dev/light_point.glb";
constexpr const char* CH_EDITOR_MODEL_GIZMO_POS   = "dev/axis.obj";
constexpr const char* CH_EDITOR_MODEL_GIZMO_ANG   = "";
constexpr const char* CH_EDITOR_MODEL_GIZMO_SCALE = "";


EditorRenderables     gEditorRenderables;


CONVAR_FLOAT( editor_gizmo_scale, 0.006, CVARF_ARCHIVE, "Scale of the Editor Gizmos" );
CONVAR_BOOL( editor_gizmo_scale_enabled, 1, "Enable Editor Gizmo Scaling" );


// adds the entity to the selection list, making sure it's not in the list multiple times
void EntEditor_AddToSelection( EditorContext_t* context, ch_handle_t entity )
{
	u32 index = context->aEntitiesSelected.index( entity );
	if ( index == UINT32_MAX )	
		context->aEntitiesSelected.push_back( entity );
}


void EntEditor_LoadEditorRenderable( ChVector< const char* >& failList, ch_handle_t& handle, SelectionRenderable& select, const char* path )
{
	ch_handle_t model = graphics->LoadModel( path );

	if ( !model )
	{
		failList.push_back( path );
		return;
	}

	handle                   = graphics->CreateRenderable( model );
	select.renderable        = handle;

	// Setup the selection renderable colors
	Renderable_t* renderable = graphics->GetRenderableData( handle );

	select.colors.resize( renderable->aMaterialCount );

	for ( u32 color : select.colors )
	{
		color = ( rand_u8( 0, 255 ) << 16 | rand_u8( 0, 255 ) << 8 | rand_u8( 0, 255 ) );
	}
}


bool EntEditor_Init()
{
	// Setup Filters for File Pickers
	gModelBrowserData.filterExt = { ".obj", ".glb", ".gltf" };
	gSkyboxFilePicker.filterExt = { ".cmt", };

	// Load Editor Models
	ChVector< const char* > failed;

	// EntEditor_LoadEditorRenderable( failed, gEditorRenderables.gizmoTranslation, gEditorRenderables.selectTranslation, CH_EDITOR_MODEL_GIZMO_POS );
	// EntEditor_LoadEditorRenderable( failed, gEditorRenderables.gizmoRotation, CH_EDITOR_MODEL_GIZMO_ANG );
	// EntEditor_LoadEditorRenderable( failed, gEditorRenderables.gizmoScale, CH_EDITOR_MODEL_GIZMO_SCALE );

	if ( failed.size() )
	{
		log_t group = Log_GroupBeginEx( 0, ELogType_Fatal );
		Log_Group( group, "Failed to load editor models:\n" );

		for ( const char* model : failed )
		{
			Log_GroupF( group, "%s\n", model );
		}

		Log_GroupEnd( group );
		return false;
	}

	// setup materials for the gizmo
	// ch_handle_t shader = graphics->GetShader( "debug" );
	// 
	// ch_handle_t matGizmoX = graphics->CreateMaterial( "gizmo_x", shader );
	// ch_handle_t matGizmoY = graphics->CreateMaterial( "gizmo_y", shader );
	// ch_handle_t matGizmoZ = graphics->CreateMaterial( "gizmo_z", shader );

	if ( !Gizmo_Init() )
		return false;

	return true;
}


void EntEditor_Shutdown()
{
}


void EntEditor_Update( float sFrameTime )
{
}


void EntEditor_DrawImporter()
{
#if 0
	if ( ImGui::Button( "Import File" ) )
	{
		gImporterFilePicker.open = true;
	}

	if ( gImporterFilePicker.open )
	{
		EFilePickerReturn status = FilePicker_Draw( gImporterFilePicker, "Import File" );

		if ( status == EFilePickerReturn_SelectedItems )
		{
			ImportSettings importSettings{};
			importSettings.inputFile = gImporterFilePicker.selectedItems[ 0 ];
			ch_handle_t importHandle = Importer_StartImport( importSettings );
		}
	}
#endif
}


static std::unordered_map< ch_handle_t, ImTextureID >   gImGuiTextures;
static std::unordered_map< ch_handle_t, TextureInfo_t > gTextureInfo;
int                                                    gTextureListViewMode = 0;


void Editor_DrawTextureInfo( TextureInfo_t& info )
{
	ImGui::Text( "Name: %s", info.name.size ? info.name.data : "UNNAMED" );

	if ( info.aPath.size )
		ImGui::TextUnformatted( info.aPath.data );

	ImGui::Text( "%d x %d - %.6f MB", info.aSize.x, info.aSize.y, ch_bytes_to_mb( info.aMemoryUsage ) );
	ImGui::TextUnformatted( "Format: TODO" );
	ImGui::TextUnformatted( "Mip Levels: TODO" );
	ImGui::Text( "GPU Index: %d", info.aGpuIndex );
	ImGui::Text( "Ref Count: %d", info.aRefCount );
}


void Editor_DrawTextureList()
{
	// Draws all currently loaded textures
	std::vector< ch_handle_t > textures   = render->GetTextureList();
	ImVec2                 windowSize = ImGui::GetWindowSize();

	glm::vec2              imageDisplaySize = { 96, 96 };

	if ( ImGui::BeginCombo( "View Type", gTextureListViewMode == 0 ? "List" : "Icons" ) )
	{
		if ( ImGui::Selectable( "List" ) )
			gTextureListViewMode = 0;

		if ( ImGui::Selectable( "Icons" ) )
			gTextureListViewMode = 1;

		ImGui::EndCombo();
	}

	// TODO: add a search bar?

	// TODO: use pages, and show only 100 textures per page, and have only 100 loaded for imgui not to freak out

	bool wrapIconList      = false;
	int  currentImageWidth = 0;
	int  imagesInRow       = 0;

	u32  memoryUsage       = 0;
	u32  rtMemoryUsage     = 0;

	for ( ch_handle_t texture : textures )
	{
		TextureInfo_t info = render->GetTextureInfo( texture );

		if ( info.aRenderTarget )
			rtMemoryUsage += info.aMemoryUsage;
		else
			memoryUsage += info.aMemoryUsage;
	}

	ImGui::Text(
	  "Count: %d | Memory: %.4f MB | Render Target Memory: %.4f MB",
	  textures.size(), ch_bytes_to_mb( memoryUsage ), ch_bytes_to_mb( rtMemoryUsage ) );

	if ( !ImGui::BeginChild( "Texture List" ) )
	{
		ImGui::EndChild();
		return;
	}

	for ( ch_handle_t texture : textures )
	{
		ImTextureID imTexture = 0;

		auto it = gImGuiTextures.find( texture );
		if ( it == gImGuiTextures.end() )
		{
			imTexture                 = render->AddTextureToImGui( texture );
			gImGuiTextures[ texture ] = imTexture;
		}
		else
		{
			imTexture = it->second;
		}

		if ( imTexture == nullptr )
			continue;

		if ( gTextureListViewMode == 0 )
		{
			TextureInfo_t info = render->GetTextureInfo( texture );

			if ( ImGui::BeginChild( (int)imTexture, { windowSize.x - 16, imageDisplaySize.y + 16 }, ImGuiChildFlags_Border, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
			{
				ImGui::Image( imTexture, { imageDisplaySize.x, imageDisplaySize.y } );

				ImGui::SameLine();

				if ( ImGui::BeginChild( (int)imTexture, { windowSize.x - 16 - imageDisplaySize.x, imageDisplaySize.y + 16 }, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
				{
					Editor_DrawTextureInfo( info );
				}

				ImGui::EndChild();
			}

			if ( ImGui::IsItemHovered() )
			{
				if ( ImGui::BeginTooltip() )
					Editor_DrawTextureInfo( info );

				ImGui::EndTooltip();
			}

			ImGui::EndChild();
		}
		else
		{
			ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 2, 2 } );

			if ( windowSize.x < currentImageWidth + imageDisplaySize.x + ( imagesInRow * 2 ) )  // imagesInRow is for padding
			{
				currentImageWidth = 0;
				imagesInRow       = 0;
			}
			else
			{
				ImGui::SameLine();
			}

			ImGui::PushStyleVar( ImGuiStyleVar_ChildBorderSize, 0 );

			if ( ImGui::BeginChild( (int)imTexture, { imageDisplaySize.x, imageDisplaySize.y }, 0, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse ) )
			{
				ImGui::Image( imTexture, { imageDisplaySize.x, imageDisplaySize.y } );

				if ( ImGui::IsItemHovered() )
				{
					if ( ImGui::BeginTooltip() )
					{
						TextureInfo_t info = render->GetTextureInfo( texture );
						Editor_DrawTextureInfo( info );
					}

					ImGui::EndTooltip();
				}
			}

			ImGui::EndChild();

			ImGui::PopStyleVar();
			ImGui::PopStyleVar();

			currentImageWidth += imageDisplaySize.x;
			imagesInRow++;
		}
	}
	
	ImGui::EndChild();
}


void Editor_DrawAssetList()
{
	if ( ImGui::BeginTabBar( "editor tabs" ) )
	{
		if ( ImGui::BeginTabItem( "Texture List" ) )
		{
			Editor_DrawTextureList();
			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Model List" ) )
		{
			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Material List" ) )
		{
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}


void EntEditor_DrawLightUI( Entity_t* spEntity )
{
	if ( !ImGui::BeginChild( "Light Data" ) )
	{
		ImGui::EndChild();
		return;
	}

	bool updateLight = false;

	updateLight |= ImGui::Checkbox( "Enabled", &spEntity->aLightEnabled );
	updateLight |= ImGui::ColorEdit4( "Color", &spEntity->apLight->color.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR );

	if ( spEntity->apLight->aType == ELightType_Directional )
	{
		// updateLight |= ImGui::SliderFloat( "Rotation X", &spEntity->apLight->aAng.x, -180, 180 );
		// updateLight |= ImGui::SliderFloat( "Rotation Y", &spEntity->apLight->aAng.y, -180, 180 );
		// updateLight |= ImGui::SliderFloat( "Rotation Z", &spEntity->apLight->aAng.z, -180, 180 );
	}
	else if ( spEntity->apLight->aType == ELightType_Point )
	{
		updateLight |= ImGui::SliderFloat( "Radius", &spEntity->apLight->aRadius, 0, 1000 );
	}
	else if ( spEntity->apLight->aType == ELightType_Cone )
	{
		// updateLight |= ImGui::DragScalarN( "Position", ImGuiDataType_Float, &spEntity->apLight->aPos.x, 3, 1.f, nullptr, nullptr, nullptr, 1.f );

		// updateLight |= ImGui::SliderFloat( "Rotation X", &spEntity->apLight->aAng.x, -180, 180 );
		// updateLight |= ImGui::SliderFloat( "Rotation Y", &spEntity->apLight->aAng.y, -180, 180 );
		// updateLight |= ImGui::SliderFloat( "Rotation Z", &spEntity->apLight->aAng.z, -180, 180 );

		updateLight |= ImGui::SliderFloat( "Inner FOV", &spEntity->apLight->aInnerFov, 0, 180 );
		updateLight |= ImGui::SliderFloat( "Outer FOV", &spEntity->apLight->aOuterFov, 0, 180 );

		updateLight |= ImGui::Checkbox( "Shadows", &spEntity->apLight->aShadow );
	}
	//else if ( spEntity->apLight->aType == ELightType_Capsule )
	//{
	//}

	// if ( updateLight )
		graphics->UpdateLight( spEntity->apLight );

	ImGui::EndChild();
}


#define NAME_LEN 64


void EntEditor_DrawBasicMaterialData( Renderable_t* renderable, u32 matI )
{
	// if ( !ImGui::BeginChild( matI + 1, {}, ImGuiChildFlags_Border ) )
	// if ( !ImGui::BeginChild( matI + 1, {} ) )

	ch_handle_t  mat     = renderable->apMaterials[ matI ];
	const char* matPath = graphics->Mat_GetName( mat );

	//ImGui::PushID( matI + 1 );

	//if ( !ImGui::TreeNode( matPath ) )
	//{
	//	ImGui::TreePop();
	//	return;
	//}

	ImVec2 size = ImGui::GetWindowSize();

	//if ( !ImGui::BeginChild( "##", { 0, 40 }, ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY, ImGuiWindowFlags_NoSavedSettings ) )
	{
		//ImGui::EndChild();
		////ImGui::TreePop();
		//ImGui::PopID();
		//return;
	}

	ImGui::TextUnformatted( matPath );

	// ImGui::SameLine();
	// 
	// if ( ImGui::Button( "Edit" ) )
	// {
	// 	// Focus Material Editor and Show this material
	// 	MaterialEditor_SetMaterial( mat );
	// }
	// 
	// ImGui::Separator();

	// TODO: use imgui tables


	// ImGui::BeginChild( "ConVar List Child", ImVec2( 0, -ImGui::GetFrameHeightWithSpacing() ), true );

#if 0
	if ( ImGui::BeginTable( matPath, 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable ) )
	{
		// Draw Shader
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex( 0 );

		ImGui::Text( "shader" );

		ImGui::TableSetColumnIndex( 1 );
		ch_handle_t shader = graphics->Mat_GetShader( mat );

		const char* shaderName = graphics->GetShaderName( shader );

		ImGui::Text( shaderName ? shaderName : "INVALID" );

		size_t varCount = graphics->Mat_GetVarCount( mat );

		for ( u32 varI = 0; varI < varCount; varI++ )
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex( 0 );


			// TODO: probably expose MaterialVar directly? this re-accessing the data a lot
			const char* varName    = graphics->Mat_GetVarName( mat, varI );
			EMatVar     matVarType = graphics->Mat_GetVarType( mat, varI );

			ImGui::Text( varName ? varName : "ERROR NAME" );

			ImGui::TableSetColumnIndex( 1 );

			switch ( matVarType )
			{
				default:
				case EMatVar_Invalid:
					break;

				case EMatVar_Texture:
				{
					ch_handle_t    texHandle = graphics->Mat_GetTexture( mat, varI );
					TextureInfo_t texInfo   = render->GetTextureInfo( texHandle );

					ImGui::Text( texInfo.name.empty() ? texInfo.aPath.c_str() : texInfo.name.c_str() );

					if ( ImGui::IsItemHovered() )
					{
						if ( ImGui::BeginTooltip() )
							Editor_DrawTextureInfo( texInfo );

						ImGui::EndTooltip();
					}

					break;
				}

				case EMatVar_Float:
				{
					float value = graphics->Mat_GetFloat( mat, varI );
					ImGui::Text( "%.6f", value );
					break;
				}

				case EMatVar_Int:
				{
					int value = graphics->Mat_GetInt( mat, varI );
					ImGui::Text( "%d", value );
					break;
				}

				case EMatVar_Bool:
				{
					bool value = graphics->Mat_GetBool( mat, varI );
					ImGui::Text( value ? "True" : "False" );
					break;
				}

				case EMatVar_Vec2:
				{
					glm::vec2 value = graphics->Mat_GetVec2( mat, varI );
					ImGui::Text( "X: %.6f, Y: %.6f", value.x, value.y );
					break;
				}

				case EMatVar_Vec3:
				{
					glm::vec3 value = graphics->Mat_GetVec3( mat, varI );
					ImGui::Text( "X: %.6f, Y: %.6f, Z: %.6f", value.x, value.y, value.z );
					break;
				}

				case EMatVar_Vec4:
				{
					glm::vec4 value = graphics->Mat_GetVec4( mat, varI );
					ImGui::Text( "X: %.6f, Y: %.6f, Z: %.6f, W: %.6f", value.x, value.y, value.z, value.w );
					break;
				}
			}

			//if ( varI + 1 < varCount )
			//	ImGui::Separator();
		}

	}

	ImGui::EndTable();
#endif

	//size_t varCount = graphics->Mat_GetVarCount( mat );
	//
	//for ( u32 varI = 0; varI < varCount; varI++ )
	//{
	//	// TODO: probably expose MaterialVar directly? this re-accessing the data a lot
	//	const char* varName = graphics->Mat_GetVarName( mat, varI );
	//	EMatVar matVarType  = graphics->Mat_GetVarType( mat, varI );
	//
	//	ImGui::Text( varName ? varName : "ERROR NAME" );
	//
	//	ImGui::SameLine();
	//
	//	switch ( matVarType )
	//	{
	//		default:
	//		case EMatVar_Invalid:
	//			break;
	//
	//		case EMatVar_Texture:
	//		{
	//			ch_handle_t texHandle  = graphics->Mat_GetTexture( mat, varI );
	//			TextureInfo_t texInfo = render->GetTextureInfo( texHandle );
	//
	//			ImGui::Text( texInfo.name.empty() ? texInfo.aPath.c_str() : texInfo.name.c_str() );
	//
	//			break;
	//		}
	//
	//		case EMatVar_Float:
	//			ImGui::Text( "Float" );
	//			break;
	//
	//		case EMatVar_Int:
	//			ImGui::Text( "Int" );
	//			break;
	//
	//		case EMatVar_Bool:
	//			ImGui::Text( "Bool" );
	//			break;
	//
	//		case EMatVar_Vec2:
	//			ImGui::Text( "Vec2" );
	//			break;
	//
	//		case EMatVar_Vec3:
	//			ImGui::Text( "Vec3" );
	//			break;
	//
	//		case EMatVar_Vec4:
	//			ImGui::Text( "Vec4" );
	//			break;
	//	}
	//
	//	if ( varI + 1 < varCount )
	//		ImGui::Separator();
	//}

	//ImGui::EndChild();
	/// ImGui::TreePop();
	//ImGui::PopID();

	// if ( matI + 1 < renderable->aMaterialCount )
	// 	ImGui::Separator();
}


void EntEditor_DrawRenderableUI( Entity_t* spEntity )
{
	if ( spEntity->aModel )
	{
		std::string_view modelPath = graphics->GetModelPath( spEntity->aModel );
		ImGui::TextUnformatted( modelPath.data() );

		if ( ImGui::IsItemHovered() )
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted( modelPath.data() );
			ImGui::EndTooltip();
		}
	}

	if ( ImGui::Button( "Load Model" ) )
	{
		gModelBrowserData.open = true;

		if ( spEntity->aModel )
		{
			std::string_view modelPath = graphics->GetModelPath( spEntity->aModel );
			gModelBrowserData.path     = FileSys_GetDirName( modelPath.data(), modelPath.size() );
		}
		else
		{
			gModelBrowserData.path = FileSys_GetExePath();
		}
	}

	ImGui::SameLine();

	if ( ImGui::Button( "Clear Model" ) )
	{

	}

	if ( spEntity->aRenderable )
	{
		Renderable_t* renderable = graphics->GetRenderableData( spEntity->aRenderable );

		ImGui::Checkbox( "Visible", &renderable->aVisible );
		ImGui::Checkbox( "Test Visibility", &renderable->aTestVis );
		ImGui::Checkbox( "Cast Shadows", &renderable->aCastShadow );

		// List Materials
		if ( ImGui::CollapsingHeader( "Materials" ) )
		{
			ImGui::Text( "Materials: %d", renderable->aMaterialCount );
			for ( u32 matI = 0; matI < renderable->aMaterialCount; matI++ )
			{
				EntEditor_DrawBasicMaterialData( renderable, matI );
			}
		}

		// Do we have blend shapes?
		if ( ImGui::CollapsingHeader( "Blend Shapes" ) && renderable->aBlendShapeWeights.size() )
		{
			bool resetBlendShapes = ImGui::Button( "Reset Blend Shapes" );

			// Display Them (TODO: names)
			if ( ImGui::BeginChild( "Blend Shapes", {}, ImGuiChildFlags_ResizeY ) )
			{
				u32 imguiID = 0;
				for ( u32 i = 0; i < renderable->aBlendShapeWeights.size(); i++ )
				{
					ImGui::PushID( imguiID++ );

					if ( ImGui::SliderFloat( "##blend_shape", &renderable->aBlendShapeWeights[ i ], -1.f, 4.f, "%.4f", 1.f ) )
					{
						renderable->aBlendShapesDirty = true;
					}

					ImGui::PopID();
					ImGui::SameLine();
					ImGui::PushID( imguiID++ );

					if ( ImGui::Button( "Reset" ) || resetBlendShapes )
					{
						renderable->aBlendShapeWeights[ i ] = 0.f;
						renderable->aBlendShapesDirty       = true;
					}

					ImGui::PopID();
				}

				ImGui::EndChild();
			}
		}
	}
}


void EntEditor_DrawEntityData()
{
	EditorContext_t* context = Editor_GetContext();

	if ( context == nullptr )
		return;

	if ( context->aEntitiesSelected.empty() )
	{
		ImGui::TextUnformatted( "No Entity Selected" );
		return;
	}

	if ( context->aEntitiesSelected.size() > 1 )
	{
		ImGui::Text( "%d Entities Selected", context->aEntitiesSelected.size() );

		for ( ch_handle_t entityHandle : context->aEntitiesSelected )
		{
			Entity_t* entity = Entity_GetData( entityHandle );
			if ( entity == nullptr )
			{
				Log_ErrorF( "Selected Entity is nullptr?\n" );
				context->aEntitiesSelected.clear();
				return;
			}

			ImGui::TextUnformatted( entity->name.data, &entity->name.data[ entity->name.size ] );
		}

		return;
	}

	ch_handle_t selectedEntity = context->aEntitiesSelected[ 0 ];
	Entity_t*  entity         = Entity_GetData( selectedEntity );

	if ( entity == nullptr )
	{
		Log_ErrorF( "Selected Entity is nullptr?\n" );
		context->aEntitiesSelected.clear();
		return;
	}

	ImGui::InputText( "Name", entity->name.data, NAME_LEN );

	// std::string entName = vstring( "Entity %zd", gSelectedEntity );
	// ImGui::Text( entName.data() );

	// -------------------------------------------------------------------------------------
	// Entity Parenting

	ch_handle_t  parent     = Entity_GetParent( selectedEntity );
	std::string parentName = "None";

	if ( parent )
	{
		Entity_t* parentData = Entity_GetData( parent );
		parentName = parentData->name.data ? parentData->name.data : vstring( "Entity %zd", parent );
	}
	
	if ( ImGui::BeginCombo( "Set Parent", parentName.c_str() ) )
	{
		if ( ImGui::Selectable( "Clear Parent" ) )
		{
			Entity_SetParent( selectedEntity, CH_INVALID_HANDLE );
		}

		// Can't parent it to any of these
		std::unordered_set< ch_handle_t > children;
		Entity_GetChildrenRecurse( selectedEntity, children );

		const ChVector< ch_handle_t >& entityHandles = context->aMap.aMapEntities;

		for ( ch_handle_t entityHandle : entityHandles )
		{
			if ( Entity_GetParent( selectedEntity ) == entityHandle || selectedEntity == entityHandle )
				continue;

			// Make sure this isn't one of our children
			auto it = children.find( entityHandle );

			// it is, we can't parent to a child, continue
			if ( it != children.end() )
				continue;

			Entity_t* entityToParent = Entity_GetData( entityHandle );
			if (entityToParent == nullptr )
			{
				Log_ErrorF( "Entity is nullptr????\n" );
				continue;
			}
	
			std::string entName = vstring( "Entity %zd", entityHandle );

			ImGui::PushID( entityHandle );
	
			if ( ImGui::Selectable( entityToParent->name.data ? entityToParent->name.data : entName.data() ) )
			{
				Entity_SetParent( selectedEntity, entityHandle );
			}
	
			ImGui::PopID();
		}

		ImGui::EndCombo();
	}

	ImGui::Separator();

	// -------------------------------------------------------------------------------------
	// Entity Transform

	bool transformChanged = false;

	transformChanged |= ImGui::DragScalarN( "Position", ImGuiDataType_Float, &entity->aTransform.aPos.x, 3, 0.0025f, nullptr, nullptr, nullptr, 1.f );
	transformChanged |= ImGui::DragScalarN( "Angle", ImGuiDataType_Float, &entity->aTransform.aAng.x, 3, 0.25f, nullptr, nullptr, nullptr, 1.f );
	transformChanged |= ImGui::DragScalarN( "Scale", ImGuiDataType_Float, &entity->aTransform.aScale.x, 3, 0.001f, nullptr, nullptr, nullptr, 1.f );

	if ( transformChanged )
	{
		Entity_SetEntitiesDirty( &context->aEntitiesSelected[ 0 ], 1 );
	}

	ImGui::Separator();

	// -------------------------------------------------------------------------------------
	// Entity Model/Renderable

	if ( ImGui::CollapsingHeader( "Renderable", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding ) )
	{
		EntEditor_DrawRenderableUI( entity );
	}

	if ( gModelBrowserData.open )
	{
		// update model for renderable
		EFilePickerReturn status = FilePicker_Draw( gModelBrowserData, "Select Renderable Model" );

		if ( status == EFilePickerReturn_SelectedItems )
		{
			gModelBrowserData.open = false;

			ch_handle_t model       = graphics->LoadModel( gModelBrowserData.selectedItems[ 0 ] );

			if ( model == CH_INVALID_HANDLE )
			{
				Log_ErrorF( "Invalid Model: %s\n", gModelBrowserData.selectedItems[ 0 ].data() );
			}
			else
			{
				if ( entity->aModel )
					graphics->FreeModel( entity->aModel );

				entity->aModel = model;

				if ( entity->aRenderable )
				{
					Renderable_t* renderable = graphics->GetRenderableData( entity->aRenderable );
					graphics->SetRenderableModel( entity->aRenderable, model );
				}
				else
				{
					entity->aRenderable = graphics->CreateRenderable( entity->aModel );
				}

				// Set Colors
				Renderable_t* renderable = graphics->GetRenderableData( entity->aRenderable );
				entity->aMaterialColors.resize( renderable->aMaterialCount );

				for ( int i = 0; i < entity->aMaterialColors.size(); i++ )
				{
					entity->aMaterialColors[ i ].r = rand_u8( 0, 255 );
					entity->aMaterialColors[ i ].g = rand_u8( 0, 255 );
					entity->aMaterialColors[ i ].b = rand_u8( 0, 255 );
				}

				Entity_SetEntitiesDirty( &context->aEntitiesSelected[ 0 ], 1 );
			}
		}
	}

	// -------------------------------------------------------------------------------------
	// Audio

	// -------------------------------------------------------------------------------------
	// Physics Model
	// entity->aPhysicsModel;

	// -------------------------------------------------------------------------------------
	// Light Editing

	if ( ImGui::CollapsingHeader( "Light", ImGuiTreeNodeFlags_DefaultOpen ) )
	{
		if ( entity->apLight )
		{
			//ImGui::SameLine();

			if ( ImGui::Button( "Destroy Light" ) )
			{
				graphics->DestroyLight( entity->apLight );
				entity->apLight = nullptr;

				Entity_SetEntitiesDirty( &context->aEntitiesSelected[ 0 ], 1 );
			}
			else
			{
				EntEditor_DrawLightUI( entity );
			}
		}
		else
		{
			if ( ImGui::BeginCombo( "Create Light", "Point Light" ) )
			{
				if ( ImGui::Selectable( "Point Light" ) )
				{
					entity->apLight          = graphics->CreateLight( ELightType_Point );
					entity->apLight->color  = { 1, 1, 1, 10 };
					entity->apLight->aRadius = 500;

					Entity_SetEntitiesDirty( &context->aEntitiesSelected[ 0 ], 1 );
				}
				else if ( ImGui::Selectable( "Cone Light" ) )
				{
					entity->apLight         = graphics->CreateLight( ELightType_Cone );
					entity->apLight->color = { 1, 1, 1, 10 };

					// weird stuff to get the angle of the light correct from the player's view matrix stuff
					// light->aAng.x = playerWorldTransform.aAng.z;
					// light->aAng.y = -playerWorldTransform.aAng.y;
					// light->aAng.z = -playerWorldTransform.aAng.x + 90.f;

					entity->apLight->aInnerFov = 0.f;  // FOV
					entity->apLight->aOuterFov = 45.f;  // FOV

					Entity_SetEntitiesDirty( &context->aEntitiesSelected[ 0 ], 1 );
				}
				else if ( ImGui::Selectable( "World Light" ) )
				{
					entity->apLight = graphics->CreateLight( ELightType_Directional );

					Entity_SetEntitiesDirty( &context->aEntitiesSelected[ 0 ], 1 );
				}
				// else if ( ImGui::Selectable( "Capsule Light" ) )
				// {
				// }

				ImGui::EndCombo();
			}
		}
	}

	// Entity Components?
	
	// ImGui::End();

#if 0
	if ( !ImGui::Begin( "Entity Component List" ) )
	{
		ImGui::End();
		return;
	}

	if ( gSelectedEntity == CH_INVALID_HANDLE )
	{
		ImGui::End();
		return;
	}

	// TODO: is there anything in imgui that would allow me to search through the components list, and have an optional combo box?
	// you might be able to hack it together if you add in an arrow button yourself and InputText()
	if ( ImGui::BeginCombo( "Add Component", "transform" ) )
	{
		for ( auto& [ name, regData ] : GetEntComponentRegistry().aComponentNames )
		{
			if ( GetEntitySystem()->HasComponent( gSelectedEntity, name.data() ) )
				continue;

			if ( ImGui::Selectable( name.data() ) )
			{
				GetEntitySystem()->AddComponent( gSelectedEntity, name.data() );
			}
		}

		ImGui::EndCombo();
	}

	ImGui::Separator();

	// go through the component registry and check to see if the entity has this component (probably slow af)
	size_t imguiID = 0;

	if ( ImGui::BeginChild( "Entity Component Data" ) )
	{
		for ( auto& [name, regData] : GetEntComponentRegistry().aComponentNames )
		{
			void* compData = GetEntitySystem()->GetComponent( gSelectedEntity, name.data() );

			if ( !compData )
				continue;

			// TreeNode

			IEntityComponentSystem* system = GetEntitySystem()->GetComponentSystem( name.data() );

			ImGui::PushID( regData );

			if ( ImGui::CollapsingHeader( name.data(), ImGuiTreeNodeFlags_None ) )
			{
				if ( ImGui::Button( "Remove Component" ) )
				{
					GetEntitySystem()->RemoveComponent( gSelectedEntity, name.data() );
					ImGui::PopID();
					continue;
				}

				// Draw Component Data
				// TODO: allow a custom function to be registered for editing component data in imgui
				for ( auto& [varOffset, varData] : regData->aVars )
				{
					char* dataChar   = static_cast< char* >( compData );
					char* dataOffset = dataChar + varOffset;

					if ( EntEditor_DrawComponentVarUI( dataOffset, varData ) && system )
					{
						// Mark variable as dirty
						if ( varData.aFlags & ECompRegFlag_IsNetVar )
						{
							char* dataChar = static_cast< char* >( compData );
							bool* isDirty  = reinterpret_cast< bool* >( dataChar + varData.aSize + varOffset );
							*isDirty       = true;
						}

						system->ComponentUpdated( gSelectedEntity, compData );
					}
				}
			}

			ImGui::PopID();
		}
	}

	ImGui::EndChild();

	ImGui::End();
#endif
}


void EntEditor_DrawMapDataUI()
{
	ch_handle_t mat = Skybox_GetMaterial();

	ImGui::TextUnformatted( "Skybox" );

	if ( mat == CH_INVALID_HANDLE )
	{
		ImGui::TextUnformatted( "No Skybox Material Loaded" );
	}
	else
	{
		std::string_view path = graphics->GetMaterialPath( mat );
		ImGui::TextUnformatted( path.data() );
	}

	if ( ImGui::Button( "Load Skybox" ) )
	{
		gSkyboxFilePicker.open = true;
	}

	if ( gSkyboxFilePicker.open )
	{
		EFilePickerReturn status = FilePicker_Draw( gSkyboxFilePicker, "Pick Skybox Material" );

		if ( status == EFilePickerReturn_SelectedItems )
		{
			Skybox_SetMaterial( gSkyboxFilePicker.selectedItems[ 0 ] );
		}
	}

	// Skybox_SetMaterial( args[ 0 ] );

}


void EntEditor_DrawEntityChildTree( EditorContext_t* context, ch_handle_t sParent )
{
	Entity_t* entity = Entity_GetData( sParent );
	if ( entity == nullptr )
	{
		Log_ErrorF( "Entity is nullptr????\n" );
		return;
	}

	// std::string entName = vstring( "Entity %zd", sParent );

	ImGui::PushID( sParent );

	bool treeOpen = ImGui::TreeNode( "##" );

	ImGui::SameLine();

	bool selected = false;
	for ( ch_handle_t selectedEnt : context->aEntitiesSelected )
	{
		if ( selectedEnt == sParent )
		{
			selected = true;
			break;
		}
	}

	if ( ImGui::Selectable( entity->name.data ? entity->name.data : "INVALID", selected ) )
	{
		if ( !input->KeyPressed( (EButton)SDL_SCANCODE_LSHIFT ) )
			context->aEntitiesSelected.clear();

		EntEditor_AddToSelection( context, sParent );
		gModelBrowserData.open = false;
	}

	if ( treeOpen )
	{
		ChVector< ch_handle_t > children;

		Entity_GetChildren( sParent, children );

		for ( ch_handle_t child : children )
		{
			EntEditor_DrawEntityChildTree( context, child );
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}


extern int gMainMenuBarHeight;
glm::vec2  gEntityListSize{};


void EntEditor_DrawEntityList( EditorContext_t* context, glm::uvec2 sOffset )
{
	int width, height;
	render->GetSurfaceSize( gToolData.graphicsWindow, width, height );

	ImGui::SetNextWindowPos( { (float)sOffset.x, (float)( sOffset.y + gMainMenuBarHeight ) } );
	// ImGui::SetNextWindowSizeConstraints(
	// 	{ 0.f, (float)( height - ( gMainMenuBarHeight + gAssetBrowserSize.y ) ) },
	// 	{ (float)width, (float)( height - ( gMainMenuBarHeight + gAssetBrowserSize.y ) ) }
	// );

	ImGui::SetNextWindowSizeConstraints(
	  { (float)sOffset.x, (float)( height - ( gMainMenuBarHeight + sOffset.y ) ) },
	  { (float)width, (float)( height - ( gMainMenuBarHeight + sOffset.y ) ) }
	);

	if ( !ImGui::Begin( "Entity List", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove ) )
	{
		ImGui::End();
		auto windowSize   = ImGui::GetWindowSize();
		gEntityListSize.x = windowSize.x;
		gEntityListSize.y = windowSize.y;
		return;
	}
	
	ImGui::BeginChild( "tabs" );
	if ( ImGui::BeginTabBar( "##editor tabs" ) )
	{
		if ( ImGui::BeginTabItem( "Entities" ) )
		{
			ImGui::BeginChild( "##Entity List_", {}, ImGuiChildFlags_ResizeX );

			// TODO: make this create entity stuff into a context menu with plenty of presets
			// also, create a duplicate entity option when selected on one, and move delete entity and clear parent to that

			// if ( ImGui::Button( "Create" ) )
			// {
			// 	gSelectedEntity = Entity_Create();
			// }
			//
			// ImGui::SameLine();

			if ( ImGui::Button( "Create" ) )
			{
				ch_handle_t selectedEntity = Entity_Create();

				context->aEntitiesSelected.clear();
				context->aEntitiesSelected.push_back( selectedEntity );

				Entity_t* entity = Entity_GetData( selectedEntity );
				if ( entity == nullptr )
				{
					Log_ErrorF( "Created Entity is nullptr????\n" );
					selectedEntity = CH_INVALID_HANDLE;
				}
				else
				{
					entity->aTransform.aPos = context->aView.aPos;
				}
			}

			ImGui::SameLine();

			if ( ImGui::Button( "Delete" ) )
			{
				for ( ch_handle_t selectedEnt : context->aEntitiesSelected )
				{
					Entity_Delete( selectedEnt );
				}

				context->aEntitiesSelected.clear();
			}

			// Entity List
			if ( ImGui::BeginChild( "Entity List", {}, true ) )
			{
				const ChVector< ch_handle_t >& entityHandles = context->aMap.aMapEntities;
				auto&                         entityParents = Entity_GetParentMap();

				for ( ch_handle_t entityHandle : entityHandles )
				{
					auto it = entityParents.find( entityHandle );

					// this entity is already parented to something, it will be drawn in the recursive function if it's the parent tree is expanded
					if ( it != entityParents.end() )
						continue;

					EntEditor_DrawEntityChildTree( context, entityHandle );
				}
			}

			ImGui::EndChild();
			ImGui::EndChild();

			ImGui::SameLine();

			ImGui::BeginChild( "##Entity Data" );
			EntEditor_DrawEntityData();
			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Model Data" ) )
		{
			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Map Data" ) )
		{
			EntEditor_DrawMapDataUI();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::EndChild();

	auto windowSize   = ImGui::GetWindowSize();
	gEntityListSize.x = windowSize.x;
	gEntityListSize.y = windowSize.y;

	ImGui::End();
}


static const glm::vec3 gSelectScale = { 10.f, 10.f, 10.f };
static const glm::vec3 gSelectBoxColor = { 1.f, 1.f, 1.f };


void EntEditor_DrawUI()
{
	EditorContext_t* context = Editor_GetContext();

	if ( !context )
		return;

	EntEditor_DrawEntityList( context, gEditorData.windowOffset );

	Renderable_t* gizmoTranslation = graphics->GetRenderableData( gEditorRenderables.gizmoTranslation );
	gizmoTranslation->aVisible     = false;

	// Renderable_t* gizmoRotation    = graphics->GetRenderableData( gEditorRenderables.gizmoRotation );
	// gizmoRotation->aVisible        = false;
	// 
	// Renderable_t* gizmoScale       = graphics->GetRenderableData( gEditorRenderables.gizmoScale );
	// gizmoScale->aVisible           = false;

	if ( context->aEntitiesSelected.empty() )
		return;

	// is an axis selected?
	if ( gEditorData.gizmo.selectedAxis != EGizmoAxis_None )
	{
		// offset all selected entities
		for ( ch_handle_t selectedEnt : context->aEntitiesSelected )
		{
			Entity_t* entity = Entity_GetData( selectedEnt );

			if ( gEditorData.gizmo.mode == EGizmoMode_Translation )
				entity->aTransform.aPos += gEditorData.gizmo.offset;
		}
	}

	// Draw Axis and Bounding Boxes around selected entities
	for ( ch_handle_t selectedEnt : context->aEntitiesSelected )
	{
		Entity_t* entity = Entity_GetData( selectedEnt );

		if ( entity->aRenderable )
		{
			// Draw a box around the selected entity
			ModelBBox_t bbox = graphics->GetRenderableAABB( entity->aRenderable );
			graphics->DrawBBox( bbox.aMin, bbox.aMax, gSelectBoxColor );
		}

		// Draw an axis where the selected is (TODO: change this to arrows, rotation, and scaling)
		glm::mat4 mat( 1.f );
		Entity_GetWorldMatrix( mat, selectedEnt );
		graphics->DrawAxis( mat, gSelectScale );
	}

	// For Now, just draw a gizmo at the first selected entity
	ch_handle_t selectedEnt  = context->aEntitiesSelected[ 0 ];
	Entity_t*  entity       = Entity_GetData( selectedEnt );

	glm::mat4  mat( 1.f );
	Entity_GetWorldMatrix( mat, selectedEnt );

	if ( gEditorData.gizmo.mode == EGizmoMode_Translation )
	{
		glm::vec3 pos    = Util_GetMatrixPosition( mat );
		glm::vec3 camPos = context->aView.aPos;
		glm::mat4 gizmoPosMat;

		if ( editor_gizmo_scale_enabled )
		{
			// Scale it based on distance so it always appears the same size, no matter how far or close you are
			float     dist   = glm::sqrt( powf( camPos.x - pos.x, 2 ) + powf( camPos.y - pos.y, 2 ) + powf( camPos.z - pos.z, 2 ) );

			glm::vec3 scale  = { dist, dist, dist };
			scale *= editor_gizmo_scale;

			gizmoPosMat = Util_ToMatrix( &pos, nullptr, &scale );
		}
		else
		{
			gizmoPosMat = Util_ToMatrix( &pos, nullptr, nullptr );
		}

		// Draw the Selection Gizmo
		gizmoTranslation->aVisible     = true;
		gizmoTranslation->aModelMatrix = gizmoPosMat;
	}
}

