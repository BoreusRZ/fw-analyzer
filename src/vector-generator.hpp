#pragma once
#include "vector.hpp"
#include <vector>
#include <rapidcheck.h>
namespace rc {
	template<typename T>
	struct Arbitrary<bor::vector<T>> {
		using self_t = bor::vector<T>;
		static Gen<self_t> arbitrary(){
			return gen::map(gen::arbitrary<std::vector<T>>(),
							[](const auto& std_vec){
								self_t ret;
								for(const auto& elem : std_vec){
									ret.push_back(elem);
								}
								return ret;
							});
		}
	};
}
