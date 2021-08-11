#include <gtest/gtest.h>
#include <rapidcheck/gtest.h>
#include "SegmentSet.hpp"
#include "SegmentSet-generator.hpp"

TEST(SegmentSet,constructors){
	PSET set;
	EXPECT_TRUE(set.isEmpty());
	set = PSET(std::vector{PSegment{}});
	EXPECT_FALSE(set.isEmpty());
}
TEST(SegmentSet,intersection){
	PSegment s1(20,200);
	PSegment s2(50,400);


	PSET set = PSET(std::vector{s1});
	set.INTERSECTION(PSET{std::vector{s2}});
	ASSERT_EQ(set.segments.size(),1);
	auto segment = set.segments[0];
	EXPECT_EQ(segment.getStart<0>(),50);
	EXPECT_EQ(segment.getEnd<0>(),200);
}
TEST(SegmentSet,applyRange_helper_single_interval_on_single_segment){
	PSET set(std::vector<PSegment>{{}});
	set.applyRange_helper<0,uint32_t>(
			std::vector<std::pair<uint32_t,uint32_t>>{
				std::pair<uint32_t,uint32_t>{1,2}
			},false);
	ASSERT_EQ(set.segments.size(),1);
	EXPECT_EQ(set.segments[0].getStart<0>(),1);
	EXPECT_EQ(set.segments[0].getEnd<0>(),2);
}
TEST(SegmentSet,applyRange_helper_three_intervals_negated){
	PSET set(std::vector<PSegment>{{}});
	set.applyRange_helper<0,uint32_t>(
			std::vector<std::pair<uint32_t,uint32_t>>{
				std::pair<uint32_t,uint32_t>{10,20},
				std::pair<uint32_t,uint32_t>{1,2},
				std::pair<uint32_t,uint32_t>{100,100},
			},true);
	ASSERT_EQ(set.segments.size(),4);
	EXPECT_EQ(set.segments[0].getStart<0>(),0);
	EXPECT_EQ(set.segments[0].getEnd<0>(),0);
	EXPECT_EQ(set.segments[1].getStart<0>(),3);
	EXPECT_EQ(set.segments[1].getEnd<0>(),9);
	EXPECT_EQ(set.segments[2].getStart<0>(),21);
	EXPECT_EQ(set.segments[2].getEnd<0>(),99);
	EXPECT_EQ(set.segments[3].getStart<0>(),101);
	EXPECT_EQ(set.segments[3].getEnd<0>(),std::numeric_limits<uint32_t>::max());
}
TEST(SegmentSet,single_segment_negate_max_segment){
	PSegment seg;
	auto negated = negate(seg);
	ASSERT_EQ(negated.segments.size(),0);
}
TEST(SegmentSet,single_segment_negate_single_constraint){
	PSegment seg(0,20);
	auto negated = negate(seg);
	ASSERT_EQ(negated.segments.size(),1);
	EXPECT_EQ(negated.segments[0].getStart<0>(),21);
}
TEST(SegmentSet,applyRange_helper_two_intervals_negated_at_border){
	PSET set(std::vector<PSegment>{{}});
	set.applyRange_helper<0,uint32_t>(
			std::vector<std::pair<uint32_t,uint32_t>>{
				std::pair<uint32_t,uint32_t>{0,20},
				std::pair<uint32_t,uint32_t>{100,std::numeric_limits<uint32_t>::max()},
			},true);
	ASSERT_EQ(set.segments.size(),1) << set.segments;
	EXPECT_EQ(set.segments[0].getStart<0>(),21);
	EXPECT_EQ(set.segments[0].getEnd<0>(),99);
}
auto max_val = std::numeric_limits<uint32_t>::max();
TEST(SegmentSet,negate_single){
	PSegment s1(20,200,0,200);;
	std::vector<PSegment> results = {PSegment(0,19,0,200),PSegment(201,max_val,0,200),PSegment(0,max_val,201,max_val)};
	PSET set = PSET(std::vector{s1});
	set.NEGATE();
	EXPECT_EQ(set.segments.size(),3);
	for(auto res : results){
		bool found = false;
		for(auto seg : set.segments){
			if(seg == res){
				found = true;
				break;
			}
		}
		EXPECT_TRUE(found);
	}
}
bool SegSetEqual(PSET s1, PSET s2){
	for(const auto& seg1 : s1.segments){
		bool found = false;
		for(const auto& seg2 : s2.segments){
			if(seg2 == seg1){
				found = true;
				break;
			}
		}
		if(!found)return false;
	}
	for(const auto& seg1 : s2.segments){
		bool found = false;
		for(const auto& seg2 : s1.segments){
			if(seg2 == seg1){
				found = true;
				break;
			}
		}
		if(!found)return false;
	}
	return true;
}
TEST(SegmentSet,intersect_negated){
	PSET s1;
	s1.segments.emplace_back(10,20,300,500);
	PSET s2;
	s2.segments.emplace_back(5,9,143,5000);
	s2.segments.emplace_back(5,21,300,500);
	PSET s3;
	s3.segments.emplace_back(5,9,143,5000);
	s3.segments.emplace_back(5,9,300,500);
	s3.segments.emplace_back(21,21,300,500);

	s2.INTERSECTION_NEGATED(s1);
	EXPECT_TRUE(SegSetEqual(s3,s2));
}
TEST(SegmentSet,negate_empty){
	PSET set;
	EXPECT_TRUE(set.isEmpty());
	set.NEGATE();
	EXPECT_FALSE(set.isEmpty());
}
TEST(SegmentSet,negate_complex){
	PSegment s1(20,200);
	PSegment s2(0,max_val,30,300);
	PSET set({s1,s2});
	set.NEGATE();
	EXPECT_EQ(set.segments.size(),4);
	std::vector<PSegment> results = {
		PSegment(0,19,0,29),
		PSegment(201,max_val,0,29),
		PSegment(201,max_val,301,max_val),
		PSegment(0,19,301,max_val)
	};
	for(auto res : results){
		bool found = false;
		for(auto seg : set.segments){
			if(seg == res){
				found = true;
				break;
			}
		}
		EXPECT_TRUE(found);
	}
}
using Seg2D = Segment<int,int>;
/* ostream& operator<<(ostream& out, const gmp::BigFloat& val){ */
/* 	return out << val.get_d(); */
/* } */
RC_GTEST_PROP(SegmentSet,INTERSECTION_par_vs_seq,(const PSET& set1, const PSET& set2)){
	auto cpy1 = set1;
	auto cpy2 = set1;
	cpy1.INTERSECTION_par(set2);
	cpy2.INTERSECTION_seq(set2);
	RC_ASSERT(cpy1 == cpy2);
}
RC_GTEST_PROP(SegmentSet,INTERSECTION_NEGATED_par_vs_seq,(const PSET& set1, const PSET& set2)){
	auto cpy1 = set1;
	auto cpy2 = set1;
	cpy1.INTERSECTION_NEGATED_par(set2);
	cpy2.INTERSECTION_NEGATED_seq(set2);
	RC_ASSERT(cpy1 == cpy2);
}
RC_GTEST_PROP(SegmentSet,compact_reduces_ipset_of_all_equall_segments_to_one,(const PSegment& seg, uint16_t repeat)){
	RC_PRE(repeat != 0);
	PSET s;
	for(size_t i = 0; i < repeat; ++i){
		s.segments.push_back(seg);
	}
	s.compact();
	RC_ASSERT(s.segments.size() == 1u);
}
RC_GTEST_PROP(SegmentSet,compact_reduces_amount_of_segments,(const PSET& set)){
	auto cpy = set;
	cpy.compact();
	RC_ASSERT(cpy.segments.size() <= set.segments.size());
}
RC_GTEST_PROP(SegmentSet,compacted_sets_are_equal_to_uncompacted,(const PSET& set)){
	auto cpy = set;
	cpy.compact();
	RC_ASSERT(cpy == set);
}
RC_GTEST_PROP(SegmentSet,single_segment_set_has_same_value_as_segment,(const PSegment& seg)){
	PSET set(std::vector<PSegment>{seg});
	RC_ASSERT(set.getAmountPointsExakt() == seg.getAmountPointsExakt());
	RC_ASSERT(set.getAmountPoints() == seg.getAmountPoints());
}
RC_GTEST_PROP(SegmentSet,same_sets_are_equal,(const PSET& set)){
	RC_ASSERT(set == set);
}
RC_GTEST_PROP(SegmentSet,PSET_getAmountPoints_equiv_getAmountPointsExakt,(const PSET& seg)){
	gmp::BigInt lhs = (double)seg.getAmountPoints() - seg.getAmountPointsExakt();
	gmp::BigInt rhs  = seg.getAmountPointsExakt()/10000;
	RC_ASSERT(lhs <= rhs);
}
RC_GTEST_PROP(SegmentSet,getAmountPoints_equiv_getAmountPointsExakt,(const PSegment& seg)){
	gmp::BigInt lhs = (double)seg.getAmountPoints() - seg.getAmountPointsExakt();
	gmp::BigInt rhs  = seg.getAmountPointsExakt()/10000;
	RC_ASSERT(lhs <= rhs);
}
RC_GTEST_PROP(SegmentSet,safe_get_value_not_chaging_if_adding_same_segment,(const PSegment& seg, uint16_t repeat)){
	PSET set(std::vector<PSegment>{seg});
	for(size_t i = 0; i < repeat; ++i){
		auto s1 = set.getAmountPointsExaktSafe();
		set.segments.push_back(seg);
		auto s2 = set.getAmountPointsExaktSafe();
		RC_ASSERT(s1 == s2);
	}
}
RC_GTEST_PROP(SegmentSet,negation_de_morgan1,(const PSET& set1, const PSET& set2)){
	RC_ASSERT(NEGATION(INTERSECTION(set1, set2))
			== UNION(NEGATION(set1),NEGATION(set2)));
}
RC_GTEST_PROP(SegmentSet,negation_de_morgan2,(const PSET& set1, const PSET& set2)){
	/* std::cout << "lhs" << std::endl; */
	auto lhs = NEGATION(UNION(set1,set2));
	/* std::cout << "rhs" << std::endl; */
	auto rhs = INTERSECTION(NEGATION(set1),NEGATION(set2));
	/* std::cout << "equiv" << std::endl; */
	RC_ASSERT(lhs == rhs);
	/* RC_ASSERT(NEGATION(UNION(set1, set2)) */
	/* 		== INTERSECTION(NEGATION(set1),NEGATION(set2))); */
}
RC_GTEST_PROP(SegmentSet,intersection_union_distibutivity,(const PSET& set1, const PSET& set2, const PSET& set3)){
	RC_ASSERT(INTERSECTION(set1, UNION(set2,set3)) 
		   == UNION(INTERSECTION(set1,set2),INTERSECTION(set1,set3)));
}
RC_GTEST_PROP(SegmentSet,intersection_associativity,(const PSET& set1, const PSET& set2, const PSET& set3)){
	RC_ASSERT(INTERSECTION(set1, INTERSECTION(set2,set3)) 
			== INTERSECTION(INTERSECTION(set1,set2),set3));
}
RC_GTEST_PROP(SegmentSet,intersect_negated_equal_to_native_model,(const PSET& set1, const PSET& set2)){
	auto cpy = set1;
	cpy.INTERSECTION_NEGATED(set2);

	RC_ASSERT(INTERSECTION(set1,NEGATION(set2)) == cpy);
}
RC_GTEST_PROP(SegmentSet,intersection_commutes,(const PSET& set1, const PSET& set2)){
	RC_ASSERT(INTERSECTION(set1, set2) == INTERSECTION(set2,set1));
}
RC_GTEST_PROP(SegmentSet,union_of_inverse_is_everything,(const PSET& set)){
	RC_PRE(!set.isEmpty());
	auto cpy = set;
	cpy.NEGATE();
	cpy.UNION(set);
	PSET all;
	all.segments.emplace_back();
	RC_ASSERT(all == cpy);
}
RC_GTEST_PROP(SegmentSet,intersection_of_negative_is_empty,(const PSET& set)){
	RC_PRE(!set.isEmpty());
	auto cpy = set;
	cpy.INTERSECTION_NEGATED(set);
	RC_ASSERT(cpy.isEmpty());
}
RC_GTEST_PROP(SegmentSet,double_negation,(const PSET& set)){
	auto temp = set;
	temp.NEGATE();
	temp.NEGATE();
	RC_ASSERT(set == temp);
}
