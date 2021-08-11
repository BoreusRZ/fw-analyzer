#pragma once
#include "Segment-generator.hpp"
#include "SegmentSet.hpp"
#include <rapidcheck.h>
namespace rc {
	template<typename ...Args>
	struct Arbitrary<Segment<Args...>> {
		using self_t = Segment<Args...>;
		using dimension_t = self_t::dimension_t;
		static Gen<self_t> arbitrary(){
			//generate segment with one interval
			Gen<self_t> ret = gen::map(
								gen::arbitrary<std::pair<dimension_t,dimension_t>>(),
								[](auto data){
									self_t ret;
									ret.template setInterval<0>(data);
									return ret;
								});
					
			//generate segment with multiple intervals
			if constexpr(!std::is_same_v<typename self_t::parent_t::dimension_t,void>){
				ret = gen::map(
						gen::arbitrary<std::pair<typename self_t::parent_t,std::pair<dimension_t,dimension_t>>>(),
						[](auto data){
							self_t ret;
							ret.setParent(data.first);
							ret.template setInterval<0>(data.second);
							return ret;
						});
					
			}

			//make sure segment is not empty 
			//by checking start <= end
			return gen::map(ret,
							[](const self_t& prev){
								auto ret = prev;
								if(ret.template getEnd<0>() < ret.template getStart<0>())
									std::swap(ret.template getEnd<0>(),ret.template getStart<0>());
								return ret;
							});
		}
	};
}
