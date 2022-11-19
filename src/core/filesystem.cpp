#include "core/filesystem.h"
#include "core/console.h"
#include "core/profiler.h"
#include "core/platform.h"
#include "core/log.h"
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

std::string                gWorkingDir;
std::string                gExePath;
std::vector< std::string > gSearchPaths;


int FileSys_Init( const char* workingDir )
{
	gWorkingDir = workingDir;
	gExePath = getcwd( 0, 0 );

	// change to desired working directory
	chdir( workingDir );

    // set default search paths
	FileSys_DefaultSearchPaths();

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


const std::vector< std::string >& FileSys_GetSearchPaths(  )
{
	return gSearchPaths;
}

void FileSys_ClearSearchPaths()
{
	gSearchPaths.clear();
}

void FileSys_DefaultSearchPaths()
{
    gSearchPaths.clear();
    FileSys_AddSearchPath( gExePath + "/bin" );
    FileSys_AddSearchPath( gExePath + "/" + gWorkingDir );
    FileSys_AddSearchPath( gExePath );
    FileSys_AddSearchPath( gExePath + "/core" );  // always add core as a search path, at least for now
}


void FileSys_AddSearchPath( const std::string& path )
{
	gSearchPaths.push_back( FileSys_CleanPath( path ) );
}

void FileSys_RemoveSearchPath( const std::string& path )
{
	vec_remove( gSearchPaths, path );
}

void FileSys_InsertSearchPath( size_t index, const std::string& path )
{
	gSearchPaths.insert( gSearchPaths.begin() + index, FileSys_CleanPath( path ) );
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


std::string FileSys_FindFileF( const char* spFmt, ... )
{
    PROF_SCOPE();

    std::string path;
	VSTRING( path, spFmt );

    return FileSys_FindFile( path );
}


std::string FileSys_FindFile( const std::string& file )
{
    PROF_SCOPE();

    // if it's an absolute path already,
    // don't bother to look in the search paths for it, and make sure it exists
	if ( FileSys_IsAbsolute( file ) )
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


std::string FileSys_FindDir( const std::string &dir )
{
    PROF_SCOPE();

    // if it's an absolute path already,
    // don't bother to look in the search paths for it, and make sure it exists
	if ( FileSys_IsAbsolute( dir ) )
        return is_dir( dir ) ? dir : "";

    for ( auto searchPath : gSearchPaths )
    {
        std::string fullPath = searchPath + "/" + dir;

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


bool FileSys_IsAbsolute( const std::string &path )
{
#ifdef _WIN32
    return !PathIsRelative( path.c_str() );
#elif __unix__
    return path.starts_with( "/" );
#else
    return std::FileSys_path( path ).is_absolute();
#endif
}


bool FileSys_IsRelative( const std::string &path )
{
#ifdef _WIN32
    return PathIsRelative( path.c_str() );
#elif __unix__
    return !path.starts_with( "/" );
#else
    return std::FileSys_path( path ).is_relative();
#endif
}


bool FileSys_IsDir( const std::string &path, bool noPaths )
{
    PROF_SCOPE();

    struct stat s;

    if ( noPaths || FileSys_IsAbsolute( path ) )
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

    if ( noPaths || FileSys_IsAbsolute( path ) )
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

    if ( noPaths || FileSys_IsAbsolute( path ) )
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


std::string FileSys_GetDirName( const std::string &path )
{
    size_t i = path.length();
    for (; i > 0; i-- )
    {
        if ( path[i] == '/' || path[i] == '\\' )
            break;
    }

    return path.substr(0, i);
}


std::string FileSys_GetBaseName( const std::string &path )
{
	return FileSys_GetDirName( path ) + PATH_SEP;
}


std::string FileSys_GetFileName( const std::string &path )
{
    size_t i = path.length();
    for ( ; i > 0; i-- )
    {
        if ( path[i] == '/' || path[i] == '\\' )
            break;
    }

    return path.substr( std::min(i+1, path.length()), path.length() );
}


std::string FileSys_GetFileExt( const std::string &path )
{
    const char *dot = strrchr( path.c_str(), '.' );
    if ( !dot || dot == path )
        return "";

    return dot + 1;
}


std::string FileSys_GetFileNameNoExt( const std::string &path )
{
	std::string name = FileSys_GetFileName( path );
    return name.substr( 0, name.rfind(".") );
}


std::string FileSys_CleanPath( const std::string &path )
{
    PROF_SCOPE();

    std::vector< std::string > v;

    int n = path.length();
    std::string out;

    std::string root;

    if ( FileSys_IsAbsolute( path ) )
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

        while ( i < n && (path[i] != '/' && path[i] != '\\') )
        {
            dir += path[i];
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
        Log_Warn( "[Filesystem] handle not in handle list??\n" );
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

    if ( !FileSys_IsAbsolute( path ) )
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

