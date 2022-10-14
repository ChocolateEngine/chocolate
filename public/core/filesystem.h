#pragma once

#include "core/platform.h"

#include <string>
#include <vector>

#include <sys/stat.h>


#ifdef _WIN32
#define stat _stat
constexpr char PATH_SEP = '\\';
#elif __unix__
constexpr char PATH_SEP = '/';
#endif


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


CORE_API int                FileSys_Init( const char* workingDir );

CORE_API const std::string& FileSys_GetWorkingDir();
CORE_API void               FileSys_SetWorkingDir( const std::string& workingDir );

CORE_API const std::string& FileSys_GetExePath();

// ================================================================================
// Modify Search Paths

CORE_API const std::vector< std::string >& FileSys_GetSearchPaths();
CORE_API void                              FileSys_ClearSearchPaths();
CORE_API void                              FileSys_DefaultSearchPaths();

CORE_API void                              FileSys_AddSearchPath( const std::string& path );
CORE_API void                              FileSys_RemoveSearchPath( const std::string& path );
CORE_API void                              FileSys_InsertSearchPath( size_t index, const std::string& path );

// ================================================================================
// Main Funcs

// Find the path to a file within the search paths with printf syntax.
CORE_API std::string FileSys_FindFileF( const char* spFmt, ... );

// Find the path to a file within the search paths.
CORE_API std::string FileSys_FindFile( const std::string& file );

// Find the path to a directory within the search paths.  */
CORE_API std::string FileSys_FindDir( const std::string& dir );

// Reads a file - Returns an empty array if it doesn't exist.  */
CORE_API std::vector< char > FileSys_ReadFile( const std::string& file );

// ================================================================================
// File Information

// Is path an absolute path?
CORE_API bool                FileSys_IsAbsolute( const std::string& path );

// Is path a relative path?
CORE_API bool                FileSys_IsRelative( const std::string& path );

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

// Return the parent path of this path - No Trailing Slash */
CORE_API std::string FileSys_GetDirName( const std::string& path );

// Return the parent path of this path - Has Trailing Slash */
CORE_API std::string FileSys_GetBaseName( const std::string& path );

// Return the filename in this path  */
CORE_API std::string FileSys_GetFileName( const std::string& path );

// Return the file extension */
CORE_API std::string FileSys_GetFileExt( const std::string& path );

// Return the file name without the extension */
CORE_API std::string FileSys_GetFileNameNoExt( const std::string& path );

// Cleans up the path, removes useless ".." and "."  */
CORE_API std::string FileSys_CleanPath( const std::string& path );

// ================================================================================
// Directory Reading

// Read the first file in a Directory  */
CORE_API DirHandle   FileSys_ReadFirst( const std::string& path, std::string& file, ReadDirFlags flags = ReadDir_None );

// Get the next file in the Directory */
CORE_API bool        FileSys_ReadNext( DirHandle dirh, std::string& file );

// Close a Directory  */
CORE_API bool        FileSys_ReadClose( DirHandle dirh );

// Scan an entire Directory  */
CORE_API std::vector< std::string > FileSys_ScanDir( const std::string& path, ReadDirFlags flags = ReadDir_None );

//DirHandle                               FileSys_ReadFirst( const std::string &path, const std::string& wildcard, std::string &file );
//std::vector< std::string >              FileSys_ScanDir( const std::string &path, const std::string& wildcard, bool allPaths = false );

