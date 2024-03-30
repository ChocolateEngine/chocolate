#pragma once

#include "core/platform.h"

#include <string>
#include <vector>

#include <sys/stat.h>


#ifdef _WIN32
#define stat _stat
constexpr char PATH_SEP = '\\';
#define        PATH_SEP_STR "\\"

constexpr char CH_PATH_SEP = '\\';
#define        CH_PATH_SEP_STR "\\"

#elif __unix__
constexpr char PATH_SEP = '/';
#define        PATH_SEP_STR "/"

constexpr char CH_PATH_SEP = '/';
#define        CH_PATH_SEP_STR "/"
#endif


using DirHandle = void*;


// TODO: make the engine have full unicode support on windows
struct ChPath_t
{
};


enum ReadDirFlags_: unsigned char
{
	ReadDir_None = 0,
	ReadDir_AllPaths = 1 << 1,  // DOES NOT WORK ON ReadFirst/ReadNext FUNCTIONS !!!
	ReadDir_AbsPaths = 1 << 2,
	ReadDir_NoDirs = 1 << 3,
	ReadDir_NoFiles = 1 << 4,
	ReadDir_Recursive = 1 << 5,
};

using ReadDirFlags = unsigned char;


// TODO: custom path types?
enum ESearchPathType
{
	ESearchPathType_Path,
	ESearchPathType_Binary,
	ESearchPathType_SourceAssets,
};


CORE_API int                FileSys_Init( const char* workingDir );

CORE_API const std::string& FileSys_GetWorkingDir();
CORE_API void               FileSys_SetWorkingDir( const std::string& workingDir );

CORE_API const std::string& FileSys_GetExePath();

// Override the $app_info$ macro, passing in an empty string resets it
CORE_API void               FileSys_SetAppPathMacro( const std::string& path );
CORE_API const std::string& FileSys_GetAppPathMacro();

// ================================================================================
// Modify Search Paths

CORE_API const std::vector< std::string >& FileSys_GetSearchPaths();
CORE_API const std::vector< std::string >& FileSys_GetBinPaths();
CORE_API const std::vector< std::string >& FileSys_GetSourcePaths();

CORE_API void                              FileSys_ClearSearchPaths();
CORE_API void                              FileSys_ClearBinPaths();
CORE_API void                              FileSys_ClearSourcePaths();

CORE_API void                              FileSys_DefaultSearchPaths();
CORE_API void                              FileSys_PrintSearchPaths();
CORE_API void                              FileSys_ReloadSearchPaths();

// Build Search Path replaces macros like $root_path$ and $app_path$ with their actual values
CORE_API std::string                       FileSys_BuildSearchPath( std::string_view sPath );

// auto builds the search path
CORE_API void                              FileSys_AddSearchPath( const std::string& path, ESearchPathType sType = ESearchPathType_Path );
CORE_API void                              FileSys_RemoveSearchPath( const std::string& path, ESearchPathType sType = ESearchPathType_Path );
CORE_API void                              FileSys_InsertSearchPath( size_t index, const std::string& path, ESearchPathType sType = ESearchPathType_Path );

// ================================================================================
// Main Funcs

// Find the path to a file within the search paths with printf syntax.
CORE_API std::string FileSys_FindFileF( ESearchPathType sType, const char* spFmt, ... );

// Find the path to a file within the search paths.
CORE_API std::string FileSys_FindBinFile( const std::string& file );

// Find the path to a file within the search paths.
CORE_API std::string FileSys_FindSourceFile( const std::string& file );

// Find the path to a file within the search paths.
CORE_API std::string FileSys_FindFile( const std::string& file );

// Find the path to a file within the search paths.
CORE_API std::string FileSys_FindFileEx( const std::string& file, ESearchPathType sType = ESearchPathType_Path );

// Find the path to a directory within the search paths.
CORE_API std::string FileSys_FindDir( const std::string& dir, ESearchPathType sType = ESearchPathType_Path );

// Reads a file - Returns an empty array if it doesn't exist.
CORE_API std::vector< char > FileSys_ReadFile( const std::string& file );

// Reads a file - Returns an empty array if it doesn't exist.
CORE_API std::vector< char > FileSys_ReadFileEx( const std::string& file, ESearchPathType sType = ESearchPathType_Path );

// Saves a file - Returns true if it succeeded.
CORE_API bool FileSys_SaveFile( const std::string& srFile, std::vector< char >& srData );

// ================================================================================
// File Information

// Is path an absolute path?
CORE_API bool                FileSys_IsAbsolute( const char* spPath );

// Is path a relative path?
CORE_API bool                FileSys_IsRelative( const char* spPath );

// Is path a directory? Set noPaths to true to not use search paths for these three functions
CORE_API bool                FileSys_IsDir( const std::string& path, bool noPaths = false );

// Is path a file?
CORE_API bool                FileSys_IsFile( const std::string& path, bool noPaths = false );

// Does a file/folder exist?
CORE_API bool                FileSys_Exists( const std::string& path, bool noPaths = false );

// Call access on a file
CORE_API int                 FileSys_Access( const char* path, int mode = 0 );

// Call stat on a file
CORE_API int                 FileSys_Stat( const char* path, struct stat* info );

// ================================================================================
// Path Utils

// Return the parent path of this path - No Trailing Slash
CORE_API std::string FileSys_GetDirName( std::string_view path );

// Return the parent path of this path - Has Trailing Slash
CORE_API std::string FileSys_GetBaseName( std::string_view path );

// Return the filename in this path
CORE_API std::string FileSys_GetFileName( std::string_view path );

// Return the file extension, will return an empty string if none found
CORE_API std::string FileSys_GetFileExt( std::string_view spPath, bool sStripPath = true );

// Return the file name without the extension 
CORE_API std::string FileSys_GetFileNameNoExt( std::string_view path );

// Cleans up the path, removes useless ".." and "." 
CORE_API std::string FileSys_CleanPath( std::string_view path );

// Rename a File or Directory
CORE_API bool        FileSys_Rename( std::string_view srOld, std::string_view srNew );

// Get Date Created/Modified on a File
CORE_API bool        FileSys_GetFileTimes( std::string_view srPath, float* spCreated, float* spModified );

// Set Date Created/Modified on a File
CORE_API bool        FileSys_SetFileTimes( std::string_view srPath, float* spCreated, float* spModified );

// Create a Directory
CORE_API bool        FileSys_CreateDirectory( std::string_view path );

// ================================================================================
// Directory Reading

// Read the first file in a Directory
CORE_API DirHandle   FileSys_ReadFirst( const std::string& path, std::string& file, ReadDirFlags flags = ReadDir_None );

// Get the next file in the Directory
CORE_API bool        FileSys_ReadNext( DirHandle dirh, std::string& file );

// Close a Directory
CORE_API bool        FileSys_ReadClose( DirHandle dirh );

// Scan an entire Directory
// TODO: add callback function version to call as were iterating through the directory?
// or would it just be faster for the disk to blast through it all like we do right now?
CORE_API std::vector< std::string > FileSys_ScanDir( const std::string& path, ReadDirFlags flags = ReadDir_None );

//DirHandle                               FileSys_ReadFirst( const std::string &path, const std::string& wildcard, std::string &file );
//std::vector< std::string >              FileSys_ScanDir( const std::string &path, const std::string& wildcard, bool allPaths = false );

