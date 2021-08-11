#pragma once
#include <string_view>
#include <fmt/core.h>

namespace mlog {
	enum class Level {
		DEBUG,WARNING,ERROR,INFO,FATAL,SUCCESS
	};
	using prefix_cb_t = std::function<std::string()>;
	void pushPrefix(prefix_cb_t);
	/* void pushPrefix(std::string); */
	
	void popPrefix();
	void setLevel(Level level);
	auto getLevel() -> Level;
	void printPostfix();
	void printPrefix();

	template<typename ... Args>
	void print(std::string_view format_string, Args...args){
		fmt::print(format_string,args...);
	}
	template<typename ... Args>
	void log(std::string_view format_string, Args...args){
		printPrefix();
		fmt::print(format_string,args...);
		printPostfix();
	}
	template<Level level, typename ... Args>
	void restoreLevel(std::string_view format_string, Args...args){
		auto prev = getLevel();
		setLevel(level);
		log(format_string,args...);
		setLevel(prev);
	}
	template<typename ... Args>
	void info(std::string_view format_string, Args...args){
		restoreLevel<Level::INFO>(format_string,args...);
	}
	template<typename ... Args>
	void error(std::string_view format_string, Args...args){
		restoreLevel<Level::ERROR>(format_string,args...);
	}
	template<typename ... Args>
	void fatal(std::string_view format_string, Args...args){
		restoreLevel<Level::FATAL>(format_string,args...);
		exit(1);
	}
	template<typename ... Args>
	void debug(__attribute((unused)) std::string_view format_string, __attribute((unused)) Args...args){
#ifndef NDEBUG
		restoreLevel<Level::DEBUG>(format_string,args...);
#endif
	}
	template<typename ... Args>
	void warn(std::string_view format_string, Args...args){
		restoreLevel<Level::WARNING>(format_string,args...);
	}
	template<typename ... Args>
	void success(std::string_view format_string, Args...args){
		restoreLevel<Level::SUCCESS>(format_string,args...);
	}
}

