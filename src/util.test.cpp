#include <gtest/gtest.h>
#include <fmt/core.h>
#include <rapidcheck/gtest.h>

#include "util.hpp"
#include "BigInt.hpp"
using namespace std;


TEST(util,bigint_signednes){
	gmp::BigInt x;
	gmp::BigInt y = 2;
	x -= 4;
	x += 6;
	EXPECT_EQ(x,y);
}

TEST(util,memory_usage_raw){
	{
		std::vector<int> arr = {1};
		EXPECT_EQ(util::getMemoryUsage_raw(arr),4);
	}
	{
		std::vector<int> arr = {1,2,3};
		EXPECT_EQ(util::getMemoryUsage_raw(arr),12);
	}
	{
		std::vector<int> arr = {1,2,3,4};
		EXPECT_EQ(util::getMemoryUsage_raw(arr),16);
	}
}
TEST(util,memory_usage_str){
	{
		std::vector<int> arr = {1};
		EXPECT_EQ(util::getMemoryUsage(arr),"4.00 B");
	}
	{
		std::vector<int> arr = {1,2,3};
		EXPECT_EQ(util::getMemoryUsage(arr),"12.00 B");
	}
	{
		std::vector<int> arr = {1,2,3,4};
		EXPECT_EQ(util::getMemoryUsage(arr),"16.00 B");
	}
}
TEST(util,unpack_iter_ref){
	auto text = "abc def ghi";
	auto range = util::split(text," ");
	auto iter = range.begin();
	auto [first,second] = util::unpack<2>(iter);

	EXPECT_EQ(first,"abc");
	EXPECT_EQ(second,"def");
	EXPECT_EQ(*iter,"def");
}
RC_GTEST_PROP(util,concatenating_splitstring_yields_original,(const std::string& str, const std::string& delim)){
	RC_PRE(!delim.empty());
	std::string res;
	bool first = true;
	for(auto asd : util::split(str,delim)){
		if(first)first = false;
		else res.append(delim);
		res.append(asd);
	}
	RC_ASSERT(res == str);
}
RC_GTEST_PROP(util,split_element_does_not_contain_delimeter,(const std::string& str)){
	for(auto asd : util::split(str,",")){
		RC_ASSERT(std::count(std::begin(asd),std::end(asd),',') == 0);
	}
}
RC_GTEST_PROP(util,count_of_splits_equals_count_of_delimieter_plus_one,(const std::string& str)){
	RC_PRE(!str.empty());
	auto commas = std::count(std::begin(str),std::end(str),',');
	int found = 0;
	for(auto asd : util::split(str,",")){
		RC_ASSERT(asd.size() <= str.size());
		found++;
	}
	RC_ASSERT(commas + 1 == found);
}
