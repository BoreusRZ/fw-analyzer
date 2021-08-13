#include "log.hpp"
#include "util.hpp"
#include <fmt/core.h>
#include <vector>

namespace mlog{
	Level level = Level::INFO;
	std::string_view level_prefix="[ INFO ]", level_postfix="";

	std::vector<prefix_cb_t> prefixes;
	/* void pushPrefix(std::string s){ */
	/* 	fmt::print("looking at {}\n",s); */
	/* 	pushPrefix([cpy=std::string(s)](){ */
	/* 				return cpy; */
	/* 			}); */
	/* } */

	void pushPrefix(prefix_cb_t p){
		prefixes.emplace_back(p);
	}
	void popPrefix(){
		prefixes.pop_back();
	}
	Level getLevel(){
		return level;
	}
	void setLevel(Level level){
		switch (level){
			case Level::INFO:
				level_prefix = "[ INFO ]";
				level_postfix = "";
				break;
			case Level::DEBUG:
				level_prefix = "[ DEBUG]";
				level_postfix = "";
				break;
			case Level::WARNING:
				if(util::isTTY()){
					level_prefix = "\033[93m[ WARN ]";
					level_postfix = "\033[0m";
				}else{
					level_prefix = "[ WARN ]";
				}
				break;
			case Level::ERROR:
				if(util::isTTY()){
					level_prefix = "\033[31m[ ERROR]";
					level_postfix = "\033[0m";
				}else{
					level_prefix = "[ ERROR]";
				}
				break;
			case Level::SUCCESS:
				if(util::isTTY()){
					level_prefix = "\033[92m[SUCCES]";
					level_postfix = "\033[0m";
				}else{
					level_prefix = "[SUCCES]";
				}
				break;
			case Level::FATAL:
				if(util::isTTY()){
					level_prefix = "\033[31;1m[ FATAL]";
					level_postfix = "\033[0m";
				}else{
					level_prefix = "[ FATAL]";
				}
				break;
		}
	}
	void printPostfix(){
		fmt::print("{}",level_postfix);
	}
	void printPrefix(){
		fmt::print("{}",level_prefix);
		for(auto& prefix : prefixes){
			fmt::print("{}",prefix());
		}
		fmt::print(": ");
	}
}
