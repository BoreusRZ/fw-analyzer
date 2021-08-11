#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>
#include "Segment.hpp"
#include "SegmentSet.hpp"
#include "Segment-generator.hpp"
#include "SegmentSet-generator.hpp"
TEST(Segment,default_constructor){
	Segment<uint32_t,uint16_t> seg;
	ASSERT_EQ(seg.getStart<0>() , 0);
	ASSERT_EQ(seg.getEnd<0>() , (uint32_t)~0u);
	ASSERT_EQ(seg.getStart<1>() , 0);
	ASSERT_EQ(seg.getEnd<1>() , (uint16_t)~0u);
}
TEST(Segment,equality){
	Segment<uint32_t,uint16_t> seg1(0,1,2,3);
	Segment<uint32_t,uint16_t> seg2(0,1,2,3);
	Segment<uint32_t,uint16_t> seg3(0,1,2,4);
	Segment<uint32_t,uint16_t> seg4(0,1,3,4);
	ASSERT_EQ(seg1,seg1);
	ASSERT_EQ(seg2,seg1);
	ASSERT_EQ(seg1,seg2);
	ASSERT_NE(seg1,seg3);
	ASSERT_NE(seg4,seg3);
}
TEST(Segment,empty){
	Segment<uint32_t,uint16_t> seg;
	EXPECT_FALSE(seg.empty());
	seg.getEnd<0>() = 1;
	EXPECT_FALSE(seg.empty());
	seg.setStart<0>(2);
	EXPECT_TRUE(seg.empty());
}
TEST(Segment,edge_case_64_bit_check_overflow){
		Segment<uint64_t> seg;
		EXPECT_NE(seg.getAmountPoints(),0);
}
TEST(Segment,edge_case_32_bit_check_overflow){
		Segment<uint32_t> seg;
		EXPECT_NE(seg.getAmountPoints(),0);
}

TEST(Segment,getAmountPoints){
	{
		Segment<uint16_t,uint16_t> seg(0,2,1,3);
		EXPECT_DOUBLE_EQ(seg.getAmountPoints(),9);
	}
	{
		Segment<uint16_t,uint16_t,uint16_t> seg(0,2,0,3,10,10);
		EXPECT_DOUBLE_EQ(seg.getAmountPoints(),12);
	}
	{
		Segment<uint16_t,uint16_t> seg(0,2,0,3);
		EXPECT_DOUBLE_EQ(seg.getAmountPoints(),12);
	}
}
RC_GTEST_PROP(Segment,segment_contained_in_merge,(const PSegment& seg)){
	auto cpy1 = seg;
	auto cpy2 = seg;
	cpy2.setStart<0>(seg.getEnd<0>());
	cpy2.getEnd<0>() += 200;
	auto cpy3 = seg;
	cpy3.getEnd<0>() += 200;
	auto merge = cpy1.merge(cpy2);
	RC_ASSERT(intersect(cpy1,merge)==cpy1);
	RC_ASSERT(intersect(cpy2,merge)==cpy2);
	RC_ASSERT(intersect(cpy3,merge)==cpy3);
}
RC_GTEST_PROP(Segment,segment_is_mergeable_to_itself,(const PSegment& seg)){
	RC_ASSERT(seg.mergeable(seg));
}
RC_GTEST_PROP(Segment,intersect_commute,(const PSegment& seg1, const PSegment& seg2)){
	RC_ASSERT(intersect(seg1,seg2) == intersect(seg2,seg1));
}
RC_GTEST_PROP(Segment,intersect_associativity,(const Segment<int,int>& seg1, const Segment<int,int>& seg2,const Segment<int,int>& seg3)){
	auto r1 = intersect(seg1,intersect(seg2,seg3));
	auto r2 = intersect(intersect(seg1,seg2),seg3);
	/* RC_TAG(r1.empty(),"empty"); */
	RC_ASSERT(r1 == r2);
}
RC_GTEST_PROP(Segment, intersect_self_idempotency,(const Segment<size_t,int>& seg)){
	RC_ASSERT(seg == intersect(seg,seg));
}
RC_GTEST_PROP(Segment,start_inc_equals_end_dec,(const Segment<size_t,size_t>& seg)){
	RC_PRE(!seg.empty());
	RC_PRE(seg.getEnd<0>() != 0u);
	RC_PRE(seg.getStart<0>() != std::numeric_limits<size_t>::max());
	auto cpy1 = seg;
	auto cpy2 = seg;
	cpy1.getEnd<0>()--;
	cpy2.getStart<0>()++;
	RC_ASSERT(cpy1.getAmountPoints() == cpy2.getAmountPoints());
}
RC_GTEST_PROP(Segment,pbt_value_inc,(const Segment<size_t,size_t>& seg)){
	RC_PRE(!seg.empty());
	auto cpy = seg;
	cpy.getEnd<0>()++;
	RC_ASSERT(seg.getAmountPoints() <= cpy.getAmountPoints());
}
