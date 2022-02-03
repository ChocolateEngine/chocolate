#pragma once

#include "core/platform.h"

#include <string>
#include <vector>

#include <sys/stat.h>


#ifdef _WIN32
#define stat _stat
constexpr char PATH_SEP = '\\';
#elif __linux__
constexpr char PATH_SEP = '/';
#endif


#ifdef _MSC_VER
	// Disable this useless warning of vars in a class needing a DLL Interface when exported
	#pragma warning(push)
	#pragma warning(disable:4251)
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


class CORE_API FileSystem
{
public:
	FileSystem();
	~FileSystem();

	int                                     Init( const char* workingDir );

	const std::string&                      GetWorkingDir(  );
	void                                    SetWorkingDir( const std::string& workingDir );

	const std::string&                      GetExePath(  );

	// ================================================================================
	// Modify Search Paths

	const std::vector< std::string >&       GetSearchPaths(  );
	void                                    ClearSearchPaths(  );
	void                                    DefaultSearchPaths(  );

	void                                    AddSearchPath( const std::string& path );
	void                                    RemoveSearchPath( const std::string& path );
	void                                    InsertSearchPath( size_t index, const std::string& path );

	// ================================================================================
	// Main Funcs

	/* Find the path to a file within the search paths.  */
	std::string                             FindFile( const std::string& file );

	/* Find the path to a directory within the search paths.  */
	std::string                             FindDir( const std::string& dir );

	/* Reads a file into a byte array.  */
	std::vector< char >                     ReadFile( const std::string& file );

	// ================================================================================
	// File Information

	/* Is path an absolute path?  */
	bool                                    IsAbsolute( const std::string &path );

	/* Is path a directory?  */
	bool                                    IsDir( const std::string &path, bool noPaths = false );

	/* Is path a file?  */
	bool                                    IsFile( const std::string &path, bool noPaths = false );

	/* Does a file/folder exist?  */
	bool                                    Exists( const std::string &path, bool noPaths = false );

	/* Call access on a file  */
	int                                     Access( const std::string &path, int mode = 0 );

	/* Call stat on a file  */
	int                                     Stat( const std::string &path, struct stat* info );

	// ================================================================================
	// Path Utils

	/* Return the parent path of this path - No Trailing Slash */
	std::string                             GetDirName( const std::string &path );

	/* Return the parent path of this path - Has Trailing Slash */
	std::string                             GetBaseName( const std::string &path );

	/* Return the filename in this path  */
	std::string                             GetFileName( const std::string &path );

	/* Return the file extension */
	std::string                             GetFileExt( const std::string &path );

	/* Return the file name without the extension */
	std::string                             GetFileNameNoExt( const std::string &path );

	/* Cleans up the path, removes useless ".." and "."  */
	std::string                             CleanPath( const std::string &path );

	// ================================================================================
	// Directory Reading

	/* Read the first file in a Directory  */
	DirHandle                               ReadFirst( const std::string &path, std::string &file, ReadDirFlags flags = ReadDir_None );

	/* Get the next file in the Directory */
	bool                                    ReadNext( DirHandle dirh, std::string& file );

	/* Close a Directory  */
	bool                                    ReadClose( DirHandle dirh );

	/* Scan an entire Directory  */
	std::vector< std::string >              ScanDir( const std::string &path, ReadDirFlags flags = ReadDir_None );

	//DirHandle                               ReadFirst( const std::string &path, const std::string& wildcard, std::string &file );
	//std::vector< std::string >              ScanDir( const std::string &path, const std::string& wildcard, bool allPaths = false );

	// ================================================================================

private:

	std::string                             aWorkingDir;
	std::string                             aExePath;

	std::vector< std::string >              aSearchPaths;
};


CORE_API extern FileSystem* filesys;

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

