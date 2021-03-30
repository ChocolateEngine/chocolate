#include <functional>
#include <string>

typedef struct
{
	std::string str;
	std::function< void( std::string ) > func;
}command_s;
