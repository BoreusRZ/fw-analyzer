#pragma once
#include <vector>
#include <string>
#include <string_view>
#include <concepts>

void parseWhitespace(std::istream& in);
std::pair<uint32_t,uint32_t> parseIP(std::string_view in, int line = -1);
std::pair<uint32_t,uint32_t> parseIP(std::istream& in, int line = -1);
std::vector<std::pair<uint16_t,uint16_t>> parsePortList(std::string_view ports);


template<typename integral>
integral toInt(const std::string_view sv){
	std::stringstream ss(sv.data());
	integral i;
	ss >> i;
	return i;
}
