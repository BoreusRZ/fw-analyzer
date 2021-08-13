#include <gtest/gtest.h>
#include <sstream>
#include "RulesetParser.hpp"

TEST(ruleset_parser,nft){
	auto ruleset_text = 
		"add table ip raw\n"
		"add chain ip raw PREROUTING { type filter hook prerouting priority -300; policy accept; }\n"
		"add rule ip raw PREROUTING iifname \"fw.config\" ip protocol udp ip saddr 10.3.1.0/24 ip daddr 10.3.1.0/24\n";
	RulesetParser parser;
	std::stringstream ruleset_input(ruleset_text);
	parser.parseRuleset_NFT(ruleset_input);
	
	auto ruleset = parser.releaseRuleset();

	ASSERT_EQ(1,ruleset.tables.size());
	ASSERT_EQ(1,ruleset.tables[0].chains.size());
	ASSERT_EQ(1,ruleset.tables[0].chains[0]->rules.size());
}
