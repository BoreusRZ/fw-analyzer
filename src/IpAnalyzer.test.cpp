#include <gtest/gtest.h>
#include <sstream>
#include "RulesetParser.hpp"

#define private public
#include "IpAnalyzer.hpp"


bool contains(auto range, auto subset, auto compare){
	for(const auto& x : subset){
		bool contained = false;
		for(const auto& y : range){
			if(compare(y,x)){
				contained = true;
				break;
			}
		}
		if(!contained)return false;
	}
	return true;
}
IpAnalyzer setupAnalyzer(const char* ruleset){
	RulesetParser parser;

	std::stringstream ruleset_input(ruleset);
	parser.parseRuleset(ruleset_input);

	return IpAnalyzer(parser.releaseRuleset());
}
IpAnalyzer setupAnalyzer(const char* ipset, const char* ruleset){
	RulesetParser parser;
	std::stringstream ip_set_input(ipset);
	parser.parseIpSets(ip_set_input);

	std::stringstream ruleset_input(ruleset);
	parser.parseRuleset(ruleset_input);

	return IpAnalyzer(parser.releaseRuleset());
}
bool lineCompare(const Rule* rule, int line){
	return rule->line == line;
}
bool linePairCompare(const std::pair<Rule*,Rule*>& rules, std::pair<int,int> lines){
	return rules.first->line == lines.first && rules.second->line == lines.second;
}
TEST(ipanalyzer,match_set){
	auto analyzer = setupAnalyzer(
		"create abc hash:net\n"
		"add abc 1.2.3.4\n",

		"*raw\n"
		"-A PREROUTING --match-set abc src -j ACCEPT\n"
		"-A PREROUTING -s 1.2.3.4/32 -j ACCEPT\n"
	);
	analyzer.analyzeDeadRules();


	auto dead_rule_lineno = {3};
	EXPECT_TRUE(contains(analyzer.deadrule_analysis_results.deadRules,dead_rule_lineno,lineCompare));
	EXPECT_EQ(analyzer.deadrule_analysis_results.deadRules.size(),1);
}
TEST(ipanalyzer,match_set_with_list_set){
	auto analyzer = setupAnalyzer(
		"create abc hash:net\n"
		"add abc 1.2.3.4\n"
		"create mylist list:set\n"
		"add mylist abc\n",

		"*raw\n"
		"-A PREROUTING --match-set mylist src -j ACCEPT\n"
		"-A PREROUTING -s 1.2.3.4/32 -j ACCEPT\n"
	);
	analyzer.analyzeDeadRules();


	auto dead_rule_lineno = {3};
	EXPECT_TRUE(contains(analyzer.deadrule_analysis_results.deadRules,dead_rule_lineno,lineCompare));
	EXPECT_EQ(analyzer.deadrule_analysis_results.deadRules.size(),1);
}

TEST(ipanalyzer,match_set2){
	auto analyzer = setupAnalyzer(
		"create abc hash:net\n"
		"add abc 1.2.3.4\n",

		"*raw\n"
		"-A PREROUTING --match-set abc dst -j ACCEPT\n"
		"-A PREROUTING -s 1.2.3.4/32 -j ACCEPT\n"
	);
	analyzer.analyzeDeadRules();


	auto not_dead_rule_lineno = {3};
	EXPECT_FALSE(contains(analyzer.deadrule_analysis_results.deadRules,not_dead_rule_lineno,lineCompare));
	EXPECT_TRUE(analyzer.deadrule_analysis_results.deadRules.empty());
}

TEST(ipanalyzer,dead_rule_by_jump_restricted){
	auto analyzer = setupAnalyzer(
		"*raw\n"
		":PREROUTING DROP [0:0]\n"
		"-A PREROUTING -s 0.0.0.0/24 -j ACCEPT\n"
		"-A PREROUTING -d 0.0.1.0/24 -sports 20 -j ACCEPT\n"
		"-A PREROUTING -d 0.0.1.2/32 -sports 20:210 -j ACCEPT\n"
		"-A PREROUTING -d 1.2.3.4/32 -dports 0 -j ACCEPT\n"
		"-A PREROUTING -d 1.2.3.6/32 -dports 0 -j ACCEPT\n"
		"-A PREROUTING -d 1.0.0.0/12 -dports 1 -j other\n"
		"-A other -d 2.2.2.2/32 -j DROP\n"
		"COMMIT\n"
	);
	analyzer.analyzeDeadRules();


	auto dead_rule_lineno = {9};
	auto dead_jump_lineno = {8};
	auto& [deadRules,deadJumps] = analyzer.deadrule_analysis_results;
	EXPECT_TRUE(contains(deadRules,dead_rule_lineno,lineCompare));
	EXPECT_EQ(deadRules.size(),1);

	EXPECT_TRUE(contains(deadJumps,dead_jump_lineno,lineCompare));
	EXPECT_EQ(deadJumps.size(),1);
}

TEST(ipanalyzer,subset_rule_ip){
	auto analyzer = setupAnalyzer(
		"*raw\n"
		":PREROUTING DROP [0:0]\n"
		"-A PREROUTING -s 1.2.3.4/32 -j ACCEPT\n"
		"-A PREROUTING -s 1.2.3.0/24 -j ACCEPT\n"
		"-A PREROUTING -s 1.2.0.0/16 -j ACCEPT\n"
		"-A PREROUTING -s 1.0.0.0/8 -j ACCEPT\n"
		"COMMIT\n"
	);
	analyzer.findSubsetRules();


	auto subset_rules_expected = {
		std::pair{3,4},
		std::pair{4,5},
		std::pair{5,6}
	};
	auto& subset_rules = analyzer.subset_rule_results.rules;
	EXPECT_TRUE(contains(subset_rules,subset_rules_expected,linePairCompare));
	EXPECT_EQ(subset_rules.size(),3);
}
TEST(ipanalyzer,subset_rule_port){
	auto analyzer = setupAnalyzer(
		"*raw\n"
		":PREROUTING DROP [0:0]\n"
		"-A PREROUTING -dports 30:31 -j ACCEPT\n"
		"-A PREROUTING -dports 29:31 -j ACCEPT\n"
		"-A PREROUTING -dports 0:3100 -j ACCEPT\n"
		"COMMIT\n"
	);
	analyzer.findSubsetRules();


	auto subset_rules_expected = {
		std::pair{3,4},
		std::pair{4,5}
	};
	auto& subset_rules = analyzer.subset_rule_results.rules;
	EXPECT_TRUE(contains(subset_rules,subset_rules_expected,linePairCompare));
	EXPECT_EQ(subset_rules.size(),2);
}
TEST(ipanalyzer,snat){
	auto analyzer = setupAnalyzer(
		"*raw\n"
		":PREROUTING DROP [0:0]\n"
		"-A PREROUTING -s 1.2.3.0/24 -j ACCEPT\n"
		"-A PREROUTING -s 1.2.3.0/24 -j ACCEPT\n"
		"-A PREROUTING -s 1.2.4.0/24 -j SNAT --to-source 1.2.3.0/24\n"
		"-A PREROUTING -s 1.2.3.4/32 -j ACCEPT\n"
		"-A PREROUTING -s 1.2.3.0/24 -j ACCEPT\n"
		"-A PREROUTING -s 1.2.4.0/24 -j ACCEPT\n"
		"COMMIT\n"
	);
	analyzer.analyzeDeadRules();


	auto deadrules_expected = {4,8};
	auto& deadRules = analyzer.deadrule_analysis_results.deadRules;
	EXPECT_TRUE(contains(deadRules,deadrules_expected,lineCompare));
	EXPECT_EQ(deadRules.size(),2);
}
TEST(ipanalyzer, different_ports_mergeable){
	auto analyzer = setupAnalyzer(
		"*raw\n"
		"-A PREROUTING -dport 10 -j ACCEPT\n"
		"-A PREROUTING -dport 11 -j ACCEPT\n"
	);
	analyzer.findMergeableRules();


	auto mergable_rules_expect = {std::pair{2,3}};
	auto& mergable_rules = analyzer.mergeable_rule_results.rules;
	EXPECT_TRUE(contains(mergable_rules,mergable_rules_expect,linePairCompare));
	EXPECT_EQ(mergable_rules.size(),1);
}
TEST(ipanalyzer, different_protocols_not_mergable){
	auto analyzer = setupAnalyzer(
		"*raw\n"
		"-A PREROUTING -s 10.111.254.106/32 -d 195.4.137.91/32 -p udp -m udp --dport 1194 -j ACCEPT\n"
		"-A PREROUTING -s 10.111.254.106/32 -d 195.4.137.91/32 -p tcp -m tcp --dport 1194 -j ACCEPT\n"
	);
	analyzer.findMergeableRules();


	auto mergable_rules_notexpect = {std::pair{2,3}};
	auto& mergable_rules = analyzer.mergeable_rule_results.rules;
	EXPECT_FALSE(contains(mergable_rules,mergable_rules_notexpect,linePairCompare));
	EXPECT_EQ(mergable_rules.size(),0);
}
TEST(ipanalyzer, iface_set){
	auto analyzer = setupAnalyzer(
		"create abc hash:net,iface\n"
		"add abc 4.3.2.1,other_interface\n"
		"add abc 1.2.3.4,myinterface_name\n",

		"*raw\n"
		"-A PREROUTING -i myinterface_name -j DROP\n"
		"-A PREROUTING -match-set abc dst,dst -j DROP\n"
		"-A PREROUTING -d 1.2.3.4/32 -o myinterface_name -j DROP\n"
		"-A PREROUTING -d 4.3.2.1/32 -o other_interface -j DROP\n"
	);
	analyzer.analyzeDeadRules();


	auto dead_rule_lineno = {4,5};
	EXPECT_TRUE(contains(analyzer.deadrule_analysis_results.deadRules,dead_rule_lineno,lineCompare));
	EXPECT_EQ(analyzer.deadrule_analysis_results.deadRules.size(),2);
}
TEST(ipanalyzer, port_set){
	auto analyzer = setupAnalyzer(
		"create abc hash:net,port\n"
		"add abc 4.3.2.1,udp:22\n"
		"add abc 1.1.1.1,icmp:1000-1100\n",

		"*raw\n"
		"-A PREROUTING -match-set abc src,src -j DROP\n"
		"-A PREROUTING -m udp -p udp -sport 23 -s 4.3.2.1 -j DROP\n"
		"-A PREROUTING -m udp -p udp -sport 22 -s 4.3.2.1 -j DROP\n"
		"-A PREROUTING -m udp -p udp -dport 22 -s 4.3.2.1 -j DROP\n"
		"-A PREROUTING -m tcp -p tcp -sport 22 -s 4.3.2.1 -j DROP\n"
		"-A PREROUTING -m icmp -p icmp -sport 22 -s 1.1.1.1 -j DROP\n"
		"-A PREROUTING -m icmp -p icmp -sport 1050 -s 1.1.1.1 -j DROP\n"
		"-A PREROUTING -m icmp -p icmp -sports 1000-1100 -s 1.1.1.1 -j DROP\n"
		"-A PREROUTING -m icmp -p icmp -sports 1000-1101 -s 1.1.1.1 -j DROP\n"
	);
	analyzer.analyzeDeadRules();


	auto dead_rule_lineno = {4,8,9};
	EXPECT_TRUE(contains(analyzer.deadrule_analysis_results.deadRules,dead_rule_lineno,lineCompare));
	EXPECT_EQ(analyzer.deadrule_analysis_results.deadRules.size(),3);
}
TEST(ipanalyzer, bug_consumers_in_output){
	auto analyzer = setupAnalyzer(
		"*raw\n"
		"-A OUTPUT -s 192.168.178.22 -j ACCEPT\n"
		"-A OUTPUT -s 192.168.178.22 -j ACCEPT\n"
		"-A OUTPUT -s 192.168.178.23 -j ACCEPT\n"
	);
	analyzer.analyzeDeadRules();
	analyzer.findDeadRuleConsumers();


	auto dead_rule_lineno = {3};
	EXPECT_TRUE(contains(analyzer.deadrule_analysis_results.deadRules,dead_rule_lineno,lineCompare));
	EXPECT_EQ(analyzer.deadrule_analysis_results.deadRules.size(),1);

	for(const auto& [dead_rule,consumers] : analyzer.deadrule_consumer_analysis_results.consumers){
		auto consumer_lineno = {2};
		EXPECT_TRUE(contains(consumers,consumer_lineno,lineCompare));
		EXPECT_EQ(consumers.size(),1);
	}
}
