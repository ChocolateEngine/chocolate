#pragma once

#include "core/platform.h"

#include <string>
#include <vector>


#ifdef _MSC_VER
	// Disable this useless warning of vars in a class needing a DLL Interface when exported
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif


class CORE_API FileSystem
{
public:
	FileSystem();
	~FileSystem();

	int                                     Init( const char* workingDir );

	const std::string&                      GetWorkingDir(  );
	void                                    SetWorkingDir( const std::string& workingDir );

	const std::string&                      GetExePath(  );

	const std::vector< std::string >&       GetSearchPaths(  );
	void                                    ClearSearchPaths(  );
	void                                    DefaultSearchPaths(  );

	void                                    AddSearchPath( const std::string& path );
	void                                    RemoveSearchPath( const std::string& path );
	void                                    InsertSearchPath( size_t index, const std::string& path );

	/* Find the path to a file within the search paths.  */
	std::string                             FindFile( const std::string& file );

	/* Find the path to a directory within the search paths.  */
	std::string                             FindDir( const std::string& dir );

	/* Reads a file into a byte array.  */
	std::vector< char >                     ReadFile( const std::string& file );

	/* Is path an absolute path?  */
	bool                                    IsAbsolute( const std::string &path );

private:

	std::string                             aWorkingDir;
	std::string                             aExePath;

	std::vector< std::string >              aSearchPaths;
};


CORE_API extern FileSystem* filesys;

#ifdef _MSC_VER
	#pragma warning(pop)
#endif

