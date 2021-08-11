#pragma once
#include <vector>
#include "Ruleset.hpp"
#include "RulesetParser.hpp"

/**
 * @brief Ruleset analysis algorithms => results => pretty printing
 * @details this class implements all of the different 
 * iptables analysis like analyzeDeadRules or findSubsetRules
 * and stores the results, which will be printed in printSummary
 * */
class IpAnalyzer{
public:

	/**
	 * @sa RulesetParser::parse
	 * */
	IpAnalyzer(Ruleset&& ruleset);
	/**
	 * dead rules are rules that never match a packet
	 * meaning preceding rules have alread matched all the packets
	 * that could match with the dead rule
	 */
	void analyzeDeadRules();
	/**
	 * subset rules are pairs of rules
	 * where one rule matches a subset of packets
	 * compared to what the other rule matches
	 * and is thus deemed unnecessary
	 * the analysis currently only compares rules from the same chain
	 * */
	void findSubsetRules();
	/**
	 * mergable rules are pairs of rules
	 * where both rules could be rewritten as a single one
	 * the analysis currently only compares rules from the same chain
	 * */
	void findMergeableRules();
	/**
	 * after dead rules have been identified
	 * this analysis looks at the rules that have 
	 * matched the packets and accepted or dropped them
	 * as a result these consumer rules will be listed with the
	 * number of packets that they took away from the dead rule
	 * */
	void findDeadRuleConsumers();
	/**
	 * identifes empty chains, cycles, unreachable chains
	 * and step cost, which corresponds to the amount of steps
	 * taken by the dead rule analysis
	 */
	void checkGraph();
	void printSummary(const parse_result_t&);

private:
	//! \returns chain in table [table_name] with name [chain_name] in ruleset or nullptr, if unsuccessful
	Chain* findChain(std::string_view table_name, std::string_view chain_name) ;

	struct PipeResult {
		bool somethingAccepted = false;
		PSET not_matched;
	};

	/**
	 * sends all packages represented by try_match through the contained rules 
	 * and chains (recursion)
	 * it is denoted in the Rule strcut wheter a rule has matched or jumped
	 * sucessfully to identify wheter it is dead or not
	 */
	PipeResult pipeChain(Chain& chain, PSET try_match);
	void pipeIfAvailable(std::string_view table_name, std::string_view chain_name);
	/**
	 * models the sequence of chains that is used in iptables
	 * by consequtive pipeChain calls
	 */
	void pipeAll(PSET try_match);
	size_t getTotalStepCost();

private:
	Ruleset ruleset_m;
	PSET accepted;///<packets that have been accepted during a pipeChain operation will be added to this set

	struct step_cost_t {
		std::unordered_map<const Chain*,size_t> cost;
	};
	std::optional<step_cost_t> step_cost_m;///<yielded by checkGraph analysis
	struct consumer_run_t {
		Chain* dead_rule_chain;
	};
	std::optional<consumer_run_t> consumer_run_m;///<yielded by checkGraph analysis

	struct graph_analysis_results_t {
		std::vector<const Chain*> emptyChains;
		std::vector<const Chain*> unreachableChains;
		std::vector<std::string> cycles;
	} graph_analysis_results;

	struct deadrule_analysis_results_t {
		std::vector<Rule*> deadRules;
		std::vector<Rule*> deadJumps;
	} deadrule_analysis_results;

	struct deadrule_consumer_analysis_results_t {
		std::unordered_map<Rule*,std::vector<Rule*>> consumers;
	} deadrule_consumer_analysis_results;

	struct subset_rule_analysis_results_t {
		std::vector<std::pair<Rule*,Rule*>> rules;//first is contained in second
	} subset_rule_results;
	struct mergeable_rule_analysis_results_t {
		std::vector<std::pair<Rule*,Rule*>> rules;
	} mergeable_rule_results;

	///!counter for progress information
	///!every rule evaluation is one step
	int total_steps = -1;
	int cur_steps = -1;
};
