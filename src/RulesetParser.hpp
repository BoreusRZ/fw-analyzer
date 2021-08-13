#pragma once
#include <unordered_map>
#include <string_view>
#include <set>
#include <string>
#include <vector>
#include "parser/IpSet.hpp"
#include "Ruleset.hpp"
#include "SegmentSet.hpp"
#include "config.hpp"
#include "util.hpp"


struct parse_result_t {
	std::set<std::string> unknownFlags;
	std::vector<std::size_t> rulesWithoutJumpTarget;//in line numbers
};

struct RulesetParser {
public:

	void parseRuleset(std::string_view filename);
	void parseRuleset(std::istream& in);
	void parseRuleset_NFT(std::string_view filename);
	void parseRuleset_NFT(std::istream& in);

	void parseIpSets(std::string_view filename);
	void parseIpSets(std::istream& in);

	parse_result_t& getInfo();
	Ruleset&& releaseRuleset();


private:
	

	Table& curTable();
	Chain& getChainOrCreate(const std::string& name);
	Chain& getChainOrCreate_NFT(const std::string_view table_name, const std::string_view chain_name);

	void parseRule(std::istream& in, std::string line_str);
	void parseTable(std::istream& in, std::string&& table_name);

	void parseRule_NFT(std::istream& in, std::string line_str);
	void parseTable_NFT(std::istream& in);
	void finishChains();

public:
	std::unordered_map<std::string,std::unique_ptr<Chain>> chains;
	std::unordered_map<std::string,std::unordered_map<std::string,std::unique_ptr<Chain>>> nft_chains;
	util::indexer<std::string,uint8_t> in_interfaces;
	util::indexer<std::string,uint8_t> out_interfaces;
	util::indexer<std::string,uint8_t> protocols;

	Ruleset ruleset;
	IpSets ip_sets;

	parse_result_t results;
	std::vector<Table> tables;
	int line = 0;
};
