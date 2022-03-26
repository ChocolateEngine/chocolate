/*
commandline.h ( Authored by Demez )

Simple Parser for command line options
*/

#pragma once

#include <string>
#include <vector>

#include "core/platform.h"


#ifdef _MSC_VER
	// Disable this useless warning of vars in a class needing a DLL Interface when exported
	#pragma warning(push)
	#pragma warning(disable:4251)
#endif


// call this after the other dlls are loaded, but not initialized yet
// it runs all startup config files, like config.cfg to store saved cvar values
void CORE_API core_post_load();


// Simple CommandLine parser
// TODO: maybe try to make this closer to argparse in python and setup arg registering
class CORE_API CommandLine
{
private:
	std::vector< std::string >          aArgs;

public:

	void                                Init( int argc, char *argv[] );
	int                                 GetIndex( const std::string& search );
	constexpr const std::vector<std::string>& GetArgs(  );
	constexpr size_t                    GetCount(  );

	bool                                Find( const std::string& search );

	const std::string&                  GetValue( const std::string& search, const std::string& fallback = "" );
	int                                 GetValue( const std::string& search, int fallback );
	float                               GetValue( const std::string& search, float fallback );
	double                              GetValue( const std::string& search, double fallback );

	// function to be able to find all values like this
	// returns true if it finds a value, false if it fails to
	bool                                GetValueNext( int& index, const std::string& search, std::string& ret );
	
	CommandLine(  );
	~CommandLine(  );
};


CORE_API extern CommandLine* cmdline;


#ifdef _MSC_VER
	#pragma warning(pop)
#endif

