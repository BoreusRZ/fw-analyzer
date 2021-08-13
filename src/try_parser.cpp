#include "try_parser.hpp"

try_parser::try_parser(std::string_view input){
	setInput(input);
}
void try_parser::setInput(std::string_view input){
	this->input = input;
	this->cur = 0;
}
void try_parser::save(){
	positions.push(cur);
}
void try_parser::restore(){
	cur = positions.top();
	positions.pop();
}
void try_parser::pop(){
	positions.pop();
}
void try_parser::nextLine(){
	while(input[cur] != '\n')++cur;
	++cur;
}
std::optional<int> try_parser::integer(){
	int ret = 0;
	bool negative = false;

	if(input[cur] == '+'){
		++cur;
	}else if(input[cur] == '-'){
		++cur;
		negative = true;
	}

	bool nodigit = true;
	while(input[cur] >= '0' && input[cur] <= '9' && cur < input.size()){
		nodigit = false;
		ret *= 10;
		ret += input[cur]-'0';
		++cur;
	}
	if(nodigit)return {};
	if(negative)ret = -ret;
	return {ret};
}
std::string_view try_parser::string(){
	size_t start = cur;
	if(input[cur] == '\"'){
		++cur;
		while(input[cur] != '\"' && cur < input.size())++cur;
		++cur;
	}else{
		while(input[cur] != ' ' && input[cur] != '\t' && input[cur] != '\n'  && cur < input.size())
			++cur;
	}
	return {&input[start],cur-start};
}
void try_parser::ws(){
	while(input[cur] == ' ' || input[cur] == '\t' || input[cur] == '\n'){
		++cur;
	}
}
bool try_parser::done(){
	return cur >= input.size();
}
bool try_parser::matchStaticString(std::string_view pattern){
	if(input.size()-cur < pattern.size())return false;

	for(size_t i = 0; i < pattern.size(); ++i){
		if(input[cur+i] != pattern[i]){
			return false;
		}
	}

	cur += pattern.size();
	return true;
}
void try_parser::skip(size_t amount){
	cur += amount;
}
std::optional<std::pair<std::string_view,std::string_view>> try_parser::until(std::string_view pattern){
	auto str = std::string{pattern};
	auto [regex_iter,b] = regex_cache.try_emplace(str,str);
	std::smatch match;
	const auto& c_input = input;
	std::regex_search(c_input.begin()+cur,c_input.end(),match,regex_iter->second);
	if(match.empty()){
		return {};
	}else{
		auto p_cur = cur;
		cur += match.position();
		return std::pair{
			std::string_view{input.begin()+p_cur,input.begin()+cur},
			std::string_view{input.begin()+cur,input.begin()+cur+match.length()}
		};
	}
}
