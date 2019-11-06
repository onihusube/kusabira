#pragma once

#include "pp_automaton_test.hpp"

namespace pp_automaton_test
{

  TEST_CASE("white space test") {
    kusabira::PP::pp_tokenizer_sm sm{};

    std::u8string input = u8"\t\v\f \n   a";

    for (int i = 0; i < 8; ++i) {
      CHECK_UNARY(sm.input_char(input.at(i)));
    }
    
    //最後の文字a、非空白
    CHECK_UNARY_FALSE(sm.input_char(input.back()));
  }

  TEST_CASE("identifer test") {
    kusabira::PP::pp_tokenizer_sm sm{};

    std::u8string input = u8"int _number l1234QAZ__{}";

    int index = 0;

    for (; index < 3; ++index) {
      CHECK_UNARY(sm.input_char(input.at(index)));
    }

    CHECK_UNARY_FALSE(sm.input_char(input.at(index++)));

    //ホワイトスペース読み込みを終了
    CHECK_UNARY_FALSE(sm.input_char(input.at(index)));

    for (; index < 11; ++index) {
      CHECK_UNARY(sm.input_char(input.at(index)));
    }

    CHECK_UNARY_FALSE(sm.input_char(input.at(index++)));

    //ホワイトスペース読み込みを終了
    CHECK_UNARY_FALSE(sm.input_char(input.at(index)));

    for (; index < 22; ++index) {
      CHECK_UNARY(sm.input_char(input.at(index)));
    }

    CHECK_UNARY_FALSE(sm.input_char(input.at(index++)));

    //記号列読み取りに入るのでtrueになる
    CHECK_UNARY(sm.input_char(input.at(index)));
  }

  TEST_CASE("punct test") {
    kusabira::PP::pp_tokenizer_sm sm{};

    std::u8string accept = u8"{}[]#()<:>%;.?*-~!+-*/^&|=&,";

    for (auto ch : accept) {
      CHECK_UNARY(sm.input_char(ch));
    }

    std::u8string input = u8"<< <=>a";
    int index = 0;

    for (; index < 2; ++index) {
      CHECK_UNARY(sm.input_char(input.at(index)));
    }

    CHECK_UNARY_FALSE(sm.input_char(input.at(index++)));

    //スペース読み取りを終了
    CHECK_UNARY_FALSE(sm.input_char(input.at(index)));

    for (; index < 6; ++index) {
      CHECK_UNARY(sm.input_char(input.at(index)));
    }

    CHECK_UNARY_FALSE(sm.input_char(input.at(index)));
  }

  TEST_CASE("string literal test") {
    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"**("2345678djfb niweruo3rp  <>?_+*}`{=~|\t\n\f\\'[]:/]/,.-")**";

      for (auto c : str) {
        CHECK_UNARY(sm.input_char(c));
      }

      CHECK_UNARY_FALSE(sm.input_char(' '));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8"'2345678djfb niweruo3rp  <>?_+*}`{=~|\t\n\f\\[]:/]/,.-\'";

      for (auto c : str) {
        CHECK_UNARY(sm.input_char(c));
      }

      CHECK_UNARY_FALSE(sm.input_char(' '));
    }
  }

  TEST_CASE("raw string literal test") {
    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(R"(abcde")")*";

      for (auto c : str) {
        CHECK_UNARY(sm.input_char(c));
      }
      CHECK_UNARY_FALSE(sm.input_char(' '));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(R"+++(abcde")+++")*";

      for (auto c : str) {
        CHECK_UNARY(sm.input_char(c));
      }
      CHECK_UNARY_FALSE(sm.input_char(' '));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(R"abcdefghijklmnopqrstuvwxyz(abcde")abcdefghijklmnopqrstuvwxyz")*";

      for (auto c : str) {
        CHECK_UNARY(sm.input_char(c));
      }
      CHECK_UNARY_FALSE(sm.input_char(' '));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(R"!"#%&'*+,-./:;<=>?[]^_{|}~(abcde")!"#%&'*+,-./:;<=>?[]^_{|}~")*";

      for (auto c : str) {
        CHECK_UNARY(sm.input_char(c));
      }
      CHECK_UNARY_FALSE(sm.input_char(' '));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string line1 = u8R"*(R"(abc)*";

      for (auto c : line1)
      {
        CHECK_UNARY(sm.input_char(c));
      }
      CHECK_UNARY(sm.input_newline());

      std::u8string line2 = u8R"(12345)";

      for (auto c : line2)
      {
        CHECK_UNARY(sm.input_char(c));
      }
      CHECK_UNARY(sm.input_newline());

      std::u8string line3 = u8R"+({}`**})")+";

      for (auto c : line3)
      {
        CHECK_UNARY(sm.input_char(c));
      }
      CHECK_UNARY_FALSE(sm.input_char(' '));
    }
  }

  TEST_CASE("comment test") {
    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(//dkshahkhfakhfahfh][]@][\n\r@[^0214030504909540]])*";

      for (auto c : str)
      {
        CHECK_UNARY(sm.input_char(c));
      }
      CHECK_UNARY_FALSE(sm.input_newline());
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(/*dkshahkhfakhfa**hfh][]@][\n\r@[^0214*030504909540]]*)*";

      for (auto c : str)
      {
        CHECK_UNARY(sm.input_char(c));
      }
      //ブロック閉じの/入力
      CHECK_UNARY_FALSE(sm.input_char(u8'/'));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string line1 = u8R"*(/*adjf*ij**a8*739)*";

      for (auto c : line1)
      {
        CHECK_UNARY(sm.input_char(c));
      }
      CHECK_UNARY(sm.input_newline());

      std::u8string line2 = u8R"(07340^*-\^[@];eaff***)";

      for (auto c : line2)
      {
        CHECK_UNARY(sm.input_char(c));
      }
      CHECK_UNARY(sm.input_newline());

      std::u8string line3 = u8R"+(*eooo38*49*:8@[]:]:*}*}{}{9*)+";

      for (auto c : line3)
      {
        CHECK_UNARY(sm.input_char(c));
      }
      //ブロック閉じの/入力
      CHECK_UNARY_FALSE(sm.input_char(u8'/'));
    }
  }

  TEST_CASE("pp-number literal test") {
    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(123'456'778'990)*";

      for (auto c : str)
      {
        CHECK_UNARY(sm.input_char(c));
      }

      //英数字とドット、'以外のもの
      CHECK_UNARY_FALSE(sm.input_char(u8';'));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(0xabcdef3074982)*";

      for (auto c : str)
      {
        CHECK_UNARY(sm.input_char(c));
      }

      //英数字とドット、'以外のもの
      CHECK_UNARY_FALSE(sm.input_char(u8';'));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(0x1.2p3)*";

      for (auto c : str)
      {
        CHECK_UNARY(sm.input_char(c));
      }

      //英数字とドット、'以外のもの
      CHECK_UNARY_FALSE(sm.input_char(u8';'));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(1e10)*";

      for (auto c : str)
      {
        CHECK_UNARY(sm.input_char(c));
      }

      //英数字とドット、'以外のもの
      CHECK_UNARY_FALSE(sm.input_char(u8';'));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(0x1ffp10)*";

      for (auto c : str)
      {
        CHECK_UNARY(sm.input_char(c));
      }

      //英数字とドット、'以外のもの
      CHECK_UNARY_FALSE(sm.input_char(u8';'));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(0xa.bp10)*";

      for (auto c : str)
      {
        CHECK_UNARY(sm.input_char(c));
      }

      //英数字とドット、'以外のもの
      CHECK_UNARY_FALSE(sm.input_char(u8';'));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};

      CHECK_UNARY(sm.input_char(u8'0'));
      CHECK_UNARY(sm.input_char(u8'F'));
      //ユーザー定義リテラルは別
      CHECK_UNARY_FALSE(sm.input_char(u8'_'));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};

      CHECK_UNARY(sm.input_char(u8'0'));
      CHECK_UNARY(sm.input_char(u8'F'));
      //16進数までしか考慮しない
      CHECK_UNARY_FALSE(sm.input_char(u8'g'));
    }
  }
}