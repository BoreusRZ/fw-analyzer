#include "common.hpp"
#include "../util.hpp"

#include <stdexcept>
#include <istream>
#include <sstream>
#include <fmt/core.h>

auto parseIP(std::string_view in, int line) -> std::pair<uint32_t,uint32_t> {
	std::stringstream ss(std::string{in});
	return parseIP(ss,line);
}
auto parseIP(std::istream& in, int line) -> std::pair<uint32_t,uint32_t> {
	uint32_t start = 0;
	constexpr auto bits_in_byte = 8;
	for(auto i = 0; i < 4; ++i){
		start <<= bits_in_byte;
		uint32_t single;
		in >> single;
		start |= single;
		if(i != 3){
			in.get();//reads '.' 
		}
	}
	if(in.peek() == '/'){
		in.get();
		uint32_t mask = 0;
		in >> mask;
		mask = 32-mask;
		mask = (1 << mask) - 1;
		if((mask & start) != 0){
			throw std::runtime_error(fmt::format("invalid [{:#0x}] mask and subnetip [{:#0x}] overlap on line {}",~mask,start,line));
		}
		return {start, start+mask};
	}
	return {start,start};
}
void parseWhitespace(std::istream& in){
	while(!in.eof()){
		char next = in.peek();
		switch(next){
			case ' ':
			case '\t':
				in.get();
				break;
			default:
				return;
		}
	}
}
auto parsePortList(std::string_view ports) -> std::vector<std::pair<uint16_t,uint16_t>> {
	std::vector<std::pair<uint16_t,uint16_t>> ret;
	for(auto port_config : util::split(ports,",")){
		const char* seperator = nullptr;
		if(port_config.find(':') != std::string::npos){
			seperator = ":";
		}else if(port_config.find('-') != std::string::npos){
			seperator = "-";
		}
		if(seperator != nullptr){
			auto [start,end] = util::unpack<2>(util::split(port_config,seperator).begin());
			auto first = toInt<uint16_t>(start);
			auto second = toInt<uint16_t>(end);
			ret.emplace_back(first,second);
		}else{
			auto port = toInt<uint16_t>(port_config);
			ret.emplace_back(port,port);
		}
	}
	return ret;
}

