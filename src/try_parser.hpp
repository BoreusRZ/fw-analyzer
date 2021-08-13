#pragma once
#include <stack>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <regex>

class try_parser {
public:
	try_parser(std::string_view);
	void setInput(std::string_view);

	std::string_view string();//reads \"[^\"]\" or [^ ]+
	void ws();//reads [ \t\n]
	void nextLine();
	void skip(size_t);
	std::optional<int> integer();//reads [+-]\d+
	std::optional<std::pair<std::string_view,std::string_view>> until(std::string_view pattern);

	bool matchStaticString(std::string_view); 

	bool done();

	void restore();
	void save();
	void pop();

private:
	std::unordered_map<std::string,std::regex> regex_cache;
	std::string input;
	std::stack<int> positions;
	size_t cur;//current position in input
};
