#pragma once

#include "doctest/doctest.h"

#include "PP/file_reader.hpp"
#include "PP/pp_automaton.hpp"
#include "PP/pp_tokenizer.hpp"
#include "test/PP/pp_filereader_test.hpp"

namespace pp_tokenizer_test {

  TEST_CASE("tokenize test") {
    using kusabira::PP::pp_token_category;

    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    REQUIRE_UNARY(std::filesystem::is_directory(testdir));
    REQUIRE_UNARY(std::filesystem::exists(testdir));

    kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm> tokenizer{testdir / "pp_test.cpp"};

//テスト出力に行番号が表示されないために泣く泣くラムダ式ではなくマクロにまとめた・・・
#define TOKEN_CHECK(caegory, str, col) \
{auto token = tokenizer.tokenize();\
REQUIRE(token);\
CHECK_EQ(token->category, caegory);\
CHECK_UNARY(token->token == str);\
CHECK_EQ(token->column, col);}

    //#include <iostream>
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"#", 0u);
    TOKEN_CHECK(pp_token_category::identifier, u8"include", 1u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 8u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"<", 9u);
    TOKEN_CHECK(pp_token_category::identifier, u8"iostream", 10u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8">", 18u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 19u);

    //#include<cmath>
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"#", 0u);
    TOKEN_CHECK(pp_token_category::identifier, u8"include", 1u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"<", 8u);
    TOKEN_CHECK(pp_token_category::identifier, u8"cmath", 9u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8">", 14u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 15u);

    //#include "hoge.hpp"
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"#", 0u);
    TOKEN_CHECK(pp_token_category::identifier, u8"include", 1u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 8u);
    TOKEN_CHECK(pp_token_category::string_literal, u8R"("hoge.hpp")", 9u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 19u);

    //import <type_traits>
    TOKEN_CHECK(pp_token_category::identifier, u8"import", 0u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 6u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"<", 7u);
    TOKEN_CHECK(pp_token_category::identifier, u8"type_traits", 8u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8">", 19u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 20u);

    TOKEN_CHECK(pp_token_category::empty, u8"", 0u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 0u);

    //#define N 10
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"#", 0u);
    TOKEN_CHECK(pp_token_category::identifier, u8"define", 1u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 7u);
    TOKEN_CHECK(pp_token_category::identifier, u8"N", 8u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 9u);
    TOKEN_CHECK(pp_token_category::pp_number, u8"10", 10u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 12u);

    TOKEN_CHECK(pp_token_category::empty, u8"", 0u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 0u);

    //main関数宣言
    TOKEN_CHECK(pp_token_category::identifier, u8"int", 0u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 3u);
    TOKEN_CHECK(pp_token_category::identifier, u8"main", 4u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"(", 8u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8")", 9u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 10u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"{", 11u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 12u);

    //int n = 0;
    TOKEN_CHECK(pp_token_category::whitespaces, u8"  ", 0u);
    TOKEN_CHECK(pp_token_category::identifier, u8"int", 2u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 5u);
    TOKEN_CHECK(pp_token_category::identifier, u8"n", 6u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 7u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"=", 8u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 9u);
    TOKEN_CHECK(pp_token_category::pp_number, u8"0", 10u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8";", 11u);
    //行コメント
    TOKEN_CHECK(pp_token_category::line_comment, u8"//line comment", 12u);
    TOKEN_CHECK(pp_token_category::newline, u8"",26u);

    TOKEN_CHECK(pp_token_category::whitespaces, u8"  ", 0u);
    //ブロックコメント
    TOKEN_CHECK(pp_token_category::block_comment, u8"/*    ", 2u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 8u);
    TOKEN_CHECK(pp_token_category::block_comment, u8"  Block", 0u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 7u);
    TOKEN_CHECK(pp_token_category::block_comment, u8"  Comment", 0u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 9u);
    TOKEN_CHECK(pp_token_category::block_comment, u8"  */", 0u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 4u);

    //u8文字列リテラル
    TOKEN_CHECK(pp_token_category::whitespaces, u8"  ", 0u);
    TOKEN_CHECK(pp_token_category::identifier, u8"auto", 2u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 6u);
    TOKEN_CHECK(pp_token_category::identifier, u8"str", 7u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 10u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"=", 11u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 12u);
    TOKEN_CHECK(pp_token_category::string_literal, u8R"(u8"string")", 13u);
    TOKEN_CHECK(pp_token_category::identifier, u8"sv", 23u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8";", 25u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 26u);

    //生文字列リテラル
    TOKEN_CHECK(pp_token_category::whitespaces, u8"  ", 0u);
    TOKEN_CHECK(pp_token_category::identifier, u8"auto", 2u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 6u);
    TOKEN_CHECK(pp_token_category::identifier, u8"rawstr", 7u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 13u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"=", 14u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 15u);
    TOKEN_CHECK(pp_token_category::raw_string_literal, u8R"*(R"(raw string)")*" ,16u);
    TOKEN_CHECK(pp_token_category::identifier, u8"_udl", 31u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8";", 35u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 36u);

    //改行あり生文字列リテラル
    TOKEN_CHECK(pp_token_category::whitespaces, u8"  ", 0u);
    TOKEN_CHECK(pp_token_category::identifier, u8"auto", 2u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 6u);
    TOKEN_CHECK(pp_token_category::identifier, u8"rawstr2", 7u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 14u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"=", 15u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 16u);
    TOKEN_CHECK(pp_token_category::during_raw_string_literal, u8R"*(R"+*(raw)*", 17u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 25u);
    TOKEN_CHECK(pp_token_category::during_raw_string_literal, u8"string", 0u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 6u);
    TOKEN_CHECK(pp_token_category::during_raw_string_literal, u8"literal", 0u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 7u);
    TOKEN_CHECK(pp_token_category::raw_string_literal, u8R"*(new line)+*")*", 0u);
    TOKEN_CHECK(pp_token_category::identifier, u8"_udl", 12u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8";", 16u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 17u);

    //浮動小数点リテラル
    TOKEN_CHECK(pp_token_category::whitespaces, u8"  ", 0u);
    TOKEN_CHECK(pp_token_category::identifier, u8"float", 2u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 7u);
    TOKEN_CHECK(pp_token_category::identifier, u8"f", 8u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 9u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"=", 10u);
    TOKEN_CHECK(pp_token_category::whitespaces, u8" ", 11u);
    TOKEN_CHECK(pp_token_category::pp_number, u8"1.0E-8f", 12u);
    TOKEN_CHECK(pp_token_category::op_or_punc, u8";", 19u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 20u);

    //main関数終端
    TOKEN_CHECK(pp_token_category::op_or_punc, u8"}", 0u);
    TOKEN_CHECK(pp_token_category::newline, u8"", 1u);

    //ファイル終端
    CHECK_UNARY_FALSE(tokenizer.tokenize());
  
  //消しとく
  #undef TOKEN_CHECK
  }


  TEST_CASE("tokenizer iterator test") {

    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    REQUIRE_UNARY(std::filesystem::is_directory(testdir));
    REQUIRE_UNARY(std::filesystem::exists(testdir));

    kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm> tokenizer{testdir / "pp_test.cpp"};

    std::size_t count = 0;

    for ([[maybe_unused]] auto&& token : tokenizer) {
      ++count;
    }

    CHECK_EQ(count, 93 + 21); //トークン+改行
  }
} // namespace pp_tokenizer_test