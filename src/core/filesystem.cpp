#include "core/filesystem.h"
#include "util.h"

#include <array>
#include <fstream>

#include <sys/stat.h>

#ifdef _WIN32
	#include <direct.h>
    //#include <Windows.h>
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
    aSearchPaths.push_back( aExePath + "/bin" );
    aSearchPaths.push_back( aExePath + "/" + aWorkingDir );
    aSearchPaths.push_back( aExePath );
    aSearchPaths.push_back( aExePath + "/core" );  // always add core as a search path, at least for now
}


void FileSystem::AddSearchPath( const std::string& path )
{
	aSearchPaths.push_back( path );
}

void FileSystem::RemoveSearchPath( const std::string& path )
{
	vec_remove( aSearchPaths, path );
}

void FileSystem::InsertSearchPath( size_t index, const std::string& path )
{
	aSearchPaths.insert( aSearchPaths.begin() + index, path );
}


std::string FileSystem::FindFile( const std::string& file )
{
    for ( auto searchPath : aSearchPaths )
    {
        std::string fullPath = searchPath + "/" + file;

        // does item exist?
        if (access(fullPath.c_str(), 0) != -1)
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
        throw std::runtime_error( "[Filesystem] Failed to find file: " + srFilePath );

    /* Open file.  */
    std::ifstream file( fullPath, std::ios::ate | std::ios::binary );
    if ( !file.is_open(  ) )
        throw std::runtime_error( "[Filesystem] Failed to open file: " + srFilePath );

    int fileSize = ( int )file.tellg(  );
    std::vector< char > buffer( fileSize );
    file.seekg( 0 );

    /* Read contents.  */
    file.read( buffer.data(  ), fileSize );
    file.close(  );

    return buffer;
}


