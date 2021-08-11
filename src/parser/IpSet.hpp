#pragma once
#include <vector>
#include <unordered_map>

#include "../Ruleset.hpp"
#include "../SegmentSet.hpp"


class RulesetParser;
class IpSet {
public:
	/**
	 * parses an input line in ipset configuration
	 * and adds the entry to this set
	 * @param parser is needed to figure out the id of interfaces and protocols
	 * @param line actual line in the ipset file e.g. "-A abc 1.2.3.4/32..."
	 */
	virtual void add(RulesetParser& parser, std::string_view line) = 0;
	/**
	 * applys the constraint of this set to a rule that called match-set on it
	 * @param rule used to insert into Rule::flag_segments
	 * @param ip_set will be constrained
	 * @param string after the "match-set" and [set-name] which can only be a comma seperated list
	 * of dst and src
	 */
	virtual void apply(Rule& rule, PSET& ip_set, std::string_view src_dst_flags) = 0;
	/**
	 * @return PSET that contains alle packages matched by only this set given the src_dst_flags parameter
	 */
	virtual PSET toPSET(std::string_view src_dst_flags) = 0;
	virtual ~IpSet() = default;
};
class IpSet_IP : public IpSet{
public:
	void add(RulesetParser&, std::string_view line) override;
	void apply(Rule& rule, PSET& ip_set, std::string_view src_dst_flags) override;
	PSET toPSET(std::string_view src_dst_flags) override;
private:
	std::vector<std::pair<uint32_t,uint32_t>> ip_segments;
};
class IpSet_listset : public IpSet {
public:
	void add(RulesetParser& parser, std::string_view line) override;
	void apply(Rule& rule, PSET& ip_set, std::string_view src_dst_flags) override;
	PSET toPSET(std::string_view src_dst_flags) override;
private:
	std::vector<IpSet*> sets;
};
class IpSet_Port : public IpSet {
public:
	void add(RulesetParser& parser, std::string_view line) override;
	void apply(Rule& rule, PSET& ip_set, std::string_view src_dst_flags) override;
	PSET toPSET(std::string_view src_dst_flags) override;
private:
	std::vector<std::tuple<uint32_t,uint32_t,uint16_t,uint16_t,uint8_t>> ip_port_protocol_segments;
};
class IpSet_Iface : public IpSet {
public:
	void add(RulesetParser& parser, std::string_view line) override;
	void apply(Rule& rule, PSET& ip_set, std::string_view src_dst_flags) override;
	PSET toPSET(std::string_view src_dst_flags) override;
private:
	std::vector<std::tuple<uint32_t,uint32_t,uint8_t,uint8_t>> ip_interface_segments;
};
class IpSets{
public:
	void create(const std::string& set_name, std::unique_ptr<IpSet> set){
		sets.emplace(std::move(set_name),std::move(set));
	}
	auto get(const std::string& set_name) -> IpSet*{
		auto iter = sets.find(set_name);
		if(iter == sets.end()){
			throw std::runtime_error(fmt::format("unrecognized set name \"{}\"",set_name));
		}
		return iter->second.get();
	}
private:
	std::unordered_map<std::string, std::unique_ptr<IpSet>> sets;
};
