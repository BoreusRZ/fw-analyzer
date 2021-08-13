#pragma once
#include <optional>
#include <string>
#include "config.hpp"

namespace args {
	inline std::optional<std::string> ruleset_filename;
	inline std::optional<std::string> ipset_filename;
	inline bool verbose;
	inline bool progress;
	inline bool nft;
	inline bool analyze_consumers;
	inline int threads;
	inline config_t config;


	/**
	 * calls the argument parser\n
	 * @effect fills variables in args namespace
	 */
	void parse(int argc, char** argv);
}
