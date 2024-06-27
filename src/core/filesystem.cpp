#include "core/filesystem.h"
#include "core/console.h"
#include "core/profiler.h"
#include "core/platform.h"
#include "core/log.h"
#include "core/json5.h"
#include "core/app_info.h"
#include "util.h"

#include <array>
#include <fstream>
#include <filesystem>

#include <sys/stat.h>

#ifdef _WIN32
	#include <direct.h>
    //#include <Windows.h>
    #include <fileapi.h> 
    #include <shlwapi.h> 
    #include <minwinbase.h> 
    #include <tchar.h> 
    #include <stdio.h>
    #include <strsafe.h>
	#include <io.h>

	// get rid of the dumb windows posix depreciation warnings
	#define mkdir _mkdir
	#define chdir _chdir
	#define access _access
	#define getcwd _getcwd

    // Unicode Variants
	#define ch_umkdir _wmkdir
	#define ch_uchdir _wchdir
	#define ch_uaccess _waccess
	#define ch_ugetcwd _wgetcwd
#else
	#include <unistd.h>
    #include <dirent.h>
    #include <string.h>

	#define ch_umkdir  mkdir
	#define chdir  chdir
	#define ch_uaccess access
	#define ch_ugetcwd getcwd
#endif


#undef FileSys_FindBinFile
#undef FileSys_FindSourceFile
#undef FileSys_FindFile
#undef FileSys_FindFileEx


LOG_REGISTER_CHANNEL( FileSystem, LogColor::DarkGray );

static ch_string                gWorkingDir;
static ch_string                gExePath;
static ch_string                gAppPathMacro;

static std::vector< ch_string > gSearchPaths;
static std::vector< ch_string > gBinPaths;
static std::vector< ch_string > gSourceAssetPaths;


CONCMD( fs_print_paths )
{
	FileSys_PrintSearchPaths();
}


CONCMD( fs_reload_paths )
{
	FileSys_ReloadSearchPaths();
}


void FileSys_WhereIsCmd( const std::string& path, ESearchPathType type )
{
	ch_string fullPath = FileSys_FindFileEx( CH_FS_FILE_LINE path.data(), path.size(), type );

    if ( !fullPath.data )
	{
		Log_Error( "File not found!\n" );
		return;
    }

	Log_MsgF( "%s\n", fullPath.data );
}


CONCMD( fs_where )
{
	if ( args.empty() )
	{
		Log_Error( "No path Specified\n" );
		return;
	}

	FileSys_WhereIsCmd( args[ 0 ], ESearchPathType_Path );
}


CONCMD( fs_where_binary )
{
	if ( args.empty() )
	{
		Log_Error( "No path Specified\n" );
		return;
	}

	FileSys_WhereIsCmd( args[ 0 ], ESearchPathType_Binary );
}


CONCMD( where )
{
	if ( args.empty() )
	{
		Log_Error( "No path Specified\n" );
		return;
	}

	FileSys_WhereIsCmd( args[ 0 ], ESearchPathType_Path );
}


bool FileSys_Init( const char* workingDir )
{
    if ( !workingDir )
	{
		Log_Error( "Failed to initialize File System, no working directory specified\n" );
		return false;
	}

	gWorkingDir   = ch_str_copy( workingDir );
	gExePath.data = getcwd( 0, 0 );

    if ( !gExePath.data )
	{
		Log_Error( "Failed to initialize File System, failed to get current working directory\n" );
		return false;
	}

	gExePath.size = strlen( gExePath.data );

	// change to desired working directory (TODO: make this happen later after modules are loaded)
	chdir( workingDir );

    FileSys_SetAppPathMacro( gWorkingDir.data, gWorkingDir.size );

    return true;
}


ch_string FileSys_GetWorkingDir()
{
	return gWorkingDir;
}


void FileSys_SetWorkingDir( const char* spPath, s32 pathLen )
{
	if ( !spPath )
		return;

    if ( gWorkingDir.data )
        ch_str_free( gWorkingDir.data );

    if ( pathLen == -1 )
		pathLen = strlen( spPath );

	gWorkingDir = ch_str_copy( spPath );
}


ch_string FileSys_GetExePath()
{
    return gExePath;
}


// Override the $app_info$ macro
void FileSys_SetAppPathMacro( const char* spPath, s32 pathLen )
{
    // Reset the macro
	if ( !spPath )
	{
		const char*  strings[] = { gExePath.data, CH_PATH_SEP_STR, gWorkingDir.data };
		const size_t lengths[] = { gExePath.size, 1, gWorkingDir.size };
		ch_string    newData   = ch_str_concat( 3, strings, lengths, gAppPathMacro.data );

		if ( !newData.data )
		{
			Log_Error( "Failed to realloc memory for App Path Macro\n" );
			return;
		}

		gAppPathMacro = newData;
		return;
	}

    // If this is -1, we need to get the length of the string
	if ( pathLen == -1 )
		pathLen = strlen( spPath );

    // If this is an absolute path, we can just set the macro to this path
    if ( FileSys_IsAbsolute( spPath, pathLen ) )
	{
		ch_string newData = ch_str_realloc( gAppPathMacro.data, spPath, pathLen );
		if ( !newData.data )
		{
			Log_Error( "Failed to realloc memory for App Path Macro\n" );
			return;
		}

		gAppPathMacro = newData;
	}
	else
	{
        // This is a relative path, so we need to append it to the root directory of the executable
		const char*  strings[] = { gExePath.data, CH_PATH_SEP_STR, spPath };
		const size_t lengths[] = { gExePath.size, 1, pathLen };
		ch_string    newData   = ch_str_concat( 3, strings, lengths, gAppPathMacro.data );

		if ( !newData.data )
		{
			Log_Error( "Failed to realloc memory for App Path Macro\n" );
			return;
		}

		gAppPathMacro = newData;
	}
}


ch_string FileSys_GetAppPathMacro()
{
	return gAppPathMacro;
}


u32 FileSys_GetSearchPaths( ch_string** paths )
{
    if ( !paths )
		return 0;

    *paths = gSearchPaths.data();
	return gSearchPaths.size();
}


u32 FileSys_GetBinPaths( ch_string** paths )
{
    if ( !paths )
		return 0;

	*paths = gBinPaths.data();
	return gBinPaths.size();
}


u32 FileSys_GetSourcePaths( ch_string** paths )
{
    if ( !paths )
		return 0;

	*paths = gSourceAssetPaths.data();
	return gSourceAssetPaths.size();
}


void FileSys_ClearSearchPathsBase( std::vector< ch_string >& paths )
{
	for ( auto& path : paths )
		ch_str_free( path.data );

	paths.clear();
}


void FileSys_ClearSearchPaths()
{
	FileSys_ClearSearchPathsBase( gSearchPaths );
}


void FileSys_ClearBinPaths()
{
	FileSys_ClearSearchPathsBase( gBinPaths );
}


void FileSys_ClearSourcePaths()
{
	FileSys_ClearSearchPathsBase( gSourceAssetPaths );
}


void FileSys_DefaultSearchPaths()
{
	FileSys_ClearSearchPathsBase( gBinPaths );
    FileSys_ClearSearchPathsBase( gSearchPaths );
	FileSys_ClearSearchPathsBase( gSourceAssetPaths );

	ch_string path1    = ch_str_concat( nullptr, 3, gExePath.data, CH_PATH_SEP_STR, "bin" );
	ch_string path2    = ch_str_concat( nullptr, 3, gExePath.data, CH_PATH_SEP_STR, gWorkingDir.data );
	ch_string path3    = ch_str_concat( nullptr, 2, gExePath.data, CH_PATH_SEP_STR );
	ch_string pathCore = ch_str_concat( nullptr, 3, gExePath.data, CH_PATH_SEP_STR, "core" );

	FileSys_AddSearchPath( path1.data, path1.size, ESearchPathType_Binary );
	FileSys_AddSearchPath( path2.data, path2.size );
	FileSys_AddSearchPath( path3.data, path3.size );
	FileSys_AddSearchPath( pathCore.data, pathCore.size );  // always add core as a search path, at least for now

	// Free the strings, as they are copied when added as a search path
	ch_str_free( path1.data );
	ch_str_free( path2.data );
	ch_str_free( path3.data );
	ch_str_free( pathCore.data );
}


void FileSys_PrintSearchPaths()
{
	LogGroup group = Log_GroupBegin( gFileSystemChannel );

    Log_Group( group, "Binary Paths:\n" );
	for ( const auto& path : gBinPaths )
	{
		Log_GroupF( group, "    %s\n", path.data );
	}

	Log_Group( group, "\nSearch Paths:\n" );
	for ( const auto& path : gSearchPaths )
	{
		Log_GroupF( group, "    %s\n", path.data );
	}

	Log_Group( group, "\nSource Asset Paths:\n" );
	for ( const auto& path : gSourceAssetPaths )
	{
		Log_GroupF( group, "    %s\n", path.data );
	}

    Log_GroupEnd( group );
}


ch_string FileSys_BuildSearchPath( const char* path, s32 pathLen )
{
	ch_string output;

    if ( !path )
		return output;

	if ( pathLen == -1 )
		pathLen = strlen( path );

	if ( pathLen == 0 )
		return output;

	// output.data = ch_str_copy( path, pathLen );
	// output.size = pathLen;

	const char* last = path;
	const char* find = strchr( path, '$' );

	while ( last )
	{
        // at a macro
		if ( find == last )
		{
			find = strchr( last + 1, '$' );
		}

		size_t dist = 0;
		if ( find )
			dist = ( find - last ) + 1;
		else
			dist = pathLen - ( last - path );

		if ( dist == 0 )
			break;

		if ( dist == 11 && strncmp( last, "$root_path$", dist ) == 0 )
		{
			const char* strings[] = { output.data, gExePath.data };
			const u64   lengths[] = { output.size, gExePath.size };
			ch_string   newData   = ch_str_concat( 2, strings, lengths, output.data );

			if ( !newData.data )
			{
				Log_Error( "Failed to realloc memory for search path\n" );
				return output;
			}

			output = newData;
		}
		else if ( dist == 10 && strncmp( last, "$app_path$", dist ) == 0 )
		{
			const char* strings[] = { output.data, gAppPathMacro.data };
			const u64   lengths[] = { output.size, gAppPathMacro.size };
			ch_string   newData   = ch_str_concat( 2, strings, lengths, output.data );

			if ( !newData.data )
			{
				Log_Error( "Failed to realloc memory for search path\n" );
				return output;
			}

			output = newData;
		}
		else
		{
			// append this string to the 
			//#undef ch_str_concat
			const char* strings[] = { output.data, last };
			// const u64   lengths[] = { output.size, dist == SIZE_MAX ? strlen( last ) : dist };
			ch_string   newData = ch_str_concat( 2, strings );

			if ( !newData.data )
			{
				Log_Error( "Failed to realloc memory for search path\n" );
				return output;
			}

			if ( output.data )
				ch_str_free( output.data );

			output = newData;
		}

		if ( !find )
			break;

		last = ++find;
		find = strchr( last, '$' );
	}

	ch_string cleanedPath = FileSys_CleanPath( output.data, output.size );
	ch_str_free( output.data );

	return cleanedPath;
}


void FileSys_AddSearchPath( const char* path, s32 pathLen, ESearchPathType sType )
{
	ch_string fullPath = FileSys_BuildSearchPath( path, pathLen );

	if ( !fullPath.data )
		return;

    switch ( sType )
	{
		default:
		case ESearchPathType_Path:
			gSearchPaths.push_back( fullPath );
			break;

		case ESearchPathType_Binary:
			gBinPaths.push_back( fullPath );
			break;

		case ESearchPathType_SourceAssets:
			gSourceAssetPaths.push_back( fullPath );
			break;
	}
}


void FileSys_RemoveSearchPath( const char* path, s32 pathLen, ESearchPathType sType )
{
	ch_string fullPath = FileSys_BuildSearchPath( path );

    if ( !fullPath.data )
        return;

    switch ( sType )
	{
		default:
		case ESearchPathType_Path:
			vec_remove_if( gSearchPaths, fullPath );
			break;

		case ESearchPathType_Binary:
			vec_remove_if( gBinPaths, fullPath );
			break;

		case ESearchPathType_SourceAssets:
			vec_remove_if( gSourceAssetPaths, fullPath );
			break;
	}

    ch_str_free( fullPath.data );
}


void FileSys_InsertSearchPath( size_t index, const char* path, s32 pathLen, ESearchPathType sType )
{
	ch_string fullPath = FileSys_BuildSearchPath( path );

	switch ( sType )
	{
		default:
		case ESearchPathType_Path:
			gSearchPaths.insert( gSearchPaths.begin() + index, fullPath );
			break;

		case ESearchPathType_Binary:
			gBinPaths.insert( gBinPaths.begin() + index, fullPath );
			break;

		case ESearchPathType_SourceAssets:
			gSourceAssetPaths.insert( gSourceAssetPaths.begin() + index, fullPath );
			break;
	}
}


void FileSys_ReloadSearchPaths()
{
	Core_ReloadSearchPaths();
}


// maybe faster calls? probably not
inline bool exists( const char* path )
{
    return ( access( path, 0 ) != -1 );
}


inline bool is_dir( const char* path )
{
    struct stat s;
    if ( ch_stat( path, &s ) == 0 )
        return (s.st_mode & S_IFDIR);

    return false;
}


#if 0
std::string FileSys_FindFileF( ESearchPathType sType, const char* spFmt, ... )
{
    PROF_SCOPE();

    std::string path;
	VSTRING( path, spFmt );

    return FileSys_FindFileEx( path, sType );
}
#endif

#undef ch_str_copy

inline ch_string FileSys_FindFileAbs( CH_FS_FILE_LINE_DEF const char* file, s32 fileLen )
{
	ch_string out;
	out.data = nullptr;
	out.size = 0;

	if ( exists( file ) )
		out = ch_str_copy( CH_FS_FILE_LINE_INT file, fileLen );

	return out;
}


#if 0
ch_string FileSys_FindFileBaseSearchPath( const ch_string& searchPath, const char* file, s32 fileLen )
{
	const char* paths[]   = { searchPath.data, CH_PATH_SEP_STR, file };
	const size_t lengths[] = { searchPath.size, 1, fileLen };
	ch_string   concat    = ch_str_concat( 3, paths, lengths );

	if ( !concat.data )
	{
		Log_Error( "Failed to allocate memory for search path\n" );
		return {};
	}

	ch_string fullPath = FileSys_CleanPath( concat.data, concat.size );
	ch_str_free( concat.data );

	// does item exist?
	if ( exists( fullPath.data ) )
	{
		return fullPath;
	}

	ch_str_free( fullPath.data );
	fullPath.size = 0;

	return fullPath;
}
#endif


ch_string FileSys_FindFileBase( CH_FS_FILE_LINE_DEF const std::vector< ch_string >& paths, const char* filePath, s32 fileLen )
{
	PROF_SCOPE();

	if ( !filePath )
		return {};

	if ( fileLen == -1 )
		fileLen = strlen( filePath );

	if ( fileLen == 0 )
		return {};

	if ( FileSys_IsAbsolute( filePath, fileLen ) )
		return FileSys_FindFileAbs( CH_FS_FILE_LINE_INT filePath, fileLen );

	for ( const ch_string& searchPath : paths )
	{
		const char*  paths[]   = { searchPath.data, CH_PATH_SEP_STR, filePath };
		const size_t lengths[] = { searchPath.size, 1, fileLen };
		ch_string    concat    = ch_str_concat( 3, paths, lengths );

		if ( !concat.data )
		{
			Log_Error( "Failed to allocate memory for search path\n" );
			return {};
		}

		ch_string fullPath = FileSys_CleanPath( concat.data, concat.size );
		ch_str_free( concat.data );

		// does item exist?
		if ( exists( fullPath.data ) )
		{
			return fullPath;
		}

		ch_str_free( fullPath.data );

		// ch_string fullPath = FileSys_FindFileBase( searchPath, filePath, pathLen );
		// 
		// if ( fullPath.data )
		// 	return fullPath;
	}

	// file not found
	return {};
}


ch_string FileSys_FindBinFile( CH_FS_FILE_LINE_DEF const char* filePath, s32 pathLen )
{
	return FileSys_FindFileBase( CH_FS_FILE_LINE_INT gBinPaths, filePath, pathLen );
}


ch_string FileSys_FindSourceFile( CH_FS_FILE_LINE_DEF const char* filePath, s32 pathLen )
{
	return FileSys_FindFileBase( CH_FS_FILE_LINE_INT gSourceAssetPaths, filePath, pathLen );
}


ch_string FileSys_FindFile( CH_FS_FILE_LINE_DEF const char* filePath, s32 pathLen )
{
	return FileSys_FindFileBase( CH_FS_FILE_LINE_INT gSearchPaths, filePath, pathLen );
}


ch_string FileSys_FindFileEx( CH_FS_FILE_LINE_DEF const char* filePath, s32 pathLen, ESearchPathType sType )
{
	switch ( sType )
	{
		default:
		case ESearchPathType_Path:
			return FileSys_FindFileBase( CH_FS_FILE_LINE_INT gSearchPaths, filePath, pathLen );

		case ESearchPathType_Binary:
			return FileSys_FindFileBase( CH_FS_FILE_LINE_INT gBinPaths, filePath, pathLen );

		case ESearchPathType_SourceAssets:
			return FileSys_FindFileBase( CH_FS_FILE_LINE_INT gSourceAssetPaths, filePath, pathLen );
	}
}


#define FileSys_FindBinFile( ... )    FileSys_FindBinFile( CH_FS_FILE_LINE __VA_ARGS__ )
#define FileSys_FindSourceFile( ... ) FileSys_FindSourceFile( CH_FS_FILE_LINE __VA_ARGS__ )
#define FileSys_FindFile( ... )       FileSys_FindFile( CH_FS_FILE_LINE __VA_ARGS__ )
#define FileSys_FindFileEx( ... )     FileSys_FindFileEx( CH_FS_FILE_LINE __VA_ARGS__ )

#if CH_STRING_MEM_TRACKING
	#define ch_str_copy( ... ) ch_str_copy( STR_FILE_LINE __VA_ARGS__ )
#endif


ch_string FileSys_FindDir( const char* path, s32 pathLen, ESearchPathType sType )
{
    PROF_SCOPE();

    // if it's an absolute path already,
    // don't bother to look in the search paths for it, and make sure it exists
	if ( FileSys_IsAbsolute( path, pathLen ) )
	{
		ch_string out;

		if ( is_dir( path ) )
			out = ch_str_copy( path );

		return out;
	}

    std::vector< ch_string >* paths = nullptr;

    switch ( sType )
	{
		default:
		case ESearchPathType_Path:
			paths = &gSearchPaths;
			break;

		case ESearchPathType_Binary:
			paths = &gBinPaths;
			break;

		case ESearchPathType_SourceAssets:
			paths = &gSourceAssetPaths;
			break;
	}

    for ( auto searchPath : *paths )
    {
		const char* paths[]   = { searchPath.data, CH_PATH_SEP_STR, path };
		const size_t lengths[] = { searchPath.size, 1, pathLen };
		ch_string   fullPath  = ch_str_concat( 3, paths, lengths );

        // does item exist?
        if ( is_dir( fullPath.data ) )
        {
            return fullPath;
        }
    }

    // file not found
	return {};
}


/* Reads a file into a byte array.  */
std::vector< char > FileSys_ReadFile( const char* path, s32 pathLen )
{
    PROF_SCOPE();

    // Find path first
	ch_string fullPath = FileSys_FindFile( path, pathLen );
	if ( !fullPath.data )
	{
		Log_ErrorF( gFileSystemChannel, "Failed to find file: %s\n", path );
		return {};
	}

    /* Open file.  */
    std::ifstream file( fullPath.data, std::ios::ate | std::ios::binary );
    if ( !file.is_open() )
	{
		Log_ErrorF( gFileSystemChannel, "Failed to open file: %s\n", path );
		ch_str_free( fullPath.data );
		return {};
    }

    int fileSize = ( int )file.tellg(  );
    std::vector< char > buffer( fileSize );
    file.seekg( 0 );

    /* Read contents.  */
    file.read( buffer.data(  ), fileSize );
    file.close(  );

    buffer.push_back( '\0' );  // adding null terminator character

	ch_str_free( fullPath.data );
    return buffer;
}


// Reads a file - Returns an empty array if it doesn't exist.
std::vector< char > FileSys_ReadFileEx( const char* path, s32 pathLen, ESearchPathType sType )
{
    PROF_SCOPE();

    // Find path first
	ch_string_auto fullPath = FileSys_FindFileEx( path, pathLen, sType );
    if ( !fullPath.data )
    {
        Log_ErrorF( gFileSystemChannel, "Failed to find file: %s\n", path );
        return {};
    }

    /* Open file.  */
	std::ifstream file( fullPath.data, std::ios::ate | std::ios::binary );
    if ( !file.is_open() )
	{
        Log_ErrorF( gFileSystemChannel, "Failed to open file: %s\n", path );
        return {};
    }

    int fileSize = ( int )file.tellg(  );
    std::vector< char > buffer( fileSize );
    file.seekg( 0 );

    /* Read contents.  */
    file.read( buffer.data(  ), fileSize );
    file.close(  );

	// free the full path
	ch_str_free( fullPath.data );

    return buffer;
}


// Saves a file - Returns true if it succeeded.
// TODO: maybe change this to not rename the old file until the new file is fully written
bool FileSys_SaveFile( const char* path, std::vector< char >& srData, s32 pathLen )
{
    // Is there a file with this name already?
	ch_string oldFile;

    if ( FileSys_Exists( path, pathLen ) )
	{
		// if it's a directory, don't save and return false
		if ( FileSys_IsDir( path, pathLen ) )
		{
			Log_ErrorF( gFileSystemChannel, "Can't Save file, file exists as directory already - \"%s\"\n", path );
			return false;
		}
		else
		{
			// rename the old file
			oldFile = ch_str_concat( nullptr, 2, path, ".bak" );

			if ( !FileSys_Rename( path, oldFile.data ) )
			{
				Log_ErrorF( gFileSystemChannel, "Failed to rename file to backup file - \"%s\"", path );
				ch_str_free( oldFile.data );
				return false;
			}
		}
	}

	// Open the file handle
	FILE* fp = fopen( path, "wb" );

	if ( fp == nullptr )
	{
		Log_ErrorF( gFileSystemChannel, "Failed to open file handle to save file: \"%s\"\n", path );
		return false;
	}

	// Write Data
	size_t amountWritten = fwrite( srData.data(), srData.size(), 1, fp );
	fclose( fp );

    // Did we have to rename an old file?
    if ( oldFile.size )
	{
        // Copy original date created on windows
		CH_ASSERT( 0 );

		ch_str_free( oldFile.data );
	}

	return true;
}


bool FileSys_IsAbsolute( const char* spPath, s32 pathLen )
{
	if ( !spPath )
		return false;

	if ( pathLen == -1 )
		pathLen = strlen( spPath );

	if ( pathLen == 0 )
		return false;

#ifdef _WIN32
	// NOTE: this doesn't work for paths like C:test.txt,
	// as that is relative to the current directory on that drive, weird windows stuff
	// https://devblogs.microsoft.com/oldnewthing/20101011-00/?p=12563
	if ( pathLen > 2 )
		return ( spPath[ 1 ] == ':' );

	return false;
	// return !PathIsRelativeA( spPath );
#elif __unix__
	if ( pathLen == 0 )
		return false;
	return spPath[ 0 ] == '/';
#else
    return std::filesystem::path( path ).is_absolute();
#endif
}


bool FileSys_IsRelative( const char* spPath, s32 pathLen )
{
	return !FileSys_IsAbsolute( spPath, pathLen );

/*
	if ( !spPath || pathLen == 0 )
		return true;
#ifdef _WIN32
	if ( pathLen > 2 )
		return ( spPath[ 1 ] != ':' );

	return true;
	// return PathIsRelativeA( spPath );
#elif __unix__
	if ( pathLen == 0 )
		return true;
	return spPath[ 0 ] != '/';
#else
	return std::filesystem::path( path ).is_relative();
#endif*/
}


bool FileSys_IsDir( const char* path, s32 pathLen, bool noPaths )
{
    PROF_SCOPE();

    struct stat s;

    if ( noPaths || FileSys_IsAbsolute( path, pathLen ) )
    {
		if ( ch_stat( path, &s ) == 0 )
			return ( s.st_mode & S_IFDIR );
    }
    else
    {
		ch_string fullPath = FileSys_FindFile( path );

		if ( !fullPath.data )
			return false;

		if ( ch_stat( fullPath.data, &s ) == 0 )
		{
			bool isDir = ( s.st_mode & S_IFDIR );
			ch_str_free( fullPath.data );
			return isDir;
		}
	}

	return false;
}


bool FileSys_IsFile( const char* path, s32 pathLen, bool noPaths )
{
    PROF_SCOPE();

    struct stat s;

    if ( noPaths || FileSys_IsAbsolute( path, pathLen ) )
    {
		if ( ch_stat( path, &s ) == 0 )
			return ( s.st_mode & S_IFREG );
    }
    else
    {
		ch_string fullPath = FileSys_FindFile( path );

		if ( !fullPath.data )
			return false;

		if ( ch_stat( fullPath.data, &s ) == 0 )
		{
			bool result = ( s.st_mode & S_IFREG );
			ch_str_free( fullPath.data );
			return result;
		}
    }

    return false;
}


bool FileSys_Exists( const char* path, s32 pathLen, bool noPaths )
{
    PROF_SCOPE();

    if ( noPaths || FileSys_IsAbsolute( path, pathLen ) )
	{
		return ( access( path, 0 ) != -1 );
	}
	else
	{
		ch_string fullPath = FileSys_FindFile( path );

		if ( !fullPath.data )
			return false;

		if ( access( fullPath.data, 0 ) != -1 )
		{
			ch_str_free( fullPath.data );
			return true;
		}
	}

	return false;
}


int FileSys_Access( const char* path, int mode )
{
    return access( path, mode );
}


int FileSys_Stat( const char* path, struct stat* info )
{
    return ch_stat( path, info );
}


ch_string FileSys_GetDirName( const char* path, s32 pathLen )
{
	if ( pathLen == -1 )
		pathLen = strlen( path );

	if ( pathLen == 0 )
		return {};

	size_t i = pathLen - 1;
	for ( ; i > 0; i-- )
    {
		if ( path[ i ] == '/' || path[ i ] == '\\' )
            break;
    }

    ch_string output = ch_str_copy( path, i );
    return output;
}


ch_string FileSys_GetBaseName( const char* path, s32 pathLen )
{
	if ( pathLen == -1 )
		pathLen = strlen( path );

	if ( pathLen == 0 )
		return {};

	size_t i = pathLen - 1;
	for ( ; i > 0; i-- )
	{
		if ( path[ i ] == '/' || path[ i ] == '\\' )
			break;
	}

	const char* strings[] = { path + i, CH_PATH_SEP_STR };
	const size_t lengths[] = { i, 1 };
	ch_string output = ch_str_concat( 2, strings, lengths );
	return output;
}


#undef FileSys_GetFileName
#undef FileSys_GetFileExt
#undef FileSys_GetFileNameNoExt

#undef ch_str_copy


ch_string FileSys_GetFileName( CH_FS_FILE_LINE_DEF const char* path, s32 pathLen )
{
	if ( pathLen == -1 )
		pathLen = strlen( path );

	if ( pathLen == 0 )
		return {};

	size_t i = pathLen - 1;
    for ( ; i > 0; i-- )
    {
		if ( path[ i ] == '/' || path[ i ] == '\\' )
            break;
    }

    // No File Extension Found
    if ( i == pathLen )
		return {};

    size_t startIndex = i + 1;

    if ( startIndex == pathLen )
		return {};

	ch_string output = ch_str_copy( CH_FS_FILE_LINE_INT & path[ startIndex ], pathLen - startIndex );
	return output;
}


ch_string FileSys_GetFileExt( CH_FS_FILE_LINE_DEF const char* path, s32 pathLen, bool sStripPath )
{
    if ( sStripPath )
	{
		ch_string fileName = FileSys_GetFileName( CH_FS_FILE_LINE_INT path, pathLen );
		if ( !fileName.data )
			return {};

		const char* dot = strrchr( fileName.data, '.' );

		// a bit weird to return a nullptr here, but the string is empty so why bother allocating memory
		if ( !dot || dot == fileName.data )
		{
			ch_str_free( fileName.data );
			return {};
		}

		ch_string output = ch_str_copy( CH_FS_FILE_LINE_INT dot + 1 );
		ch_str_free( fileName.data );
		return output;
	}

	const char* dot = strrchr( path, '.' );

	if ( !dot || dot == path )
		return {};

	ch_string output = ch_str_copy( CH_FS_FILE_LINE_INT dot + 1 );
	return output;
}


ch_string FileSys_GetFileNameNoExt( CH_FS_FILE_LINE_DEF const char* path, s32 pathLen )
{
	ch_string name = FileSys_GetFileName( CH_FS_FILE_LINE_INT path, pathLen );

	if ( !name.data )
		return {};

	const char* dot = strrchr( name.data, '.' );

	if ( !dot || dot == name.data )
		return name;

	ch_string output = ch_str_copy( CH_FS_FILE_LINE_INT name.data, dot - name.data );
	ch_str_free( name.data );
	return output;
}


#if CH_STRING_MEM_TRACKING
	#define ch_str_copy( ... ) ch_str_copy( STR_FILE_LINE __VA_ARGS__ )
#endif



ch_string FileSys_CleanPath( const char* path, const s32 pathLen, char* data )
{
    PROF_SCOPE();

#if 1
	std::vector< ch_string > pathSegments;

	#if __unix__
	if ( FileSys_IsAbsolute( path, pathLen ) )
	{
		pathSegments.push_back( ch_str_copy( "/" ) );
	}
	#endif

	s32 startIndex = 0;
	s32 endIndex   = 0;

	while ( endIndex < pathLen )
	{
	#if 1
		while ( endIndex < pathLen )
		{
			if ( path[ endIndex ] == '/' || path[ endIndex ] == '\\' )
				break;

			endIndex++;
		}

	#else

		// stupid windows
		const char* pathSepA = strchr( path + startIndex, '/' );
		const char* pathSepB = strchr( path + startIndex, '\\' );

		const char* pathSep  = nullptr;

		// find the first path separator
		if ( pathSepA && pathSepB )
			pathSep = ( pathSepA < pathSepB ) ? pathSepA : pathSepB;
		else if ( pathSepA )
			pathSep = pathSepA;
		else if ( pathSepB )
			pathSep = pathSepB;

		if ( !pathSep )
			endIndex = pathLen;
		else
			endIndex = pathSep - path;
	#endif

		// this might occur on unix systems if the path starts with "/", an absolute path, where startIndex and endIndex are 0
		// or if we have a path like "C://test", with extra path separators for some reason
		if ( endIndex == startIndex )
		{
			startIndex++;
			endIndex++;
			continue;
		}

		// check if it's a "." segment and skip it
		if ( endIndex - startIndex == 1 && path[ startIndex ] == '.' )
		{
			startIndex = ++endIndex;
			continue;
		}
		
		// check if this is a ".." segment and remove last path segment
		if ( endIndex - startIndex == 2 && path[ startIndex ] == '.' && path[ startIndex + 1 ] == '.' )
		{
			if ( !pathSegments.empty() )
			{
				// pop the last segment
				ch_str_free( pathSegments.back().data );
				pathSegments.pop_back();
			}
		}
		else if ( endIndex - startIndex > 1 )  // if it's not an empty segment
		{
			ch_string segment = ch_str_copy( &path[ startIndex ], endIndex - startIndex );

			if ( !segment.data )
			{
				Log_Error( "Failed to allocate memory for path segment when cleaning path\n" );
				ch_str_free( pathSegments.data(), pathSegments.size() );
				return {};
			}

			pathSegments.push_back( segment );
		}

		startIndex = ++endIndex;
	}

	// build the cleaned path
	ch_string finalString = ch_str_join( pathSegments.size(), pathSegments.data(), CH_PATH_SEP_STR, data );

	// free the path segments
	ch_str_free( pathSegments.data(), pathSegments.size() );

#else
	std::vector< ChVector< char > > pathSegments;
	ch_string                       root;

    if ( FileSys_IsAbsolute( path, pathLen ) )
    {
#ifdef _WIN32

#elif __unix__
        root.data = "/";
		root.size = 1;
#endif
    }

    // TODO: have this use strchr instead to check for '/', '\\', '.',
    // and just check the next character for if it's ".."
    for ( int i = 0; i < pathLen; i++ )
    {
		ChVector< char > dir;
        // forming the current directory.

        while ( i < pathLen && ( path[ i ] != '/' && path[ i ] != '\\' ) )
		{
			dir.push_back( path[ i ] );
            i++;
        }

        // if ".." , we pop.
		if ( dir.size() == 2 && dir[ 0 ] == '.' && dir[ 1 ] == '.' )
        {
            if ( !pathSegments.empty() )
                pathSegments.pop_back();
        }
		else if ( dir.size() && dir[ 0 ] != '.' )
        {
            // push the current directory into the vector.
            pathSegments.push_back( dir );
        }
    }
    
    // build the cleaned path
	ChVector< char > out;
	
	if ( root.size )
	{
		out.reserve( root.size );
		for ( size_t i = 0; i < root.size; i++ )
			out.push_back( root.data[ i ] );
	}
	
	// TODO: remove usage of ChVector
	// should setup the above function to use a start index and end index of each segment,
	// then allocate memory for that segment and copy it over to a vector element
	#undef ch_str_join
	ch_str_join( STR_FILE_LINE pathSegments.size(), pathSegments.data(), CH_PATH_SEP_STR );
	
	for ( size_t i = 0; i < pathSegments.size(); i++ )
	{
		out.reserve( out.size() + pathSegments[ i ].size() );

		for ( size_t j = 0; j < pathSegments[ i ].size(); j++ )
			out.push_back( pathSegments[ i ][ j ] );

		if ( i + 1 != pathSegments.size() )
		{
			out.push_back( CH_PATH_SEP );
		}
	}

	ch_string finalString = ch_str_copy( out.data(), out.size() );
#endif

	if ( !finalString.data )
	{
		Log_Error( "Failed to allocate memory for cleaned path\n" );
		return finalString;
	}

	return finalString;
}


// Rename a File or Directory
bool FileSys_Rename( const char* spOld, const char* spNew )
{
	return ( rename( spOld, spNew ) == 0 );
}


// Get Date Created/Modified on a File
bool FileSys_GetFileTimes( const char* spPath, float* spCreated, float* spModified )
{
#ifdef WIN32
	// open first file
	HANDLE file = CreateFileA(
	  spPath,                 // file to open
	  GENERIC_READ,           // open for reading
	  FILE_SHARE_READ,        // share for reading
	  NULL,                   // default security
	  OPEN_EXISTING,          // existing file only
	  FILE_ATTRIBUTE_NORMAL,  // normal file
	  NULL                    // no attribute template
	);

    // Check if this is a valid file
    if ( file == INVALID_HANDLE_VALUE )
	{
		Log_ErrorF( gFileSystemChannel, "Failed to open file handle: \"%s\" - error %d\n", spPath, sys_get_error() );
		return false;
	}

    FILETIME create, modified;
	if ( !GetFileTime( file, &create, nullptr, &modified ) )
	{
		Log_ErrorF( gFileSystemChannel, "Failed to get file times of file: \"%s\" - error %d\n", spPath, sys_get_error() );
		return false;
	}

    return true;
#else
#endif
}


// Set Date Created/Modified on a File
bool FileSys_SetFileTimes( std::string_view srPath, float* spCreated, float* spModified )
{
#ifdef WIN32
	// if ( !GetFileTime( hFile1, &ftCreate1, &ftAccess1, &ftWrite1 ) )
	// {
	// 	printf( "GetFileTime Failed!\n" );
	// 	return 1;
	// }
#else
#endif

    return false;
}


// Create a Directory
bool FileSys_CreateDirectory( const char* path )
{
	int ret = mkdir( path );
	return ret != 0;
}


struct SearchParams
{
    ReadDirFlags flags;
    std::string rootPath;
    std::string relPath;
    std::string wildcard;
};

std::unordered_map< DirHandle, SearchParams > gSearchParams;


bool FitsWildcard( DirHandle dirh, const std::string &path )
{
    auto it = gSearchParams.find( dirh );

    if ( it != gSearchParams.end() )
    {
		Log_Msg( gFileSystemChannel, "TODO: wildcard check oh god\n" );
        return false;
    }
    else
    {
        return true;
    }
}

#if 0

/* Read the first file in a Directory  */
DirHandle FileSys_ReadFirst( const std::string &path, std::string &file, ReadDirFlags flags )
{
    PROF_SCOPE();

#ifdef _WIN32
    std::string readPath = path + "\\*";

    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    std::string absSearchPath;

    for ( auto searchPath: gSearchPaths )
    {
        absSearchPath = searchPath + PATH_SEP + readPath;
        hFind = FindFirstFile( absSearchPath.c_str(), &ffd );

        if ( INVALID_HANDLE_VALUE != hFind )
            break;
    }

    if ( INVALID_HANDLE_VALUE == hFind )
        return nullptr;

    absSearchPath = absSearchPath.substr( 0, absSearchPath.size() - 2 );
    gSearchParams[hFind] = {flags, absSearchPath, ""};

    file = ((flags & ReadDir_AbsPaths) ? absSearchPath : path) + PATH_SEP + ffd.cFileName;
    
    return hFind;

#elif __unix__
    return nullptr;
#else
#endif
}


/* Get the next file in the directory */
bool FileSys_ReadNext( DirHandle dirh, std::string &file )
{
    PROF_SCOPE();

#ifdef _WIN32
    WIN32_FIND_DATA ffd;

    if ( !FindNextFile( dirh, &ffd ) )
        return false;

    auto it = gSearchParams.find(dirh);
    if ( it != gSearchParams.end() )
    {
        if ( it->second.flags & ReadDir_AbsPaths )
            file = it->second.rootPath + PATH_SEP + it->second.relPath + PATH_SEP + ffd.cFileName;
        else
            file = it->second.relPath + PATH_SEP + ffd.cFileName;

        return true;
    }
    else
    {
        Log_Warn( gFileSystemChannel, "handle not in handle list??\n" );
        return false;
    }

#elif __unix__
    // somehow do stuff with wildcards
    // probably have to have an std::unordered_map< handles, search string > variable so you can do a comparison here

    return false;
#else
#endif
}


/* Close a Directory */
bool FileSys_ReadClose( DirHandle dirh )
{
#ifdef _WIN32
    return FindClose( dirh );

#elif __unix__
    return closedir( ( DIR* )dirh );

#else
#endif
}

#endif


// TODO: make a faster linux direct system call version like i have for windows here
bool sys_scandir( const char* root, size_t rootLen, const char* path, size_t pathLen, std::vector< ch_string >& files, ReadDirFlags flags = ReadDir_None )
{
    PROF_SCOPE();

#ifdef _WIN32
	ch_string scanDir;
	ch_string scanDirWildcard;

	ChVector< ch_string > directories;
	
	{
		const char*  strings[] = { root, "\\", path, "\\" };
		const size_t lengths[] = { rootLen, 1, pathLen, 1 };
		scanDir                = ch_str_concat( 4, strings, lengths );
	}

	{
		const char*  strings[] = { scanDir.data, "*" };
		const size_t lengths[] = { scanDir.size, 1 };
		scanDirWildcard        = ch_str_concat( 2, strings, lengths );
	}

	WIN32_FIND_DATA ffd;
	HANDLE          hFind = FindFirstFile( scanDirWildcard.data, &ffd );

	ch_str_free( scanDirWildcard.data );

    if ( INVALID_HANDLE_VALUE == hFind )
	{
		ch_str_free( scanDir.data );
		Log_ErrorF( gFileSystemChannel, "Failed to find first file in directory: \"%s\"\n", path );
		return false;
	}

	ch_string relPath;
    while ( FindNextFile( hFind, &ffd ) != 0 )
    {
        bool isDir = ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
		size_t fileNameLen = strlen( ffd.cFileName );

		const char*  strings[] = { path, "\\", ffd.cFileName };
		const size_t lengths[] = { pathLen, 1, fileNameLen };
		relPath  = ch_str_concat( 3, strings, lengths, relPath.data );

		if ( ( flags & ReadDir_Recursive ) && isDir && strncmp( ffd.cFileName, "..", 2 ) != 0 )
        {
			sys_scandir( root, rootLen, relPath.data, relPath.size, files, flags );
        }

		if ( ( flags & ReadDir_NoDirs ) && isDir )
        {
            continue;
        }

        if ( ( flags & ReadDir_NoFiles ) && FileSys_IsFile( relPath.data, relPath.size, true ) )
        {
            continue;
        }

		if ( flags & ReadDir_AbsPaths )
		{
			ch_string pathCopy;

			const char* strings[] = { scanDir.data, ffd.cFileName };
			const size_t lengths[] = { scanDir.size, fileNameLen };
			pathCopy = ch_str_concat( 2, strings, lengths );

			files.push_back( pathCopy );
		}
		else
		{
			ch_string pathCopy = ch_str_copy( relPath.data, relPath.size );
			files.push_back( pathCopy );
		}
    }

    FindClose( hFind );
	ch_str_free( relPath.data );
	ch_str_free( scanDir.data );

	// Scan the directories
	// for ( const auto& dir : directories )
	// {
	// 	if ( !sys_scandir( root, rootLen, dir.data, dir.size, files, flags ) )
	// 		return false;
	// }

    return true;
#else
    if ( !FileSys_IsDir( path ) )
        return false;

    // NOTE: the _WIN32 one adds ".." to the path, while the directory iterator doesn't
    // so i manually add it myself to match the win32 one

	const char*  strings[] = { path, CH_PATH_SEP_STR, ".." };
	const size_t lengths[] = { pathLen, 1, 2 };
	ch_string    basePath  = ch_str_concat( 3, strings, lengths );

	for ( const auto& rFile : std::filesystem::directory_iterator( path ) )
	{
		if ( rFile.is_directory() )
		{
			if ( ( flags & ReadDir_Recursive ) && rFile.path().filename() != ".." )
			{
				sys_scandir( root, rootLen, rFile.path().string().c_str(), rFile.path().string().size(), files, flags );
			}
		}
		else if ( ( flags & ReadDir_NoFiles ) && rFile.is_regular_file() )
		{
			continue;
		}
		else if ( ( flags & ReadDir_NoDirs ) && rFile.is_directory() )
		{
			continue;
		}

		if ( flags & ReadDir_AbsPaths )
		{
			ch_string pathCopy = ch_str_copy( rFile.path().string().data(), rFile.path().string().size() );
			files.push_back( pathCopy );
		}
		else
		{
			fs::path filename = rFile.path().filename();
			ch_string pathCopy = ch_str_copy( filename.string().data(), filename.string().size() );
			files.push_back( pathCopy );
		}
	}

	return true;
#endif
}


/* Scan an entire Directory  */
std::vector< ch_string > FileSys_ScanDir( const char* path, size_t pathLen, ReadDirFlags flags )
{
    PROF_SCOPE();

    std::vector< ch_string > files;

	if ( !path || pathLen == 0 )
		return files;

	if ( pathLen == -1 )
		pathLen = strlen( path );

    if ( !FileSys_IsAbsolute( path, pathLen ) )
    {
        for ( auto searchPath : gSearchPaths )
        {
            if ( !sys_scandir( searchPath.data, searchPath.size, path, pathLen, files, flags ) )
                continue;

            if ( !( flags & ReadDir_AllPaths ) )
                break;
        }
    }
    else
    {
        sys_scandir( path, pathLen, "", 0, files, flags );
	}

	return files;
}


