#include <gtest/gtest.h>
#include "try_parser.hpp"

TEST(try_parser,ws_done){
	auto text = "  \t \n  ";
	try_parser parser(text);
	EXPECT_FALSE(parser.done());
	parser.ws();
	EXPECT_TRUE(parser.done());
}

TEST(try_parser,whole_string){
	auto text = "\"abc\"";
	try_parser parser(text);
	auto p_text = parser.string();
	
	EXPECT_EQ(text,p_text);
	EXPECT_TRUE(parser.done());
}

TEST(try_parser,two_strings){
	auto text = "abc def";
	try_parser parser(text);
	auto p_text1 = parser.string();
	parser.ws();
	auto p_text2 = parser.string();
	
	EXPECT_EQ("abc",p_text1);
	EXPECT_EQ("def",p_text2);
	EXPECT_TRUE(parser.done());
}
TEST(try_parser,three_numbers){
	auto text = "2 \t-2323 0";
	try_parser parser(text);
	auto num1 = parser.integer();
	parser.ws();
	auto num2 = parser.integer();
	parser.ws();
	auto num3 = parser.integer();
	
	EXPECT_EQ(2,num1);
	EXPECT_EQ(-2323,num2);
	EXPECT_EQ(0,num3);
	EXPECT_TRUE(parser.done());
}
TEST(try_parser,nextLine){
	auto text = "abc\ndef";
	try_parser parser(text);
	parser.nextLine();
	auto p_text = parser.string();
	
	EXPECT_EQ("def",p_text);
	EXPECT_TRUE(parser.done());
}
TEST(try_parser,save_restore){
	auto text = "abc\ndef";
	try_parser parser(text);
	parser.save();
	parser.nextLine();
	auto p_text1 = parser.string();
	parser.restore();
	parser.save();
	auto p_text2 = parser.string();
	parser.restore();
	auto p_text3 = parser.string();

	
	EXPECT_EQ("def",p_text1);
	EXPECT_EQ("abc",p_text2);
	EXPECT_EQ("abc",p_text3);
	EXPECT_FALSE(parser.done());
}
TEST(try_parser,empty_text){
	auto text = "";
	try_parser parser(text);
	EXPECT_TRUE(parser.done());
}
TEST(try_parser,until){
	auto text = "abcdef";
	try_parser parser(text);
	auto [ab,cd] = *parser.until("cd");
	parser.skip(cd.size());
	auto rest = parser.string();

	EXPECT_EQ(ab,"ab");
	EXPECT_EQ(cd,"cd");
	EXPECT_EQ(rest,"ef");
}
TEST(try_parser,until_regex){
	auto text = "01:02:03-04:05:06";
	try_parser parser(text);
	int i = 0;
	for(; i < 10; ++i){
		auto result = parser.until("[:-]");
		if(!result)break;
		parser.skip(result->second.size());
		EXPECT_EQ(2,result->first.size());
	}
	EXPECT_EQ(i,5);
}
