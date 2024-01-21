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
#else
	#include <unistd.h>
    #include <dirent.h>
    #include <string.h>
#endif


LOG_REGISTER_CHANNEL( FileSystem, LogColor::DarkGray );

static std::string                gWorkingDir;
static std::string                gExePath;
static std::vector< std::string > gSearchPaths;
static std::vector< std::string > gBinPaths;
static std::vector< std::string > gSourceAssetPaths;


CONCMD( fs_print_paths )
{
	FileSys_PrintSearchPaths();
}


CONCMD( fs_reload_paths )
{
	FileSys_ReloadSearchPaths();
}


int FileSys_Init( const char* workingDir )
{
	gWorkingDir = workingDir;
	gExePath = getcwd( 0, 0 );

	// change to desired working directory
	chdir( workingDir );

    return 0;
}


const std::string& FileSys_GetWorkingDir()
{
	return gWorkingDir;
}

void FileSys_SetWorkingDir( const std::string& workingDir )
{
	gWorkingDir = workingDir;
}


const std::string& FileSys_GetExePath()
{
    return gExePath;
}


const std::vector< std::string >& FileSys_GetSearchPaths()
{
	return gSearchPaths;
}


const std::vector< std::string >& FileSys_GetBinPaths()
{
	return gBinPaths;
}


const std::vector< std::string >& FileSys_GetSourcePaths()
{
	return gSourceAssetPaths;
}


void FileSys_ClearSearchPaths()
{
	gSearchPaths.clear();
}


void FileSys_ClearBinPaths()
{
	gBinPaths.clear();
}


void FileSys_ClearSourcePaths()
{
	gSourceAssetPaths.clear();
}


void FileSys_DefaultSearchPaths()
{
	gBinPaths.clear();
    gSearchPaths.clear();
	gSourceAssetPaths.clear();
    FileSys_AddSearchPath( gExePath + "/bin", ESearchPathType_Binary );
    FileSys_AddSearchPath( gExePath + "/" + gWorkingDir );
    FileSys_AddSearchPath( gExePath );
    FileSys_AddSearchPath( gExePath + "/core" );  // always add core as a search path, at least for now
}


void FileSys_PrintSearchPaths()
{
	LogGroup group = Log_GroupBegin( gFileSystemChannel );

    Log_Group( group, "Binary Paths:\n" );
    for ( const auto& path : gBinPaths )
        Log_GroupF( group, "    %s\n", path.data() );

	Log_Group( group, "\nSearch Paths:\n" );
    for ( const auto& path : gSearchPaths )
        Log_GroupF( group, "    %s\n", path.data() );

	Log_Group( group, "\nSource Asset Paths:\n" );
	for ( const auto& path : gSourceAssetPaths )
        Log_GroupF( group, "    %s\n", path.data() );

    Log_GroupEnd( group );
}


std::string FileSys_BuildSearchPath( std::string_view sPath )
{
	std::string output;

	size_t      len  = sPath.size();

	const char* last = sPath.data();
	const char* find = strchr( sPath.data(), '$' );

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
			dist = len - ( last - sPath.data() );

		if ( dist == 0 )
			break;

        if ( dist == 11 && strncmp( last, "$root_path$", dist ) == 0 )
		{
			output += gExePath;
		}
        else if ( dist == 10 && strncmp( last, "$app_path$", dist ) == 0 )
		{
			output += gExePath + PATH_SEP_STR + gWorkingDir;
		}
		else
		{
			output.append( last, dist );
		}

		if ( !find )
			break;

		last = ++find;
		find = strchr( last, '$' );
	}

    return FileSys_CleanPath( output );
}


void FileSys_AddSearchPath( const std::string& path, ESearchPathType sType )
{
	std::string fullPath = FileSys_BuildSearchPath( path );

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


void FileSys_RemoveSearchPath( const std::string& path, ESearchPathType sType )
{
	std::string fullPath = FileSys_BuildSearchPath( path );

    switch ( sType )
	{
		default:
		case ESearchPathType_Path:
			vec_remove_if( gSearchPaths, path );
			break;

		case ESearchPathType_Binary:
			vec_remove_if( gBinPaths, path );
			break;

		case ESearchPathType_SourceAssets:
			vec_remove_if( gSourceAssetPaths, path );
			break;
	}
}


void FileSys_InsertSearchPath( size_t index, const std::string& path, ESearchPathType sType )
{
	std::string fullPath = FileSys_BuildSearchPath( path );

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
inline bool exists( const std::string& path )
{
    return ( access( path.c_str(), 0 ) != -1 );
}


inline bool is_dir( const std::string &path )
{
    struct stat s;
    if ( stat( path.c_str(), &s ) == 0 )
        return (s.st_mode & S_IFDIR);

    return false;
}


std::string FileSys_FindFileF( ESearchPathType sType, const char* spFmt, ... )
{
    PROF_SCOPE();

    std::string path;
	VSTRING( path, spFmt );

    return FileSys_FindFileEx( path, sType );
}


std::string FileSys_FindBinFile( const std::string& file )
{
    PROF_SCOPE();

    // if it's an absolute path already,
    // don't bother to look in the search paths for it, and make sure it exists
	if ( FileSys_IsAbsolute( file.data() ) )
        return exists( file ) ? file : "";

    if ( file == "" )
        return file;

    for ( auto searchPath : gBinPaths )
    {
		std::string fullPath = FileSys_CleanPath( searchPath + PATH_SEP + file );

        // does item exist?
        if ( exists( fullPath ) )
        {
            return fullPath;
        }
	}

    // file not found
    return "";
}


std::string FileSys_FindSourceFile( const std::string& file )
{
    PROF_SCOPE();

    // if it's an absolute path already,
    // don't bother to look in the search paths for it, and make sure it exists
	if ( FileSys_IsAbsolute( file.data() ) )
        return exists( file ) ? file : "";

    if ( file == "" )
        return file;

    for ( auto searchPath : gSourceAssetPaths )
    {
		std::string fullPath = FileSys_CleanPath( searchPath + PATH_SEP + file );

        // does item exist?
        if ( exists( fullPath ) )
        {
            return fullPath;
        }
	}

    // file not found
    return "";
}


std::string FileSys_FindFile( const std::string& file )
{
	// if it's an absolute path already,
	// don't bother to look in the search paths for it, and make sure it exists
	if ( FileSys_IsAbsolute( file.data() ) )
		return exists( file ) ? file : "";

	if ( file == "" )
		return file;

	for ( auto searchPath : gSearchPaths )
	{
		std::string fullPath = FileSys_CleanPath( searchPath + PATH_SEP + file );

		// does item exist?
		if ( exists( fullPath ) )
		{
			return fullPath;
		}
	}

	// file not found
	return "";
}


std::string FileSys_FindFileEx( const std::string& file, ESearchPathType sType )
{
	switch ( sType )
	{
		default:
		case ESearchPathType_Path:
			return FileSys_FindFile( file );

		case ESearchPathType_Binary:
			return FileSys_FindBinFile( file );

		case ESearchPathType_SourceAssets:
			return FileSys_FindSourceFile( file );
	}
}


std::string FileSys_FindDir( const std::string& dir, ESearchPathType sType )
{
    PROF_SCOPE();

    // if it's an absolute path already,
    // don't bother to look in the search paths for it, and make sure it exists
	if ( FileSys_IsAbsolute( dir.data() ) )
        return is_dir( dir ) ? dir : "";

    std::vector< std::string >* paths = nullptr;

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
        std::string fullPath = searchPath + CH_PATH_SEP_STR + dir;

        // does item exist?
        if ( is_dir( fullPath ) )
        {
            return fullPath;
        }
    }

    // file not found
    return "";
}


/* Reads a file into a byte array.  */
std::vector< char > FileSys_ReadFile( const std::string& srFilePath )
{
    PROF_SCOPE();

    // Find path first
	std::string fullPath = FileSys_FindFile( srFilePath );
    if ( fullPath == "" )
    {
        Log_ErrorF( gFileSystemChannel, "Failed to find file: %s\n", srFilePath.c_str() );
        return {};
    }

    /* Open file.  */
    std::ifstream file( fullPath, std::ios::ate | std::ios::binary );
    if ( !file.is_open() )
    {
        Log_ErrorF( gFileSystemChannel, "Failed to open file: %s\n", srFilePath.c_str() );
        return {};
    }

    int fileSize = ( int )file.tellg(  );
    std::vector< char > buffer( fileSize );
    file.seekg( 0 );

    /* Read contents.  */
    file.read( buffer.data(  ), fileSize );
    file.close(  );

    return buffer;
}


// Reads a file - Returns an empty array if it doesn't exist.
std::vector< char > FileSys_ReadFileEx( const std::string& srFilePath, ESearchPathType sType )
{
    PROF_SCOPE();

    // Find path first
	std::string fullPath = FileSys_FindFileEx( srFilePath, sType );
    if ( fullPath == "" )
    {
        Log_ErrorF( gFileSystemChannel, "Failed to find file: %s\n", srFilePath.c_str() );
        return {};
    }

    /* Open file.  */
    std::ifstream file( fullPath, std::ios::ate | std::ios::binary );
    if ( !file.is_open() )
    {
        Log_ErrorF( gFileSystemChannel, "Failed to open file: %s\n", srFilePath.c_str() );
        return {};
    }

    int fileSize = ( int )file.tellg(  );
    std::vector< char > buffer( fileSize );
    file.seekg( 0 );

    /* Read contents.  */
    file.read( buffer.data(  ), fileSize );
    file.close(  );

    return buffer;
}


// Saves a file - Returns true if it succeeded.
// TODO: maybe change this to not rename the old file until the new file is fully written
bool FileSys_SaveFile( const std::string& srFile, std::vector< char >& srData )
{
    // Is there a file with this name already?
	std::string oldFile;

    if ( FileSys_Exists( srFile ) )
	{
		// if it's a directory, don't save and return false
		if ( FileSys_IsDir( srFile ) )
		{
			Log_ErrorF( gFileSystemChannel, "Can't Save file, file exists as directory already - \"%s\"\n", srFile.c_str() );
			return false;
		}
		else
		{
			oldFile = srFile + ".bak";
			if ( !FileSys_Rename( srFile.data(), oldFile.data() ) )
			{
				Log_ErrorF( gFileSystemChannel, "Failed to rename file to backup file - \"%s\"", srFile.c_str() );
				return false;
			}
		}
	}

	// Open the file handle
	FILE* fp = fopen( srFile.c_str(), "wb" );

	if ( fp == nullptr )
	{
		Log_ErrorF( gFileSystemChannel, "Failed to open file handle to save file: \"%s\"\n", srFile.c_str() );
		return false;
	}

	// Write Data
	size_t amountWritten = fwrite( srData.data(), srData.size(), 1, fp );
	fclose( fp );

    // Did we have to rename an old file?
    if ( oldFile.size() )
	{
        // Copy original date created on windows
	}

	return true;
}


bool FileSys_IsAbsolute( const char* spPath )
{
	if ( !spPath )
		return false;
#ifdef _WIN32
	return !PathIsRelative( spPath );
#elif __unix__
	if ( strlen( spPath ) == 0 )
		return false;
	return spPath[ 0 ] == '/';
#else
    return std::filesystem::path( path ).is_absolute();
#endif
}


bool FileSys_IsRelative( const char* spPath )
{
	if ( !spPath )
		return true;
#ifdef _WIN32
	return PathIsRelative( spPath );
#elif __unix__
	if ( strlen( spPath ) == 0 )
		return true;
	return spPath[ 0 ] != '/';
#else
	return std::filesystem::path( path ).is_relative();
#endif
}


bool FileSys_IsDir( const std::string &path, bool noPaths )
{
    PROF_SCOPE();

    struct stat s;

    if ( noPaths || FileSys_IsAbsolute( path.data() ) )
    {
        if ( stat( path.c_str(), &s ) == 0 )
            return (s.st_mode & S_IFDIR);
    }
    else
    {
		if ( stat( FileSys_FindFile( path ).c_str(), &s ) == 0 )
            return (s.st_mode & S_IFDIR);
    }

    return false;
}


bool FileSys_IsFile( const std::string &path, bool noPaths )
{
    PROF_SCOPE();

    struct stat s;

    if ( noPaths || FileSys_IsAbsolute( path.data() ) )
    {
        if ( stat( path.c_str(), &s ) == 0 )
            return (s.st_mode & S_IFREG);
    }
    else
    {
		if ( stat( FileSys_FindFile( path ).c_str(), &s ) == 0 )
            return (s.st_mode & S_IFREG);
    }

    return false;
}


bool FileSys_Exists( const std::string &path, bool noPaths )
{
    PROF_SCOPE();

    if ( noPaths || FileSys_IsAbsolute( path.data() ) )
        return (access( path.c_str(), 0 ) != -1);
    else
		return ( access( FileSys_FindFile( path ).c_str(), 0 ) != -1 );
}


int FileSys_Access( const char* path, int mode )
{
    return access( path, mode );
}


int FileSys_Stat( const char* path, struct stat* info )
{
    return stat( path, info );
}


std::string FileSys_GetDirName( std::string_view sPath )
{
	if ( sPath.size() == 0 )
		return "";

	size_t i = sPath.size() - 1;
	for ( ; i > 0; i-- )
    {
		if ( sPath[ i ] == '/' || sPath[ i ] == '\\' )
            break;
    }

    std::string output( sPath.data(), i );
    return output;
}


std::string FileSys_GetBaseName( std::string_view sPath )
{
	return FileSys_GetDirName( sPath ) + PATH_SEP;
}


std::string FileSys_GetFileName( std::string_view sPath )
{
	if ( sPath.size() == 0 )
		return "";

	size_t i = sPath.size() - 1;
    for ( ; i > 0; i-- )
    {
		if ( sPath[ i ] == '/' || sPath[ i ] == '\\' )
            break;
    }

    // No File Extension Found
    if ( i == sPath.size() )
		return "";

    size_t startIndex = i + 1;

    if ( startIndex == sPath.size() )
		return "";

	std::string output( &sPath[ startIndex ], sPath.size() - startIndex );
	return output;
}


std::string FileSys_GetFileExt( std::string_view sPath, bool sStripPath )
{
    if ( sStripPath )
	{
		std::string fileName = FileSys_GetFileName( sPath );
		if ( fileName.empty() )
			return "";

		const char* dot = strrchr( fileName.data(), '.' );

		if ( !dot || dot == fileName.data() )
			return "";

		return dot + 1;
	}

	const char* dot = strrchr( sPath.data(), '.' );

	if ( !dot || dot == sPath.data() )
		return "";

	return dot + 1;
}


std::string FileSys_GetFileNameNoExt( std::string_view sPath )
{
	std::string name = FileSys_GetFileName( sPath );
    return name.substr( 0, name.rfind(".") );
}


std::string FileSys_CleanPath( std::string_view sPath )
{
    PROF_SCOPE();

    std::vector< std::string > v;

    int n = sPath.size();
    std::string out;

    std::string root;

    if ( FileSys_IsAbsolute( sPath.data() ) )
    {
#ifdef _WIN32

#elif __unix__
        root = "/";
#endif
    }

    // TODO: have this use strchr instead to check for '/', '\\', '.',
    // and just check the next character for if it's ".."
#if 1
    for ( int i = 0; i < n; i++ )
    {
        std::string dir = "";
        // forming the current directory.

        while ( i < n && ( sPath[ i ] != '/' && sPath[ i ] != '\\' ) )
        {
			dir += sPath[ i ];
            i++;
        }

        // if ".." , we pop.
        if ( dir == ".." )
        {
            if ( !v.empty() )
                v.pop_back();
        }
        else if ( dir != "." && dir != "" )
        {
            // push the current directory into the vector.
            v.push_back( dir );
        }
    }
    
    // build the cleaned path
    size_t len = v.size();
    for ( size_t i = 0; i < len; i++ )
    {
        out += (i+1 == len) ? v[i] : v[i] + PATH_SEP;
    }
#endif

    // vector is empty
    if ( out == "" )
        return root;  // PATH_SEP
    
    return root + out;
}


// Rename a File or Directory
bool FileSys_Rename( std::string_view srOld, std::string_view srNew )
{
	return ( rename( srOld.data(), srNew.data() ) == 0 );
}


// Get Date Created/Modified on a File
bool FileSys_GetFileTimes( std::string_view srPath, float* spCreated, float* spModified )
{
#ifdef WIN32
	// open first file
	HANDLE file = CreateFile(
	  srPath.data(),          // file to open
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
		Log_ErrorF( gFileSystemChannel, "Failed to open file handle: \"%s\" - error %d\n", srPath.data(), sys_get_error() );
		return false;
	}

    FILETIME create, modified;
	if ( !GetFileTime( file, &create, nullptr, &modified ) )
	{
		Log_ErrorF( gFileSystemChannel, "Failed to get file times of file: \"%s\" - error %d\n", srPath.data(), sys_get_error() );
		return false;
	}

    return true;
#else
	#error "FileSys_GetCreateTime()"
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
	#error "FileSys_GetCreateTime()"
#endif

    return false;
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


// TODO: make a faster linux direct system call version like i have for windows here
bool sys_scandir( const std::string& root, const std::string& path, std::vector< std::string >& files, ReadDirFlags flags = ReadDir_None )
{
    PROF_SCOPE();

#ifdef _WIN32
    WIN32_FIND_DATA ffd;

    std::string scanDir = root + "\\" + path + "\\";

    HANDLE hFind = FindFirstFile( (scanDir + "*").c_str(), &ffd );

    if ( INVALID_HANDLE_VALUE == hFind )
        return false;

    while ( FindNextFile( hFind, &ffd ) != 0 )
    {
        bool isDir = ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

        if ( (flags & ReadDir_Recursive) && isDir && strncmp(ffd.cFileName, "..", 2) != 0 )
        {
            sys_scandir( root, path + "\\" + ffd.cFileName, files, flags );
        }

        if ( (flags & ReadDir_NoDirs) && isDir )
        {
            continue;
        }

        if ( (flags & ReadDir_NoFiles) && FileSys_IsFile( path + "\\" + ffd.cFileName, true ) )
        {
            continue;
        }

        files.push_back( (flags & ReadDir_AbsPaths) ? scanDir + ffd.cFileName : path + "\\" + ffd.cFileName );
    }

    FindClose( hFind );

    return true;
#else
    if ( !FileSys_IsDir( path ) )
        return false;

    // NOTE: the _WIN32 one adds ".." to the path, while the directory iterator doesn't
    // so i manually add it myself to match the win32 one
    files.push_back( path + PATH_SEP + ".." );

    for ( const auto& rFile : std::filesystem::directory_iterator( path ) )
        files.push_back( rFile.path().string() );

    return true;
#endif
}


/* Scan an entire Directory  */
std::vector< std::string > FileSys_ScanDir( const std::string &path, ReadDirFlags flags )
{
    PROF_SCOPE();

    std::vector< std::string > files;

    if ( !FileSys_IsAbsolute( path.data() ) )
    {
        for ( auto searchPath : gSearchPaths )
        {
            if ( !sys_scandir( searchPath, path, files, flags ) )
                continue;

            if ( !(flags & ReadDir_AllPaths) )
                break;
        }
    }
    else
    {
        sys_scandir( path, "", files, flags );
    }

    return files;
}

