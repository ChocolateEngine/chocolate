#include "../../inc/shared/util.h"

#include <algorithm>


void str_upper(std::string &string)
{
	std::transform(string.begin(), string.end(), string.begin(), ::toupper);
}

void str_lower(std::string &string)
{
	std::transform(string.begin(), string.end(), string.begin(), ::tolower);
}

