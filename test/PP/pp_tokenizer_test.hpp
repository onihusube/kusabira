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

      //1行目
      auto line = fr.readline();

      CHECK_UNARY(line.length() == 5);
      //文字列末尾にCRコードが残っているはず
      auto p = line.data() + 4;
      CHECK_EQ(*p, u8'\x0d');

      //2行目
      line = fr.readline();

      CHECK_UNARY(line.length() == 43);
      //文字列末尾にCRコードが残っているはず
      p = line.data() + 42;
      CHECK_EQ(*p, u8'\x0d');

      //3行目、最終行、改行なし
      line = fr.readline();
      CHECK_UNARY(line.length() == 4);
    }

    //LF改行ファイルのテスト
    {
      kusabira::PP::filereader fr{testdir / "reader_lf.txt"};

      CHECK_UNARY(bool(fr));

      //1行目
      auto line = fr.readline();

      CHECK_UNARY(line.length() == 4);

      //2行目
      line = fr.readline();
      CHECK_UNARY(line.length() == 15);

      //3行目、最終行、改行なし
      line = fr.readline();
      CHECK_UNARY(line.length() == 2);
    }
  }

} // namespace pp_tokenaizer_test