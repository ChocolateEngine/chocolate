#pragma once

#include "core/platform.h"
#include "core/string.h"

#include <string>
#include <vector>

#include <sys/stat.h>


using DirHandle = void*;


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
enum ESearchPathType : u8
{
	ESearchPathType_Path,
	ESearchPathType_Binary,
	ESearchPathType_SourceAssets,

	ESearchPathType_Count,
};


// ================================================================================
// FileSystem Core

CORE_API bool      FileSys_Init( const char* desiredWorkingDir );
CORE_API void      FileSys_Shutdown();

// does not allocate a new string, just returns the internal working directory string
CORE_API ch_string FileSys_GetWorkingDir();
CORE_API bool      FileSys_SetWorkingDir( const char* spPath );

CORE_API ch_string FileSys_GetExePath();

// Override the $app_info$ macro, passing in nullptr resets it
// passing in an empty string will not reset it, but just set it to the exe path (is this what we want?)
CORE_API void      FileSys_SetAppPathMacro( const char* spPath, s32 pathLen = -1 );
CORE_API ch_string FileSys_GetAppPathMacro();

// ================================================================================
// Modify Search Paths

// Returns the number of search paths, and the paths themselves
CORE_API u32       FileSys_GetSearchPaths( ch_string** paths );
CORE_API u32       FileSys_GetBinPaths( ch_string** paths );
CORE_API u32       FileSys_GetSourcePaths( ch_string** paths );

CORE_API void      FileSys_ClearPaths( ESearchPathType type );
CORE_API void      FileSys_ClearAllPathTypes();

CORE_API void      FileSys_DefaultSearchPaths();
CORE_API void      FileSys_PrintSearchPaths();
CORE_API void      FileSys_ReloadSearchPaths();

// Build Search Path replaces macros like $root_path$ and $app_path$ with their actual values
CORE_API ch_string FileSys_BuildSearchPath( const char* path, s32 pathLen = -1 );

// auto builds the search path
CORE_API void      FileSys_AddSearchPath( const char* path, s32 pathLen = -1, ESearchPathType sType = ESearchPathType_Path );
// this one is weird, it has to build the path to remove it, why not just pass an index to remove?
CORE_API void      FileSys_RemoveSearchPath( const char* path, s32 pathLen = -1, ESearchPathType sType = ESearchPathType_Path );
CORE_API void      FileSys_InsertSearchPath( u32 index, const char* file, s32 pathLen = -1, ESearchPathType sType = ESearchPathType_Path );

// ================================================================================
// Main Funcs

// Find the path to a file within the search paths with printf syntax.
// CORE_API ch_string FileSys_FindFileF( ESearchPathType sType, const char* spFmt, ... );

// Find the path to a file within the search paths.
CORE_API ch_string FileSys_FindBinFile( STR_FILE_LINE_DEF const char* filePath, s32 pathLen = -1 );

// Find the path to a file within the search paths.
CORE_API ch_string FileSys_FindSourceFile( STR_FILE_LINE_DEF const char* filePath, s32 pathLen = -1 );

// Find the path to a file within the search paths.
CORE_API ch_string FileSys_FindFile( STR_FILE_LINE_DEF const char* filePath, s32 pathLen = -1 );

// Find the path to a file within the search paths.
CORE_API ch_string FileSys_FindFileEx( STR_FILE_LINE_DEF const char* filePath, ESearchPathType sType = ESearchPathType_Path );
CORE_API ch_string FileSys_FindFileEx( STR_FILE_LINE_DEF const char* filePath, s32 pathLen, ESearchPathType sType = ESearchPathType_Path );

// Find the path to a directory within the search paths.
CORE_API ch_string FileSys_FindDir( const char* dir, ESearchPathType sType = ESearchPathType_Path );
CORE_API ch_string FileSys_FindDir( const char* dir, s32 dirLen, ESearchPathType sType = ESearchPathType_Path );

// Reads a file - Returns a nullptr for the data if it doesn't exist.
CORE_API ch_string FileSys_ReadFile( const char* path, s32 pathLen = -1, ESearchPathType sType = ESearchPathType_Path );

// Saves a file - Returns true if it succeeded.
CORE_API bool      FileSys_SaveFile( const char* path, std::vector< char >& srData, s32 pathLen = -1 );

// ================================================================================
// File Information

// Is path an absolute path?
CORE_API bool      FileSys_IsAbsolute( const char* path, s32 pathLen = -1 );

// Is path a relative path?
CORE_API bool      FileSys_IsRelative( const char* path, s32 pathLen = -1 );

// Is path a directory? Set noPaths to true to not use search paths for these three functions
CORE_API bool      FileSys_IsDir( const char* path, s32 pathLen = -1, bool noPaths = false );

// Is path a file?
CORE_API bool      FileSys_IsFile( const char* path, s32 pathLen = -1, bool noPaths = false );

// Does a file/folder exist?
CORE_API bool      FileSys_Exists( const char* path, s32 pathLen = -1, bool noPaths = false );

// Call access on a file
CORE_API int       FileSys_Access( const char* path, int mode = 0 );

// ================================================================================
// Path Utils

// Return the parent path of this path - No Trailing Slash
CORE_API ch_string FileSys_GetDirName( const char* path, s32 pathLen = -1 );
#define FileSys_GetParentPath FileSys_GetDirName

// Return the parent path of this path - Has Trailing Slash
CORE_API ch_string FileSys_GetBaseName( const char* path, s32 pathLen = -1 );

// Return the filename in this path
CORE_API ch_string FileSys_GetFileName( STR_FILE_LINE_DEF const char* path, s32 pathLen = -1 );

// Return the file extension, will return an empty string if none found
CORE_API ch_string FileSys_GetFileExt( STR_FILE_LINE_DEF const char* path, s32 pathLen = -1 );

// Return the file name without the extension
CORE_API ch_string FileSys_GetFileNameNoExt( STR_FILE_LINE_DEF const char* path, s32 pathLen = -1 );

// Cleans up the path, removes useless ".." and "."
CORE_API ch_string FileSys_CleanPath( STR_FILE_LINE_DEF const char* path, s32 pathLen = -1, char* data = nullptr );  // reuses the data memory

// Rename a File or Directory
CORE_API bool      FileSys_Rename( const char* spOld, const char* spNew );

// Get Date Created/Modified on a File
CORE_API bool      FileSys_GetFileTimes( const char* spPath, float* spCreated, float* spModified );
CORE_API bool      FileSys_GetFileTimes( const char* spPath, s32 pathLen, float* spCreated, float* spModified );

// Set Date Created/Modified on a File (NOT IMPLEMENTED)
// CORE_API bool                FileSys_SetFileTimes( const char* path, s32 pathLen = -1, float* spCreated, float* spModified );

// Create a Directory
CORE_API bool      FileSys_CreateDirectory( const char* path );

// ================================================================================
// Directory Reading

// unused
#if 0

// Read the first file in a Directory
CORE_API DirHandle           FileSys_ReadFirst( const std::string& path, std::string& file, ReadDirFlags flags = ReadDir_None );

// Get the next file in the Directory
CORE_API bool                FileSys_ReadNext( DirHandle dirh, std::string& file );

// Close a Directory
CORE_API bool                FileSys_ReadClose( DirHandle dirh );

#endif

// Scan an entire Directory
// TODO: add callback function version to call as were iterating through the directory?
// or would it just be faster for the disk to blast through it all like we do right now?
CORE_API std::vector< ch_string > FileSys_ScanDir( const char* path, size_t pathLen = -1, ReadDirFlags flags = ReadDir_None );

//DirHandle                               FileSys_ReadFirst( const std::string &path, const std::string& wildcard, std::string &file );
//std::vector< std::string >              FileSys_ScanDir( const std::string &path, const std::string& wildcard, bool allPaths = false );


// ================================================================================
// Macros to help string tracking

#define FileSys_FindBinFile( ... )            FileSys_FindBinFile( STR_FILE_LINE __VA_ARGS__ )
#define FileSys_FindSourceFile( ... )         FileSys_FindSourceFile( STR_FILE_LINE __VA_ARGS__ )
#define FileSys_FindFile( ... )               FileSys_FindFile( STR_FILE_LINE __VA_ARGS__ )
#define FileSys_FindFileEx( ... )             FileSys_FindFileEx( STR_FILE_LINE __VA_ARGS__ )

#define FileSys_CleanPath( ... )              FileSys_CleanPath( STR_FILE_LINE __VA_ARGS__ )

#define FileSys_GetFileName( path, ... )      FileSys_GetFileName( STR_FILE_LINE path, __VA_ARGS__ )
#define FileSys_GetFileExt( path, ... )       FileSys_GetFileExt( STR_FILE_LINE path, __VA_ARGS__ )
#define FileSys_GetFileNameNoExt( path, ... ) FileSys_GetFileNameNoExt( STR_FILE_LINE path, __VA_ARGS__ )

