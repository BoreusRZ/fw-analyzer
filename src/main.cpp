#include "RulesetParser.hpp"
#include "log.hpp"
#include "IpAnalyzer.hpp"
#include <argparse/argparse.hpp>
#include "args.hpp"
#include <chrono>


int main(int argc, char** argv){
	auto t_start = std::chrono::high_resolution_clock::now();
	args::parse(argc,argv);


	RulesetParser parser;
	if(args::ipset_filename){
		parser.parseIpSets(*args::ipset_filename);
	}
	if(args::nft){
		parser.parseRuleset_NFT(*args::ruleset_filename);
	}else{
		parser.parseRuleset(*args::ruleset_filename);
	}

	IpAnalyzer analyzer(parser.releaseRuleset());
	auto& parse_results = parser.getInfo();

	analyzer.checkGraph();
	analyzer.analyzeDeadRules();
	if(args::analyze_consumers){
		analyzer.findDeadRuleConsumers();
	}
	analyzer.findSubsetRules();
	analyzer.findMergeableRules();
	analyzer.printSummary(parse_results);


	auto t_end = std::chrono::high_resolution_clock::now();
	auto elapsed = std::chrono::duration<double,std::milli>(t_end-t_start).count()/1000;
	mlog::info("took {:.2f} s\n",elapsed);
}
