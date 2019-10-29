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
  fni get_testfiles_dir() -> std::filesystem::path {
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

} // namespace pp_tokenaizer_test