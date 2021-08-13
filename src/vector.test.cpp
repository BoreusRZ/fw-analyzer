#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>
#include "SegmentSet-generator.hpp"
#include "SegmentSet.hpp"

#include "vector-generator.hpp"
#include "vector.hpp"

/* TEST(vector,copy_into_larger_capcity){ */
/* 	bor::vector<int> x; */
/* 	for(int i = 0; i < 100;++i) */
/* 		x.push_back(1); */
/* 	x.clear(); */
/* 	bor::vector<int> y; */
/* 	y.push_back(1); */
/* 	x = y; */
/* 	EXPECT_EQ(x,y); */
/* } */
/* TEST(vector,emplace_back){ */
/* 	struct X { */
/* 		int val = 2; */
/* 	}; */
/* 	{ */
/* 		bor::vector<X> vec; */
/* 		vec.emplace_back(); */
/* 		ASSERT_EQ(vec.size(),1); */
/* 		EXPECT_EQ(vec[0].val,2); */
/* 	} */
/* 	{ */
/* 		bor::vector<PSegment> vec; */
/* 		vec.emplace_back(); */
/* 		ASSERT_EQ(vec.size(),1); */
/* 		EXPECT_EQ(vec[0],PSegment{}); */
/* 	} */

/* } */
/* RC_GTEST_PROP(vector,cpy,(const bor::vector<int>& data)){ */
/* 	{ */
/* 		bor::vector<int> cpy = data; */
/* 		RC_ASSERT(data == cpy); */
/* 	} */
/* 	{ */
/* 		std::vector<int> cpy(std::begin(data),std::end(data)); */
/* 		RC_ASSERT(data == cpy); */
/* 	} */
/* } */
/* RC_GTEST_PROP(vector,pop_back,(bor::vector<int> vec)){ */
/* 	while(!vec.empty()){ */
/* 		auto prev_size = vec.size(); */
/* 		vec.pop_back(); */
/* 		RC_ASSERT(prev_size - 1 == vec.size()); */
/* 	} */
/* } */
/* RC_GTEST_PROP(vector,emplace_back_multiple,(std::vector<int> vec,int val,uint16_t repeat)){ */
/* 	bor::vector<int> cpy = vec; */
/* 	for(size_t i = 0; i < repeat; ++i){ */
/* 		vec.emplace_back(val); */
/* 		cpy.emplace_back(val); */
/* 	} */
/* 	RC_ASSERT(vec == cpy); */
/* } */
/* RC_GTEST_PROP(vector,supports_std_copy,(bor::vector<int> vec)){ */
/* 	bor::vector<int> other; */ 
/* 	std::copy(std::begin(vec),std::end(vec),std::back_inserter(other)); */
/* 	RC_ASSERT(other == vec); */
/* } */
/* RC_GTEST_PROP(vector,supports_std_copy2,(bor::vector<int> vec1, bor::vector<int> vec2)){ */
/* 	auto prev_size = vec2.size(); */
/* 	std::copy(std::begin(vec1),std::end(vec1),std::back_inserter(vec2)); */
/* 	RC_ASSERT(vec2.size() == vec1.size()+prev_size); */
/* } */
/* RC_GTEST_PROP(vector,emplace_back_single,(bor::vector<int> vec,int val)){ */
/* 	vec.emplace_back(val); */
/* 	RC_ASSERT(vec.back() == val); */
/* } */
/* RC_GTEST_PROP(vector,push_back_single,(bor::vector<int> vec,int val)){ */
/* 	vec.push_back(val); */
/* 	RC_ASSERT(vec.back() == val); */
/* } */
/* RC_GTEST_PROP(vector,push_back_multiple,(std::vector<int> vec,int val,uint16_t repeat)){ */
/* 	bor::vector<int> cpy = vec; */
/* 	for(size_t i = 0; i < repeat; ++i){ */
/* 		vec.push_back(val); */
/* 		cpy.push_back(val); */
/* 	} */
/* 	RC_ASSERT(vec == cpy); */
/* } */
/* RC_GTEST_PROP(vector,resize_std_equall,(std::vector<PSegment> vec,uint16_t amount)){ */
/* 	bor::vector<PSegment> bor = vec; */
/* 	std::vector<PSegment> std = vec; */
/* 	std.resize(amount); */
/* 	bor.resize(amount); */
/* 	RC_ASSERT(bor == std); */
/* } */
/* RC_GTEST_PROP(vector,push_back_std_equall,(std::vector<int> vec,int val,int val2)){ */
/* 	bor::vector<int> cpy = vec; */
/* 	RC_ASSERT(vec == cpy); */

/* 	vec.push_back(val); */
/* 	cpy.push_back(val); */
/* 	RC_ASSERT(vec == cpy); */

/* 	vec.push_back(val2); */
/* 	RC_ASSERT(vec != cpy); */

/* 	cpy.push_back(val2); */
/* 	RC_ASSERT(vec == cpy); */

/* 	RC_PRE(val != val2); */
/* 	vec.push_back(val ); */
/* 	cpy.push_back(val2); */
/* 	RC_ASSERT(vec != cpy); */
/* } */
