#include "config.hpp"
#include "util.hpp"

#include <ryml.hpp>
#include <iostream>
#include <fstream>

auto loadFile(const std::string& name) -> std::string {
	std::ifstream file(name);
	return std::string(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());
}
auto toSV(c4::csubstr sub) -> std::string_view {
	return  {sub.str,sub.len};
}
config_t::config_t(const std::string& filename) :
	file(loadFile(filename))
{
	auto substr = c4::substr(file.data(),file.data()+file.size());
	auto tree = ryml::parse(substr);

	for(const auto& elem : tree["silence"]["unknown_flags"]){
		silence.unknown_flags.insert(toSV(elem.val()));
	}
	if(tree.rootref().has_child("silence")){
		if(tree["silence"].has_child("no_jump_target")){
			for(const auto& elem : tree["silence"]["no_jump_target"]){
				silence.no_jump_target.insert(toSV(elem.val()));
			}
		}
		if(tree["silence"].has_child("empty_chains")){
			for(const auto& elem : tree["silence"]["empty_chains"]){
				silence.empty_chains.insert(toSV(elem.val()));
			}
		}
	}
	if(tree.rootref().has_child("disable")){
		if(tree["disable"].has_child("rules")){
			if(tree["disable"]["rules"].has_child("with_jump_target")){
				for(const auto& elem : tree["disable"]["rules"]["with_jump_target"]){
					disable.rules.with_jump_target.insert(toSV(elem.val()));
				}
			}

			if(tree["disable"]["rules"].has_child("with_flags")){
				for(const auto& elem : tree["disable"]["rules"]["with_flags"]){
					auto val = toSV(elem.val());
					std::string_view flag;
					std::optional<std::string_view> args;
					if(val.find(' ') != std::string_view::npos){
						auto [l_flag,l_arg] = util::unpack<2>(util::split(val," ").begin());
						flag = l_flag;
						args = l_arg;
					}else{
						flag = val;
					}
					disable.rules.with_flags.emplace(flag,args);
				}
			}
		}
	}
	if(tree.rootref().has_child("check_chains")){
		for(const auto& elem : tree["check_chains"]){
			check_chains.insert(toSV(elem.val()));
		}
	}
	if(tree.rootref().has_child("interfaces")){
		for(const auto& elem : tree["interfaces"]){
			interfaces.insert(toSV(elem.val()));
		}
	}
}
void config_t::print(){
	std::cout << file << std::endl;
}
