#include "main.h"


int                                           gTextureListViewMode = 1;
std::unordered_map< ch_handle_t, ImTextureID > gImGuiTextures;


void ResourceUsage_DrawTextures()
{
	// Draws all currently loaded textures
	std::vector< ch_handle_t > textures         = render->GetTextureList();
	ImVec2                    windowSize       = ImGui::GetWindowSize();

	glm::vec2                 imageDisplaySize = { 96, 96 };

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

		render->FreeTextureInfo( info );
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

		auto        it        = gImGuiTextures.find( texture );
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
					Util_DrawTextureInfo( info );
				}

				ImGui::EndChild();
			}

			if ( ImGui::IsItemHovered() )
			{
				if ( ImGui::BeginTooltip() )
					Util_DrawTextureInfo( info );

				ImGui::EndTooltip();
			}

			ImGui::EndChild();

			render->FreeTextureInfo( info );
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
						Util_DrawTextureInfo( info );
						render->FreeTextureInfo( info );
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


void ResourceUsage_DrawMaterials()
{
	if ( !ImGui::BeginChild( "Material List" ) )
	{
		ImGui::EndChild();
		return;
	}

	u32 matCount = graphics->GetMaterialCount();

	ImGui::Text( "Count: %d", matCount );

	for ( u32 i = 0; i < matCount; i++ )
	{
		ch_handle_t  mat     = graphics->GetMaterialByIndex( i );
		const char* matName = graphics->Mat_GetName( mat );

		ImGui::PushID( i );

		if ( ImGui::Selectable( matName ? matName : "nullptr" ) )
		{
			// launch it?
		}

		ImGui::PopID();
	}

	ImGui::EndChild();
}


void ResourceUsage_DrawStats()
{
	std::vector< ch_handle_t > textures      = render->GetTextureList();
	u32                       matCount      = graphics->GetMaterialCount();
	u32                       renderCount   = graphics->GetRenderableCount();

	u32                       memoryUsage   = 0;
	u32                       rtMemoryUsage = 0;

	for ( ch_handle_t texture : textures )
	{
		TextureInfo_t info = render->GetTextureInfo( texture );

		if ( info.aRenderTarget )
			rtMemoryUsage += info.aMemoryUsage;
		else
			memoryUsage += info.aMemoryUsage;

		render->FreeTextureInfo( info );
	}

	u64 stringCount  = ch_str_get_alloc_count();
	u64 stringMemory = ch_str_get_alloc_size();

	ImGui::Text( "String Count: %d", stringCount );
	ImGui::Text( "String Memory Usage: %.6f KB", ch_bytes_to_kb( stringMemory ) );

	ImGui::Text( "Renderable Count: %d", renderCount );
	ImGui::Text( "Model Count: TODO - EXPOSE THIS" );
	ImGui::Text( "Material Count: %d", matCount );
	ImGui::Text( "Texture Count: %d", textures.size() );
	ImGui::Text( "Texture Memory: %.4f MB", ch_bytes_to_mb( memoryUsage ) );
	ImGui::Text( "Render Target Memory: %.4f MB", ch_bytes_to_mb( rtMemoryUsage ) );
}


void ResourceUsage_Draw()
{
	if ( ImGui::BeginTabBar( "editor tabs" ) )
	{
		if ( ImGui::BeginTabItem( "Stats" ) )
		{
			ResourceUsage_DrawStats();
			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Textures" ) )
		{
			ResourceUsage_DrawTextures();
			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Models" ) )
		{
			ImGui::EndTabItem();
		}

		if ( ImGui::BeginTabItem( "Materials" ) )
		{
			ResourceUsage_DrawMaterials();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

