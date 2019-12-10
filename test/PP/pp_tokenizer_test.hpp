#pragma once

#include "doctest/doctest.h"

#include "PP/pp_tokenizer.hpp"

namespace kusabira::test {

  /**
  * @brief kusabiraのテストファイル用ディレクトリを雑に取得する
  * @detail /buildで実行された時と、/kusabiraで実行された時で同じディレクトリを取得できるようにするもの
  * @detail その他変なところでやられると全くうまく動かない
  * @return kusabiraのテストファイル群があるトップのディレクトリ
  */
  ifn get_testfiles_dir() -> std::filesystem::path {
    auto current_dir = std::filesystem::current_path();
    const auto parentdir_name = current_dir.parent_path().filename().string();

    std::filesystem::path result{};

    if (parentdir_name == "kusabira") {
      result = current_dir.parent_path();
    } else {
      result = current_dir;
    }

    return result / "test/files";
  }
}

namespace pp_tokenaizer_test
{
  TEST_CASE("filereader test") {
    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    CHECK_UNARY(std::filesystem::is_directory(testdir));
    CHECK_UNARY(std::filesystem::exists(testdir));

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

      //7行目、最終行、改行なし
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 4);
        CHECK_EQ(line->phisic_line, now_line++);
      }

      //8行目はない
      {
        auto line = fr.readline();
        CHECK_UNARY_FALSE(line.has_value());
      }
    }

    //LF改行ファイルのテスト
    {
      kusabira::PP::filereader fr{testdir / "reader_lf.txt"};

      CHECK_UNARY(bool(fr));

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

      //7行目、最終行、改行なし
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 2);
        CHECK_EQ(line->phisic_line, now_line++);
      }

      //8行目はない
      {
        auto line = fr.readline();
        CHECK_UNARY_FALSE(line.has_value());
      }
    }
  }

  TEST_CASE("tokenize test") {
    using kusabira::PP::pp_tokenize_status;

    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    CHECK_UNARY(std::filesystem::is_directory(testdir));
    CHECK_UNARY(std::filesystem::exists(testdir));

    kusabira::PP::tokenizer tokenizer{testdir / "pp_test.cpp"};

    auto check = [&tokenizer](pp_tokenize_status caegory, std::u8string_view str) {
      auto token = tokenizer.tokenize();

      REQUIRE(token);
      CHECK_EQ(token->kind, caegory);
      CHECK_EQ(token->token, str);
    };

    check(pp_tokenize_status::OPorPunc, u8"#");
    check(pp_tokenize_status::Identifier, u8"include");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::OPorPunc, u8"<");
    check(pp_tokenize_status::Identifier, u8"iostream");
    check(pp_tokenize_status::OPorPunc, u8">");

    check(pp_tokenize_status::OPorPunc, u8"#");
    check(pp_tokenize_status::Identifier, u8"include");
    check(pp_tokenize_status::OPorPunc, u8"<");
    check(pp_tokenize_status::Identifier, u8"cmath");
    check(pp_tokenize_status::OPorPunc, u8">");

    check(pp_tokenize_status::Empty, u8"");

    check(pp_tokenize_status::OPorPunc, u8"#");
    check(pp_tokenize_status::Identifier, u8"define");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::Identifier, u8"N");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::NumberLiteral, u8"10");

    check(pp_tokenize_status::Empty, u8"");

    /*check(pp_tokenize_status::Identifier, u8"int");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::Identifier, u8"main");
    check(pp_tokenize_status::OPorPunc, u8"(");
    check(pp_tokenize_status::OPorPunc, u8")");
    check(pp_tokenize_status::Whitespaces, u8" ");
    check(pp_tokenize_status::OPorPunc, u8"{");*/

    //check(pp_tokenize_status::OPorPunc, u8"}");
  }
} // namespace pp_tokenaizer_test