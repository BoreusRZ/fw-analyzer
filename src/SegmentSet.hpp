#pragma once
#include "util.hpp"
#include "vector.hpp"
#include "Segment.hpp"
#include <vector>
#include <iostream>

using PSegment_type = Segment<uint32_t,uint32_t,uint16_t,uint16_t,uint8_t,uint8_t,uint8_t>;
/**
 * @brief represents a segment of packets an iptables rule can match
 * @sa PSET
 */
class PSegment : public PSegment_type {
	public:
	/**
	 * @brief indicies to access the proper segment-intervals e.g. this->getStart<SRC_IP_INDEX>()
	 */
	enum segment_indicies {
		SRC_IP_INDEX = 0,
		DST_IP_INDEX = 1,
		SRC_PORT_INDEX = 2,
		DST_PORT_INDEX = 3,
		IN_INTERFACE_INDEX = 4,
		OUT_INTERFACE_INDEX = 5,
		PROTOCOL_INDEX = 6
	};

	using PSegment_type::PSegment_type;
	PSegment(const PSegment_type&);

	friend std::ostream& operator<<(std::ostream&,const PSegment&);
};
/**
 * @brief Compression Datastructure for arbitrary sets of points
 * @details SegmentSet expects a derived type of Segment as segment_t\n
 * SegmentSet can in contrast to Segment represent arbitrary point collections\n
 * You can think of it like an 'or' of Segments\n
 * in the following complexity descriptions:
 * - @b n denotes the amount of segments in @b this objects vector
 * - @b m denotes the amount of segments in @b other objects vector
 * - @b d denotes the dimensionality of the segment type
 */
template<typename segment_t>
class SegmentSet {
public:

	SegmentSet() = default;
	SegmentSet(bor::vector<segment_t>&&);
	/**
	 * the resulting set contains all segments from both sets\n
	 * runtime O(n+m)
	 */
	auto UNION(const SegmentSet&) -> void;
	auto UNION_par(const SegmentSet&) -> void;
	auto UNION_seq(const SegmentSet&) -> void;
	/**
	 * the resulting set contains all points that where contained in both sets\n
	 * runtime O(n*m)
	 */
	auto INTERSECTION(const SegmentSet&) -> void;
	auto INTERSECTION_par(const SegmentSet&) -> void;
	auto INTERSECTION_seq(const SegmentSet&) -> void;
	/**
	 * the resulting set contains all points previously not contained\n
	 * in the set that are represntable by segment_t\n
	 * runtime O(d^n)\n
	 * */
	auto NEGATE() -> void;
	auto NEGATE_par() -> void;
	auto NEGATE_seq() -> void;

	/**
	 * the resulting set contains all points previously not contained\n
	 * in the set that are representable by segment_t\n
	 * runtime O(n*d^m)\n
	 * */
	auto INTERSECTION_NEGATED(const SegmentSet&) -> void;
	auto INTERSECTION_NEGATED_par(const SegmentSet&) -> void;
	auto INTERSECTION_NEGATED_seq(const SegmentSet&) -> void;

	auto getAmountPoints() const -> long double;
	auto getAmountPointsExakt() const -> gmp::BigInt;
	auto getAmountPointsExaktSafe() -> gmp::BigInt;

	/**
	 * @return whether or not this set contains any points
	 */
	[[nodiscard]]
	auto isEmpty() const noexcept -> bool;

	/**
	 * since the internal representation of a SegmentSet is not unique\n
	 * algorithms can lead to segmentation which will lead to slowdown or\n
	 * exceeding the memory limit\n
	 * this method will try to reduce the number of segments in the current state\n
	 * runtime O(n^2)\n
	 * */
	void compact();

	/**
	 * this function aids in construction of SegmentSets\n
	 * it expects a list of one dimensional intervals on a given dimension @b index \n
	 * it is assumed that the segment set is not jet restricted in this dimension\n
	 * meaning all segments are at the maximum size in the @b index'th dimension\n
	 * all exisiting segments will be copied by how many elements @b ranges contains\n
	 * for each copy of a segment one element of @b ranges is applied to the @b index'th dimension\n
	 * \n
	 * if the @b negated flag is set to true\n
	 * the @b ranges provided will be transformed into the intervals inbetween them\n
	 * so that all values falling into the original intervals will later @b not be matched
	 */
	template<int range_index, typename T>
	void applyRange_helper(std::vector<std::pair<T,T>> ranges, bool negated){
		if(negated){
			std::ranges::sort(ranges,
					[](const auto& pair1, const auto& pair2){
						return pair1.first < pair2.first;
					});
			decltype(ranges) new_ranges;
			T cur = std::numeric_limits<T>::min();
			for(size_t i = 0; i < ranges.size(); ++i){
				if(ranges[i].first > cur){
					new_ranges.emplace_back(cur,ranges[i].first-1);
					cur = ranges[i].second+1;
				}else{
					cur = ranges[i].second+1;
				}
			}
			if(ranges.back().second != std::numeric_limits<T>::max()){
				new_ranges.emplace_back(cur,std::numeric_limits<T>::max());
			}
			applyRange_helper<range_index>(new_ranges,false);
			return;
		}
		SegmentSet<segment_t> ret_set;
		auto& ret = ret_set.segments;
		ret.reserve(ranges.size()*(negated ? 2 : 1)*this->segments.size());

		auto prevSize = this->segments.size();
		for(size_t i = 0; i < ranges.size(); ++i){
			auto [start,end] = ranges[i];
			for(size_t j = 0; j < prevSize; ++j){
				PSegment next = this->segments[j];
				next.setInterval<range_index>(start,end);
				ret.push_back(next);
			}
		}
		*this = std::move(ret_set);
	}
	template<typename T>
	friend SegmentSet<T> UNION(
			const SegmentSet<T>& s1,
			const SegmentSet<T>& s2){
		auto ret = s1;
		ret.UNION(s2);
		return ret;
	}
	template<typename T>
	friend SegmentSet<T> NEGATION(
			const SegmentSet<T>& s1){
		auto ret = s1;
		ret.NEGATE();
		return ret;
	}
	template<typename T>
	friend SegmentSet<T> INTERSECTION_NEGATED(
			const SegmentSet<T>& s1,
			const SegmentSet<T>& s2){
		auto ret = s1;
		ret.INTERSECTION_NEGATED(s2);
		return ret;
	}
	template<typename T>
	friend SegmentSet<T> INTERSECTION(
			const SegmentSet<T>& s1,
			const SegmentSet<T>& s2){
		auto ret = s1;
		ret.INTERSECTION(s2);
		return ret;
	}

	template<typename T>
	friend std::ostream& operator<<(std::ostream&,const SegmentSet<T>&);

	bor::vector<segment_t> segments;	
};

/**
 * checks wheter these set contain exactly the same points\n
 * runtime O(n*d^m+m*d^n)
 */
template<typename segment_t>
bool operator==(const SegmentSet<segment_t>& set1, const SegmentSet<segment_t>& set2) {
	if(!INTERSECTION_NEGATED(set1,set2).isEmpty())return false;
	if(!INTERSECTION_NEGATED(set2,set1).isEmpty())return false;
	return true;

}

/**
 * @returns SegmentSet representation of the inverse set of points
 * to a single segment 
 */
template<typename segment_t>
auto negate(const segment_t& segment) -> SegmentSet<segment_t> {
	bor::vector<segment_t> ret;
	auto cur = segment;
	util::constexpr_for<0,segment_t::dimensions,1>([&](auto i){
				auto& start = cur.template getStart<i>();
				auto& end = cur.template getEnd<i>();
				auto pstart = start;
				auto pend = end;
				using start_t = typename std::remove_reference_t<decltype(start)>;
				using end_t = typename std::remove_reference_t<decltype(start)>;
				if(pstart != std::numeric_limits<start_t>::min()){
					start = std::numeric_limits<start_t>::min();
					end = pstart-1;
					ret.push_back(cur);
				}
				if(pend != std::numeric_limits<end_t>::max()){
					start = pend+1;
					end = std::numeric_limits<end_t>::max();
					ret.push_back(cur);
				}
				start = std::numeric_limits<start_t>::min();
				end = std::numeric_limits<end_t>::max();
			});

	return {std::move(ret)};
}


//force instantiation
template class SegmentSet<PSegment>;
using PSET = SegmentSet<PSegment>;

template<typename ... Args>
class fmt::formatter<Segment<Args...>>{
	public:
	using segment = Segment<Args...>;
	
	constexpr auto parse(format_parse_context& ctx){
		return ctx.begin();
	}
	template<typename FormatContext>
	auto format(const segment& seg, FormatContext& ctx){
		auto ret = ctx.out();
			ret = fmt::format_to(ret,"{},{}",seg.start_m,seg.end_m);
		if constexpr(!std::is_same_v<typename segment::parent_t,Segment<void>>){
			ret = fmt::format_to(ret,",{}",typename segment::parent_t(seg));
		}
		return ret;
	}
};

template<>
class fmt::formatter<PSegment> : public fmt::formatter<PSegment_type> {};
