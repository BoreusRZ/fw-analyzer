#include "SegmentSet.hpp"
#include "log.hpp"
#include "util.hpp"
#include <omp.h>
#include <ranges>

template<typename T>
void multi_vector_merge(bor::vector<T>& target, const bor::vector<bor::vector<T>>& partial){
	std::vector<size_t> prefixSum(partial.size()+1);
	prefixSum[0] = 0;
	for(size_t i = 1; i < prefixSum.size(); ++i){
		prefixSum[i] = prefixSum[i-1] + partial[i-1].size();
	}
	target.clear();
	target.resize_no_init(prefixSum.back());
/* #pragma omp parallel for collapse(2) */
/* 	for(size_t i = prefixSum[0]; i < prefixSum.back(); ++i){ */
/* 		for(size_t j = 0; j < partial.size() ; ++j){ */
/* 			if(i >= prefixSum[j] && i < prefixSum[j+1]) */
/* 				target[i] = partial[j][i-prefixSum[j]]; */
/* 		} */
/* 	} */
/* #pragma omp parallel for */
/* 	for(size_t i = 0; i < partial.size(); ++i){ */
/* 		for(size_t j = 0; j < partial[i].size() ; ++j){ */
/* 			target[j+prefixSum[i]] = partial[i][j]; */
/* 		} */
/* 	} */
#pragma omp parallel for
	for(size_t i = 0; i < partial.size(); ++i){
		if(partial[i].empty()) continue;
		memcpy(target.data()+prefixSum[i],partial[i].data(),partial[i].size()*sizeof(T));
	}
}

template<typename segment_t>
SegmentSet<segment_t>::SegmentSet(bor::vector<segment_t>&& segments) 
	: segments(std::move(segments))
{}

template<typename segment_t>
void SegmentSet<segment_t>::UNION(const SegmentSet<segment_t>& other){
	UNION_seq(other);
}
template<typename segment_t>
void SegmentSet<segment_t>::UNION_par(const SegmentSet<segment_t>& other){
	bor::vector<bor::vector<segment_t>> partial = {std::move(segments),other.segments};
	multi_vector_merge(segments,partial);
}
template<typename segment_t>
void SegmentSet<segment_t>::UNION_seq(const SegmentSet<segment_t>& other){
	std::copy(
			other.segments.begin(),
			other.segments.end(),
			std::back_inserter(segments));
}
template<typename segment_t>
bool SegmentSet<segment_t>::isEmpty() const noexcept{
	return segments.empty();
}
PSegment::PSegment(const PSegment_type& other) : PSegment_type(other) {}
std::ostream& operator<<(std::ostream& out,const PSegment& segment){
	return out << PSegment_type(segment);
}
template<typename segment_t>
gmp::BigInt SegmentSet<segment_t>::getAmountPointsExaktSafe() {
	compact();

	gmp::BigInt ret = 0;
	for(size_t i = 0; i < segments.size(); i++){
		ret += segments[i].getAmountPointsExakt();
		PSET intersections;

		for(size_t j = i+1; j < segments.size(); j++){
			auto intersection = intersect(segments[i],segments[j]);
			intersections.segments.push_back(intersection);
		}

		if(intersections.segments.size() == 1){
			ret -= intersections.segments[0].getAmountPointsExakt();
		}else if(intersections.segments.size() == 0){
	
		}else{
			ret -= intersections.getAmountPointsExaktSafe();
		}
	}
	return ret;
}
template<typename segment_t>
gmp::BigInt SegmentSet<segment_t>::getAmountPointsExakt() const{
	gmp::BigInt ret = 0;
	for(auto& segment : segments){
		ret += segment.getAmountPointsExakt();
	}
	return ret;
}
template<typename segment_t>
long double SegmentSet<segment_t>::getAmountPoints() const{
	long double ret = 0;
	for(auto& segment : segments){
		ret += segment.getAmountPoints();
	}
	return ret;
}

template<typename segment_t>
void SegmentSet<segment_t>::INTERSECTION_NEGATED(const SegmentSet& other){
	INTERSECTION_NEGATED_seq(other);
}
#include <atomic>
template<typename segment_t>
void SegmentSet<segment_t>::INTERSECTION_NEGATED_par(const SegmentSet& other){
	auto negated = other;
	negated.NEGATE_seq();
	bor::vector<bor::vector<PSegment>> partial;
	std::vector<std::atomic<bool>> intersects(segments.size());
#pragma omp parallel
	{
#pragma omp single
		{
			partial.resize(omp_get_num_threads());
		}
#pragma omp for collapse(2)
	for(size_t i = 0; i < segments.size(); ++i){
		for(size_t j = 0; j < other.segments.size(); ++j){
			if(!intersect(segments[i],other.segments[j]).empty()){
				intersects[i] = true;
			}
		}
	}
#pragma omp barrier
#pragma omp for 
	for(size_t i = 0; i < segments.size(); ++i){
		if(!intersects[i]){
			partial[omp_get_thread_num()].push_back(segments[i]);
		}else{
			for(size_t j = 0; j < negated.segments.size(); ++j){
				auto intersection = intersect(segments[i],negated.segments[j]);
				if(intersection.empty()) continue;
				partial[omp_get_thread_num()].push_back(intersection);
			}
		}
	}
	}

	multi_vector_merge(segments,partial);
}
template<typename segment_t>
void SegmentSet<segment_t>::INTERSECTION_NEGATED_seq(const SegmentSet& other){
	auto negated = other;
	negated.NEGATE_seq();
	bor::vector<PSegment> result;
	for(const auto& seg1 : segments){
		bool intersects = false;
		for(const auto& seg2 : other.segments){
			if(!intersect(seg1,seg2).empty()){
				intersects = true;
				break;
			}
		}
		if(!intersects){
			result.push_back(seg1);
		}else{
			for(const auto& seg2 : negated.segments){
				auto intersection = intersect(seg1,seg2);
				if(intersection.empty()) continue;
				result.push_back(intersection);
			}
			
		}
	}

	segments = std::move(result);
}
template<typename segment_t>
void SegmentSet<segment_t>::INTERSECTION_par(const SegmentSet& other){
	bor::vector<bor::vector<segment_t>> partial;
#pragma omp parallel 
	{
#pragma omp single
		{
			partial.resize(omp_get_num_threads());
		}
#pragma omp for collapse(2)
		for(size_t i = 0; i < segments.size(); i++){
			for(size_t j = 0; j < other.segments.size(); j++){
				auto intersection = intersect(segments[i],other.segments[j]);
				if(!intersection.empty()){
					partial[omp_get_thread_num()].push_back(intersection);
				}
			}
		}
	}
	multi_vector_merge(segments,partial);
}

template<typename segment_t>
void SegmentSet<segment_t>::INTERSECTION(const SegmentSet& other){
	INTERSECTION_seq(other);
}
template<typename segment_t>
void SegmentSet<segment_t>::INTERSECTION_seq(const SegmentSet& other){
	bor::vector<segment_t> result;
	for(size_t i = 0; i < segments.size(); i++){
		for(size_t j = 0; j < other.segments.size(); j++){
			auto intersection = intersect(segments[i],other.segments[j]);
			if(intersection.empty()) continue;
			result.push_back(intersection);
		}
	}
	segments = std::move(result);
}




template<typename segment_t>
void SegmentSet<segment_t>::NEGATE(){
	NEGATE_seq();
}
template<typename segment_t>
void SegmentSet<segment_t>::NEGATE_par(){
	if(isEmpty()){
		segments.emplace_back();
		return;
	}
	SegmentSet<segment_t> result = negate(segments[0]);
	for(size_t i = 1; i < segments.size(); ++i){
		result.INTERSECTION_NEGATED_par(bor::vector{segments[i]});
	}
	segments = std::move(result.segments);
}
template<typename segment_t>
void SegmentSet<segment_t>::NEGATE_seq(){
	if(isEmpty()){
		segments.emplace_back();
		return;
	}
	SegmentSet<segment_t> result = negate(segments[0]);
	SegmentSet<segment_t> temp;
	for(const auto& segment : segments | std::views::drop(1)){
		temp.segments.clear();
		temp.segments.push_back(segment);
		result.INTERSECTION_NEGATED_seq(temp);
	}
	segments = std::move(result.segments);
}

template<typename segment_t>
std::ostream& operator<<(std::ostream& out,const SegmentSet<segment_t>& ip_set){
	out << '{' << std::endl;
	for(const auto& seg : ip_set.segments){
		out << '\t' << seg << std::endl;
	}
	out << '}';
	return out;
}
template std::ostream& operator<<(std::ostream& out,const PSET& ip_set);

template<typename segment_t>
void SegmentSet<segment_t>::compact(){

	for(size_t i = 0; i < segments.size(); i++){
		for(size_t j = i+1; j < segments.size(); j++){

			auto intersection = intersect(segments[i],segments[j]);
			if(segments[i] == intersection){
				std::swap(segments[i],segments.back());
				segments.pop_back();
				j = i;
			}
			else if(segments[j] == intersection){
				std::swap(segments[j],segments.back());
				segments.pop_back();
				--j;
			}
		}
	}
	for(size_t i = 0; i < segments.size(); i++){
		for(size_t j = i+1; j < segments.size(); j++){
			if(segments[i].mergeable(segments[j])){
				segments[i] = segments[i].merge(segments[j]);
				std::swap(segments[j],segments.back());
				segments.pop_back();
				--j;
			}
		}
	}

}
