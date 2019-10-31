#pragma once

#include "pp_automaton_test.hpp"

namespace pp_automaton_test
{

  TEST_CASE("white space test") {
    kusabira::PP::pp_tokenizer_sm sm{};

    std::u8string input = u8"\t\v\f \n   a";

    for (int i = 0; i < 8; ++i) {
      CHECK_UNARY(sm.read(input.at(i)));
      CHECK_UNARY_FALSE(sm.should_line_continue());
    }
    
    //最後の文字a、非空白
    CHECK_UNARY_FALSE(sm.read(input.back()));
    CHECK_UNARY_FALSE(sm.should_line_continue());
  }

  TEST_CASE("identifer test") {
    kusabira::PP::pp_tokenizer_sm sm{};

    std::u8string input = u8"int _number l1234QAZ__{}";

    int index = 0;

    for (; index < 3; ++index) {
      CHECK_UNARY(sm.read(input.at(index)));
      CHECK_UNARY_FALSE(sm.should_line_continue());
    }

    CHECK_UNARY_FALSE(sm.read(input.at(index++)));
    CHECK_UNARY_FALSE(sm.should_line_continue());

    //ホワイトスペース読み込みを終了
    CHECK_UNARY_FALSE(sm.read(input.at(index)));

    for (; index < 11; ++index) {
      CHECK_UNARY(sm.read(input.at(index)));
      CHECK_UNARY_FALSE(sm.should_line_continue());
    }

    CHECK_UNARY_FALSE(sm.read(input.at(index++)));
    CHECK_UNARY_FALSE(sm.should_line_continue());

    //ホワイトスペース読み込みを終了
    CHECK_UNARY_FALSE(sm.read(input.at(index)));

    for (; index < 22; ++index) {
      CHECK_UNARY(sm.read(input.at(index)));
      CHECK_UNARY_FALSE(sm.should_line_continue());
    }

    CHECK_UNARY_FALSE(sm.read(input.at(index++)));
    CHECK_UNARY_FALSE(sm.should_line_continue());

    //記号列読み取りに入るのでtrueになる
    CHECK_UNARY(sm.read(input.at(index)));
    CHECK_UNARY_FALSE(sm.should_line_continue());
  }

  TEST_CASE("punct test") {
    kusabira::PP::pp_tokenizer_sm sm{};

    std::u8string accept = u8"{}[]#()<:>%;.?*-~!+-*/^&|=&,";

    for (auto ch : accept) {
      CHECK_UNARY(sm.read(ch));
      CHECK_UNARY_FALSE(sm.should_line_continue());
    }

    std::u8string input = u8"<< <=>a";
    int index = 0;

    for (; index < 2; ++index) {
      CHECK_UNARY(sm.read(input.at(index)));
      CHECK_UNARY_FALSE(sm.should_line_continue());
    }

    CHECK_UNARY_FALSE(sm.read(input.at(index++)));
    CHECK_UNARY_FALSE(sm.should_line_continue());

    //スペース読み取りを終了
    CHECK_UNARY_FALSE(sm.read(input.at(index)));

    for (; index < 6; ++index) {
      CHECK_UNARY(sm.read(input.at(index)));
      CHECK_UNARY_FALSE(sm.should_line_continue());
    }

    CHECK_UNARY_FALSE(sm.read(input.at(index)));
    CHECK_UNARY_FALSE(sm.should_line_continue());
  }

  TEST_CASE("string literal test") {
    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"**("2345678djfb niweruo3rp  <>?_+*}`{=~|\t\n\f\\'[]:/]/,.-")**";

      for (auto c : str) {
        CHECK_UNARY(sm.read(c));
      }

      CHECK_UNARY_FALSE(sm.read(' '));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8"'2345678djfb niweruo3rp  <>?_+*}`{=~|\t\n\f\\[]:/]/,.-\'";

      for (auto c : str) {
        CHECK_UNARY(sm.read(c));
      }

      CHECK_UNARY_FALSE(sm.read(' '));
    }
  }
}