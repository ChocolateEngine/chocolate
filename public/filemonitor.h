#pragma once

#include <functional>
#include <vector>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

class MonitoredFile {
protected:
	std::filesystem::file_time_type aModifiedTime;
public:
	std::string                     aPath;
	std::function< void() >         aCallback;
	/* Returns the last modification time of the file.  */
        fs::file_time_type GetTime() { return aModifiedTime; }
	void               SetTime( fs::file_time_type sTime ) { aModifiedTime = sTime; }
};

class FileMonitor {
	using Files = std::vector< MonitoredFile >;
protected:
	Files aFiles;
public:
	void AddOperation( const std::string& rPath, const auto&& srFunc ) {
		MonitoredFile f{};
		f.aPath     = rPath;
		f.aCallback = srFunc;
		try {
			f.SetTime( fs::last_write_time( rPath ) );
		}
		catch ( fs::filesystem_error ) {
			return;
		}
		aFiles.push_back( f );
	        f.aCallback();
	}
	void Update() {
		for ( auto&& rFile : aFiles ) {
			if ( rFile.GetTime() < fs::last_write_time( rFile.aPath ) ) {
				rFile.aCallback();
				rFile.SetTime( fs::last_write_time( rFile.aPath ) );
			}
		}
	}
};
