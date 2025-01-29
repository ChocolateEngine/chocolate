#pragma once


enum EFilePickerReturn
{
	EFilePickerReturn_None,
	EFilePickerReturn_Close,
	EFilePickerReturn_SelectedItems,
};


struct FilePickerData_t
{
	EFilePickerReturn          output;
	bool                       childWindow;
	bool                       open;
	bool                       allowMultiSelect;  // NOT IMPLEMENTED YET
	ch_string                  path;
	std::vector< std::string > filterExt;
	std::vector< std::string > selectedItems;     // Get the selected item here
	std::vector< std::string > filesInFolder;
};


EFilePickerReturn FilePicker_Draw( FilePickerData_t& srData, const char* spWindowName );

