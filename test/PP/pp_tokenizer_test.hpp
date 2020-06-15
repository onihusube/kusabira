#pragma once

#include "doctest/doctest.h"

#include "PP/file_reader.hpp"
#include "PP/pp_automaton.hpp"
#include "PP/pp_tokenizer.hpp"
#include "test/PP/pp_filereader_test.hpp"

namespace pp_tokenizer_test {

  TEST_CASE("tokenize test") {
    using kusabira::PP::pp_tokenize_status;

    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    REQUIRE_UNARY(std::filesystem::is_directory(testdir));
    REQUIRE_UNARY(std::filesystem::exists(testdir));

    kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm> tokenizer{testdir / "pp_test.cpp"};

//テスト出力に行番号が表示されないために泣く泣くラムダ式ではなくマクロにまとめた・・・
#define TOKEN_CHECK(caegory, str, col) \
{auto token = tokenizer.tokenize();\
REQUIRE(token);\
CHECK_EQ(token->kind, caegory);\
CHECK_UNARY(token->token == str);\
CHECK_EQ(token->column, col);}

    //#include <iostream>
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"#", 0u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"include", 1u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 8u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"<", 9u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"iostream", 10u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8">", 18u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 19u);

    //#include<cmath>
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"#", 0u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"include", 1u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"<", 8u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"cmath", 9u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8">", 14u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 15u);

    //#include "hoge.hpp"
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"#", 0u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"include", 1u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 8u);
    TOKEN_CHECK(pp_tokenize_status::StringLiteral, u8R"("hoge.hpp")", 9u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 19u);

    //import <type_traits>
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"import", 0u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 6u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"<", 7u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"type_traits", 8u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8">", 19u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 20u);

    TOKEN_CHECK(pp_tokenize_status::Empty, u8"", 0u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 0u);

    //#define N 10
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"#", 0u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"define", 1u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 7u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"N", 8u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 9u);
    TOKEN_CHECK(pp_tokenize_status::NumberLiteral, u8"10", 10u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 12u);

    TOKEN_CHECK(pp_tokenize_status::Empty, u8"", 0u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 0u);

    //main関数宣言
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"int", 0u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 3u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"main", 4u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"(", 8u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8")", 9u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 10u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"{", 11u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 12u);

    //int n = 0;
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8"  ", 0u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"int", 2u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 5u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"n", 6u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 7u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"=", 8u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 9u);
    TOKEN_CHECK(pp_tokenize_status::NumberLiteral, u8"0", 10u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8";", 11u);
    //行コメント
    TOKEN_CHECK(pp_tokenize_status::LineComment, u8"//line comment", 12u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"",26u);

    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8"  ", 0u);
    //ブロックコメント
    TOKEN_CHECK(pp_tokenize_status::BlockComment, u8"/*    ", 2u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 8u);
    TOKEN_CHECK(pp_tokenize_status::BlockComment, u8"  Block", 0u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 7u);
    TOKEN_CHECK(pp_tokenize_status::BlockComment, u8"  Comment", 0u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 9u);
    TOKEN_CHECK(pp_tokenize_status::BlockComment, u8"  */", 0u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 4u);

    //u8文字列リテラル
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8"  ", 0u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"auto", 2u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 6u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"str", 7u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 10u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"=", 11u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 12u);
    TOKEN_CHECK(pp_tokenize_status::StringLiteral, u8R"(u8"string")", 13u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"sv", 23u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8";", 25u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 26u);

    //生文字列リテラル
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8"  ", 0u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"auto", 2u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 6u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"rawstr", 7u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 13u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"=", 14u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 15u);
    TOKEN_CHECK(pp_tokenize_status::RawStrLiteral, u8R"*(R"(raw string)")*" ,16u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"_udl", 31u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8";", 35u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 36u);

    //改行あり生文字列リテラル
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8"  ", 0u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"auto", 2u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 6u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"rawstr2", 7u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 14u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"=", 15u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 16u);
    TOKEN_CHECK(pp_tokenize_status::DuringRawStr, u8R"*(R"+*(raw)*", 17u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 25u);
    TOKEN_CHECK(pp_tokenize_status::DuringRawStr, u8"string", 0u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 6u);
    TOKEN_CHECK(pp_tokenize_status::DuringRawStr, u8"literal", 0u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 7u);
    TOKEN_CHECK(pp_tokenize_status::RawStrLiteral, u8R"*(new line)+*")*", 0u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"_udl", 12u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8";", 16u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 17u);

    //浮動小数点リテラル
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8"  ", 0u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"float", 2u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 7u);
    TOKEN_CHECK(pp_tokenize_status::Identifier, u8"f", 8u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 9u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"=", 10u);
    TOKEN_CHECK(pp_tokenize_status::Whitespaces, u8" ", 11u);
    TOKEN_CHECK(pp_tokenize_status::NumberLiteral, u8"1.0E-8f", 12u);
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8";", 19u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 20u);

    //main関数終端
    TOKEN_CHECK(pp_tokenize_status::OPorPunc, u8"}", 0u);
    TOKEN_CHECK(pp_tokenize_status::NewLine, u8"", 1u);

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