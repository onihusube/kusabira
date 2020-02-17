#pragma once

#include "doctest/doctest.h"

#include "PP/file_reader.hpp"
#include "PP/pp_automaton.hpp"
#include "PP/pp_tokenizer.hpp"

namespace kusabira::test {

  /**
  * @brief kusabiraのテストファイル用ディレクトリを雑に取得する
  * @detail /buildで実行された時と、/kusabiraで実行された時で同じディレクトリを取得できるようにするもの
  * @detail その他変なところでやられると全くうまく動かない
  * @return kusabiraのテストファイル群があるトップのディレクトリ
  */
  ifn get_testfiles_dir() -> std::filesystem::path {
    std::filesystem::path result = std::filesystem::current_path();

    while (result.filename() != "kusabira") {
      result = result.parent_path();
    }

    return result / "test/files";
  }
}

namespace pp_tokenaizer_test
{
  TEST_CASE("filereader test") {
    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    REQUIRE_UNARY(std::filesystem::is_directory(testdir));
    REQUIRE_UNARY(std::filesystem::exists(testdir));

    //CRLF改行ファイルのテスト
    {
      kusabira::PP::filereader fr{testdir / "reader_crlf.txt"};

      CHECK_UNARY(bool(fr));

      std::size_t now_line = 1;

      //1行目
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());

        CHECK_EQ(line->line.length(), 6);
        CHECK_EQ(line->phisic_line, now_line++);
        //文字列末尾にCRコードが残っていないはず
        auto p = line->line.back();
        CHECK_UNARY(p != u8'\x0d');
      }

      //2行目
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());

        CHECK_EQ(line->line.length(), 42);
        CHECK_EQ(line->phisic_line, now_line++);
        //文字列末尾にCRコードが残っていないはず
        auto p = line->line.back();
        CHECK_UNARY(p != u8'\x0d');
      }

      //3~5行目
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 15);
        CHECK_UNARY(line->line == u8"line continuous");
        CHECK_EQ(line->phisic_line, now_line++);
        CHECK_EQ(line->line_offset.size(), 2);
        CHECK_EQ(line->line_offset[0], 5);
        CHECK_EQ(line->line_offset[1], 5 + line->line_offset[0]);
        now_line += line->line_offset.size();
      }

      //6行目、空行
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.empty());
        CHECK_EQ(line->phisic_line, now_line++);
      }

      //7~8行目、行継続のち空行
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 4);
        CHECK_UNARY(line->line == u8"test");
        CHECK_EQ(line->phisic_line, now_line++);
        CHECK_EQ(line->line_offset.size(), 1);
        CHECK_EQ(line->line_offset[0], 4);
        now_line += line->line_offset.size();
      }

      //9行目、最終行、改行なし
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 4);
        CHECK_EQ(line->phisic_line, now_line++);
      }

      //10行目はない
      {
        auto line = fr.readline();
        CHECK_UNARY_FALSE(line.has_value());
      }
    }

    //LF改行ファイルのテスト
    {
      kusabira::PP::filereader fr{testdir / "reader_lf.txt"};

      REQUIRE_UNARY(bool(fr));

      std::size_t now_line = 1;

      //1行目
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 4);
        CHECK_EQ(line->phisic_line, now_line++);
      }

      //2行目
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 15);
        CHECK_EQ(line->phisic_line, now_line++);
      }

      //3~5行目
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 15);
        CHECK_UNARY(line->line == u8"line continuous");
        CHECK_EQ(line->phisic_line, now_line++);
        CHECK_EQ(line->line_offset.size(), 2);
        CHECK_EQ(line->line_offset[0], 5);
        CHECK_EQ(line->line_offset[1], 5 + line->line_offset[0]);
        now_line += line->line_offset.size();
      }

      //6行目、空行
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.empty());
        CHECK_EQ(line->phisic_line, now_line++);
      }

      //7~8行目、行継続のち空行
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 4);
        CHECK_UNARY(line->line == u8"test");
        CHECK_EQ(line->phisic_line, now_line++);
        CHECK_EQ(line->line_offset.size(), 1);
        CHECK_EQ(line->line_offset[0], 4);
        now_line += line->line_offset.size();
      }

      //9行目、最終行、改行なし
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 2);
        CHECK_EQ(line->phisic_line, now_line++);
      }

      //10行目はない
      {
        auto line = fr.readline();
        CHECK_UNARY_FALSE(line.has_value());
      }
    }
  }



  TEST_CASE("tokenize test") {
    using kusabira::PP::pp_tokenize_status;

    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    REQUIRE_UNARY(std::filesystem::is_directory(testdir));
    REQUIRE_UNARY(std::filesystem::exists(testdir));

    kusabira::PP::tokenizer<kusabira::PP::filereader, kusabira::PP::pp_tokenizer_sm> tokenizer{testdir / "pp_test.cpp"};

//テスト出力に行番号が表示されないために泣く泣くラムダ式ではなくマクロにまとめた・・・
#define check(caegory, str) \
{auto token = tokenizer.tokenize();\
REQUIRE(token);\
CHECK_EQ(token->kind, caegory);\
CHECK_UNARY(token->token == str);}

    //#include <iostream>
    check(pp_tokenize_status::OPorPunc, u8"#");
    check(pp_tokenize_status::Identifier, u8"include");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::OPorPunc, u8"<");
    check(pp_tokenize_status::Identifier, u8"iostream");
    check(pp_tokenize_status::OPorPunc, u8">");
    check(pp_tokenize_status::NewLine, u8"");

    //#include<cmath>
    check(pp_tokenize_status::OPorPunc, u8"#");
    check(pp_tokenize_status::Identifier, u8"include");
    check(pp_tokenize_status::OPorPunc, u8"<");
    check(pp_tokenize_status::Identifier, u8"cmath");
    check(pp_tokenize_status::OPorPunc, u8">");
    check(pp_tokenize_status::NewLine, u8"");

    //#include "hoge.hpp"
    check(pp_tokenize_status::OPorPunc, u8"#");
    check(pp_tokenize_status::Identifier, u8"include");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::StringLiteral, u8R"("hoge.hpp")");
    check(pp_tokenize_status::NewLine, u8"");

    //import <type_traits>
    check(pp_tokenize_status::Identifier, u8"import");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::OPorPunc, u8"<");
    check(pp_tokenize_status::Identifier, u8"type_traits");
    check(pp_tokenize_status::OPorPunc, u8">");
    check(pp_tokenize_status::NewLine, u8"");

    check(pp_tokenize_status::Empty, u8"");
    check(pp_tokenize_status::NewLine, u8"");

    //#define N 10
    check(pp_tokenize_status::OPorPunc, u8"#");
    check(pp_tokenize_status::Identifier, u8"define");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::Identifier, u8"N");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::NumberLiteral, u8"10");
    check(pp_tokenize_status::NewLine, u8"");

    check(pp_tokenize_status::Empty, u8"");
    check(pp_tokenize_status::NewLine, u8"");

    //main関数宣言
    check(pp_tokenize_status::Identifier, u8"int");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::Identifier, u8"main");
    check(pp_tokenize_status::OPorPunc, u8"(");
    check(pp_tokenize_status::OPorPunc, u8")");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::OPorPunc, u8"{");
    check(pp_tokenize_status::NewLine, u8"");

    //  int n = 0;
    check(pp_tokenize_status::Whitespaces, u8"  ");
    check(pp_tokenize_status::Identifier, u8"int");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::Identifier, u8"n");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::OPorPunc, u8"=");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::NumberLiteral, u8"0");
    check(pp_tokenize_status::OPorPunc, u8";");
    //行コメント
    check(pp_tokenize_status::LineComment, u8"//line comment");
    check(pp_tokenize_status::NewLine, u8"");

    check(pp_tokenize_status::Whitespaces, u8"  ");
    //ブロックコメント
    check(pp_tokenize_status::BlockComment, u8"/*    ");
    check(pp_tokenize_status::NewLine, u8"");
    check(pp_tokenize_status::BlockComment, u8"  Block");
    check(pp_tokenize_status::NewLine, u8"");
    check(pp_tokenize_status::BlockComment, u8"  Comment");
    check(pp_tokenize_status::NewLine, u8"");
    check(pp_tokenize_status::BlockComment, u8"  */");
    check(pp_tokenize_status::NewLine, u8"");

    //u8文字列リテラル
    check(pp_tokenize_status::Whitespaces, u8"  ");
    check(pp_tokenize_status::Identifier, u8"auto");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::Identifier, u8"str");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::OPorPunc, u8"=");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::StringLiteral, u8R"(u8"string")");
    check(pp_tokenize_status::Identifier, u8"sv");
    check(pp_tokenize_status::OPorPunc, u8";");
    check(pp_tokenize_status::NewLine, u8"");

    //生文字列リテラル
    check(pp_tokenize_status::Whitespaces, u8"  ");
    check(pp_tokenize_status::Identifier, u8"auto");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::Identifier, u8"rawstr");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::OPorPunc, u8"=");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::RawStrLiteral, u8R"*(R"(raw string)")*");
    check(pp_tokenize_status::Identifier, u8"_udl");
    check(pp_tokenize_status::OPorPunc, u8";");
    check(pp_tokenize_status::NewLine, u8"");

    //改行あり生文字列リテラル
    check(pp_tokenize_status::Whitespaces, u8"  ");
    check(pp_tokenize_status::Identifier, u8"auto");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::Identifier, u8"rawstr2");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::OPorPunc, u8"=");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::DuringRawStr, u8R"*(R"+*(raw)*");
    check(pp_tokenize_status::NewLine, u8"");
    check(pp_tokenize_status::DuringRawStr, u8"string");
    check(pp_tokenize_status::NewLine, u8"");
    check(pp_tokenize_status::DuringRawStr, u8"literal");
    check(pp_tokenize_status::NewLine, u8"");
    check(pp_tokenize_status::RawStrLiteral, u8R"*(new line)+*")*");
    check(pp_tokenize_status::Identifier, u8"_udl");
    check(pp_tokenize_status::OPorPunc, u8";");
    check(pp_tokenize_status::NewLine, u8"");

    //浮動小数点リテラル
    check(pp_tokenize_status::Whitespaces, u8"  ");
    check(pp_tokenize_status::Identifier, u8"float");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::Identifier, u8"f");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::OPorPunc, u8"=");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::NumberLiteral, u8"1.0E-8f");
    check(pp_tokenize_status::OPorPunc, u8";");
    check(pp_tokenize_status::NewLine, u8"");

    //main関数終端
    check(pp_tokenize_status::OPorPunc, u8"}");
    check(pp_tokenize_status::NewLine, u8"");

    //ファイル終端
    CHECK_UNARY_FALSE(tokenizer.tokenize());
  
  //消しとく
  #undef check
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
} // namespace pp_tokenaizer_test