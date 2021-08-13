#pragma once
#include <type_traits>
#include <limits>
#include <iostream>
#include <fmt/format.h>
#include <cassert>
#include "BigInt.hpp"

/**
 * @brief This class-template represents an extension on the concept of intervals 
 * into higher dimensions
 * @details A Segment<int,int> has 2 dimensions each of type int. \n
 * It represents all Points (x,y), where \n
 * x is contained in the interval of the first dimension (getStart<0>(),getEnd<0>()) and \n
 * y is contained in the interval of the second dimension (getStart<1>(),getEnd<1>())\n
 * therfore it can be thought of like a Rectangle\n
 * */
template<typename dimension = void, typename ... Rest>
class Segment : public Segment<Rest...> {
private:
	dimension start_m, end_m;

public:
	using self_t = Segment<dimension,Rest...>;
	using parent_t = Segment<Rest...>;
	using dimension_t = dimension;
	constexpr static int dimensions = 1+sizeof...(Rest);


	/*!
		\return the end of the index'th dimension
	*/
	template<int index>
	auto getEnd() const{
		if constexpr (index != 0){
			return parent_t::template getEnd<index-1>();
		}else{
			return end_m;
		}
	}

	template<int index>
	auto& getEnd(){
		if constexpr (index != 0){
			return parent_t::template getEnd<index-1>();
		}else{
			return end_m;
		}
	}

	/*!
		\return the start of the index'th dimension
	*/
	template<int index>
	auto getStart() const{
		if constexpr (index != 0){
			return parent_t::template getStart<index-1>();
		}else{
			return start_m;
		}
	}
	template<int index>
	void setEnd(auto val){
		getEnd<index>() = val;
	}
	template<int index>
	void setStart(auto val){
		getStart<index>() = val;
	}
	template<int index>
	auto& getStart(){
		if constexpr (index != 0){
			return parent_t::template getStart<index-1>();
		}else{
			return start_m;
		}
	}
	template<int index>
	auto getInterval() const {
		return std::make_pair(getStart<index>(), getEnd<index>());
	}
	template<int index>
	void setInterval(auto start, auto end){
		setStart<index>(start);
		setEnd<index>(end);
	}
	template<int index>
	void setInterval(auto start_end){
		setInterval<index>(start_end,start_end);
	}
	template<int index>
	void setInterval(std::pair<dimension_t,dimension_t> start_end){
		setInterval<index>(start_end.first,start_end.second);
	}


	/*! constructs the Segment as large as the types allow */
	template<typename...Args>
	constexpr Segment(
			dimension start=std::numeric_limits<dimension>::min(),
			dimension end=std::numeric_limits<dimension>::max(),
			Args ... args) :
		parent_t(args...),
		start_m(start),
		end_m(end)
	{}

	/**
	 * @return the amount of points in the segment
	 */
	gmp::BigInt getAmountPointsExakt() const{
		gmp::BigInt ret;
		if(end_m < start_m)return ret;
		if constexpr (!std::is_same_v<typename parent_t::dimension_t,void>){
			ret += parent_t::getAmountPointsExakt();
		}else{
			ret = 1;
		}
		ret *= (end_m - start_m) + 1;

		return ret;
	}
	auto getAmountPoints() const -> long double {
		if(empty())return 0;
		long double f1 = parent_t::getAmountPoints();
		long double f2 = end_m-(long double)start_m+1;
		return f1*f2;
	}

	/**
	 * checks if this segment doesnt contain any points
	 */
	constexpr bool empty() const{
		if(end_m < start_m) return true;
		else return parent_t::empty();
	}

	/**
	 * if two segments are \sa mergable
	 */
	auto merge(const self_t& other) const -> self_t {
		assert(mergeable(other));
		auto ret = *this;
		ret.merge_helper(other);
		return ret;
	}
	void merge_helper(const self_t& other) {
		if(other.start_m == start_m && other.end_m == end_m)parent_t::merge_helper(other);
		else{
			start_m = std::min(start_m,other.start_m);
			end_m = std::max(end_m,other.end_m);
		}
	}
	bool mergeable(const self_t& other) const{
		return std::get<0>(mergeable_helper(other));
	}
	auto mergeable_helper(const self_t& other) const -> std::tuple<bool,bool> {
		auto lower = parent_t::mergeable_helper(other);
		if(!std::get<0>(lower))return lower;
		if(start_m == other.start_m && end_m == other.end_m)
			return lower;
		if(std::get<1>(lower) && other.end_m >= start_m && other.start_m <= end_m){
			return {true,false};
		}
		return {false,false};
	}

	/**
	 * the resulting segment will contain all points
	 * that were contained in both segments
	 */
	void intersect(const self_t& other) {
		start_m = std::max(start_m,other.start_m);
		end_m = std::min(end_m,other.end_m);

		parent_t::intersect(other);
	}
	bool operator==(const self_t& other) const{
		return start_m == other.start_m && end_m == other.end_m && parent_t::operator==(other);
	}
	/* bool operator!=(const self_t& other) const{ */
	/* 	return !(*this == other); */
	/* } */
	friend self_t intersect(const self_t& s1, const self_t& s2){
		self_t ret = s1;
		ret.intersect(s2);
		return ret;
	}
	void setParent(parent_t value){
		parent_t::operator=(value);
	}
};
template<typename ... SegArgs>
std::ostream& operator<<(std::ostream& out, const Segment<SegArgs...>& segment) {
	using self_t = Segment<SegArgs...>;
	if constexpr (! std::is_same_v<self_t,Segment<void>>){
		if constexpr ( std::is_same_v<decltype(segment.template getStart<0>()),uint8_t>) {
			out << static_cast<size_t>(segment.template getStart<0>()) << "," << static_cast<size_t>(segment.template getEnd<0>()) << ",";
		}else{
			out << segment.template getStart<0>() << "," << segment.template getEnd<0>() << ",";
		}
		out << typename Segment<SegArgs...>::parent_t(segment);
	}
	return out;
}
template<>
class Segment<void> {
	public:
	using self_t = Segment<void>;
	using dimension_t = void;
	constexpr static int dimensions = 0;
	void intersect(__attribute__((unused)) const self_t& other) {}
	constexpr bool empty() const{
		return false;
	}
	bool operator==(__attribute__((unused)) const self_t& other) const{
		return true;
	}
	bool operator!=(__attribute__((unused)) const self_t& other) const{
		return false;
	}
	auto getAmountPoints() const -> long double {
		return 1;
	}
	auto mergeable_helper(__attribute__((unused)) const self_t& other) const -> std::tuple<bool,bool> {
		return {true,true};
	}
	void merge_helper(__attribute__((unused)) const self_t& other) {
	}
};

