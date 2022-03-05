#include "core/filesystem.h"
#include "core/console.h"
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


DLL_EXPORT FileSystem* filesys = nullptr;


FileSystem::FileSystem()
{
}


FileSystem::~FileSystem()
{
}


int FileSystem::Init( const char* workingDir )
{
	aWorkingDir = workingDir;
	aExePath = getcwd( 0, 0 );

	// change to desired working directory
	chdir( workingDir );

    // set default search paths
    DefaultSearchPaths(  );

    return 0;
}


const std::string& FileSystem::GetWorkingDir(  )
{
	return aWorkingDir;
}

void FileSystem::SetWorkingDir( const std::string& workingDir )
{
	aWorkingDir = workingDir;
}


const std::string& FileSystem::GetExePath(  )
{
    return aExePath;
}


const std::vector< std::string >& FileSystem::GetSearchPaths(  )
{
	return aSearchPaths;
}

void FileSystem::ClearSearchPaths(  )
{
    aSearchPaths.clear();
}

void FileSystem::DefaultSearchPaths(  )
{
    aSearchPaths.clear();
    AddSearchPath( aExePath + "/bin" );
    AddSearchPath( aExePath + "/" + aWorkingDir );
    AddSearchPath( aExePath );
    AddSearchPath( aExePath + "/core" );  // always add core as a search path, at least for now
}


void FileSystem::AddSearchPath( const std::string& path )
{
	aSearchPaths.push_back( CleanPath( path ) );
}

void FileSystem::RemoveSearchPath( const std::string& path )
{
	vec_remove( aSearchPaths, path );
}

void FileSystem::InsertSearchPath( size_t index, const std::string& path )
{
	aSearchPaths.insert( aSearchPaths.begin() + index, CleanPath( path ) );
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


std::string FileSystem::FindFile( const std::string& file )
{
    // if it's an absolute path already,
    // don't bother to look in the search paths for it, and make sure it exists
    if ( IsAbsolute( file ) )
        return exists( file ) ? file : "";

    if ( file == "" )
        return file;

    for ( auto searchPath : aSearchPaths )
    {
        std::string fullPath = searchPath + "/" + file;

        // does item exist?
        if ( exists( fullPath ) )
        {
            return fullPath;
        }
    }

    // file not found
    return "";
}


std::string FileSystem::FindDir( const std::string &dir )
{
    // if it's an absolute path already,
    // don't bother to look in the search paths for it, and make sure it exists
    if ( IsAbsolute( dir ) )
        return is_dir( dir ) ? dir : "";

    for ( auto searchPath : aSearchPaths )
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
std::vector< char > FileSystem::ReadFile( const std::string& srFilePath )
{
    // Find path first
    std::string fullPath = FindFile( srFilePath );
    if ( fullPath == "" )
    {
        LogError( "[Filesystem] Failed to find file: %s", srFilePath.c_str() );
        return {};
    }

    /* Open file.  */
    std::ifstream file( fullPath, std::ios::ate | std::ios::binary );
    if ( !file.is_open() )
    {
        LogError( "[Filesystem] Failed to open file: %s", srFilePath.c_str() );
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


bool FileSystem::IsAbsolute( const std::string &path )
{
#ifdef _WIN32
    return !PathIsRelative( path.c_str() );
#elif __unix__
    return path.starts_with( "/" );
#else
    return std::filesystem::path( path ).is_absolute();
#endif
}


bool FileSystem::IsRelative( const std::string &path )
{
#ifdef _WIN32
    return PathIsRelative( path.c_str() );
#elif __unix__
    return !path.starts_with( "/" );
#else
    return std::filesystem::path( path ).is_relative();
#endif
}


bool FileSystem::IsDir( const std::string &path, bool noPaths )
{
    struct stat s;

    if ( noPaths || IsAbsolute( path ) )
    {
        if ( stat( path.c_str(), &s ) == 0 )
            return (s.st_mode & S_IFDIR);
    }
    else
    {
        if ( stat( FindFile( path ).c_str(), &s ) == 0 )
            return (s.st_mode & S_IFDIR);
    }

    return false;
}


bool FileSystem::IsFile( const std::string &path, bool noPaths )
{
    struct stat s;

    if ( noPaths || IsAbsolute( path ) )
    {
        if ( stat( path.c_str(), &s ) == 0 )
            return (s.st_mode & S_IFREG);
    }
    else
    {
        if ( stat( FindFile( path ).c_str(), &s ) == 0 )
            return (s.st_mode & S_IFREG);
    }

    return false;
}


bool FileSystem::Exists( const std::string &path, bool noPaths )
{
    if ( noPaths || IsAbsolute( path ) )
        return (access( path.c_str(), 0 ) != -1);
    else
        return (access( FindFile(path).c_str(), 0 ) != -1);
}


int FileSystem::Access( const std::string &path, int mode )
{
    return access( path.c_str(), mode );
}

int FileSystem::Stat( const std::string &path, struct stat* info )
{
    return stat( path.c_str(), info );
}


std::string FileSystem::GetDirName( const std::string &path )
{
    size_t i = path.length();
    for (; i > 0; i-- )
    {
        if ( path[i] == '/' || path[i] == '\\' )
            break;
    }

    return path.substr(0, i);
}


std::string FileSystem::GetBaseName( const std::string &path )
{
    return GetDirName( path ) + PATH_SEP;
}


std::string FileSystem::GetFileName( const std::string &path )
{
    size_t i = path.length();
    for ( ; i > 0; i-- )
    {
        if ( path[i] == '/' || path[i] == '\\' )
            break;
    }

    return path.substr( std::min(i+1, path.length()), path.length() );
}


std::string FileSystem::GetFileExt( const std::string &path )
{
    const char *dot = strrchr( path.c_str(), '.' );
    if ( !dot || dot == path )
        return "";

    return dot + 1;
}


std::string FileSystem::GetFileNameNoExt( const std::string &path )
{
    std::string name = GetFileName( path );

    size_t i = name.length();
    for ( ; i > 0; i-- )
    {
        if ( name[i] == '.' )
            break;
    }

    return name.substr( 0, i );
}


std::string FileSystem::CleanPath( const std::string &path )
{
    std::vector< std::string > v;

    int n = path.length();
    std::string out;

    std::string root;

    if ( IsAbsolute( path ) )
    {
#ifdef _WIN32

#elif __unix__
        root = "/";
#endif
    }

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
        Puts( "TODO: wildcard check oh god\n" );
        return false;
    }
    else
    {
        return true;
    }
}


/* Read the first file in a Directory  */
DirHandle FileSystem::ReadFirst( const std::string &path, std::string &file, ReadDirFlags flags )
{
#ifdef _WIN32
    std::string readPath = path + "\\*";

    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    std::string absSearchPath;

    for ( auto searchPath: aSearchPaths )
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
bool FileSystem::ReadNext( DirHandle dirh, std::string &file )
{
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
        LogWarn( "[Filesystem] handle not in handle list??\n" );
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
bool FileSystem::ReadClose( DirHandle dirh )
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

        if ( (flags & ReadDir_NoFiles) && filesys->IsFile( path + "\\" + ffd.cFileName, true ) )
        {
            continue;
        }

        files.push_back( (flags & ReadDir_AbsPaths) ? scanDir + ffd.cFileName : path + "\\" + ffd.cFileName );
    }

    FindClose( hFind );

    return true;
#else
    if ( !filesys->IsDir( path ) )
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
std::vector< std::string > FileSystem::ScanDir( const std::string &path, ReadDirFlags flags )
{
    std::vector< std::string > files;

    if ( !IsAbsolute( path ) )
    {
        for ( auto searchPath : aSearchPaths )
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

