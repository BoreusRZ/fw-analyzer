#pragma once
#include <rapidcheck.h>
#include "Segment-generator.hpp"

namespace rc{
	template<>
	struct Arbitrary<PSegment> {
		static Gen<PSegment> arbitrary(){
			return gen::map<PSegment_type>([](auto x){return PSegment{x};});
		};
	};
	template<class segment_t>
	struct Arbitrary<SegmentSet<segment_t>> {
		using self_t = SegmentSet<segment_t>;
		static Gen<self_t> arbitrary(){
			return gen::scale(0.3,
					gen::map<std::vector<segment_t>>([](auto x){return self_t{std::move(x)};}));
		};
	};
}
