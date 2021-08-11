#pragma once
#include "SegmentSet.hpp"
#include <optional>
#include <memory>
#include <array>
#include <set>

constexpr auto RAW_TABLE = "raw";
constexpr auto MANGLE_TABLE = "mangle";
constexpr auto NAT_TABLE = "nat";
constexpr auto FILTER_TABLE = "filter";

constexpr auto PREROUNTING_CHAIN = "PREROUTING";
constexpr auto POSTROUTING_CHAIN = "POSTROUTING";
constexpr auto INPUT_CHAIN = "INPUT";
constexpr auto OUTPUT_CHAIN = "OUTPUT";
constexpr auto FORWARD_CHAIN = "FORWARD";
constexpr auto CONNMARK_CHAIN = "CONNMARK";
constexpr auto NFLOG_CHAIN = "NFLOG";
constexpr auto CT_CHAIN = "CT";
constexpr auto DNAT_CHAIN = "DNAT";
constexpr auto SNAT_CHAIN = "SNAT";

enum class JumpType {
	GOTO,
	JUMP
};
class Chain;
/**
 * @brief container of iptables rule
 */
class Rule {
public:
	//! represents all Packets that could ever get matched by this rule
	PSET maximumMatchingSet;


	//! value of this enum is used to index into \link flag_segments
	enum MatchingFlag{
		SRC_IP, DST_IP,
		SRC_PORT,DST_PORT,
		IN_INTERFACE,OUT_INTERFACE,
		PROTOCOL,
		_SIZE
	};
	
	//! individual ranges of propertys that are matched by flags\n
	//! used to check if rules are \link mergeable
	std::array<std::set<std::pair<uint32_t,uint32_t>>,MatchingFlag::_SIZE> flag_segments = {};

	//! ptr to chain in a Ruleset. corresponds to the Chain after -j or -g flag
	Chain* jumpTarget = nullptr;
	JumpType jumpType = JumpType::JUMP;
	int line; ///<line in ruleset file where rule was defined 
	std::string line_str;///< text in ruleset file at \link line

	std::string_view table_name;///< name of the table this rule occured in

	/**
	 * Represents the Packet Transformation if this Rule jumps to DNAT or SNAT\n
	 * to rerout the packet
	 */
	struct NAT_Transform {
		uint32_t start_ip, end_ip;
		bool has_port_change = false;
		uint16_t start_port, end_port;
		bool operator==(const NAT_Transform&) const = default;
	};
	std::optional<NAT_Transform> nat;///<contains the nat transformation, if rule jumps to DNAT or SNAT

	/**
	 * the configuration can lead to certain rules being disabled\n
	 * \sa config_t
	 */
	bool shouldBeIgnored = false;
	bool jumpTargetIsEmpty = false;///<set in IpAnalyzer::checkGraph, checked in IpAnalyzer::pipeChain



	//data used when analyzing
	bool touched = false;
	long double matched = 0;
	int aliveMatch = 0;
	int deadMatch = 0;
	int aliveJump = 0;
	int deadJump = 0;

	//this data can be reseted with the [reset] function
	void reset();
	friend bool mergeable(const Rule& r1, const Rule& r2);
};
/**
 * @brief container of iptables chain
 */
class Chain {
public:
	/**
	 * @return true, if the Chain is DROP or REJECT
	 */
	bool isDropping() const;
	/**
	 * @return true, if the Chain is a Special Jump Target
	 * or is PREROUTING-, POSTROUNTING-...,-Chain
	 */
	bool isPredefined() const;

	std::vector<Rule> rules;///< all rules contained in this chain
	std::string name;///<name of the chain
	int line;///<line number of first declaration in ruleset-file
	enum class Policy {
		ACCEPT,DROP,NONE
	} policy = Policy::NONE;///<policy of chain if the packet passes through
	enum class Special{
		ACCEPT,DROP,RETURN,REJECT,NONE,
		DNAT,SNAT
	} special = Special::NONE;///<special chains are predefined jump targets
};
/**
 * @brief container of iptables table
 */
class Table {
public:
	/**
	 * looks for chain whit chain_name\n
	 * \return ptr to Chain with this name\n
	 * if such a chain has not been defined in this table\n
	 * nullptr will be returned
	 */
	Chain* findChain(std::string_view chain_name) ;

	/**
	 * chains defined in this table\n
	 * unique_ptr is used so that they can be referenced\n
	 * by rules without invalidating the memory location
	 */
	std::vector<std::unique_ptr<Chain>> chains;
	std::string name;///<name of the table
};
/**
 * @brief container of iptables-ruleset data
 */
class Ruleset {
public:
	//!resets data on rules used when analyzing
	void resetRules();
	//!executes given function on every rule in ruleset
	void forEachRule(std::function<void(Rule&)>);

	/**
	 * looks for table whit table_name\n
	 * \return ptr to Table with this name\n
	 * if such a table has not been defined in this ruleset\n
	 * nullptr will be returned
	 */
	Table* findTable(std::string_view table_name) ;
	/**
	 * looks for chain whit chain_name in a table with table_name\n
	 * if the table cant be found or the table doesnt have\n
	 * a chain with chain_name\n
	 * nullptr will be returned
	 */
	Chain* findChain(std::string_view table_name, std::string_view chain_name) ;
	
	std::vector<Table> tables;///<tables in this ruleset
};
