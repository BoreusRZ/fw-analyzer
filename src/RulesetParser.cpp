#include "RulesetParser.hpp"
#include "try_parser.hpp"
#include "log.hpp"
#include <functional>
#include <fstream>
#include <sstream>
#include <tuple>
#include "args.hpp"
#include "util.hpp"
#include "parser/common.hpp"

bool parseNot(std::istream& in){
	parseWhitespace(in);
	if(in.peek() == '!'){
		in.get();
		return true;
	}
	return false;
}
std::string parseArg(std::istream& in){
	parseWhitespace(in);
	char c = in.peek();
	bool stringMode = false;
	if ( c == '\"' )stringMode = true;
	if(stringMode){
		std::ostringstream oss;
		oss << c;
		in.get();
		c = 0;
		while(c != '\"'){
			c = in.get();
			oss << c;
		}
		return oss.str();
	}else{
		std::string ret;
		in >> ret;
		return ret;
	}
}


Table& RulesetParser::curTable(){
	return tables.back();
}
Chain& RulesetParser::getChainOrCreate(const std::string& name){
	auto& chain_uptr = chains[name];
	if(chain_uptr.get() == nullptr){
		chain_uptr = std::make_unique<Chain>();
		chain_uptr->name = name;
		chain_uptr->line = line;
	}
	return *chain_uptr.get();
}

void RulesetParser::parseRule(std::istream& in, std::string line_str){
	Rule ret;
	ret.line_str = line_str;

	parseWhitespace(in);
	std::string chain_name;
	in >> chain_name;

	PSET result = PSET(bor::vector{PSegment{}});


	while(!in.eof()){
		parseWhitespace(in);
		bool prefixNot = parseNot(in);
		parseWhitespace(in);
		char c = in.get();
		if(in.eof())break;
		if(c != '-'){
			throw std::runtime_error(fmt::format("unexpected {} in line {}",c,line));
		}
		c = in.peek();
		if(c == '-')in.get();
		std::string flag_name;
		in >> flag_name;
		bool negation = parseNot(in)^prefixNot;
		if(args::config.disable.rules.with_flags.contains(flag_name) && 
				args::config.disable.rules.with_flags[flag_name] == std::nullopt){
			ret.shouldBeIgnored = true;
		}

		if(flag_name == "g" || flag_name == "j"){
			std::string jump_target_name;
			in >> jump_target_name;
			ret.jumpTarget = &getChainOrCreate(jump_target_name);
			if(flag_name == "g"){
				ret.jumpType = JumpType::GOTO;
			}else{
				ret.jumpType = JumpType::JUMP;
			}
		}else if(flag_name == "match-set"){
			std::string set_name;
			in >> set_name;
			parseWhitespace(in);
			std::string flags;
			in >> flags;
			ip_sets.get(set_name)->apply(ret,result,flags);
		}else if(flag_name.find("port") != std::string::npos){
			std::string port_list;
			in >> port_list;
			auto port_ranges = parsePortList(port_list);
			if(flag_name == "ports"){
				result.applyRange_helper<PSegment::SRC_PORT_INDEX>(port_ranges,negation);
				result.applyRange_helper<PSegment::DST_PORT_INDEX>(port_ranges,negation);
				ret.flag_segments[Rule::DST_PORT].insert(std::begin(port_ranges),std::end(port_ranges));
				ret.flag_segments[Rule::SRC_PORT].insert(std::begin(port_ranges),std::end(port_ranges));
			}else if(flag_name == "sports" || flag_name == "sport"){
				result.applyRange_helper<PSegment::SRC_PORT_INDEX>(port_ranges,negation);
				ret.flag_segments[Rule::SRC_PORT].insert(std::begin(port_ranges),std::end(port_ranges));
			}else if(flag_name == "dports" || flag_name == "dport"){
				result.applyRange_helper<PSegment::DST_PORT_INDEX>(port_ranges,negation);
				ret.flag_segments[Rule::DST_PORT].insert(std::begin(port_ranges),std::end(port_ranges));
			}
		}else if(flag_name == "i" || flag_name == "o"){
			std::string interface;
			in >> interface;

			auto& interface_map = (flag_name == "i" ? in_interfaces : out_interfaces);

			auto value = interface_map[interface];
			if(flag_name == "i"){
				result.applyRange_helper<PSegment::IN_INTERFACE_INDEX>(std::vector{std::pair{value,value}},negation);
				ret.flag_segments[Rule::IN_INTERFACE].emplace(value,value);
			}else{
				result.applyRange_helper<PSegment::OUT_INTERFACE_INDEX>(std::vector{std::pair{value,value}},negation);
				ret.flag_segments[Rule::OUT_INTERFACE].emplace(value,value);
			}
		}else if(flag_name == "p"){
			std::string interface;
			in >> interface;
			auto value = protocols[interface];
			result.applyRange_helper<PSegment::PROTOCOL_INDEX>(std::vector{std::pair{value,value}},negation);
			ret.flag_segments[Rule::PROTOCOL].emplace(value,value);
		}else if(flag_name == "d"){
			auto [start,end] = parseIP(in,line);
			result.applyRange_helper<PSegment::DST_IP_INDEX>(std::vector{std::pair{start,end}},negation);
			ret.flag_segments[Rule::DST_IP].emplace(start,end);
		}else if(flag_name == "s") {
			auto [start,end] = parseIP(in,line);
			result.applyRange_helper<PSegment::SRC_IP_INDEX>(std::vector{std::pair{start,end}},negation);
			ret.flag_segments[Rule::SRC_IP].emplace(start,end);
		}else if(flag_name == "to-destination" || flag_name == "to-source"){
			auto [start_ip,end_ip] = parseIP(in,line);
			if(in.peek() == '-'){
				in.get();
				end_ip = parseIP(in,line).first;
			}
			bool port_change = false;
			uint16_t start_port,end_port;
			if(in.peek() == ':'){
				port_change = true;
				in.get();
				in >> start_port;
				if(in.peek() == '-'){
					in.get();
					in >> end_port;
				}else{
					end_port = start_port;
				}
			}
			ret.nat = {start_ip,end_ip,port_change,start_port,end_port};
		}else if(flag_name == "restore-mark"){
		}else if(flag_name == "m"){
			std::string s;
			in >> s;
			parseNot(in);
		}else if(flag_name == "ctstate"){
			mlog::debug("unsupported flag ctstate ignoring rule on line {}\n",line);
			results.unknownFlags.insert(flag_name);
			parseArg(in);
			ret.shouldBeIgnored = true;
		}else if(flag_name == "tcp-flags"){
			mlog::debug("unsupported flag tcp-flags ignoring rule on line {}\n",line);
			results.unknownFlags.insert(flag_name);
			parseArg(in);
			parseArg(in);
			ret.shouldBeIgnored = true;
		}else if(flag_name == "comment") {
			parseArg(in);
		}else{
			auto inplace_args = {"transparent","any","set","rcheck","rsource"};
			bool done = false;
			for(auto x : inplace_args) 
				if(flag_name == x){
					done = true;
					mlog::debug("unrecognized flag \"{}\"", flag_name);
					break;
				}
			if(!done){
				auto args = parseArg(in);
				if(args::config.disable.rules.with_flags.contains(flag_name) && 
						args::config.disable.rules.with_flags[flag_name] == args){
					mlog::info("ignoring {} because of {} and {}\n", ret.line_str,flag_name,args);
					ret.shouldBeIgnored = true;
				}
				mlog::debug("unrecognized flag \"{}\" with arguments \'{}\'\n", flag_name, args);
			}
			results.unknownFlags.insert(flag_name);
		}
	}

	if(ret.jumpTarget == nullptr){
		mlog::debug("rule without jumptarget on line {}, will be ignored: {}\n",line,ret.line_str);
		if(!args::config.silence.no_jump_target.contains(chain_name)){
			results.rulesWithoutJumpTarget.push_back(line);
		}
		ret.shouldBeIgnored = true;
	}else{
		if(args::config.disable.rules.with_jump_target.contains(ret.jumpTarget->name)){
			ret.shouldBeIgnored = true;
		}
	}
	ret.table_name = curTable().name;
	ret.maximumMatchingSet = std::move(result);
	ret.line = line;
	getChainOrCreate(chain_name).rules.emplace_back(std::move(ret));
}
void RulesetParser::parseTable(std::istream& in, std::string&& table_name){


	tables.emplace_back();
	curTable().name = std::move(table_name);

	std::string line;
	while(true){
		std::getline(in,line);
		if(in.eof() && line.empty())break;
		this->line++;
		if(line == "COMMIT")break;

		std::stringstream line_in(line);
		
		char c = line_in.get();
		bool parsed = false;
		if(c == ':'){
			std::string chain_name;
			std::string policy;
			line_in >> chain_name >> policy;
			Chain& chain = getChainOrCreate(chain_name);
			if(policy == "ACCEPT")chain.policy = Chain::Policy::ACCEPT;
			else if(policy == "DROP")chain.policy = Chain::Policy::DROP;
			else if(policy == "-")chain.policy = Chain::Policy::NONE;
			else {
				mlog::log("unrecognized chain policy \"{}\"\n",policy);
			}
			parsed = true;
		} else if(c == '-'){
			line_in >> c;
			if(c == 'A'){
				parseRule(line_in,line);
				parsed = true;
			}
		}
		if(!parsed){
			mlog::log("ignoring \"{}\"\n",line);
		}
	}
	finishChains();
}
void RulesetParser::finishChains(){
	if(tables.empty())return;
	for (auto& [chain_name, chain]  : chains) {
		if(chain_name == "ACCEPT")chain->special = Chain::Special::ACCEPT;
		else if(chain_name == "DROP")chain->special = Chain::Special::DROP;
		else if(chain_name == "REJECT")chain->special = Chain::Special::REJECT;
		else if(chain_name == "RETURN")chain->special = Chain::Special::RETURN;
		else if(chain_name == "DNAT")chain->special = Chain::Special::DNAT;
		else if(chain_name == "SNAT")chain->special = Chain::Special::SNAT;
		curTable().chains.emplace_back(std::move(chain));
	}
	chains.clear();
}
void RulesetParser::parseRuleset_NFT(std::string_view filename){
	std::ifstream file(filename.data());
	if(!file.good()){
		mlog::fatal("could not open \"{}\"\n",filename);
	}
	mlog::log("parsing ruleset file \"{}\"\n",filename);
	parseRuleset_NFT(file);
	mlog::success("done parsing ruleset file \"{}\"\n",filename);
}
Chain& RulesetParser::getChainOrCreate_NFT(const std::string_view table_name, const std::string_view chain_name){
	auto& chain_entry = nft_chains[std::string{table_name}][std::string{chain_name}];
	if(!chain_entry.get()){
		chain_entry = std::make_unique<Chain>();
		chain_entry->name = chain_name;
	}
	return *chain_entry.get();
}
void RulesetParser::parseRuleset_NFT(std::istream& in){
	std::string line;
	while(true){
		std::getline(in,line);
		this->line++;
		if(in.eof() || !in.good())break;

		try_parser parser{line};
		auto add = parser.string();
		if(add == "#"){
			continue;
		}
		if(add != "add"){
			mlog::warn("unknown word \"{}\" in line {}",add,this->line);
		}

		parser.ws();
		auto add_type = parser.string();
		parser.ws();
		auto family = parser.string();
		parser.ws();
		auto table_name = parser.string();
		parser.ws();

		if(add_type == "table"){
			tables.emplace_back();
			curTable().name = std::move(table_name);
		}else if(add_type == "chain"){
			auto chain_name = parser.string();
			parser.ws();
			auto& chain = getChainOrCreate_NFT(table_name,chain_name);
			chain.line = this->line;

			while(!parser.done()){
				parser.ws();
				auto w = parser.string();
				if(w == "policy"){
					parser.ws();
					auto type = parser.string();
					type.remove_suffix(1);//remove ';'
					if(type == "accept"){
						chain.policy = Chain::Policy::ACCEPT;
					}else if(type == "drop"){
						chain.policy = Chain::Policy::ACCEPT;
					}else {
						mlog::warn("unknown policy \"{}\" in line {}",type,this->line);
					}
					break;
				}
			}
		}else if(add_type == "rule"){
			Rule ret;
			auto& result = ret.maximumMatchingSet;
			result = {{{}}};
			ret.line = this->line;
			parser.ws();
			auto chain_name = parser.string();
			ret.table_name = curTable().name;
			while(!parser.done()){
				parser.ws();
				auto cmd = parser.string();

				if(cmd == "accept"){
					ret.jumpTarget = &getChainOrCreate_NFT(table_name,"ACCEPT");
				}else if(cmd == "drop" || cmd == "queue"){
					ret.jumpTarget = &getChainOrCreate_NFT(table_name,"DROP");
				}else if(cmd == "continue"){
					ret.shouldBeIgnored = true;
				}else if(cmd == "return"){
					ret.jumpTarget = &getChainOrCreate_NFT(table_name,"RETURN");
				}else if(cmd == "jump" || cmd == "goto"){
					parser.ws();
					auto target = parser.string();
					ret.jumpTarget = &getChainOrCreate_NFT(table_name, std::string{target});

					if(cmd == "jump")ret.jumpType = JumpType::JUMP;
					else ret.jumpType = JumpType::GOTO;
				}else if(cmd == "oifname" || cmd == "iifname"){
					parser.ws();
					auto negation = parser.matchStaticString("!=");
					parser.ws();

					auto name = parser.string();
					name.remove_prefix(1);//remove '"'
					name.remove_suffix(1);//remove '"'

					auto& interface_map = (cmd[0] == 'i' ? in_interfaces : out_interfaces);
					auto value = interface_map[std::string{name}];
					if(cmd == "iifname"){
						result.applyRange_helper<PSegment::IN_INTERFACE_INDEX>(std::vector{std::pair{value,value}},negation);
						ret.flag_segments[Rule::IN_INTERFACE].emplace(value,value);
					}else{
						result.applyRange_helper<PSegment::OUT_INTERFACE_INDEX>(std::vector{std::pair{value,value}},negation);
						ret.flag_segments[Rule::OUT_INTERFACE].emplace(value,value);
					}
				}else if(cmd == "saddr"){
					parser.ws();
					auto negation = parser.matchStaticString("!=");
					parser.ws();

					auto [start,end] = parseIP(parser.string(),this->line);
					result.applyRange_helper<PSegment::SRC_IP_INDEX>(std::vector{std::pair{start,end}},negation);
					ret.flag_segments[Rule::SRC_IP].emplace(start,end);
				}else if(cmd == "daddr"){
					parser.ws();
					auto negation = parser.matchStaticString("!=");
					parser.ws();

					parser.ws();
					auto [start,end] = parseIP(parser.string(),this->line);
					result.applyRange_helper<PSegment::DST_IP_INDEX>(std::vector{std::pair{start,end}},negation);
					ret.flag_segments[Rule::DST_IP].emplace(start,end);
				}else if(cmd == "dport" || cmd == "sport"){
					parser.ws();
					auto negation = parser.matchStaticString("!=");
					parser.ws();

					std::vector<std::pair<uint16_t,uint16_t>> port_ranges;
					if(parser.matchStaticString("{")){
						bool first = true;
						while(true){
							parser.ws();
							if(first)first = false;
							else{
								if(parser.matchStaticString("}"))break;
								if(!parser.matchStaticString(",")){
									mlog::warn("parsing issue in port list\n{}:\"{}\"",this->line,line);
								}
								parser.ws();
							}
	
							auto v1 = parser.integer();
							if(parser.matchStaticString("-")){
								auto v2 = parser.integer();
								port_ranges.emplace_back(*v1,*v2);
							}else{
								port_ranges.emplace_back(*v1,*v1);
							}
						}
					}else{
						auto v1 = parser.integer();
						if(parser.matchStaticString("-")){
							auto v2 = parser.integer();
							port_ranges.emplace_back(*v1,*v2);
						}else{
							port_ranges.emplace_back(*v1,*v1);
						}
					}
					if(cmd == "sport"){
						result.applyRange_helper<PSegment::SRC_PORT_INDEX>(port_ranges,negation);
						ret.flag_segments[Rule::SRC_PORT].insert(std::begin(port_ranges),std::end(port_ranges));
					}else if(cmd == "dport"){
						result.applyRange_helper<PSegment::DST_PORT_INDEX>(port_ranges,negation);
						ret.flag_segments[Rule::DST_PORT].insert(std::begin(port_ranges),std::end(port_ranges));
					}
				}else if(cmd == "protocol" || cmd == "udp" || cmd == "tcp" || cmd == "icmp"){
					parser.ws();
					auto negation = parser.matchStaticString("!=");
					parser.ws();

					uint8_t protocol_index;
					if(cmd == "protocol"){
						auto protocol_name = std::string{parser.string()};
						parser.ws();
						protocol_index = protocols[protocol_name];
					}else{
						protocol_index = protocols[std::string{cmd}];
					}

					result.applyRange_helper<PSegment::PROTOCOL_INDEX>(std::vector{std::pair{protocol_index,protocol_index}},negation);
					ret.flag_segments[Rule::PROTOCOL].emplace(protocol_index,protocol_index);
				}else if(cmd == "snat" || cmd == "dnat"){
					parser.ws();
					parser.matchStaticString("to");
					parser.ws();
					auto result = parser.until("-");
					std::string_view ip;
					if(result){
						ip = result->first;
						parser.skip(1);
					}else{
						ip = parser.string();
					}

					auto [start_ip,end_ip] = parseIP(ip,this->line);
					if(result){
						auto next = parser.string();
						end_ip = parseIP(next,this->line).second;
					}
					parser.ws();
					ret.nat = {start_ip,end_ip,false,0,0};
				}else{
					mlog::debug("unknown cmd {}\n",cmd);
				}
			}
			if(!ret.jumpTarget)ret.shouldBeIgnored = true;
			ret.line_str = std::move(line);
			getChainOrCreate_NFT(table_name,chain_name).rules.emplace_back(std::move(ret));
		}else{
			mlog::warn("unknown word \"{}\" in line {}",add_type,this->line);
		}
	}
	for(auto& table : tables){
		for(auto& [chain_name,chain] : nft_chains[table.name]){
			if(chain_name == "ACCEPT")chain->special = Chain::Special::ACCEPT;
			else if(chain_name == "DROP")chain->special = Chain::Special::DROP;
			else if(chain_name == "REJECT")chain->special = Chain::Special::REJECT;
			else if(chain_name == "RETURN")chain->special = Chain::Special::RETURN;
			else if(chain_name == "DNAT")chain->special = Chain::Special::DNAT;
			else if(chain_name == "SNAT")chain->special = Chain::Special::SNAT;
			table.chains.emplace_back(std::move(chain));
		}
	}
	ruleset.tables = std::move(tables);
}
void RulesetParser::parseRuleset(std::istream& in){
	std::string line;
	std::getline(in,line);
	while(!in.eof() && in.good()){
		std::stringstream line_in(line);
		this->line++;
		char c;
		line_in >> c;
		if (c=='*'){//anouncing table
			std::string table_name;
			line_in >> table_name;
			parseTable(in,std::move(table_name));
		}else{
			mlog::debug("[parser_error] ignoring \"{}\"\n",line);
		}
		std::getline(in,line);
	}
	mlog::debug("parsed {} tables\n",tables.size());
	int sum_rules = 0;
	int sum_chains = 0;
	for(const auto& table : tables){
		for(const auto& chain : table.chains){
			if(chain->special == Chain::Special::NONE)sum_chains++;
			sum_rules += chain->rules.size();
		}
	}
	mlog::debug("parsed {} chains\n",sum_chains);
	mlog::debug("parsed {} rules\n",sum_rules);
	ruleset.tables = std::move(tables);
}
void RulesetParser::parseIpSets(std::string_view filename){
	std::ifstream file(filename.data());
	if(!file.good())mlog::fatal("cant open ipset file {}\n",filename);

	mlog::log("parsing ipset file \"{}\"\n",filename);
	parseIpSets(file);
	mlog::success("done parsing ipset file \"{}\"\n",filename);
}
void RulesetParser::parseIpSets(std::istream& file){
	std::string line;
	size_t entrys = 0;
	size_t set_count = 0;
	int line_counter = 0;
	while(true){
		line_counter++;
		std::getline(file,line);
		if(file.eof())break;

		auto range = util::split(line," ");
		auto iter = std::begin(range);
		auto command = *iter;

		if(!(command == "add" || command == "create"))continue;
		entrys++;
		++iter;
		std::string set_name = std::string(*iter);
		++iter;
		if(command == "create"){
			std::unique_ptr<IpSet> new_set;
			if(*iter == "hash:net,iface"){
				new_set = std::make_unique<IpSet_Iface>();
			}else if(*iter == "list:set") {
				new_set = std::make_unique<IpSet_listset>();
			}else if(*iter == "hash:net,port" || *iter == "hash:ip,port"){
				new_set = std::make_unique<IpSet_Port>();
			}else if(*iter == "hash:net" || *iter == "hash:ip"){
				new_set = std::make_unique<IpSet_IP>();
			}else{
				mlog::fatal("unknown set type {}",*iter);
			}
			ip_sets.create(set_name,std::move(new_set));
			set_count++;
			continue;
		}
		ip_sets.get(set_name)->add(*this,line);
	}
	mlog::debug("Parsed {} ipsets\n",set_count);
	mlog::debug("with {} entrys\n",entrys);
}

void RulesetParser::parseRuleset(std::string_view filename){
	std::ifstream file(filename.data());
	if(!file.good()){
		mlog::fatal("could not open \"{}\"\n",filename);
	}
	mlog::log("parsing ruleset file \"{}\"\n",filename);
	parseRuleset(file);
	mlog::success("done parsing ruleset file \"{}\"\n",filename);
}

Ruleset&& RulesetParser::releaseRuleset(){
	return std::move(ruleset);
}
parse_result_t& RulesetParser::getInfo(){
	return this->results;
}
