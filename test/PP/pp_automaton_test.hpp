#pragma once

#include "PP/pp_automaton.hpp"

namespace pp_automaton_test
{

  TEST_CASE("white space test") {
    kusabira::PP::pp_tokenizer_sm sm{};

    std::u8string input = u8"\t\v\f \n   a";

    for (int i = 0; i < 8; ++i) {
      CHECK_UNARY_FALSE(sm.input_char(input.at(i)));
    }
    
    //最後の文字a、非空白
    auto res = sm.input_char(input.back());
    CHECK_UNARY(res);
    CHECK_EQ(res, kusabira::PP::pp_token_category::whitespace);
  }

  TEST_CASE("identifer test") {
    kusabira::PP::pp_tokenizer_sm sm{};

    std::u8string input = u8"int _number l1234QAZ__{} ";

    int index = 0;

    for (; index < 3; ++index) {
      CHECK_UNARY_FALSE(sm.input_char(input.at(index)));
    }

    //intを受理
    auto res = sm.input_char(input.at(index));
    CHECK_UNARY(res);
    CHECK_EQ(res, kusabira::PP::pp_token_category::identifier);

    //ホワイトスペース読み込みを終了
    CHECK_UNARY_FALSE(sm.input_char(input.at(index++)));
    CHECK_UNARY(sm.input_char(input.at(index)));

    for (; index < 11; ++index) {
      CHECK_UNARY_FALSE(sm.input_char(input.at(index)));
    }

    //_numberを受理
    res = sm.input_char(input.at(index));
    CHECK_UNARY(res);
    CHECK_EQ(res, kusabira::PP::pp_token_category::identifier);

    //ホワイトスペース読み込みを終了
    CHECK_UNARY_FALSE(sm.input_char(input.at(index++)));
    CHECK_UNARY(sm.input_char(input.at(index)));

    for (; index < 22; ++index) {
      CHECK_UNARY_FALSE(sm.input_char(input.at(index)));
    }

    //l1234QAZ__を受理
    res = sm.input_char(input.at(index));
    CHECK_UNARY(res);
    CHECK_EQ(res, kusabira::PP::pp_token_category::identifier);

    //{読み取り
    CHECK_UNARY_FALSE(sm.input_char(input.at(index++)));
    res = sm.input_char(input.at(index));
    CHECK_UNARY(res);
    CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);

    //}読み取り
    CHECK_UNARY_FALSE(sm.input_char(input.at(index++)));
    res = sm.input_char(input.at(index));
    CHECK_UNARY(res);
    CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);
  }

  TEST_CASE("punct test") {
    kusabira::PP::pp_tokenizer_sm sm{};

    std::u8string onechar = u8"{}[]#()~,?:.=!+-*/%^&|<>;";
    
    //1文字記号の受理
    CHECK_UNARY_FALSE(sm.input_char(onechar.at(0)));
    for (auto i = 1u; i < onechar.length(); ++i) {
      auto res = sm.input_char(onechar.at(i));
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);
      //現在文字から読み始めるために再入力
      CHECK_UNARY_FALSE(sm.input_char(onechar.at(i)));
    }

    auto semicolon = sm.input_newline();
    CHECK_UNARY(semicolon);
    CHECK_EQ(semicolon, kusabira::PP::pp_token_category::op_or_punc);

    std::u8string_view twochar[] = { u8"<: ", u8":> ", u8"<% ", u8"%> ", u8"%: "/*, u8":: "*/, u8".* ", u8"-> ", u8"+= ", u8"-= ", u8"*= ", u8"/= ", u8"%= ", u8"^= ", u8"&= ", u8"|= ", u8"== ", u8"!= ", u8"<= ", u8">= ", u8"&& ", u8"|| ", u8"<< ", u8">> ", u8"## " };

    //2文字記号の受理
    for (auto op : twochar) {
      CHECK_UNARY_FALSE(sm.input_char(op.at(0)));
      CHECK_UNARY_FALSE(sm.input_char(op.at(1)));
      //3文字目の読み取り（スペース）によって受理状態へ
      auto res = sm.input_char(op.at(2));
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);
    }

    std::u8string_view threechar[] = { u8"... ", u8"<<= ", u8">>= ", u8"->* ", u8"<=> " };

    for (auto op : threechar) {
      CHECK_UNARY_FALSE(sm.input_char(op.at(0)));
      CHECK_UNARY_FALSE(sm.input_char(op.at(1)));
      CHECK_UNARY_FALSE(sm.input_char(op.at(2)));
      //4文字目の読み取り（スペース）によって受理状態へ
      auto res = sm.input_char(op.at(3));
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);
    }

    //最長一致規則の例外対応、代替トークン<:はそのまま受理
    std::u8string_view exception_op[] = {u8":: ", u8"<::> ", u8"<:::>= ", u8"<::: "};

    //::
    {
      auto op = exception_op[0];
      CHECK_UNARY_FALSE(sm.input_char(op.at(0)));
      CHECK_UNARY_FALSE(sm.input_char(op.at(1)));
      //::はまとめる
      auto res = sm.input_char(op.at(2));
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);
    }

    //<::>
    {
      auto op = exception_op[1];
      CHECK_UNARY_FALSE(sm.input_char(op.at(0)));
      CHECK_UNARY_FALSE(sm.input_char(op.at(1)));
      //<:受理
      auto res = sm.input_char(op.at(2));
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);

      CHECK_UNARY_FALSE(sm.input_char(op.at(2)));
      CHECK_UNARY_FALSE(sm.input_char(op.at(3)));
      //:>受理
      res = sm.input_char(op.at(4));
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);
    }

    //<:::>=
    {
      auto op = exception_op[2];
      CHECK_UNARY_FALSE(sm.input_char(op.at(0)));
      CHECK_UNARY_FALSE(sm.input_char(op.at(1)));
      //<:受理
      auto res = sm.input_char(op.at(2));
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);

      //::受理
      CHECK_UNARY_FALSE(sm.input_char(op.at(2)));
      CHECK_UNARY_FALSE(sm.input_char(op.at(3)));
      res = sm.input_char(op.at(4));
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);

      CHECK_UNARY_FALSE(sm.input_char(op.at(4)));
      CHECK_UNARY_FALSE(sm.input_char(op.at(5)));
      //>=受理
      res = sm.input_char(op.at(6));
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);
    }

    //<:::
    {
      auto op = exception_op[3];
      CHECK_UNARY_FALSE(sm.input_char(op.at(0)));
      CHECK_UNARY_FALSE(sm.input_char(op.at(1)));
      //<:受理
      auto res = sm.input_char(op.at(2));
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);

      //::受理
      CHECK_UNARY_FALSE(sm.input_char(op.at(2)));
      CHECK_UNARY_FALSE(sm.input_char(op.at(3)));
      res = sm.input_char(op.at(4));
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);
    }
  }

  TEST_CASE("string literal test") {
    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"**("2345678djfb niweruo3rp  <>?_+*}`{=~|\t\n\f\\'[]:/]/,.-")**";

      for (auto c : str) {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //文字列リテラル読み込み終了
      auto res = sm.input_char(' ');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::string_literal);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8"'2345678djfb niweruo3rp  <>?_+*}`{=~|\t\n\f\\[]:/]/,.-\'";

      for (auto c : str) {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //文字リテラル読み込み終了
      auto res = sm.input_char(' ');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::string_literal);
    }
  }

  TEST_CASE("raw string literal test") {
    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(R"(abcde")")*";

      for (auto c : str) {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //生文字列リテラル読み込み終了
      auto res = sm.input_char(' ');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::raw_string_literal);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(R"+++(abcde")+++")*";

      for (auto c : str) {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //生文字列リテラル読み込み終了
      auto res = sm.input_char(' ');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::raw_string_literal);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(R"abcdefghijklmnop(abcde")abcdefghijklmnop")*";

      for (auto c : str) {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //生文字列リテラル読み込み終了
      auto res = sm.input_char(' ');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::raw_string_literal);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(R"!"#%&'*+,-./:;<=(abcde")!"#%&'*+,-./:;<=")*";

      for (auto c : str) {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //生文字列リテラル読み込み終了
      auto res = sm.input_char(' ');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::raw_string_literal);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string line1 = u8R"*(R"(abc)*";

      for (auto c : line1)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }
      //一旦受理する
      CHECK_UNARY(sm.input_newline());

      std::u8string line2 = u8R"(12345)";

      for (auto c : line2)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }
      //一旦受理する
      CHECK_UNARY(sm.input_newline());

      std::u8string line3 = u8R"+({}`**})")+";

      for (auto c : line3)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //生文字列リテラル読み込み終了
      auto res = sm.input_char(' ');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::raw_string_literal);
    }
  }

  TEST_CASE("comment test") {
    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(//dkshahkhfakhfahfh][]@][\n\r@[^0214030504909540]])*";

      for (auto c : str)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //改行入力、行コメント終了
      auto res = sm.input_newline();
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::line_comment);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(/*dkshahkhfakhfa**hfh][]@][\n\r@[^0214*030504909540]]*/)*";

      for (auto c : str)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //ブロック閉じ後の1文字入力
      auto res = sm.input_char(u8' ');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::block_comment);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string line1 = u8R"*(/*adjf*ij**a8*739)*";

      for (auto c : line1)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }
      //一旦受理する
      CHECK_UNARY(sm.input_newline());

      std::u8string line2 = u8R"(07340^*-\^[@];eaff***)";

      for (auto c : line2)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }
      //一旦受理する
      CHECK_UNARY(sm.input_newline());

      std::u8string line3 = u8R"+(*eooo38*49*:8@[]:]:*}*}{}{9*/)+";

      for (auto c : line3)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //ブロック閉じ後の1文字入力
      auto res = sm.input_char(u8' ');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::block_comment);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};

      //単体の/演算子
      CHECK_UNARY_FALSE(sm.input_char(u8'/'));
      CHECK_UNARY(sm.input_char(u8'a'));
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};

      //単体の/演算子
      CHECK_UNARY_FALSE(sm.input_char(u8'/'));
      CHECK_UNARY(sm.input_char(u8'9'));
    }
  }

  TEST_CASE("pp-number literal test") {
    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(123'456'778'990)*";

      for (auto c : str)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //英数字とドット、'以外のもの
      auto res = sm.input_char(u8';');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::pp_number);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(0xabcdef3074982)*";

      for (auto c : str)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //英数字とドット、'以外のもの
      auto res = sm.input_char(u8';');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::pp_number);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(0x1.2p3)*";

      for (auto c : str)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //英数字とドット、'以外のもの
      auto res = sm.input_char(u8';');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::pp_number);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(1e10)*";

      for (auto c : str)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //英数字とドット、'以外のもの
      auto res = sm.input_char(u8';');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::pp_number);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(0x1ffp10)*";

      for (auto c : str)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //英数字とドット、'以外のもの
      auto res = sm.input_char(u8';');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::pp_number);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};
      std::u8string str = u8R"*(0xa.bp10)*";

      for (auto c : str)
      {
        CHECK_UNARY_FALSE(sm.input_char(c));
      }

      //英数字とドット、'以外のもの
      auto res = sm.input_char(u8';');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::pp_number);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};

      CHECK_UNARY_FALSE(sm.input_char(u8'0'));
      CHECK_UNARY_FALSE(sm.input_char(u8'F'));
      //ユーザー定義リテラルは別
      auto res = sm.input_char(u8'_');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::pp_number);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};

      CHECK_UNARY_FALSE(sm.input_char(u8'0'));
      CHECK_UNARY_FALSE(sm.input_char(u8'F'));
      //16進数までしか考慮しない
      auto res = sm.input_char(u8'g');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::pp_number);
    }

    {
      kusabira::PP::pp_tokenizer_sm sm{};

      CHECK_UNARY_FALSE(sm.input_char(u8'.'));
      CHECK_UNARY_FALSE(sm.input_char(u8'1'));
      CHECK_UNARY_FALSE(sm.input_char(u8'f'));
      auto res = sm.input_char(u8'_');
      CHECK_UNARY(res);
      CHECK_EQ(res, kusabira::PP::pp_token_category::pp_number);
    }
  }

  TEST_CASE("input newline test") {

    kusabira::PP::pp_tokenizer_sm sm{};

    //スペース列入力
    CHECK_UNARY_FALSE(sm.input_char(u8' '));
    CHECK_UNARY_FALSE(sm.input_char(u8' '));
    CHECK_UNARY_FALSE(sm.input_char(u8' '));

    auto res = sm.input_newline();
    CHECK_UNARY(res);
    CHECK_EQ(res, kusabira::PP::pp_token_category::whitespace);


    //コメントっぽい？
    CHECK_UNARY_FALSE(sm.input_char(u8'/'));

    res = sm.input_newline();
    CHECK_UNARY(res);
    //記号です
    CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);


    //生文字列リテラルぽい？
    CHECK_UNARY_FALSE(sm.input_char(u8'R'));

    res = sm.input_newline();
    CHECK_UNARY(res);
    //識別子
    CHECK_EQ(res, kusabira::PP::pp_token_category::identifier);

    //生文字列リテラルぽい？2
    CHECK_UNARY_FALSE(sm.input_char(u8'R'));
    CHECK_UNARY_FALSE(sm.input_char(u8'"'));
    CHECK_UNARY_FALSE(sm.input_char(u8'('));
    CHECK_UNARY_FALSE(sm.input_char(u8'a'));

    res = sm.input_newline();
    CHECK_UNARY(res);
    //生文字列リテラルの途中
    CHECK_EQ(res, kusabira::PP::pp_token_category::during_raw_string_literal);

    CHECK_UNARY_FALSE(sm.input_char(u8'b'));
    CHECK_UNARY_FALSE(sm.input_char(u8')'));
    CHECK_UNARY_FALSE(sm.input_char(u8'"'));

    res = sm.input_newline();
    CHECK_UNARY(res);
    //生文字列リテラル
    CHECK_EQ(res, kusabira::PP::pp_token_category::raw_string_literal);

    //L文字列リテラルぽい？
    CHECK_UNARY_FALSE(sm.input_char(u8'L'));

    res = sm.input_newline();
    CHECK_UNARY(res);
    //識別子
    CHECK_EQ(res, kusabira::PP::pp_token_category::identifier);


    //u文字列リテラルぽい？
    CHECK_UNARY_FALSE(sm.input_char(u8'u'));

    res = sm.input_newline();
    CHECK_UNARY(res);
    //識別子
    CHECK_EQ(res, kusabira::PP::pp_token_category::identifier);


    //文字列リテラル
    CHECK_UNARY_FALSE(sm.input_char(u8'"'));

    res = sm.input_newline();
    CHECK_UNARY(res);
    //エラー
    CHECK_EQ(res, kusabira::PP::pp_token_category::UnexpectedNewLine);


    //文字リテラル
    CHECK_UNARY_FALSE(sm.input_char(u8'\''));

    res = sm.input_newline();
    CHECK_UNARY(res);
    //エラー
    CHECK_EQ(res, kusabira::PP::pp_token_category::UnexpectedNewLine);


    //エスケープシーケンス
    CHECK_UNARY_FALSE(sm.input_char(u8'"'));
    CHECK_UNARY_FALSE(sm.input_char(u8'a'));
    CHECK_UNARY_FALSE(sm.input_char(u8'\\'));

    res = sm.input_newline();
    CHECK_UNARY(res);
    //エラー
    CHECK_EQ(res, kusabira::PP::pp_token_category::UnexpectedNewLine);


    //識別子
    CHECK_UNARY_FALSE(sm.input_char(u8'_'));

    res = sm.input_newline();
    CHECK_UNARY(res);
    CHECK_EQ(res, kusabira::PP::pp_token_category::identifier);


    //数値リテラル
    CHECK_UNARY_FALSE(sm.input_char(u8'0'));
    CHECK_UNARY_FALSE(sm.input_char(u8'x'));
    CHECK_UNARY_FALSE(sm.input_char(u8'1'));

    res = sm.input_newline();
    CHECK_UNARY(res);
    CHECK_EQ(res, kusabira::PP::pp_token_category::pp_number);


    //浮動小数点リテラル、指数部
    CHECK_UNARY_FALSE(sm.input_char(u8'0'));
    CHECK_UNARY_FALSE(sm.input_char(u8'.'));
    CHECK_UNARY_FALSE(sm.input_char(u8'E'));

    res = sm.input_newline();
    CHECK_UNARY(res);
    CHECK_EQ(res, kusabira::PP::pp_token_category::pp_number);


    //記号列
    CHECK_UNARY_FALSE(sm.input_char(u8'<'));
    CHECK_UNARY_FALSE(sm.input_char(u8'='));

    res = sm.input_newline();
    CHECK_UNARY(res);
    CHECK_EQ(res, kusabira::PP::pp_token_category::op_or_punc);
  }
}