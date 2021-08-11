#include "IpAnalyzer.hpp"
#include "log.hpp"
#include "args.hpp"
#include <tabulate/tabulate.hpp>

#include <set>
#include <vector>
#include <cassert>
#include <ranges>
#include <algorithm>
void IpAnalyzer::findMergeableRules(){
	mlog::info("BEGINN MERGEABLE-RULE ANALYSIS\n");
	for(auto& table : ruleset_m.tables){
		for(auto& chain : table.chains){
			auto& rules = chain->rules;
			for(size_t i = 0; i < rules.size(); ++i){
				if(rules[i].shouldBeIgnored)continue;
				for(size_t j = i+1; j < rules.size(); ++j){
					if(rules[j].shouldBeIgnored)continue;
					auto intersection = INTERSECTION(rules[i].maximumMatchingSet,rules[j].maximumMatchingSet);
					if(mergeable(rules[i],rules[j])){
						mergeable_rule_results.rules.emplace_back(&rules[i],&rules[j]);
						break;
					}
					if(!intersection.isEmpty())break;
				}
			}
		}
	}
	std::ranges::sort(mergeable_rule_results.rules,[](const auto& p1, const auto& p2){
				return p1.first->line < p2.first->line;
			});
	if(args::verbose){
		for(auto [r1,r2] : mergeable_rule_results.rules){
			fmt::print("{}: {}\nmergeable with\n{}: {}\n\n",r1->line,r1->line_str,r2->line,r2->line_str);
		}
	}
	mlog::success("DONE MERGEABLE-RULE ANALYSIS\n");
}

void IpAnalyzer::findSubsetRules(){
	mlog::info("BEGINN SUBSET-RULE ANALYSIS\n");
	for(auto& table : ruleset_m.tables){
		for(auto& chain : table.chains){
			auto& rules = chain->rules;
			for(size_t i = 0; i < rules.size(); ++i){
				if(rules[i].shouldBeIgnored)continue;
				for(size_t j = i+1; j < rules.size(); ++j){
					if(rules[j].shouldBeIgnored)continue;
					auto intersection = INTERSECTION(rules[i].maximumMatchingSet,rules[j].maximumMatchingSet);
					if(rules[j].jumpTarget == rules[i].jumpTarget && intersection == rules[i].maximumMatchingSet){
						subset_rule_results.rules.emplace_back(&rules[i],&rules[j]);
						break;
					}
					if(!intersection.isEmpty())break;
				}
			}
		}
	}
	std::ranges::sort(subset_rule_results.rules,[](const auto& p1, const auto& p2){
				return p1.first->line < p2.first->line;
			});
	if(args::verbose){
		for(auto [r1,r2] : subset_rule_results.rules){
			fmt::print("{}: {}\nis subset of\n{}: {}\n\n",r1->line,r1->line_str,r2->line,r2->line_str);
		}
	}
	mlog::success("DONE SUBSET-RULE ANALYSIS\n");
}
IpAnalyzer::IpAnalyzer(Ruleset&& ruleset) : ruleset_m(std::forward<decltype(ruleset_m)>(ruleset))
{}

IpAnalyzer::PipeResult IpAnalyzer::pipeChain(Chain& chain, PSET try_match){
	switch(chain.special){
		case Chain::Special::RETURN:
			 throw std::runtime_error("return shouldnt be piped");
		case Chain::Special::ACCEPT:
			  accepted.UNION(try_match);
			  [[fallthrough]];
		case Chain::Special::DROP:
			  [[fallthrough]];
		case Chain::Special::REJECT:
			  return {!try_match.isEmpty(),{}};//packets are not further matched
		default:
			  break;
	}
	IpAnalyzer::PipeResult ret;
	for(auto& rule : chain.rules){
		cur_steps++;
		if(rule.shouldBeIgnored){
			if(step_cost_m && args::progress){
				cur_steps += step_cost_m->cost[rule.jumpTarget];
			}
			continue;
		}
		rule.touched = true;
		if(args::progress){
			mlog::log("starting ({}): {}\n",rule.line,rule.line_str);
			mlog::debug("input size {} ^= {}\n", try_match.segments.size(), util::getMemoryUsage(try_match.segments));
		}
		std::cout << std::flush;

		auto match = INTERSECTION(rule.maximumMatchingSet,try_match);
		if(match.isEmpty()){
			if(step_cost_m && args::progress){
				cur_steps += step_cost_m->cost[rule.jumpTarget];
			}
			rule.deadMatch++;
			rule.deadJump++;
			continue;
		}else{
			rule.aliveMatch++;
			if(consumer_run_m){
				rule.matched += match.getAmountPoints();
			}
		}
		assert(rule.jumpTarget != nullptr);
		if(rule.jumpTarget->special == Chain::Special::RETURN){
			ret.not_matched.UNION(match);
			continue;
		}
		try_match.INTERSECTION_NEGATED(rule.maximumMatchingSet);
		if(rule.jumpTarget->special == Chain::Special::DNAT){
			assert(rule.nat.has_value());
			const Rule::NAT_Transform& transform = *rule.nat;

			for(auto& seg : match.segments){
				seg.setInterval<PSegment::DST_IP_INDEX>(
						transform.start_ip,
						transform.end_ip);
				if(transform.has_port_change){
					seg.setInterval<PSegment::DST_PORT_INDEX>(
						transform.start_port,
						transform.end_port);
				}
			}

			match.compact();
			try_match.UNION(match);
			ret.somethingAccepted = true;
			continue;
		}else if(rule.jumpTarget->special == Chain::Special::SNAT){
			assert(rule.nat.has_value());
			const Rule::NAT_Transform& transform = *rule.nat;
			for(auto& seg : match.segments){
				seg.setInterval<PSegment::SRC_IP_INDEX>(
						transform.start_ip,
						transform.end_ip);
				if(transform.has_port_change){
					seg.setInterval<PSegment::SRC_PORT_INDEX>(
						transform.start_port,
						transform.end_port);
				}else{
					auto& p_start = seg.getStart<PSegment::SRC_PORT_INDEX>();
					auto& p_end = seg.getEnd<PSegment::SRC_PORT_INDEX>();
					if(p_start <= 511)p_start = 0;
					else if(p_start <= 1023)p_start = 512;
					else p_start = 1024;
					if(p_end >= 1024)p_end = std::numeric_limits<std::remove_reference_t<decltype(p_end)>>::max();
					else if(p_end >= 512)p_end = 1023;
					else p_end = 511;
				}
			}
			match.compact();
			try_match.UNION(match);
			ret.somethingAccepted = true;
			continue;
		}

		auto [somethingAccepted,remaining] = pipeChain(*rule.jumpTarget,match);
		if(!somethingAccepted)remaining = std::move(match);
		if(somethingAccepted){
			rule.aliveJump++;
		}else{
			rule.deadJump++;
		}
		if(rule.jumpType == JumpType::GOTO){
			ret.not_matched.UNION(remaining);
		}
		else try_match.UNION(remaining);
		ret.somethingAccepted |= somethingAccepted;
	}
	ret.not_matched.UNION(try_match);
	switch(chain.policy){
		case Chain::Policy::ACCEPT:
			accepted.UNION(ret.not_matched);
			[[fallthrough]];
		case Chain::Policy::DROP:
			[[fallthrough]];
		default:
		   break;
	}
	return ret;
}
void IpAnalyzer::pipeIfAvailable(std::string_view table_name, std::string_view chain_name){
	auto chain = findChain(table_name, chain_name);
	if(chain == nullptr)return;
	auto try_match = std::move(accepted);
	accepted = PSET();

	/* mlog::pushPrefix(fmt::format("[{}|{}]",table_name,chain_name)); */
	auto prefix = fmt::format("[{}|{}]",table_name,chain_name);
	mlog::pushPrefix([&](){return prefix;});

	if(step_cost_m){
		total_steps = step_cost_m->cost[chain];
		cur_steps = 0;
		mlog::pushPrefix([&](){
					return fmt::format("[{:6.2f}%]", 100*(cur_steps/(double)(total_steps+1)));
				});
	}else{
		total_steps = -1;
	}


	pipeChain(*chain,try_match);


	mlog::popPrefix();
	if(step_cost_m)mlog::popPrefix();
}
Chain* IpAnalyzer::findChain(std::string_view table_name, std::string_view chain_name) {
	auto table = ruleset_m.findTable(table_name);
	if(table == nullptr){
		mlog::debug("table \"{}\" not found\n", table_name);
		return nullptr;
	}
	auto chain = table->findChain(chain_name);
	if(chain == nullptr){
		mlog::debug("chain \"{}\" not found in {}-table\n", chain_name, table_name);
		return nullptr;
	}
	return chain;
}
size_t IpAnalyzer::getTotalStepCost(){
	if(!step_cost_m)return 0;
	size_t ret = 0;
	auto stepCostIfAvailable = [&](std::string_view table_name, std::string_view chain_name)->size_t{
		auto chain = findChain(table_name,chain_name);
		if(chain)return step_cost_m->cost[chain];
		return 0;
	};
	ret += stepCostIfAvailable(RAW_TABLE,PREROUNTING_CHAIN);
	ret += stepCostIfAvailable(MANGLE_TABLE,PREROUNTING_CHAIN);
	ret += stepCostIfAvailable(NAT_TABLE,PREROUNTING_CHAIN);

	ret += stepCostIfAvailable(MANGLE_TABLE,INPUT_CHAIN);
	ret += stepCostIfAvailable(NAT_TABLE,INPUT_CHAIN);
	ret += stepCostIfAvailable(FILTER_TABLE,INPUT_CHAIN);

	ret += stepCostIfAvailable(MANGLE_TABLE,FORWARD_CHAIN);
	ret += stepCostIfAvailable(FILTER_TABLE,FORWARD_CHAIN);
	ret += stepCostIfAvailable(MANGLE_TABLE,POSTROUTING_CHAIN);
	ret += stepCostIfAvailable(NAT_TABLE,POSTROUTING_CHAIN);

	ret += stepCostIfAvailable(RAW_TABLE,OUTPUT_CHAIN);
	ret += stepCostIfAvailable(MANGLE_TABLE,OUTPUT_CHAIN);
	ret += stepCostIfAvailable(NAT_TABLE,OUTPUT_CHAIN);
	ret += stepCostIfAvailable(FILTER_TABLE,OUTPUT_CHAIN);

	ret += stepCostIfAvailable(MANGLE_TABLE,POSTROUTING_CHAIN);
	ret += stepCostIfAvailable(NAT_TABLE,POSTROUTING_CHAIN);
	return ret;
}
void IpAnalyzer::pipeAll(PSET try_match){
	accepted = try_match;

	pipeIfAvailable(RAW_TABLE,PREROUNTING_CHAIN);
	pipeIfAvailable(MANGLE_TABLE,PREROUNTING_CHAIN);
	pipeIfAvailable(NAT_TABLE,PREROUNTING_CHAIN);

	PSET store_accepted = accepted;

	pipeIfAvailable(MANGLE_TABLE,INPUT_CHAIN);
	pipeIfAvailable(NAT_TABLE,INPUT_CHAIN);
	pipeIfAvailable(FILTER_TABLE,INPUT_CHAIN);

	accepted = std::move(store_accepted);

	pipeIfAvailable(MANGLE_TABLE,FORWARD_CHAIN);
	pipeIfAvailable(FILTER_TABLE,FORWARD_CHAIN);

	pipeIfAvailable(MANGLE_TABLE,POSTROUTING_CHAIN);
	pipeIfAvailable(NAT_TABLE,POSTROUTING_CHAIN);

	accepted = std::move(try_match);

	pipeIfAvailable(RAW_TABLE,OUTPUT_CHAIN);
	pipeIfAvailable(MANGLE_TABLE,OUTPUT_CHAIN);
	pipeIfAvailable(NAT_TABLE,OUTPUT_CHAIN);
	pipeIfAvailable(FILTER_TABLE,OUTPUT_CHAIN);

	pipeIfAvailable(MANGLE_TABLE,POSTROUTING_CHAIN);
	pipeIfAvailable(NAT_TABLE,POSTROUTING_CHAIN);
}
void IpAnalyzer::analyzeDeadRules(){
	mlog::info("BEGINN DEAD RULE ANALYSIS\n");
	mlog::info("ruleset complexity = {}\n",getTotalStepCost());
	pipeAll(PSET{{{}}});

	for(auto& table : ruleset_m.tables){
		for(auto& chain : table.chains){
			for(auto& rule : chain->rules){
				if(!rule.touched)continue;
				if(rule.aliveMatch == 0){
					deadrule_analysis_results.deadRules.push_back(&rule);
				} else if(rule.aliveJump == 0 
						&& !rule.jumpTarget->isPredefined() 
						&& !rule.jumpTarget->rules.empty()
						&& !args::config.silence.no_jump_target.contains(rule.jumpTarget->name)
				){
					deadrule_analysis_results.deadJumps.push_back(&rule);
				}
			}
		}
	}
	if(args::verbose){
		for(auto r : deadrule_analysis_results.deadRules){
			fmt::print("dead rule:\n{}: {}\n\n",r->line,r->line_str);
		}
		for(auto r : deadrule_analysis_results.deadJumps){
			fmt::print("dead jump:\n{}: {}\n\n",r->line,r->line_str);
		}
	}
	mlog::success("DONE DEAD RULE ANALYSIS\n");
}
	
void IpAnalyzer::checkGraph(){
	std::vector<const Chain*> chains;
	std::unordered_map<const Chain *,size_t> id;
	//assign ids
	for(const auto& table : ruleset_m.tables){
		for(const auto& chain : table.chains){
			for(const auto& rule : chain->rules){
				//insert [rule.jumpTarget] into [id] and [chains]
				if(rule.jumpTarget == nullptr)continue;
				auto iter = id.find(rule.jumpTarget);
				if(iter == std::end(id)){
					chains.push_back(rule.jumpTarget);
					id[rule.jumpTarget] = id.size();
				}
			}
			//insert [chain] into [id] and [chains]
			auto iter = id.find(chain.get());
			if(iter == std::end(id)){
				chains.push_back(chain.get());
				id[chain.get()] = id.size();
			}
		}
	}

	//build graph
	std::vector<std::vector<size_t>> graph(id.size());
	{
		std::vector<std::set<size_t>> graph_temp(id.size());
		for(const auto& table : ruleset_m.tables){
			for(const auto& chain : table.chains){
				auto id1 = id[chain.get()];
				for(const auto& rule : chain->rules){
					if(rule.jumpTarget == nullptr)continue;
					auto id2 = id[rule.jumpTarget];
					graph_temp[id1].insert(id2);
				}
			}
		}

		//copy adjecency sets to adjecency lists
		for(size_t i = 0; i < id.size(); ++i){
			std::move(std::begin(graph_temp[i]),std::end(graph_temp[i]),std::back_inserter(graph[i]));
		}
	}

	mlog::log("BEGIN GRAPH ANALYSIS\n");
	{
		//check if graph is tree
		std::vector<size_t> incomming_count(chains.size());//counts incomming edges
		for(size_t i = 0; i < chains.size(); ++i){
			for(auto nb : graph[i])incomming_count[nb]++;
		}
		bool tree = true;
		for(auto value : incomming_count){
			if(value > 1){
				tree = false;
				break;
			}
		}
		if(tree){
			mlog::log("graph is a TREE\n");
		}else{
			mlog::log("graph is NOT a TREE looking for cycles\n");

			//find cycles
			bool hasCycles = false;
			for(size_t i = 0; i < chains.size(); ++i){
				std::vector<bool> visiting(chains.size());
				std::function<bool(size_t)> dfs = [&](size_t node){
					if(visiting[node]){
						hasCycles = true;
						mlog::error("cycle found\n");
						mlog::error("{}",chains[node]->name);
						visiting[node] = false;
						return true;
					}
					visiting[node] = true;
					for(auto nb : graph[node]){
						if(dfs(nb)){
							mlog::print(" <- {}",chains[node]->name);
							if(visiting[node] == false){
								mlog::print("\n");
							}else{
								return true;
							}
						}
					}
					visiting[node] = false;
					return false;
				};
				dfs(i);
			}
			if(hasCycles){
				mlog::fatal("Terminating because of cylce(s)\n");
				exit(1);
			}else{
				mlog::success("no cycles detected\n");
			}
		}
	}
	{
		//find step costs
		step_cost_t step_cost;
		for(size_t i = 0; i < chains.size(); ++i){
			auto& cost = step_cost.cost;
			std::function<size_t(size_t)> dfs = [&chains,&cost,&dfs,&id](size_t node)->size_t{
				auto iter = cost.find(chains[node]);
				if(iter != std::end(cost))return iter->second;
				size_t self_cost = chains[node]->rules.size();
				for(const auto& rule : chains[node]->rules){
					if(rule.jumpTarget == nullptr)continue;
					self_cost += dfs(id[rule.jumpTarget]);
				}
				cost[chains[node]] = self_cost;
				return self_cost;
			};
			dfs(i);
		}
		for(auto& table : ruleset_m.tables){
			for(auto& chain : table.chains){
				mlog::debug("{}|{} [{}]:\n",table.name,chain->name,step_cost.cost[chain.get()]);
			}
		}
		step_cost_m = std::move(step_cost);
	}
	{
		//find unreachable chains
		std::vector<bool> reachable(chains.size());
		for(size_t i = 0; i < chains.size(); ++i){
			std::function<void(size_t)> dfs = [&](size_t node) -> void{
				if(reachable[node])return;
				reachable[node] = true;
				for(auto nb : graph[node]){
					dfs(nb);
				}
			};
			if(chains[i]->isPredefined())dfs(i);
		}
		for(size_t i = 0; i < chains.size(); ++i){
			if(!reachable[i]){
				graph_analysis_results.unreachableChains.push_back(chains[i]);
			}else if(chains[i]->rules.empty() && !chains[i]->isPredefined()
			 	&& !args::config.silence.empty_chains.contains(chains[i]->name)){

				graph_analysis_results.emptyChains.push_back(chains[i]);
				ruleset_m.forEachRule([&](Rule& rule){
							if(rule.jumpTarget == chains[i]){
								rule.jumpTargetIsEmpty = true;
							}
						});
			}
		}
	}

	if(args::verbose){
		for(auto chain : graph_analysis_results.emptyChains){
			fmt::print("empty chain:\n{}: {}\n\n",chain->line,chain->name);
		}
		for(auto chain : graph_analysis_results.unreachableChains){
			fmt::print("unreachable chain:\n{}: {}\n\n",chain->line,chain->name);
		}
	}
	mlog::success("DONE GRAPH ANALYSIS\n");


}
void IpAnalyzer::findDeadRuleConsumers(){
	if(!deadrule_analysis_results.deadRules.empty()){
		consumer_run_m.emplace();
		mlog::info("BEGINN DEAD RULE CONSUMER IDENTIFICATION\n");
		for(auto dead_rule : deadrule_analysis_results.deadRules){
			mlog::log("===========================================================\n");
			mlog::log("starting ({}): {}\n",dead_rule->line,dead_rule->line_str);

			ruleset_m.resetRules();
			pipeAll(dead_rule->maximumMatchingSet);

			std::vector<Rule*> consumers;
			ruleset_m.forEachRule([&](auto& rule){
						if(rule.aliveMatch != 0 && (rule.jumpTarget->isDropping()
									|| (rule.table_name == dead_rule->table_name && rule.jumpTarget->special != Chain::Special::NONE))){
							consumers.push_back(&rule);
						}
					});
			std::ranges::sort(consumers,[](const auto& r1, const auto& r2){
						return r1->matched > r2->matched;
					});
			for(auto rule : consumers){
				mlog::log("consumed by ({}) {}: {}\n",rule->line,rule->matched,rule->line_str);
			}
			deadrule_consumer_analysis_results.consumers[dead_rule] = std::move(consumers);
		}
		mlog::success("DONE DEAD RULE CONSUMER IDENTIFICATION\n");
	}else{
		mlog::info("there are no dead rules\n");
	}
}
void IpAnalyzer::printSummary(const parse_result_t& parse_results){
	mlog::info("BEGINN SUMMARY\n");
	auto& dead_chains = graph_analysis_results.unreachableChains;
	auto& empty_chains = graph_analysis_results.emptyChains;;
	auto& deadrules = deadrule_analysis_results.deadRules;
	auto& deadjumps = deadrule_analysis_results.deadJumps;
	tabulate::Table table;

	auto join_basic = [](const auto& range, std::string sep = " ", std::string onEmpty = "-"){
		std::string ret;
		bool first = true;
		for(const auto& elem : range){
			if(first)first = false;
			else ret.append(sep);
			ret.append(fmt::format("{}",elem));
		}
		if(ret.empty())return onEmpty;
		return ret;
	};
	auto join = [&,join_basic](const auto& range, std::string sep = " ", std::string onEmpty = "-"){
		if(range.empty())return onEmpty;
		std::vector<size_t> lineNumbers;
		for(const auto& elem: range){
			lineNumbers.push_back(elem->line);
		}
		std::ranges::sort(lineNumbers);
		return join_basic(lineNumbers,sep,onEmpty);
	};
	table.add_row({"ANALYSIS","AMOUNT","LINE NUMBERS"});
	table.add_row({"dead rules", fmt::format("{}",deadrules.size()),join(deadrules)});
	table.add_row({"dead jumps", fmt::format("{}",deadjumps.size()),join(deadjumps)});
	if(!empty_chains.empty())
		table.add_row({"empty chains",fmt::format("{}",empty_chains.size()),join(empty_chains)});
	if(!dead_chains.empty())
		table.add_row({"unreachable chains",fmt::format("{}",dead_chains.size()),join(dead_chains)});


	/* auto unknownFlags = std::ranges::set_difference( */
	/* 		parse_results.unknownFlags, */
	/* 		args::config.silence.unknown_flags); */
	auto unknownFlags = parse_results.unknownFlags;
	for(auto i = begin(unknownFlags); i != end(unknownFlags); ){
		if(args::config.silence.unknown_flags.contains(*i)){
			auto after = next(i);
			unknownFlags.erase(i);
			i = after;
		}else{
			++i;
		}
	}
	std::set<std::string_view> ruleset_chain_names;
	for(const auto& table : ruleset_m.tables){
		for(const auto& chain : table.chains){
			ruleset_chain_names.insert(chain->name);
		}
	}
	std::vector<std::string_view> not_contained_chains;

	std::ranges::set_difference(
			args::config.check_chains,
			ruleset_chain_names,
			std::back_inserter(not_contained_chains));

	if(!unknownFlags.empty())
		table.add_row({"unknown flags",fmt::format("{}",unknownFlags.size()),join_basic(unknownFlags)});
	if(!parse_results.rulesWithoutJumpTarget.empty())
		table.add_row({"rules without jumptarget",fmt::format("{}",parse_results.rulesWithoutJumpTarget.size()),join_basic(parse_results.rulesWithoutJumpTarget)});
	table.add_row({"subset rules",fmt::format("{}",subset_rule_results.rules.size()),
		join_basic(std::ranges::views::transform(
			subset_rule_results.rules,
			[](const auto& pair){
				return fmt::format("{}->{}",pair.first->line,pair.second->line);
			}))
		});
	table.add_row({"mergeable rules",fmt::format("{}",mergeable_rule_results.rules.size()),
		join_basic(std::ranges::views::transform(
			mergeable_rule_results.rules,
			[](const auto& pair){
				return fmt::format("{}+{}",pair.first->line,pair.second->line);
			}))
		});
	if(!not_contained_chains.empty()){
		table.add_row({"not contained mandatory chains", fmt::format("{}", not_contained_chains.size()), join_basic(not_contained_chains)});
	}
	for(int i = 0; i < 3; ++i){
		table[0][i].format()
			.font_align(tabulate::FontAlign::center)
			.font_style({tabulate::FontStyle::bold});
	}
	table.column(2).format().width(50);

	std::cout << table << std::endl;
	mlog::success("DONE SUMMARY\n");
}
