#pragma once

#include "doctest/doctest.h"

#include "PP/file_reader.hpp"


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

namespace pp_filereader_test {

  TEST_CASE("filereader test") {
    auto testdir = kusabira::test::get_testfiles_dir() / "PP";

    REQUIRE_UNARY(std::filesystem::is_directory(testdir));
    REQUIRE_UNARY(std::filesystem::exists(testdir));

    //CRLF改行ファイルのテスト
    {
      kusabira::PP::filereader fr{testdir / "reader_crlf.txt"};

      CHECK_UNARY(bool(fr));

      //1行目
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());

        CHECK_EQ(line->line.length(), 6);
        CHECK_EQ(1, line->phisic_line_num);
        CHECK_EQ(1, line->logical_line_num);
        //文字列末尾にCRコードが残っていないはず
        auto p = line->line.back();
        CHECK_UNARY(p != u8'\x0d');
      }

      //2行目
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());

        CHECK_EQ(line->line.length(), 42);
        CHECK_EQ(2, line->phisic_line_num);
        CHECK_EQ(2, line->logical_line_num);
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
        CHECK_EQ(3, line->phisic_line_num);
        CHECK_EQ(3, line->logical_line_num);
        CHECK_EQ(2, line->line_offset.size());
        CHECK_EQ(5, line->line_offset[0]);
        CHECK_EQ(line->line_offset[1], 5 + line->line_offset[0]);
      }

      //6行目、空行
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.empty());
        CHECK_EQ(6, line->phisic_line_num);
        CHECK_EQ(4, line->logical_line_num);
      }

      //7~8行目、行継続のち空行
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 4);
        CHECK_UNARY(line->line == u8"test");
        CHECK_EQ(7, line->phisic_line_num);
        CHECK_EQ(5, line->logical_line_num);
        CHECK_EQ(1, line->line_offset.size());
        CHECK_EQ(4, line->line_offset[0]);
      }

      //9行目、最終行、改行なし
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 4);
        CHECK_EQ(9, line->phisic_line_num);
        CHECK_EQ(6, line->logical_line_num);
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

      //1行目
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 4);
        CHECK_EQ(1, line->phisic_line_num);
        CHECK_EQ(1, line->logical_line_num);
      }

      //2行目
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 15);
        CHECK_EQ(2, line->phisic_line_num);
        CHECK_EQ(2, line->logical_line_num);
      }

      //3~5行目
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 15);
        CHECK_UNARY(line->line == u8"line continuous");
        CHECK_EQ(3, line->phisic_line_num);
        CHECK_EQ(3, line->logical_line_num);
        CHECK_EQ(2, line->line_offset.size());
        CHECK_EQ(5, line->line_offset[0]);
        CHECK_EQ(line->line_offset[1], 5 + line->line_offset[0]);
      }

      //6行目、空行
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.empty());
        CHECK_EQ(6, line->phisic_line_num);
        CHECK_EQ(4, line->logical_line_num);
      }

      //7~8行目、行継続のち空行
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 4);
        CHECK_UNARY(line->line == u8"test");
        CHECK_EQ(7, line->phisic_line_num);
        CHECK_EQ(5, line->logical_line_num);
        CHECK_EQ(1, line->line_offset.size());
        CHECK_EQ(4, line->line_offset[0]);
      }

      //9行目、最終行、改行なし
      {
        auto line = fr.readline();
        CHECK_UNARY(line.has_value());
        CHECK_UNARY(line->line.length() == 2);
        CHECK_EQ(9, line->phisic_line_num);
        CHECK_EQ(6, line->logical_line_num);
      }

      //10行目はない
      {
        auto line = fr.readline();
        CHECK_UNARY_FALSE(line.has_value());
      }
    }
  }
} // namespace pp_filereader_test