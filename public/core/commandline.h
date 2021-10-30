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


// Simple CommandLine parser
// TODO: maybe try to make this closer to argparse in python and setup arg registering
class CORE_API CommandLine
{
private:
	std::vector< std::string >          aArgs;

public:

	void                                Init( int argc, char *argv[] );
	int                                 GetIndex( const std::string& search );
	const std::vector<std::string>&     GetAll(  );

	bool                                Find( const std::string& search );

	const std::string&                  GetValue( const std::string& search, const std::string& fallback = "" );
	int                                 GetValue( const std::string& search, int fallback );
	float                               GetValue( const std::string& search, float fallback );
	double                              GetValue( const std::string& search, double fallback );
	
	CommandLine(  );
	~CommandLine(  );
};


CORE_API extern CommandLine* cmd;


#ifdef _MSC_VER
	#pragma warning(pop)
#endif

