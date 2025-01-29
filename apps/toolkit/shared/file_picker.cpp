#include "file_picker.h"

#include "imgui/imgui.h"


static int FilePicker_PathInput( ImGuiInputTextCallbackData* data )
{
	std::string* path = (std::string*)data->UserData;
	path->resize( data->BufTextLen );

	return 0;
}


static void FilePicker_ScanFolder( FilePickerData_t& srData, const char* spFile, u64 size )
{
	srData.path                       = FileSys_CleanPath( spFile, size );
	std::vector< ch_string > fileList = FileSys_ScanDir( srData.path.data, srData.path.size, ReadDir_AbsPaths );

	srData.filesInFolder.clear();

	for ( size_t i = 0; i < fileList.size(); i++ )
	{
		ch_string file  = fileList[ i ];
		bool isDir = FileSys_IsDir( file.data, file.size, true );

		if ( !isDir && srData.filterExt.size() )
		{
			bool valid = false;
			for ( size_t filterI = 0; filterI < srData.filterExt.size(); filterI++ )
			{
				std::string_view ext = srData.filterExt[ filterI ];
				if ( ch_str_ends_with( file, ext.data(), ext.size() ) )
				{
					valid = true;
					break;
				}
			}

			if ( !valid )
				continue;
		}

		srData.filesInFolder.emplace_back( file.data, file.size );
	}

	ch_str_free( fileList.data(), fileList.size() );
}


#define PATH_LEN 1024

EFilePickerReturn FilePicker_Draw( FilePickerData_t& srData, const char* spWindowName )
{
	srData.output = EFilePickerReturn_None;

	if ( srData.childWindow )
	{
		if ( !ImGui::BeginChild( spWindowName ? spWindowName : "File Picker" ) )
		{
			ImGui::EndChild();
			srData.output = EFilePickerReturn_None;
			return EFilePickerReturn_None;
		}
	}
	else
	{
		if ( !ImGui::Begin( spWindowName ? spWindowName : "File Picker" ) )
		{
			ImGui::End();
			srData.output = EFilePickerReturn_None;
			return EFilePickerReturn_None;
		}
	}

	if ( !srData.path.data )
	{
		// Default to Root Path
		srData.path.data = ch_malloc< char >( PATH_LEN );
		srData.path.size = FileSys_GetExePath().size;

		memcpy( srData.path.data, FileSys_GetExePath().data, FileSys_GetExePath().size * sizeof( char ) );
	}

	// Draw Title Bar
	if ( ImGui::Button( "Up" ) )
	{
		ch_string dirName = FileSys_GetDirName( srData.path.data, srData.path.size );

		if ( dirName.data )
		{
			// wipe memory first
			memset( srData.path.data, 0, PATH_LEN );
			memcpy( srData.path.data, dirName.data, dirName.size );
			srData.path.size = dirName.size;
		}

		srData.filesInFolder.clear();
	}

	ImGui::SameLine();

	if ( ImGui::InputText( "Path", srData.path.data, PATH_LEN, ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_EnterReturnsTrue, FilePicker_PathInput, &srData.path ) )
	{
		srData.filesInFolder.clear();
	}

	ImGui::SameLine();

	if ( ImGui::Button( "Go" ) )
	{
		srData.filesInFolder.clear();
	}

	if ( srData.filesInFolder.empty() )
	{
		FilePicker_ScanFolder( srData, srData.path.data, srData.path.size );
	}

	// Draw Items in Folder
	// ImGui::BeginChild()

	bool   selectedItems = false;
	size_t selectedIndex = SIZE_MAX;

	ImVec2 currentWindowSize = ImGui::GetWindowSize();
	
	ImGui::SetNextWindowSizeConstraints( {}, { currentWindowSize.x, currentWindowSize.y - 82 } );

	if ( ImGui::BeginChild( "##Files", {}, ImGuiChildFlags_Border ) )
	{
		for ( size_t i = 0; i < srData.filesInFolder.size(); i++ )
		{
			std::string_view file = srData.filesInFolder[ i ];
			if ( ImGui::Selectable( file.data() ) )
			{
				bool isDir = FileSys_IsDir( file.data(), true );

				if ( isDir )
				{
					FilePicker_ScanFolder( srData, file.data(), file.size() );
					srData.selectedItems.clear();
					selectedIndex = SIZE_MAX;
				}
				else
				{
					if ( srData.selectedItems.size() )
						srData.selectedItems.clear();

					srData.selectedItems.push_back( file.data() );
					selectedIndex = i;
					selectedItems = true;
					srData.open   = false;
				}
			}
		}
	}

	ImGui::EndChild();

	// Draw Status Bar
	ImGui::Text( "%d Items in Folder", srData.filesInFolder.size() );

	ImGui::SameLine();
	// ImGui::Spacing();
	// ImGui::SameLine();

	if ( ImGui::Button( "Close" ) )
	{
		srData.open = false;

		if ( srData.childWindow )
			ImGui::EndChild();
		else
			ImGui::End();

		srData.selectedItems.clear();
		srData.output = EFilePickerReturn_Close;
		return EFilePickerReturn_Close;
	}
	
	if ( srData.childWindow )
		ImGui::EndChild();
	else	
		ImGui::End();

	srData.output = selectedItems ? EFilePickerReturn_SelectedItems : EFilePickerReturn_None;
	return srData.output;
}

