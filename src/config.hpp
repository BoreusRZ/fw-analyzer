#pragma once
#include <string_view>
#include <string>
#include <optional>
#include <set>
#include <map>

/**
 * @brief class for loading and storing the configuration file
 * @details the file uses yaml format
 * and the names of the members represent the hirarchy
 */
struct config_t {
	
	struct silence_t {
		/** 
		 * flags that will not be listed as unknown flags
		 * in IpAnalyzer::printSummary
		 * flags may not contain the prefix '-' or '--'
		 */
		std::set<std::string_view> unknown_flags;
		/**
		 * chain names if a rule has one of these names as jump target
		 * it will never be printed as a dead jump
		 */
		std::set<std::string_view> no_jump_target;
		/**
		 * chain names that are allowed to be empty
		 */
		std::set<std::string_view> empty_chains;
	} silence;
	struct disable_t {
		struct rules_t {
			/**
			 * rules with these jump targets will be ignored
			 * @sa Rule::shouldBeIgnored
			 */
			std::set<std::string_view> with_jump_target;
			/**
			 * rules with these flags will be ignored\n
			 * there can be a space sperated argument e.g. m policy\n
			 * flags may not contain the prefix '-' or '--'\n
			 */
			std::map<std::string_view,std::optional<std::string_view>> with_flags;
		} rules;
	} disable;
	std::set<std::string_view> interfaces;
	std::set<std::string_view> check_chains;

	std::string file;
	config_t() = default;
	config_t(const std::string& filename);
	void print();

};
