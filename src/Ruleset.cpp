#include "Ruleset.hpp"

void Ruleset::forEachRule(std::function<void(Rule&)> cb){
	for(auto& table: tables)
		for(auto& chain: table.chains)
			for(auto& rule : chain->rules)
				cb(rule);
}
void Ruleset::resetRules(){
	forEachRule(
		[](auto& rule){
			rule.reset();
		}
	);
}
void Rule::reset(){
	touched = false;
	aliveMatch = 0;
	deadMatch = 0;
	aliveJump = 0;
	deadJump = 0;
	matched = 0;
}
bool Chain::isDropping() const{
	return special == Special::DROP || special == Special::REJECT;
}
bool Chain::isPredefined() const{
	return 
		(special != Special::NONE) ||
		(name == PREROUNTING_CHAIN || name == INPUT_CHAIN || name == OUTPUT_CHAIN || name == FORWARD_CHAIN || name == POSTROUTING_CHAIN) ||
		(name == CONNMARK_CHAIN || name == NFLOG_CHAIN || name == CT_CHAIN || name == DNAT_CHAIN || name == SNAT_CHAIN);
}

Chain* Table::findChain(std::string_view chain_name) {
	for(auto& chain : chains){
		if(chain->name == chain_name)
			return chain.get();
	}
	return nullptr;
}
Table* Ruleset::findTable(std::string_view table_name) {
	for(auto& table : tables){
		if(table.name == table_name) 
			return &table;
	}
	return nullptr;
}
Chain* Ruleset::findChain(std::string_view table_name, std::string_view chain_name) {
	auto table = findTable(table_name);
	if(table == nullptr){
		return nullptr;
	}
	return table->findChain(chain_name);
}
bool mergeable(const Rule& r1, const Rule& r2){
	if(r1.jumpTarget != r2.jumpTarget || r1.nat != r2.nat){
		return false;
	}
	if(r1.flag_segments[Rule::PROTOCOL] != r2.flag_segments[Rule::PROTOCOL])return false;
	int diff = 0; 
	bool r1_is_subset = false;
	bool r2_is_subset = false;
	for(size_t i = 0; i < Rule::_SIZE; ++i){
		if(r1.flag_segments[i] != r2.flag_segments[i] ){
			if(std::ranges::includes(r1.flag_segments[i],r2.flag_segments[i])){
				r2_is_subset = true;
			} else if (std::ranges::includes(r2.flag_segments[i],r1.flag_segments[i])){
				r1_is_subset = true;
			} else diff++;
		}
	}
	if(r1_is_subset && r2_is_subset){
		return false;
	}
	diff += r1_is_subset;
	diff += r2_is_subset;
	if(diff > 1){
		return false;
	}
	if(diff == 0){
		return true;
	}
	bool possible = true;
	auto check_single_ip_range_mergeability = [](const auto& set1, const auto& set2){
		if(set1.empty() && set2.empty())return true;
		if(set1.size() == 1 && set2.size() == 1){
			auto [start1,end1] = *std::begin(set1);
			auto [start2,end2] = *std::begin(set2);
			if(start1 != start2 || end1 != end2){
				if((start1 >= start2 && end1 <= end2) || (start2 >= start1 && end2 <= end1))return true;
				if(end1-start1 == end2-start2){
					if(!(start1 == end2 || start2 == end1)){
						return false;
					}
				}else{
					return false;
				}
			}
			return true;
		}
		return false;
	};
	possible &= check_single_ip_range_mergeability(r1.flag_segments[Rule::SRC_IP],r2.flag_segments[Rule::SRC_IP]);
	possible &= check_single_ip_range_mergeability(r1.flag_segments[Rule::DST_IP],r2.flag_segments[Rule::DST_IP]);
	return possible;
}
