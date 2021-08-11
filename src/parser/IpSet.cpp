#include "../RulesetParser.hpp"
#include "IpSet.hpp"
#include "../util.hpp"
#include "common.hpp"

#include <sstream>

PSET IpSet_IP::toPSET(std::string_view src_dst_flags)  {
	PSET ret;
	auto [type] = util::unpack<1>(util::split(src_dst_flags,",").begin());
	bool srcIP = type == "src";
	for(auto [start_ip,end_ip] : ip_segments){
		PSegment next;
		if(srcIP){
			next.setInterval<PSegment::SRC_IP_INDEX>(start_ip,end_ip);
		}else{
			next.setInterval<PSegment::DST_IP_INDEX>(start_ip,end_ip);
		}
		ret.segments.push_back(next);
	}

	return ret;
}
void IpSet_IP::add(RulesetParser&, std::string_view line)  {
	auto [cmd,name,ip] = util::unpack<3>(util::split(line," ").begin());
	ip_segments.emplace_back(parseIP(ip));
}
void IpSet_IP::apply(Rule& rule, PSET& ip_set, std::string_view src_dst_flags)  {
	auto [type] = util::unpack<1>(util::split(src_dst_flags,",").begin());
	if( type == "src"){
		ip_set.applyRange_helper<PSegment::SRC_IP_INDEX>(ip_segments,false);
		rule.flag_segments[Rule::SRC_IP].insert(std::begin(ip_segments),std::end(ip_segments));
	} else {
		ip_set.applyRange_helper<PSegment::DST_IP_INDEX>(ip_segments,false);
		rule.flag_segments[Rule::DST_IP].insert(std::begin(ip_segments),std::end(ip_segments));
	}
}
void IpSet_Iface::add(RulesetParser& parser, std::string_view line)  {
	const auto [add,name,ip_iface] = util::unpack<3>(util::split(line," ").begin());
	const auto [ip,iface] = util::unpack<2>(util::split(ip_iface,",").begin());
	const auto [ip_start,ip_end] = parseIP(ip);
	const auto in_id = parser.in_interfaces[std::string{iface}];
	const auto out_id = parser.out_interfaces[std::string{iface}];
	ip_interface_segments.emplace_back(ip_start,ip_end,in_id,out_id);
}
void IpSet_Iface::apply(Rule& rule, PSET& ip_set, std::string_view src_dst_flags)  {
	auto [type_ip,type_iface] = util::unpack<2>(util::split(src_dst_flags,",").begin());
	PSET set;
	set.segments.clear();
	bool srcIP = type_ip == "src";
	bool srcInterface = type_iface == "src";
	for(auto [start_ip,end_ip,in_interface_id,out_interface_id] : ip_interface_segments){
		PSegment new_segment;
		if(srcIP){
			new_segment.setInterval<PSegment::SRC_IP_INDEX>(start_ip,end_ip);
			rule.flag_segments[Rule::SRC_IP].emplace(start_ip,end_ip);
		}else{
			new_segment.setInterval<PSegment::DST_IP_INDEX>(start_ip,end_ip);
			rule.flag_segments[Rule::DST_IP].emplace(start_ip,end_ip);
		}
		if(srcInterface){
			new_segment.setInterval<PSegment::IN_INTERFACE_INDEX>(in_interface_id);
			rule.flag_segments[Rule::IN_INTERFACE].emplace(in_interface_id,in_interface_id);
		}else{
			new_segment.setInterval<PSegment::OUT_INTERFACE_INDEX>(in_interface_id);
			rule.flag_segments[Rule::OUT_INTERFACE].emplace(out_interface_id,out_interface_id);
		}
		set.segments.push_back(new_segment);
	}
	ip_set.INTERSECTION(set);
}
PSET IpSet_Iface::toPSET(std::string_view src_dst_flags)  {
	auto [type_ip,type_iface] = util::unpack<2>(util::split(src_dst_flags,",").begin());
	PSET set;
	set.segments.clear();
	bool srcIP = type_ip == "src";
	bool srcInterface = type_iface == "src";
	for(auto [start_ip,end_ip,in_interface_id,out_interface_id] : ip_interface_segments){
		PSegment new_segment;
		if(srcIP){
			new_segment.setInterval<PSegment::SRC_IP_INDEX>(start_ip, end_ip);
		}else{
			new_segment.setInterval<PSegment::DST_IP_INDEX>(start_ip, end_ip);
		}
		if(srcInterface){
			new_segment.setInterval<PSegment::IN_INTERFACE_INDEX>(in_interface_id);
		}else{
			new_segment.setInterval<PSegment::OUT_INTERFACE_INDEX>(out_interface_id);
		}
		set.segments.push_back(new_segment);
	}
	return set;
}

PSET IpSet_listset::toPSET(std::string_view src_dst_flags){
	PSET result={{{}}};
	for(auto* set : sets){
		result.UNION(set->toPSET(src_dst_flags));
	}
	return result;
}
void IpSet_listset::apply(Rule&,PSET& set, std::string_view src_dst_flags){
	set.INTERSECTION(this->toPSET(src_dst_flags));
	//TODO add flag segments
}
void IpSet_listset::add(RulesetParser& parser, std::string_view line){
	const auto [add,name,contained_set_name] = util::unpack<3>(util::split(line," ").begin());
	sets.push_back(parser.ip_sets.get(std::string{contained_set_name}));
}








PSET IpSet_Port::toPSET(std::string_view src_dst_flags)  {
	PSET ret;
	auto [typeIP,typePort] = util::unpack<2>(util::split(src_dst_flags,",").begin());
	bool srcIP = typeIP == "src";
	bool srcPort = typePort == "src";
	for(auto [start_ip,end_ip,start_port,end_port,protocol] : ip_port_protocol_segments){
		PSegment next;
		if(srcIP){
			next.setInterval<PSegment::SRC_IP_INDEX>(start_ip,end_ip);
		}else{
			next.setInterval<PSegment::DST_IP_INDEX>(start_ip,end_ip);
		}
		if(srcPort){
			next.setInterval<PSegment::SRC_PORT_INDEX>(start_port,end_port);
		}else{
			next.setInterval<PSegment::DST_PORT_INDEX>(start_port,end_port);
		}
		next.setInterval<PSegment::PROTOCOL_INDEX>(protocol,protocol);
		ret.segments.push_back(next);
	}

	return ret;
}
void IpSet_Port::add(RulesetParser& parser, std::string_view line)  {
	auto [cmd,name,ip_proto_port] = util::unpack<3>(util::split(line," ").begin());
	auto [ip,proto_port] = util::unpack<2>(util::split(ip_proto_port,",").begin());
	uint16_t start_port, end_port;
	uint8_t protocol;

	if(proto_port.find(':') == std::string_view::npos){//check if protocol is contained
		protocol = parser.protocols.getIndex("tcp");

		auto portList = parsePortList(proto_port);

		assert(portList.size() == 1);

		start_port = portList[0].first;
		end_port = portList[0].second;
	}else{
		auto [proto, port] = util::unpack<2>(util::split(proto_port,":").begin());
		auto portList = parsePortList(port);

		assert(portList.size() == 1);

		start_port = portList[0].first;
		end_port = portList[0].second;
		protocol = parser.protocols.getIndex(std::string{proto});
	}

	auto [start_ip,end_ip] = parseIP(ip);
	ip_port_protocol_segments.emplace_back(start_ip,end_ip,start_port,end_port,protocol);
}
void IpSet_Port::apply(Rule& rule, PSET& ip_set, std::string_view src_dst_flags)  {
	PSET ret;
	auto [typeIP,typePort] = util::unpack<2>(util::split(src_dst_flags,",").begin());
	bool srcIP = typeIP == "src";
	bool srcPort = typePort == "src";
	for(auto [start_ip,end_ip,start_port,end_port,protocol] : ip_port_protocol_segments){
		PSegment next;
		if(srcIP){
			next.setInterval<PSegment::SRC_IP_INDEX>(start_ip,end_ip);
			rule.flag_segments[Rule::SRC_IP].emplace(start_ip,end_ip);
		}else{
			next.setInterval<PSegment::DST_IP_INDEX>(start_ip,end_ip);
			rule.flag_segments[Rule::DST_IP].emplace(start_ip,end_ip);
		}
		if(srcPort){
			next.setInterval<PSegment::SRC_PORT_INDEX>(start_port,end_port);
			rule.flag_segments[Rule::SRC_PORT].emplace(start_ip,end_ip);
		}else{
			next.setInterval<PSegment::DST_PORT_INDEX>(start_port,end_port);
			rule.flag_segments[Rule::DST_PORT].emplace(start_ip,end_ip);
		}
		next.setInterval<PSegment::PROTOCOL_INDEX>(protocol,protocol);
		rule.flag_segments[Rule::DST_PORT].emplace(protocol,protocol);
		ret.segments.push_back(next);
	}

	ip_set.INTERSECTION(ret);
}
