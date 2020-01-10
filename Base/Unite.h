#pragma once


#include <string>
#include <stdint.h>
#include <sstream>
#include <vector>


inline std::vector<std::string> spliteString(std::string str, char splite)
{
	std::vector<std::string> strings;
	std::istringstream is(str);
	std::string tmp;
	while (std::getline(is, tmp, splite)) {
		if (!tmp.empty())
			strings.push_back(tmp);
	}

	return strings;
}
