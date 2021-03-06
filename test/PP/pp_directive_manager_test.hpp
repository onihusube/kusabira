#pragma once

#include "doctest/doctest.h"
#include "PP/pp_directive_manager.hpp"
#include "../report_output_test.hpp"

#include <iomanip>
#include <ranges>

namespace kusabira_test::preprocessor {
  using kusabira::PP::logical_line;
  using kusabira::PP::pp_token;
  using kusabira::PP::pp_token_category;
  using namespace std::string_view_literals;

  TEST_CASE("rawstrliteral extract test") {

    {
      auto rstr = u8R"++++(u8R"(normal raw string literal)")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(rstr, true);

      CHECK_UNARY_FALSE(result.empty());
      CHECK_UNARY(result == u8"normal raw string literal");
    }

    {
      auto rstr = u8R"++++(LR"()")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(rstr, true);

      CHECK_UNARY(result.empty());
    }

    {
      auto rstr = u8R"++++(UR"abcdefghijklmnop(max delimiter)abcdefghijklmnop")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(rstr, true);

      CHECK_UNARY_FALSE(result.empty());
      CHECK_UNARY(result == u8"max delimiter");
    }

    {
      auto rstr = u8R"++++(UR"abcdefghijklmnop()abcdefghijklmnop")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(rstr, true);

      CHECK_UNARY(result.empty());
    }
  }

  TEST_CASE("strliteral extract test") {

    {
      auto str = u8R"++++("normal string literal")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(str, false);

      CHECK_UNARY_FALSE(result.empty());
      CHECK_UNARY(result == u8"normal string literal");
    }

    {
      auto str = u8R"++++(U"")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(str, false);

      CHECK_UNARY(result.empty());
    }

    {
      auto str = u8R"++++(u8"string " literal")++++";

      auto result = kusabira::PP::extract_string_from_strtoken(str, false);

      CHECK_UNARY_FALSE(result.empty());
      CHECK_UNARY(result == u8"string \" literal");
    }
  }

  TEST_CASE("parse_macro_args test") {
    {
      // M(a1, a2, a3)
      std::vector<pp_token> tokens{
        pp_token{pp_token_category::identifier, u8"a1"},
        pp_token{pp_token_category::op_or_punc, u8","},
        pp_token{pp_token_category::identifier, u8"a2"},
        pp_token{pp_token_category::op_or_punc, u8","},
        pp_token{pp_token_category::identifier, u8"a3"},
        pp_token{pp_token_category::op_or_punc, u8")"}
      };

      auto it = std::ranges::begin(tokens);
      auto args = kusabira::PP::parse_macro_args(it, tokens.end());

      CHECK_EQ(args.size(), 3);

      it = std::ranges::begin(tokens);
      for (auto& list : args) {
        CHECK_EQ(list.size(), 1);
        CHECK_UNARY(std::ranges::equal(list, std::ranges::subrange{it, std::ranges::next(it)}));
        it = std::ranges::next(it, 2);
      }
    }
    {
      // M(1 * 2 * 1 + 0, (arg + 1 ))
      std::vector<pp_token> tokens{
        pp_token{pp_token_category::pp_number, u8"1"},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::op_or_punc, u8"*"},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::pp_number, u8"2"},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::op_or_punc, u8"*"},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::pp_number, u8"1"},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::op_or_punc, u8"+"},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::pp_number, u8"0"},
        pp_token{pp_token_category::op_or_punc, u8","},
        pp_token{pp_token_category::op_or_punc, u8"("},
        pp_token{pp_token_category::identifier, u8"arg"},
        pp_token{pp_token_category::op_or_punc, u8"+"},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::pp_number, u8"1"},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::op_or_punc, u8")"},
        pp_token{pp_token_category::op_or_punc, u8")"}
      };

      auto it = std::ranges::begin(tokens);
      auto args = kusabira::PP::parse_macro_args(it, tokens.end());

      CHECK_EQ(args.size(), 2);
      CHECK_EQ(args[0].size(), 13);
      CHECK_EQ(args[1].size(), 7);

      it = std::ranges::begin(tokens);
      CHECK_UNARY(std::ranges::equal(args[0], std::ranges::subrange{it, std::ranges::next(it, 13)}));
      CHECK_UNARY(std::ranges::equal(args[1], std::ranges::subrange{std::ranges::next(it, 14), std::ranges::next(it, 14 + 7)}));
    }
    {
      // M()
      std::vector<pp_token> tokens{
        pp_token{pp_token_category::op_or_punc, u8")"}
      };

      auto it = std::ranges::begin(tokens);
      auto args = kusabira::PP::parse_macro_args(it, tokens.end());

      // 空の呼び出しでも空の引数がパースされる
      CHECK_EQ(args.size(), 1);
      CHECK_UNARY(std::ranges::empty(args[0]));
    }
    {
      // M(   )
      std::vector<pp_token> tokens{
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::op_or_punc, u8")"}
      };

      auto it = std::ranges::begin(tokens);
      auto args = kusabira::PP::parse_macro_args(it, tokens.end());

      // 空の呼び出しでも空の引数がパースされる
      CHECK_EQ(args.size(), 1);
      CHECK_UNARY(std::ranges::empty(args[0]));
    }
    {
      // M( , 1)
      std::vector<pp_token> tokens{
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::op_or_punc, u8","},
        pp_token{pp_token_category::pp_number, u8"1"},
        pp_token{pp_token_category::op_or_punc, u8")"}
      };

      auto it = std::ranges::begin(tokens);
      auto args = kusabira::PP::parse_macro_args(it, tokens.end());

      CHECK_EQ(args.size(), 2);
      CHECK_EQ(args[0].size(), 0);
      CHECK_EQ(args[1].size(), 1);

      it = std::ranges::begin(tokens);
      CHECK_UNARY(std::ranges::equal(args[1], std::ranges::subrange{std::ranges::next(it, 2), std::ranges::next(it, 3)}));
    }
    {
      // M(, ,,,    )
      std::vector<pp_token> tokens{
        pp_token{pp_token_category::op_or_punc, u8","},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::op_or_punc, u8","},
        pp_token{pp_token_category::op_or_punc, u8","},
        pp_token{pp_token_category::op_or_punc, u8","},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::whitespaces, u8" "},
        pp_token{pp_token_category::op_or_punc, u8")"}
      };

      auto it = std::ranges::begin(tokens);
      auto args = kusabira::PP::parse_macro_args(it, tokens.end());

      CHECK_EQ(args.size(), 5);

      for (auto& list : args) {
        CHECK_UNARY(std::ranges::empty(list));
      }
    }
  }

  TEST_CASE("object like macro test") {

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_line_directive.hpp"};

    auto pos = ll.before_begin();
    pos = ll.emplace_after(pos, 0, 0);
    (*pos).line = u8"#define N 3";

    //単純な定数定義
    {
      //PPトークン列
      std::pmr::list<pp_token> pptokens{&kusabira::def_mr};

      //字句トークン
      pp_token lt1{pp_token_category::identifier, u8"N", 8, pos};

      //PPトークン列
      pptokens.emplace_back(pp_token_category::pp_number, u8"3", 10, pos);

      //マクロ登録
      CHECK_UNARY(pp.define(*reporter, lt1, pptokens));

      //登録のチェック
      const auto is_func = pp.is_macro(lt1.token);
      REQUIRE_UNARY(bool(is_func));
      CHECK_UNARY_FALSE(*is_func);

      const auto [success, complete, list, memo] = pp.expand_objmacro(*reporter, lt1);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(list.size(), 1u);
      CHECK_EQ(pptokens, list);
    }

    pos = ll.emplace_after(pos, 1, 1);
    (*pos).line = u8"#define MACRO macro replacement test";

    //長めの置換リスト
    {
      //PPトークン列
      std::pmr::list<pp_token> pptokens{&kusabira::def_mr};

      //字句トークン
      pp_token lt1{pp_token_category::identifier, u8"MACRO", 8, pos};

      //PPトークン列
      pptokens.emplace_back(pp_token_category::identifier, u8"macro", 14, pos);
      pptokens.emplace_back(pp_token_category::whitespaces, u8" ", 19, pos);
      pptokens.emplace_back(pp_token_category::identifier, u8"replacement", 20, pos);
      pptokens.emplace_back(pp_token_category::whitespaces, u8" ", 31, pos);
      pptokens.emplace_back(pp_token_category::identifier, u8"test", 32, pos);

      CHECK_UNARY(pp.define(*reporter, lt1, pptokens));

      //登録のチェック
      const auto is_func = pp.is_macro(lt1.token);
      REQUIRE_UNARY(bool(is_func));
      CHECK_UNARY_FALSE(*is_func);

      const auto [success, complete, list, memo] = pp.expand_objmacro(*reporter, lt1);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(list.size(), 3u);

      std::erase_if(pptokens, [](const auto& token) {
        return token.category == pp_token_category::whitespaces;
      });
      CHECK_EQ(pptokens, list);
    }

    //適当なトークンを投げてみる
    CHECK_UNARY_FALSE(bool(pp.is_macro(u8"M")));
    CHECK_UNARY_FALSE(bool(pp.is_macro(u8"NN")));
    CHECK_UNARY_FALSE(bool(pp.is_macro(u8"TEST")));
    CHECK_UNARY_FALSE(bool(pp.is_macro(u8"NUMBER")));
    CHECK_UNARY_FALSE(bool(pp.is_macro(u8"MACRO_")));

    {
      pp.undef(u8"N"sv);

      //登録のチェック
      const auto is_macro = pp.is_macro(u8"N"sv);
      CHECK_UNARY_FALSE(bool(is_macro));
    }
    {
      pp.undef(u8"MACRO"sv);

      //登録のチェック
      const auto is_macro = pp.is_macro(u8"MACRO"sv);
      CHECK_UNARY_FALSE(bool(is_macro));
    }
  }

  TEST_CASE("function like macro") {

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_funcmacro.hpp"};

    auto pos = ll.before_begin();
    pos = ll.emplace_after(pos, 0, 0);
    (*pos).line = u8"#define max(a, b) ((a) > (b) ? (a) : (b))";
    pp_token lt1{pp_token_category::identifier, u8"max", 8, pos};

    {
      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //マクロ仮引数トークン列
      params.emplace_back(u8"a");
      params.emplace_back(u8"b");
      //置換リスト
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 18, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 19, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"a", 20, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 21, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 22, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8">", 23, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 24, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 25, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"b", 26, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 27, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 28, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"?", 29, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 30, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 31, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"a", 32, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 33, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 34, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8":", 35, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 36, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 37, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"b", 38, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 39, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 40, pos);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, lt1, rep_list, params, false));

      //登録のチェック
      const auto is_func = pp.is_macro(lt1.token);
      REQUIRE_UNARY(bool(is_func));
      CHECK_UNARY(*is_func);

      //実引数リスト作成
      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"max(1, 2)";

      std::pmr::list<pp_token> token1{ &kusabira::def_mr };
      token1.emplace_back(pp_token_category::pp_number, u8"1", 4, pos);
      std::pmr::list<pp_token> token2{ &kusabira::def_mr };
      token2.emplace_back(pp_token_category::pp_number, u8"2", 7, pos);

      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      args.emplace_back(std::move(token1));
      args.emplace_back(std::move(token2));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, lt1, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(rep_list.size() - 6, result.size());

      std::pmr::list<pp_token> expect{ &kusabira::def_mr };
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 18, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 19, pos);
      expect.emplace_back(pp_token_category::pp_number, u8"1", 4, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 21, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8">", 23, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 25, pos);
      expect.emplace_back(pp_token_category::pp_number, u8"2", 7, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 27, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"?", 29, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 31, pos);
      expect.emplace_back(pp_token_category::pp_number, u8"1", 4, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 33, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8":", 35, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 37, pos);
      expect.emplace_back(pp_token_category::pp_number, u8"2", 7, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 39, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 40, pos);

      //結果の比較
      CHECK_EQ(expect, result);

      //引数の数が合わないような呼び出し、失敗
      args.pop_back();
      const auto [success2, ignore1, ignore2, memo2] = pp.expand_funcmacro(*reporter, lt1, args);
      CHECK_UNARY_FALSE(success2);

      //再登録、成功する（登録は行われない）
      CHECK_UNARY(pp.define(*reporter, lt1, rep_list, params, false));
  
      //一部でも違うと失敗（'a'だけ
      params.pop_back();
      CHECK_UNARY_FALSE(pp.define(*reporter, lt1, rep_list, params, false));

      //'a, a'で登録
      params.emplace_back(u8"a");
      rep_list.pop_back();
      CHECK_UNARY_FALSE(pp.define(*reporter, lt1, rep_list, params, false));
    }

    // 引数に複数のトークン列を含む
    {
      //実引数リスト作成
      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"max(1 + 2, a + b - c)";

      std::pmr::list<pp_token> token1{&kusabira::def_mr};
      token1.emplace_back(pp_token_category::pp_number, u8"1", 0, pos);
      token1.emplace_back(pp_token_category::whitespaces, u8" ", 1, pos);
      token1.emplace_back(pp_token_category::op_or_punc, u8"+", 2, pos);
      token1.emplace_back(pp_token_category::whitespaces, u8" ", 3, pos);
      token1.emplace_back(pp_token_category::pp_number, u8"2", 4, pos);
      std::pmr::list<pp_token> token2{&kusabira::def_mr};
      token2.emplace_back(pp_token_category::identifier, u8"a", 7, pos);
      token2.emplace_back(pp_token_category::whitespaces, u8" ", 8, pos);
      token2.emplace_back(pp_token_category::op_or_punc, u8"+", 9, pos);
      token2.emplace_back(pp_token_category::whitespaces, u8" ", 10, pos);
      token2.emplace_back(pp_token_category::identifier, u8"b", 11, pos);
      token2.emplace_back(pp_token_category::whitespaces, u8" ", 12, pos);
      token2.emplace_back(pp_token_category::op_or_punc, u8"-", 13, pos);
      token2.emplace_back(pp_token_category::whitespaces, u8" ", 14, pos);
      token2.emplace_back(pp_token_category::identifier, u8"c", 15, pos);

      //実引数トークン列
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      args.emplace_back(token1);
      args.emplace_back(token2);

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, lt1, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(29ull, result.size());

      std::erase_if(token1, [](const auto& token) {
        return token.category == pp_token_category::whitespaces;
      });
      std::erase_if(token2, [](const auto& token) {
        return token.category == pp_token_category::whitespaces;
      });

      std::pmr::list<pp_token> expect{&kusabira::def_mr};
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 18, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 19, pos);
      expect.splice(expect.end(), std::pmr::list<pp_token>{token1, &kusabira::def_mr});
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 21, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8">", 23, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 25, pos);
      expect.splice(expect.end(), std::pmr::list<pp_token>{token2, &kusabira::def_mr});
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 17, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"?", 29, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 31, pos);
      expect.splice(expect.end(), token1);
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 18, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8":", 35, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 37, pos);
      expect.splice(expect.end(), token2);
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 39, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 40, pos);

      //結果の比較
      CHECK_EQ(expect, result);
    }

    //適当なトークンを投げてみる（登録されていない場合は例外を投げると思うのでやらない）
    CHECK_UNARY_FALSE(bool(pp.is_macro(u8"min")));
    CHECK_UNARY_FALSE(bool(pp.is_macro(u8"max_")));
    CHECK_UNARY_FALSE(bool(pp.is_macro(u8"MAX")));

    //登録解除
    {
      pp.undef(u8"max"sv);

      //登録のチェック
      const auto is_macro = pp.is_macro(u8"max"sv);
      CHECK_UNARY_FALSE(bool(is_macro));
    }
  }

  TEST_CASE("VA macro") {

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{ "/kusabira/test_vamacro.hpp" };

    auto pos = ll.before_begin();
    pos = ll.emplace_after(pos, 0, 0);
    (*pos).line = u8"#define F(...) f(__VA_ARGS__)";

    {
      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{ &kusabira::def_mr };
      std::pmr::list<pp_token> rep_list{ &kusabira::def_mr };

      //字句トークン
      pp_token lt2{pp_token_category::identifier, u8"F", 8, pos };

      //マクロ仮引数トークン列
      params.emplace_back(u8"...");
      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"f", 15, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 16, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 17, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 28, pos);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, lt2, rep_list, params, true));

      //登録のチェック
      const auto is_func = pp.is_macro(lt2.token);
      REQUIRE_UNARY(bool(is_func));
      CHECK_UNARY(*is_func);

      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 1, 1);
      (*pos2).line = u8R"++(F(x, 1, 0, a + b, "str"))++";

      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};

      token_list.emplace_back(pp_token_category::identifier, u8"x", 2, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::pp_number, u8"1", 5, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::pp_number, u8"0", 8, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"a", 11, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 12, pos2);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"+", 13, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 14, pos2);
      token_list.emplace_back(pp_token_category::identifier, u8"b", 15, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::string_literal, u8R"("str")", 18, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, lt2, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(14ull, result.size());

      std::pmr::list<pp_token> expect{&kusabira::def_mr};
      expect.emplace_back(pp_token_category::identifier, u8"f", 15, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 16, pos);
      expect.emplace_back(pp_token_category::identifier, u8"x", 2, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::pp_number, u8"1", 5, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::pp_number, u8"0", 8, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::identifier, u8"a", 11, pos2);
      expect.emplace_back(pp_token_category::op_or_punc, u8"+", 13, pos2);
      expect.emplace_back(pp_token_category::identifier, u8"b", 15, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::string_literal, u8R"("str")", 18, pos2);
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 28, pos);

      //結果の比較
      CHECK_EQ(expect, result);
    }
  
    //引数なし実行
    {
      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, pp_token{pp_token_category::identifier, u8"F", 8, pos}, std::pmr::vector<std::pmr::list<pp_token>>{&kusabira::def_mr});

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(3ull, result.size());

      std::pmr::list<pp_token> expect{&kusabira::def_mr};
      expect.emplace_back(pp_token_category::identifier, u8"f", 15, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 16, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 28, pos);
      //結果の比較
      CHECK_EQ(expect, result);
    }

    //登録解除
    {
      pp.undef(u8"F"sv);

      //登録のチェック
      const auto is_macro = pp.is_macro(u8"F"sv);
      CHECK_UNARY_FALSE(bool(is_macro));
    }
  }

  TEST_CASE("search_close_parenthesis ") {
    struct test_token {
      std::u8string_view token;
    };

    //普通の探索
    {
      std::vector<test_token> token_seq = {test_token{u8"X"}, test_token{u8"##"}, test_token{u8"X"}, test_token{u8" "}, test_token{u8"X"}, test_token{u8"##"}, test_token{u8"X"}, test_token{u8" "}, test_token{u8")"}, test_token{u8" "}, test_token{u8"__VA_ARGS__"}};

      auto pos = kusabira::PP::search_close_parenthesis(token_seq.begin(), token_seq.end());

      REQUIRE_UNARY(pos != token_seq.end());
      CHECK_UNARY((*pos).token == u8")"sv);
    }

    //入れ子括弧がある
    {
      std::vector<test_token> token_seq = {test_token{u8"("}, test_token{u8"X"}, test_token{u8")"}, test_token{u8" "}, test_token{u8"("}, test_token{u8"("}, test_token{u8"token"}, test_token{u8")"}, test_token{u8" "}, test_token{u8")"}, test_token{u8")"}};

      auto pos = kusabira::PP::search_close_parenthesis(token_seq.begin(), token_seq.end());

      REQUIRE_UNARY(pos != token_seq.end());
      CHECK_EQ(pos, --token_seq.end());
      CHECK_UNARY((*pos).token == u8")"sv);
    }

    //見つからない
    {
      std::vector<test_token> token_seq = {test_token{u8"("}, test_token{u8"X"}, test_token{u8")"}, test_token{u8" "}, test_token{u8"("}, test_token{u8"("}, test_token{u8"token"}, test_token{u8")"}, test_token{u8" "}, test_token{u8")"}, test_token{u8";"}};

      auto pos = kusabira::PP::search_close_parenthesis(token_seq.begin(), token_seq.end());

      CHECK_EQ(pos, token_seq.end());
    }
  }

  TEST_CASE("__VA_OPT__ test") {
    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{ "/kusabira/test_vaopt.hpp" };

    auto pos = ll.before_begin();

    //基本的なやつ
    {
      pos = ll.emplace_after(pos, 0, 0);
      (*pos).line = u8"#define F(...) f(0 __VA_OPT__(,) __VA_ARGS__)";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //字句トークン
      pp_token lt2{pp_token_category::identifier, u8"F", 8, pos};
      pp_token lt4{pp_token_category::op_or_punc, u8"...", 10, pos};

      //マクロ仮引数トークン列
      params.emplace_back(lt4.token);
      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"f", 15, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 16, pos);
      rep_list.emplace_back(pp_token_category::pp_number, u8"0", 17, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ",  18, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_OPT__", 19, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 29, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8",", 30, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 31, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 32, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 33, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 44, pos);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, lt2, rep_list, params, true));

      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 1, 1);
      (*pos2).line = u8"F(a, b, c)";

      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"a", 2, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"b", 5, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"c", 8, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, lt2, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(10ull, result.size());

      std::pmr::list<pp_token> expect{&kusabira::def_mr};

      expect.emplace_back(pp_token_category::identifier, u8"f", 15, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 16, pos);
      expect.emplace_back(pp_token_category::pp_number, u8"0", 17, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8",", 30, pos);
      expect.emplace_back(pp_token_category::identifier, u8"a", 2, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::identifier, u8"b", 5, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::identifier, u8"c", 8, pos2);
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 28, pos);
      
      //結果の比較
      CHECK_EQ(expect, result);
    }

    //引数なし呼び出し
    {
      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, pp_token{pp_token_category::identifier, u8"F", 8, pos}, std::pmr::vector<std::pmr::list<pp_token>>{&kusabira::def_mr});

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(4ull, result.size());

      std::pmr::list<pp_token> expect{&kusabira::def_mr};
      expect.emplace_back(pp_token_category::identifier, u8"f", 15, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 16, pos);
      expect.emplace_back(pp_token_category::pp_number, u8"0", 17, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 28, pos);
      //結果の比較
      CHECK_EQ(expect, result);
    }

    //引数なし呼び出し2（F(,)みたいな呼び出し
    //ただしこれは引数ありとみなされる（clangとGCCで試した限りは
    {
      //空の引数
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      args.emplace_back(std::pmr::list<pp_token>{&kusabira::def_mr});
      args.emplace_back(std::pmr::list<pp_token>{&kusabira::def_mr});

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, pp_token{pp_token_category::identifier, u8"F", 8, pos}, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(6ull, result.size());

      std::pmr::list<pp_token> expect{&kusabira::def_mr};
      expect.emplace_back(pp_token_category::identifier, u8"f", 15, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"(", 16, pos);
      expect.emplace_back(pp_token_category::pp_number, u8"0", 17, pos);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::op_or_punc, u8")", 28, pos);
      //結果の比較
      CHECK_EQ(expect, result);
    }
  }

  TEST_CASE("__VA_OPT__ test2") {
    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{ "/kusabira/test_vaopt.hpp" };

    auto pos = ll.before_begin();

    //変な例
    {
      pos = ll.emplace_after(pos, 0, 0);
      (*pos).line = u8"#define SDEF(sname, ...) S sname __VA_OPT__(= { __VA_ARGS__ })";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //字句トークン
      pp_token macro_name{pp_token_category::identifier, u8"SDEF", 8, pos};

      //マクロ仮引数トークン列
      params.emplace_back(u8"sname");
      params.emplace_back(u8"...");
      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"S", 25, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 26, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"sname", 27, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 28, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_OPT__", 33, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 43, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"=", 44, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 45, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"{", 46, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 47, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 48, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 49, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"}", 60, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 61, pos);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, macro_name, rep_list, params, true));

      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 1, 1);
      (*pos2).line = u8"SDEF(bar, 1, 2, 3 + 4)";

      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"bar", 5, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::pp_number, u8"1", 10, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::pp_number, u8"2", 13, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::pp_number, u8"3", 16, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 17, pos2);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"+", 18, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 19, pos2);
      token_list.emplace_back(pp_token_category::pp_number, u8"4", 20, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, macro_name, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(12ull, result.size());

      std::pmr::list<pp_token> expect{&kusabira::def_mr};

      expect.emplace_back(pp_token_category::identifier, u8"S", 25, pos);
      expect.emplace_back(pp_token_category::identifier, u8"bar", 5, pos2);

      expect.emplace_back(pp_token_category::op_or_punc, u8"=", 44, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8"{", 46, pos);

      expect.emplace_back(pp_token_category::pp_number, u8"1", 10, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::pp_number, u8"2", 13, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::pp_number, u8"3", 16, pos2);
      expect.emplace_back(pp_token_category::op_or_punc, u8"+", 18, pos2);
      expect.emplace_back(pp_token_category::pp_number, u8"4", 20, pos2);

      expect.emplace_back(pp_token_category::op_or_punc, u8"}", 60, pos);
      
      //結果の比較
      CHECK_EQ(expect, result);
    }

    //引数なし呼び出し
    {
      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 1, 1);
      (*pos2).line = u8"SDEF(foo)";

      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"foo", 5, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, pp_token{pp_token_category::identifier, u8"SDEF", 8, pos}, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(2ull, result.size());

      std::pmr::list<pp_token> expect{&kusabira::def_mr};
      expect.emplace_back(pp_token_category::identifier, u8"S", 25, pos);
      expect.emplace_back(pp_token_category::identifier, u8"foo", 5, pos2);
      //結果の比較
      CHECK_EQ(expect, result);

      //SDEF(foo,) みたいな呼び出し
      {
        args.emplace_back(std::pmr::list<pp_token>{&kusabira::def_mr});

        //関数マクロ実行
        const auto [success2, complete2, result2, memo2] = pp.expand_funcmacro(*reporter, pp_token{pp_token_category::identifier, u8"SDEF", 8, pos}, args);

      CHECK_UNARY(success2);
      CHECK_UNARY(complete2);

        //引数なしだとみなされる
        CHECK_EQ(2ull, result2.size());
        CHECK_EQ(expect, result2);
      }
    }
  }

  TEST_CASE("pp_stringize test") {
    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    auto pos = ll.before_begin();
    pos = ll.emplace_after(pos, 0, 0);

    std::vector<pp_token> vec{};
    vec.emplace_back(pp_token_category::identifier, u8"test"sv, 0, pos);
    vec.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
    vec.emplace_back(pp_token_category::string_literal, u8R"(L"abcd\aaa\\ggg\"sv)"sv, 0, pos);
    vec.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
    vec.emplace_back(pp_token_category::pp_number, u8"12345ull"sv, 0, pos);

    {
      auto str_list = kusabira::PP::pp_stringize<true>(vec);

      CHECK_EQ(1ull, str_list.size());

      const auto& pptoken = str_list.front();
      CHECK_EQ(pptoken.category, pp_token_category::string_literal);
      CHECK_EQ(5ull, std::distance(pptoken.composed_tokens.begin(), pptoken.composed_tokens.end()));
      auto str = u8R"**("test, L\"abcd\\aaa\\\\ggg\\\"sv, 12345ull")**"sv;
      CHECK_UNARY(pptoken.token == str);
    }
    vec.clear();

    //空の入力
    {
      auto empty_str_list = kusabira::PP::pp_stringize<true>(vec);
      CHECK_EQ(1ull, empty_str_list.size());

      const auto& emptystr_token = empty_str_list.front();
      CHECK_EQ(emptystr_token.category, pp_token_category::string_literal);
      CHECK_EQ(0ull, std::distance(emptystr_token.composed_tokens.begin(), emptystr_token.composed_tokens.end()));
      CHECK_UNARY(emptystr_token.token == u8R"("")"sv);
    }

    vec.clear();

    // 実質空の入力
    vec.emplace_back(pp_token_category::whitespaces, u8" "sv, 0, pos);
    vec.emplace_back(pp_token_category::placemarker_token, u8""sv, 0, pos);
    vec.emplace_back(pp_token_category::placemarker_token, u8""sv, 0, pos);
    vec.emplace_back(pp_token_category::whitespaces, u8" "sv, 0, pos);
    vec.emplace_back(pp_token_category::placemarker_token, u8""sv, 0, pos);
    {
      auto empty_str_list = kusabira::PP::pp_stringize<true>(vec);
      CHECK_EQ(1ull, empty_str_list.size());

      const auto& emptystr_token = empty_str_list.front();
      CHECK_EQ(emptystr_token.category, pp_token_category::string_literal);
      CHECK_EQ(0ull, std::distance(emptystr_token.composed_tokens.begin(), emptystr_token.composed_tokens.end()));
      CHECK_UNARY(emptystr_token.token == u8R"("")"sv);
    }

    vec.clear();

    // ホワイトスペースとプレイスメーカートークンに囲まれた入力
    vec.emplace_back(pp_token_category::whitespaces, u8" "sv, 0, pos);
    vec.emplace_back(pp_token_category::whitespaces, u8" "sv, 0, pos);
    vec.emplace_back(pp_token_category::placemarker_token, u8""sv, 0, pos);
    vec.emplace_back(pp_token_category::placemarker_token, u8""sv, 0, pos);
    vec.emplace_back(pp_token_category::identifier, u8"test"sv, 0, pos);
    vec.emplace_back(pp_token_category::placemarker_token, u8""sv, 0, pos);
    vec.emplace_back(pp_token_category::placemarker_token, u8""sv, 0, pos);
    vec.emplace_back(pp_token_category::whitespaces, u8" "sv, 0, pos);
    vec.emplace_back(pp_token_category::whitespaces, u8" "sv, 0, pos);
    vec.emplace_back(pp_token_category::placemarker_token, u8""sv, 0, pos);
    {
      auto str_list = kusabira::PP::pp_stringize<true>(vec);
      CHECK_EQ(1ull, str_list.size());

      const auto& emptystr_token = str_list.front();
      CHECK_EQ(emptystr_token.category, pp_token_category::string_literal);
      CHECK_EQ(1ull, std::distance(emptystr_token.composed_tokens.begin(), emptystr_token.composed_tokens.end()));
      CHECK_UNARY(emptystr_token.token == u8R"("test")"sv);
    }
  }

  TEST_CASE("# operator test") {

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_sharp.hpp"};

    auto pos = ll.before_begin();

    //基本的な文字列化
    {
      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8R"(#define to_str(s) # s "_append" ;)";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //字句トークン
      pp_token lt2{pp_token_category::identifier, u8"to_str", 8, pos};
      pp_token lt6{pp_token_category::identifier, u8"s", 15, pos};

      //マクロ仮引数トークン列
      params.emplace_back(lt6.token);

      //置換リスト
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"#", 18, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 19, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"s", 20, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 21, pos);
      rep_list.emplace_back(pp_token_category::string_literal, u8R"("_append")", 22, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 31, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8";", 32, pos);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, lt2, rep_list, params, false));

      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 1, 1);
      (*pos2).line = u8R"++(to_str(strncmp("abc\0d", "abc", '\4') == 0))++";

      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"strncmp", 7, pos2);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 14, pos2);
      token_list.emplace_back(pp_token_category::string_literal, u8R"("abc\0d")", 15, pos2);
      token_list.emplace_back(pp_token_category::op_or_punc, u8",", 23, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 24, pos2);
      token_list.emplace_back(pp_token_category::string_literal, u8R"("abc")", 25, pos2);
      token_list.emplace_back(pp_token_category::op_or_punc, u8",", 30, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 31, pos2);
      token_list.emplace_back(pp_token_category::charcter_literal, u8R"('\4')", 32, pos2);
      token_list.emplace_back(pp_token_category::op_or_punc, u8")", 36, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 37, pos2);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"==", 38, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 40, pos2);
      token_list.emplace_back(pp_token_category::pp_number, u8"0", 41, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, lt2, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(3u, result.size());

      std::pmr::list<pp_token> expect{&kusabira::def_mr};
      expect.emplace_back(pp_token_category::string_literal, u8R"++("strncmp(\"abc\\0d\", \"abc\", '\\4') == 0")++", 0, pos2);
      expect.emplace_back(pp_token_category::string_literal, u8R"("_append")", 22, pos);
      expect.emplace_back(pp_token_category::op_or_punc, u8";", 32, pos);

      CHECK_EQ(expect, result);
    }

    //可変長文字列化
    {
      pos = ll.emplace_after(pos, 2, 2);
      (*pos).line = u8"#define str(...) #__VA_ARGS__";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //字句トークン
      pp_token lt2{pp_token_category::identifier, u8"str", 8, pos};
      pp_token lt6{pp_token_category::op_or_punc, u8"...", 12, pos};

      //マクロ仮引数トークン列
      params.emplace_back(lt6.token);

      //置換リスト
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"#", 17, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 18, pos);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, lt2, rep_list, params, true));

      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 1, 1);
      (*pos2).line = u8R"++(str(1, "x", int, a * b+0))++";

      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::pp_number, u8"1", 4, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::string_literal, u8R"("x")", 7, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"int", 12, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"a", 17, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 18, pos2);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"*", 19, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 20, pos2);
      token_list.emplace_back(pp_token_category::identifier, u8"b", 21, pos2);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"+", 22, pos2);
      token_list.emplace_back(pp_token_category::pp_number, u8"0", 23, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, lt2, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(1u, result.size());

      const auto& pptoken = result.front();
      CHECK_EQ(pptoken.category, pp_token_category::string_literal);
      auto tmpstr = u8R"("1, \"x\", int, a * b+0")"sv;
      CHECK_UNARY(pptoken.token == tmpstr);

      //空で呼ぶ例
      auto pos3 = ll.emplace_after(pos2, 1, 1);
      (*pos3).line = u8R"(str())";
      args.clear();

      //関数マクロ実行
      const auto [success2, complete2, result2, memo2] = pp.expand_funcmacro(*reporter, lt2, args);

      CHECK_UNARY(success2);
      CHECK_UNARY(complete2);
      CHECK_EQ(1u, result2.size());

      const auto &empty_token = result2.front();
      CHECK_EQ(empty_token.category, pp_token_category::string_literal);
      CHECK_UNARY(empty_token.token == u8R"("")"sv);
    }
    {
      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 2, 2);
      (*pos2).line = u8"str(The first, second, and third items.)";

      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"The", 4, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 18, pos2);
      token_list.emplace_back(pp_token_category::identifier, u8"first", 8, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"second", 15, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"and", 23, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 18, pos2);
      token_list.emplace_back(pp_token_category::identifier, u8"third", 27, pos2);
      token_list.emplace_back(pp_token_category::whitespaces, u8" ", 18, pos2);
      token_list.emplace_back(pp_token_category::identifier, u8"items.", 33, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"str", 8, pos}, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(1u, result.size());

      const auto &pptoken = result.front();
      CHECK_EQ(pptoken.category, pp_token_category::string_literal);
      CHECK_UNARY(pptoken.token == u8R"("The first, second, and third items.")"sv);
    }

    //#__VA_OPT__の処理
    {
      pos = ll.emplace_after(pos, 3, 3);
      (*pos).line = u8"#define str2(...) #__VA_OPT__(__VA_ARGS__)";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //字句トークン
      pp_token macro_name{pp_token_category::identifier, u8"str2", 8, pos};
      pp_token lt6{pp_token_category::op_or_punc, u8"...", 12, pos};

      //マクロ仮引数トークン列
      params.emplace_back(lt6.token);

      //置換リスト
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"#", 17, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_OPT__", 18, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 28, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 29, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 40, pos);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, macro_name, rep_list, params, true));

      {
        //実引数リスト作成
        auto pos2 = ll.emplace_after(pos, 2, 2);
        (*pos2).line = u8"str2(, 0, a, b)";

        std::pmr::list<pp_token> token_list{&kusabira::def_mr};
        std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
        args.emplace_back(std::move(token_list));
        token_list.emplace_back(pp_token_category::pp_number, u8"0", 7, pos2);
        args.emplace_back(std::move(token_list));
        token_list.emplace_back(pp_token_category::identifier, u8"a", 10, pos2);
        args.emplace_back(std::move(token_list));
        token_list.emplace_back(pp_token_category::identifier, u8"b", 13, pos2);
        args.emplace_back(std::move(token_list));

        //関数マクロ実行
        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, macro_name, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1u, result.size());

        const auto &pptoken = result.front();
        CHECK_EQ(pptoken.category, pp_token_category::string_literal);
        CHECK_UNARY(pptoken.token == u8R"(",0,a,b")"sv);
      }
      // ホワイトスペースを含んでいる場合
      {
        //実引数リスト作成
        auto pos2 = ll.emplace_after(pos, 2, 2);
        (*pos2).line = u8"str2(, 0, a, b)";

        std::pmr::list<pp_token> token_list{&kusabira::def_mr};
        std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
        args.emplace_back(std::move(token_list));
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 6, pos2);
        token_list.emplace_back(pp_token_category::pp_number, u8"0", 7, pos2);
        args.emplace_back(std::move(token_list));
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 9, pos2);
        token_list.emplace_back(pp_token_category::identifier, u8"a", 10, pos2);
        args.emplace_back(std::move(token_list));
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 10, pos2);
        token_list.emplace_back(pp_token_category::identifier, u8"b", 13, pos2);
        args.emplace_back(std::move(token_list));

        //関数マクロ実行
        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, macro_name, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1u, result.size());

        const auto &pptoken = result.front();
        CHECK_EQ(pptoken.category, pp_token_category::string_literal);
        CHECK_UNARY(pptoken.token == u8R"(", 0, a, b")"sv);
      }
    }

    //#__VA_OPT__の再帰（登録失敗）
    {
      pos = ll.emplace_after(pos, 3, 3);
      (*pos).line = u8"#define R(...) __VA_OPT__(__VA_OPT__())";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{ &kusabira::def_mr };
      std::pmr::list<pp_token> rep_list{ &kusabira::def_mr };

      //字句トークン
      pp_token lt2{pp_token_category::identifier, u8"R", 8, pos };
      pp_token lt4{pp_token_category::op_or_punc, u8"...", 10, pos };

      //マクロ仮引数トークン列
      params.emplace_back(lt4.token);

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_OPT__", 15, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 25, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_OPT__", 26, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 36, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 37, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 38, pos);

      //エラー出力をクリア
      report::test_out::extract_string();

      //関数マクロ登録、失敗する
      CHECK_UNARY_FALSE(pp.define(*reporter, lt2, rep_list, params, true));

      auto expect_err_str = u8R"(test_sharp.hpp:3:26: error: __VA_OPT__は再帰してはいけません。
#define R(...) __VA_OPT__(__VA_OPT__())
)"sv;
      auto err_str = report::test_out::extract_string();

      CHECK_UNARY(expect_err_str == err_str);
    }
  }

  TEST_CASE("## operator test") {

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_sharp2.hpp"};
    auto pos = ll.before_begin();

    //普通の関数マクロ
    {
      pos = ll.emplace_after(pos, 0, 0);
      (*pos).line = u8"#define glue(a, b) pre ## a ## b";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{ &kusabira::def_mr };
      std::pmr::list<pp_token> rep_list{ &kusabira::def_mr };

      //字句トークン
      pp_token lt2{pp_token_category::identifier, u8"glue", 8, pos };
      pp_token lt4{pp_token_category::identifier, u8"a", 13, pos };
      pp_token lt6{pp_token_category::identifier, u8"b", 16, pos };

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"pre", 19, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 20, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 23, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 24, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"a", 26, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 24, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 29, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 30, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"b", 31, pos);
      //マクロ仮引数トークン列
      params.emplace_back(lt4.token);
      params.emplace_back(lt6.token);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, lt2, rep_list, params, false));

      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 1, 1);
      (*pos2).line = u8"glue(proce, ssing)";

      std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
      std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
      token_list.emplace_back(pp_token_category::identifier, u8"proce", 5, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"ssing", 12, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, lt2, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(1u, result.size());

      auto& token = result.front();

      CHECK_UNARY(token.token == u8"preprocessing"sv);
      CHECK_EQ(pp_token_category::identifier, token.category);

      //空で呼び出し
      auto pos3 = ll.emplace_after(pos2, 1, 1);
      (*pos3).line = u8R"(glue(,))";
      args.clear();
      args.emplace_back(std::pmr::list<pp_token>{ &kusabira::def_mr });
      args.emplace_back(std::pmr::list<pp_token>{ &kusabira::def_mr });

      //関数マクロ実行
      const auto [success2, complete2, result2, memo2] = pp.expand_funcmacro(*reporter, lt2, args);

      CHECK_UNARY(success2);
      CHECK_UNARY(complete2);
      CHECK_EQ(1u, result2.size());
      CHECK_UNARY(result2.front().token == u8"pre"sv);
    }

    //普通の関数マクロ
    {
      pos = ll.emplace_after(pos, 0, 0);
      (*pos).line = u8"#define debug(s, t) printf(x ## s, x ## t)";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{ &kusabira::def_mr };
      std::pmr::list<pp_token> rep_list{ &kusabira::def_mr };

      //マクロ名
      pp_token macro_name{pp_token_category::identifier, u8"debug", 8, pos};
      //マクロ仮引数トークン列
      params.emplace_back(u8"s");
      params.emplace_back(u8"t");

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"printf", 20, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 26, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"x", 27, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 28, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 29, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 28, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"s", 32, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8",", 33, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 28, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"x", 35, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 28, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 37, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 38, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"t", 40, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 41, pos);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, macro_name, rep_list, params, false));

      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 1, 1);
      (*pos2).line = u8"debug(yz1, wv2)";

      std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
      std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
      token_list.emplace_back(pp_token_category::identifier, u8"yz1", 6, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"wv2", 11, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, macro_name, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(6u, result.size());

      std::pmr::list<pp_token> expect = {
        pp_token{pp_token_category::identifier, u8"printf"},
        pp_token{pp_token_category::op_or_punc, u8"("},
        pp_token{pp_token_category::identifier, u8"xyz1"},
        pp_token{pp_token_category::op_or_punc, u8","},
        pp_token{pp_token_category::identifier, u8"xwv2"},
        pp_token{pp_token_category::op_or_punc, u8")"}
      };

      CHECK_EQ(expect, result);
    }

    //可変長マクロの例
    {
      pos = ll.emplace_after(pos, 0, 0);
      (*pos).line = u8"#define F(X, ...) X ## __VA_ARGS__ ## X";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //字句トークン
      pp_token lt2{pp_token_category::identifier, u8"F", 8, pos};
      pp_token lt4{pp_token_category::identifier, u8"X", 13, pos};
      pp_token lt6{pp_token_category::op_or_punc, u8"...", 16, pos};

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"X", 19, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 20, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 23, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 25, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 26, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 37, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 38, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 40, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"X", 41, pos);
      //マクロ仮引数トークン列
      params.emplace_back(lt4.token);
      params.emplace_back(lt6.token);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, lt2, rep_list, params, true));

      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 1, 1);
      (*pos2).line = u8"F(a, b, cd, e, fgh)";

      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"a", 2, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"b", 5, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"cd", 8, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"e", 12, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"fgh", 15, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, lt2, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(7u, result.size());

      std::pmr::list<pp_token> expect{&kusabira::def_mr};

      expect.emplace_back(pp_token_category::identifier, u8"ab", 2, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::identifier, u8"cd", 8, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::identifier, u8"e", 12, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::identifier, u8"fgha", 15, pos2);

      CHECK_EQ(expect, result);

      //空で呼ぶ
      args.clear();
      token_list.emplace_back(pp_token_category::identifier, u8"a", 2, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success2, complete2, result2, memo2] = pp.expand_funcmacro(*reporter, lt2, args);

      CHECK_UNARY(success2);
      CHECK_UNARY(complete2);
      CHECK_EQ(1u, result2.size());

      auto &result_token = result2.front();
      CHECK_EQ(pp_token_category::identifier, result_token.category);
      CHECK_UNARY(result_token.token == u8"aa"sv);
    }

    //VA_OPTの例
    {
      pos = ll.emplace_after(pos, 0, 0);
      (*pos).line = u8"#define G(X, ...) X ## __VA_OPT__(bcd) ## X";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //字句トークン
      pp_token macro_name{pp_token_category::identifier, u8"G", 8, pos};

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"X", 18, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 19, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 20, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 22, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_OPT__", 23, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 33, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"bcd", 34, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 37, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 38, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 39, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 41, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"X", 42, pos);
      //マクロ仮引数トークン列
      params.emplace_back(u8"X");
      params.emplace_back(u8"...");

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, macro_name, rep_list, params, true));

      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 1, 1);
      (*pos2).line = u8"G(a, b, cd, e, fgh)";

      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"a", 2, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"b", 5, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"cd", 8, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"e", 12, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"fgh", 15, pos2);
      args.emplace_back(std::move(token_list));

      {
        //関数マクロ実行
        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, macro_name, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1u, result.size());

        auto& result_token = result.front();
        CHECK_EQ(pp_token_category::identifier, result_token.category);
        CHECK_UNARY(result_token.token == u8"abcda"sv);
      }

      //空で呼ぶ
      {
        args.clear();
        token_list.emplace_back(pp_token_category::identifier, u8"a", 2, pos2);
        args.emplace_back(std::move(token_list));

        //関数マクロ実行
        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, macro_name, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1u, result.size());

        auto &result_token = result.front();
        CHECK_EQ(pp_token_category::identifier, result_token.category);
        CHECK_UNARY(result_token.token == u8"aa"sv);
      }
      
    }

    //変な例1
    {
      pos = ll.emplace_after(pos, 10, 10);
      (*pos).line = u8"#define hash_hash # ## #";

      //仮引数列と置換リスト
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //字句トークン
      pp_token lt2{pp_token_category::identifier, u8"hash_hash", 8, pos};

      //置換リスト
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"#", 18, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 19, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 20, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 22, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"#", 23, pos);

      //オブジェクトマクロ登録
      CHECK_UNARY(pp.define(*reporter, lt2, rep_list));

      //オブジェクトマクロ実行
      const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, lt2);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(1u, result.size());

      pp_token expect_token{pp_token_category::op_or_punc, u8"##", 18, pos};
      CHECK_EQ(expect_token, result.front());
    }

    //変な例2
    {
      pos = ll.emplace_after(pos, 10, 10);
      (*pos).line = u8"#define H(b) R ## b ## sv";

      //仮引数列と置換リスト
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};

      //字句トークン
      pp_token lt2{pp_token_category::identifier, u8"H", 8, pos};
      pp_token lt5{pp_token_category::identifier, u8"b", 10, pos};

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"R", 13, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 14, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 15, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 17, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"b", 18, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 19, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 20, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 22, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"sv", 23, pos);

      //マクロ仮引数トークン列
      params.emplace_back(lt5.token);

      //関数登録
      CHECK_UNARY(pp.define(*reporter, lt2, rep_list, params, false));

      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 1, 1);
      (*pos2).line = u8R"+(H("(string literal\n)"))+";

      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::string_literal, u8R"+("(string literal\n)")+", 2, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, lt2, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(1u, result.size());

      auto& result_token = result.front();
      CHECK_EQ(pp_token_category::user_defined_raw_string_literal, result_token.category);
      CHECK_UNARY(result_token.token == u8R"++(R"(string literal\n)"sv)++"sv);
    }

    // __VA_OPT__の中にある例
    {
      pos = ll.emplace_after(pos, 15, 15);
      (*pos).line = u8"#define H2(X, Y, ...) __VA_OPT__(X ## Y,) __VA_ARGS__";

      //仮引数列と置換リスト
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};

      //字句トークン
      pp_token macro_name{pp_token_category::identifier, u8"H2", 8, pos};

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_OPT__", 22, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 32, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"X", 33, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 34, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 35, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 37, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"Y", 38, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8",", 39, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 40, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 41, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 42, pos);

      //マクロ仮引数トークン列
      params.emplace_back(u8"X"sv);
      params.emplace_back(u8"Y"sv);
      params.emplace_back(u8"..."sv);

      //関数登録
      CHECK_UNARY(pp.define(*reporter, macro_name, rep_list, params, true));

      //実引数リスト作成
      auto pos2 = ll.emplace_after(pos, 16, 16);
      (*pos2).line = u8"H2(a, b, c, d)";

      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"a", 3, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"b", 6, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"c", 9, pos2);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"d", 12, pos2);
      args.emplace_back(std::move(token_list));

      //関数マクロ実行
      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, macro_name, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(5u, result.size());

      std::pmr::list<pp_token> expect{&kusabira::def_mr};

      expect.emplace_back(pp_token_category::identifier, u8"ab", 2, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::identifier, u8"c", 9, pos2);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::identifier, u8"d", 12, pos2);

      CHECK_EQ(expect, result);
    }

    //エラー
    {
      pos = ll.emplace_after(pos, 11, 11);
      (*pos).line = u8"#define hash_hash2(a) # ## # a";

      //仮引数列と置換リスト
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};

      //字句トークン
      pp_token lt2{pp_token_category::identifier, u8"hash_hash2", 8, pos};
      pp_token lt4{pp_token_category::identifier, u8"a", 19, pos};

      //置換リスト
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"#", 22, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 23, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 24, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 26, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"#", 27, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 28, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"a", 29, pos);

      //マクロ仮引数トークン列
      params.emplace_back(lt4.token);

      //エラー出力をクリア
      report::test_out::extract_string();

      //関数マクロ登録、失敗
      CHECK_UNARY_FALSE(pp.define(*reporter, lt2, rep_list, params, true));

      auto expect_err_str = u8R"(test_sharp2.hpp:11:24: error: #トークンは仮引数名の前だけに現れなければなりません。
#define hash_hash2(a) # ## # a
)"sv;
      auto err_str = report::test_out::extract_string();

      CHECK_UNARY(expect_err_str == err_str);
    }

    //エラー
    {
      pos = ll.emplace_after(pos, 12, 12);
      (*pos).line = u8"#define ERR_VAOPT(X, ...) X __VA_OPT__(##) __VA_ARGS__";

      //仮引数列と置換リスト
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};

      //字句トークン
      pp_token lt2{pp_token_category::identifier, u8"ERR_VAOPT", 8, pos};
      pp_token lt4{pp_token_category::identifier, u8"X", 18, pos};
      pp_token lt6{pp_token_category::op_or_punc, u8"...", 21, pos};

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"X", 26, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 27, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_OPT__", 28, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 38, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 39, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 41, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 42, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 43, pos);

      //マクロ仮引数トークン列
      params.emplace_back(lt4.token);
      params.emplace_back(lt6.token);

      //エラー出力をクリア
      report::test_out::extract_string();

      //関数マクロ登録、失敗
      CHECK_UNARY_FALSE(pp.define(*reporter, lt2, rep_list, params, true));

      auto expect_err_str = u8R"(test_sharp2.hpp:12:39: error: ##トークンは置換リストの内部に現れなければなりません。
#define ERR_VAOPT(X, ...) X __VA_OPT__(##) __VA_ARGS__
)"sv;
      auto err_str = report::test_out::extract_string();

      CHECK_UNARY(expect_err_str == err_str);
    }
  }

  TEST_CASE("macro replacement in argument test") {
    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_further_replacement1.hpp"};
    auto pos = ll.before_begin();

    {

      pos = ll.emplace_after(pos, 0, 0);
      (*pos).line = u8"#define F(a, b, c, d) a #b ## c d";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //字句トークン
      pp_token lt1{ pp_token_category::identifier, u8"a", 10, pos };
      pp_token lt2{ pp_token_category::identifier, u8"b", 13, pos };
      pp_token lt3{ pp_token_category::identifier, u8"c", 16, pos };
      pp_token lt4{ pp_token_category::identifier, u8"d", 19, pos };

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"a", 22, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 23, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"#", 24, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"b", 25, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 26, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 27, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 29, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"c", 30, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 31, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"d", 32, pos);
      //マクロ仮引数トークン列
      params.emplace_back(lt1.token);
      params.emplace_back(lt2.token);
      params.emplace_back(lt3.token);
      params.emplace_back(lt4.token);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, { pp_token_category::identifier, u8"F", 8, pos }, rep_list, params, false));

      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"#define G(a) a";
      rep_list.clear();
      params.clear();

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"a", 13, pos);
      //マクロ仮引数トークン列
      params.emplace_back(u8"a"sv);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, { pp_token_category::identifier, u8"G", 8, pos }, rep_list, params, false));

      pos = ll.emplace_after(pos, 2, 2);
      (*pos).line = u8"#define H test";
      rep_list.clear();
      params.clear();

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"test", 10, pos);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, { pp_token_category::identifier, u8"H", 8, pos }, rep_list));

      pos = ll.emplace_after(pos, 3, 3);
      (*pos).line = u8"F(G(u8), G(a), H, H)";

      //実引数リスト
      std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
      std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
      token_list.emplace_back(pp_token_category::identifier, u8"G", 2, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 3, pos);
      token_list.emplace_back(pp_token_category::identifier, u8"u8", 4, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8")", 6, pos);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"G", 9, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 10, pos);
      token_list.emplace_back(pp_token_category::identifier, u8"a", 11, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8")", 12, pos);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"H", 15, pos);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::identifier, u8"H", 18, pos);
      args.emplace_back(std::move(token_list));

      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"F", 0, pos }, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);

      std::pmr::list<pp_token> expect{ &kusabira::def_mr };

      //結果は「u8 "G(a)"H test」となる
      expect.emplace_back(pp_token_category::identifier, u8"u8", 4, pos);
      expect.emplace_back(pp_token_category::user_defined_string_literal , u8R"+("G(a)"H)+", 9, pos);
      expect.emplace_back(pp_token_category::identifier, u8"test", 18, pos);

      CHECK_EQ(expect, result);
    }
    
    {
      pos = ll.emplace_after(pos, 5, 5);
      (*pos).line = u8"#define V(...) __VA_ARGS__ vatest";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{ &kusabira::def_mr };
      std::pmr::list<pp_token> rep_list{ &kusabira::def_mr };

      //字句トークン
      pp_token lt1{ pp_token_category::identifier, u8"...", 10, pos };

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 15, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 16, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"vatest", 27, pos);
      //マクロ仮引数トークン列
      params.emplace_back(lt1.token);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, { pp_token_category::identifier, u8"V", 8, pos }, rep_list, params, true));

      pos = ll.emplace_after(pos, 3, 3);
      (*pos).line = u8"G(V(a, b, cde, f, gh))";

      //実引数リスト
      std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
      std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
      token_list.emplace_back(pp_token_category::identifier, u8"V", 2, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 3, pos);
      token_list.emplace_back(pp_token_category::identifier, u8"a", 4, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8",", 5, pos);
      token_list.emplace_back(pp_token_category::identifier, u8"b", 7, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8",", 8, pos);
      token_list.emplace_back(pp_token_category::identifier, u8"cde", 10, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8",", 13, pos);
      token_list.emplace_back(pp_token_category::identifier, u8"f", 15, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8",", 16, pos);
      token_list.emplace_back(pp_token_category::identifier, u8"gh", 18, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8")", 20, pos);
      args.emplace_back(std::move(token_list));

      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"G", 0, pos }, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);

      std::pmr::list<pp_token> expect{ &kusabira::def_mr };

      //結果は「a , b , cde , f , gh vatest」となる
      expect.emplace_back(pp_token_category::identifier, u8"a", 4, pos);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::identifier, u8"b", 7, pos);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::identifier, u8"cde", 10, pos);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::identifier, u8"f", 15, pos);
      expect.emplace_back(pp_token_category::op_or_punc).token = u8","sv;
      expect.emplace_back(pp_token_category::identifier, u8"gh", 18, pos);
      expect.emplace_back(pp_token_category::identifier, u8"vatest");

      CHECK_EQ(expect, result);
    }
  }

  TEST_CASE("complex macro test1") {
    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_complex1.hpp"};
    auto pos = ll.before_begin();

    //1個め登録
    {
      pos = ll.emplace_after(pos, 0, 0);
      (*pos).line = u8"#define LPAREN() (";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //置換リスト、引数なし
      rep_list.emplace_back(pp_token_category::identifier, u8"(", 17, pos);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"LPAREN", 8, pos}, rep_list, params, false));
    }

    //2個め登録
    {
      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"#define G(Q) 42";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //仮引数
      params.emplace_back(u8"Q"sv);

      //置換リスト
      rep_list.emplace_back(pp_token_category::pp_number, u8"42", 13, pos);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"G", 8, pos}, rep_list, params, false));
    }

    //3個め登録
    {
      pos = ll.emplace_after(pos, 2, 2);
      (*pos).line = u8"#define F(R, X, ...)  __VA_OPT__(G R X) )";

      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //仮引数
      params.emplace_back(u8"R"sv);
      params.emplace_back(u8"X"sv);
      params.emplace_back(u8"..."sv);

      //置換リスト
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_OPT__", 22, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 32, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"G", 33, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 34, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"R", 35, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 36, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"X", 37, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 38, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 39, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 40, pos);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"F", 8, pos}, rep_list, params, true));
    }
    {
      //呼び出し
      pos = ll.emplace_after(pos, 3, 3);
      (*pos).line = u8"F(LPAREN(), 0, <:-)";

      //実引数リスト
      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"LPAREN", 2, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 8, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8")", 9, pos);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::pp_number, u8"0", 12, pos);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::op_or_punc, u8"<:", 15, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"-", 17, pos);
      args.emplace_back(std::move(token_list));

      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"F", 0, pos}, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);

      CHECK_EQ(result.size(), 1);
      auto& result_token = result.front();
      CHECK_UNARY(result_token.token == u8"42"sv);
      CHECK_UNARY(result_token.category == pp_token_category::pp_number);
    }
    {
      //呼び出し
      pos = ll.emplace_after(pos, 4, 4);
      (*pos).line = u8"F(LPAREN(), 0)";

      //実引数リスト
      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"LPAREN", 2, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 8, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8")", 9, pos);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::pp_number, u8"0", 12, pos);
      args.emplace_back(std::move(token_list));

      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"F", 0, pos}, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);

      CHECK_EQ(result.size(), 1);
      auto& result_token = result.front();
      CHECK_UNARY(result_token.token == u8")"sv);
      CHECK_UNARY(result_token.category == pp_token_category::op_or_punc);
    }
    {
      //呼び出し
      pos = ll.emplace_after(pos, 4, 4);
      (*pos).line = u8"F(LPAREN(), 0,)";  // これは可変引数なし呼び出し

      //実引数リスト
      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"LPAREN", 2, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 8, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8")", 9, pos);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::pp_number, u8"0", 12, pos);
      args.emplace_back(std::move(token_list));
      args.emplace_back(std::move(token_list));

      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"F", 0, pos}, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);

      CHECK_EQ(result.size(), 1);
      auto& result_token = result.front();
      CHECK_UNARY(result_token.token == u8")"sv);
      CHECK_UNARY(result_token.category == pp_token_category::op_or_punc);
    }
    {
      //呼び出し
      pos = ll.emplace_after(pos, 4, 4);
      (*pos).line = u8"F(LPAREN(), 0,,)";  // これは可変引数あり呼び出し

      //実引数リスト
      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"LPAREN", 2, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 8, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8")", 9, pos);
      args.emplace_back(std::move(token_list));
      token_list.emplace_back(pp_token_category::pp_number, u8"0", 12, pos);
      args.emplace_back(std::move(token_list));
      args.emplace_back(std::move(token_list));
      args.emplace_back(std::move(token_list));

      const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"F", 0, pos}, args);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);

      CHECK_EQ(result.size(), 1);
      auto &result_token = result.front();
      CHECK_UNARY(result_token.token == u8"42"sv);
      CHECK_UNARY(result_token.category == pp_token_category::pp_number);
    }
  }

  TEST_CASE("inner argment macro test") {
    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_complex1.hpp"};
    auto pos = ll.before_begin();

    // 登録
    {
      pos = ll.emplace_after(pos, 0, 0);
      (*pos).line = u8"#define A 20";

      // 置換リスト
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};
      rep_list.emplace_back(pp_token_category::pp_number, u8"20", 10, pos);

      // 関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"A", 8, pos}, rep_list));

      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"#define t(a) a";

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::identifier, u8"a", 13, pos);

      // 仮引数
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      params.emplace_back(u8"a"sv);

      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"t", 8, pos}, rep_list, params, false));

      pos = ll.emplace_after(pos, 2, 2);
      (*pos).line = u8"#define F1(a1, a2) a1 a2";

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::identifier, u8"a1", 19, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 21, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"a2", 22, pos);

      // 仮引数
      params.clear();
      params.emplace_back(u8"a1"sv);
      params.emplace_back(u8"a2"sv);

      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"F1", 8, pos}, rep_list, params, false));

      pos = ll.emplace_after(pos, 3, 3);
      (*pos).line = u8"#define F2(a1, a2) #a1 # a2";

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"#", 18, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"a1", 19, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 21, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"#", 22, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 23, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"a2", 24, pos);

      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"F2", 8, pos}, rep_list, params, false));

      pos = ll.emplace_after(pos, 4, 4);
      (*pos).line = u8"#define F3(a1, a2) a1 ## a2";

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::identifier, u8"a1", 19, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 20, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 21, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 22, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"a2", 23, pos);

      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"F3", 8, pos}, rep_list, params, false));

      pos = ll.emplace_after(pos, 5, 5);
      (*pos).line = u8"#define F4(...) __VA_ARGS__";

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 16, pos);

      // 仮引数
      params.clear();
      params.emplace_back(u8"..."sv);

      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"F4", 8, pos}, rep_list, params, true));

      pos = ll.emplace_after(pos, 6, 6);
      (*pos).line = u8"#define F5(...) # __VA_ARGS__";

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"#", 16, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 17, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 18, pos);

      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"F5", 8, pos}, rep_list, params, true));

      pos = ll.emplace_after(pos, 7, 7);
      (*pos).line = u8"#define F6(...) __VA_OPT__(__VA_ARGS__)";

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_OPT__", 16, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 26, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 27, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 38, pos);

      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"F6", 8, pos}, rep_list, params, true));

      pos = ll.emplace_after(pos, 8, 8);
      (*pos).line = u8"#define F7(...) __VA_OPT__(# __VA_ARGS__)";

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_OPT__", 16, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 26, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"#", 27, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 28, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 29, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 40, pos);

      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"F7", 8, pos}, rep_list, params, true));

      pos = ll.emplace_after(pos, 9, 9);
      (*pos).line = u8"#define F8(...) __VA_OPT__(__VA_ARGS__) ## __VA_ARGS__";

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_OPT__", 16, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 26, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 27, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 38, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 39, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"##", 40, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 41, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"__VA_ARGS__", 42, pos);

      CHECK_UNARY(pp.define(*reporter, {pp_token_category::identifier, u8"F8", 8, pos}, rep_list, params, true));

      pos = ll.emplace_after(pos, 10, 10);
      (*pos).line = u8"#define g t";

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::identifier, u8"t", 10, pos);

      // オブジェクトマクロ登録
      CHECK_UNARY(pp.define(*reporter, { pp_token_category::identifier, u8"g", 8, pos }, rep_list));

      pos = ll.emplace_after(pos, 11, 11);
      (*pos).line = u8"#define fn(a) fn(2 * (a))";

      // 仮引数
      params.clear();
      params.emplace_back(u8"a"sv);

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::identifier, u8"fn", 8, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 10, pos);
      rep_list.emplace_back(pp_token_category::pp_number, u8"2", 11, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 12, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"*", 13, pos);
      rep_list.emplace_back(pp_token_category::whitespaces, u8" ", 14, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"(", 15, pos);
      rep_list.emplace_back(pp_token_category::identifier, u8"a", 16, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 17, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8")", 18, pos);

      // 関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, { pp_token_category::identifier, u8"fn", 8, pos }, rep_list, params, true));

      pos = ll.emplace_after(pos, 10, 10);
      (*pos).line = u8"#define g2 fn";

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::identifier, u8"fn", 10, pos);

      // オブジェクトマクロ登録
      CHECK_UNARY(pp.define(*reporter, { pp_token_category::identifier, u8"g2", 8, pos }, rep_list));

      pos = ll.emplace_after(pos, 10, 10);
      (*pos).line = u8"#define z z[0]";

      // 置換リスト
      rep_list.clear();
      rep_list.emplace_back(pp_token_category::identifier, u8"z", 10, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"[", 11, pos);
      rep_list.emplace_back(pp_token_category::pp_number, u8"0", 12, pos);
      rep_list.emplace_back(pp_token_category::op_or_punc, u8"]", 13, pos);

      // オブジェクトマクロ登録
      CHECK_UNARY(pp.define(*reporter, { pp_token_category::identifier, u8"z", 8, pos }, rep_list));
    }

    // 実行
    {
      pos = ll.emplace_after(pos, 17, 17);
      (*pos).line = u8"F1(A, t(0))";

      // 実引数1つのリストと実引数全体のリスト
      std::pmr::list<pp_token> token_list{&kusabira::def_mr};
      std::pmr::vector<std::pmr::list<pp_token>> args{&kusabira::def_mr};
      token_list.emplace_back(pp_token_category::identifier, u8"A", 3, pos);
      args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));
      token_list.emplace_back(pp_token_category::identifier, u8"t", 6, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 7, pos);
      token_list.emplace_back(pp_token_category::pp_number, u8"0", 8, pos);
      token_list.emplace_back(pp_token_category::op_or_punc, u8")", 9, pos);
      args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

      // F1の実行（単純な置換のみ）
      {
        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"F1", 0, pos}, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 2);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{pp_token_category::pp_number, u8"20"}, pp_token{pp_token_category::pp_number, u8"0"}});
      }

      // F2の実行（文字列化）
      pos = ll.emplace_after(pos, 8, 8);
      (*pos).line = u8"F2(A, t(0))";

      {
        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"F2", 0, pos}, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 2);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{pp_token_category::string_literal, u8R"++("A")++"}, pp_token{pp_token_category::string_literal, u8R"++("t(0)")++"}});
      }

      // F3の実行（トークン結合）
      pos = ll.emplace_after(pos, 8, 8);
      (*pos).line = u8"F3(A, t(0))";

      {
        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"F3", 0, pos}, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 4);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{pp_token_category::identifier, u8"At"}, pp_token{pp_token_category::op_or_punc, u8"("}, pp_token{pp_token_category::pp_number, u8"0"}, pp_token{pp_token_category::op_or_punc, u8")"}});
      }

      // F4の実行（可変引数マクロ）
      pos = ll.emplace_after(pos, 8, 8);
      (*pos).line = u8"F4(A, t(0))";

      {
        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"F4", 0, pos}, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 3);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{pp_token_category::pp_number, u8"20"}, pp_token{pp_token_category::op_or_punc, u8","}, pp_token{pp_token_category::pp_number, u8"0"}});
      }

      // F5の実行（可変引数マクロの文字列化）
      pos = ll.emplace_after(pos, 8, 8);
      (*pos).line = u8"F5(A, t(0))";

      {
        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"F5", 0, pos}, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 1);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{pp_token_category::string_literal, u8R"++("A, t(0)")++"}});
      }

      // F6の実行（VA_OPT内のVA_ARGS）
      pos = ll.emplace_after(pos, 8, 8);
      (*pos).line = u8"F6(A, t(0))";

      {
        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"F6", 0, pos}, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 3);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{pp_token_category::pp_number, u8"20"}, pp_token{pp_token_category::op_or_punc, u8","}, pp_token{pp_token_category::pp_number, u8"0"}});
      }

      // F7の実行（VA_OPT内VA_ARGSの文字列化）
      pos = ll.emplace_after(pos, 8, 8);
      (*pos).line = u8"F7(A, t(0))";

      {
        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"F7", 0, pos}, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 1);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{pp_token_category::string_literal, u8R"++("A, t(0)")++"}});
      }

      // F8の実行（VA_OPT内VA_ARGSとVA_ARGSの結合）
      pos = ll.emplace_after(pos, 8, 8);
      (*pos).line = u8"F8(A, t(0))";

      {
        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, {pp_token_category::identifier, u8"F8", 0, pos}, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 5);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{pp_token_category::pp_number, u8"20"}, pp_token{pp_token_category::op_or_punc, u8","}, pp_token{pp_token_category::pp_number, u8"0A"}, pp_token{pp_token_category::op_or_punc, u8","}, pp_token{pp_token_category::pp_number, u8"0"}});
      }
    }

    // コーナーケースのチェック
    {

      pos = ll.emplace_after(pos, 30, 30);
      (*pos).line = u8"F4(t   (0))";


      // マクロの開きかっこまでにホワイトスペースがある場合
      {
        // 実引数1つのリストと実引数全体のリスト
        std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
        std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
        token_list.emplace_back(pp_token_category::identifier, u8"t", 3, pos);
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 4, pos);
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 5, pos);
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 6, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 7, pos);
        token_list.emplace_back(pp_token_category::pp_number, u8"0", 8, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 9, pos);
        args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"F4", 0, pos }, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 1);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{ pp_token_category::pp_number, u8"0" }});
      }

      // マクロの開きかっこまでに別の文字がある場合
      pos = ll.emplace_after(pos, 31, 31);
      (*pos).line = u8"F4(t a(0))";

      {
        // 実引数1つのリストと実引数全体のリスト
        std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
        std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
        token_list.emplace_back(pp_token_category::identifier, u8"t", 3, pos);
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 4, pos);
        token_list.emplace_back(pp_token_category::identifier, u8"a", 5, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 6, pos);
        token_list.emplace_back(pp_token_category::pp_number, u8"0", 7, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 8, pos);
        args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"F4", 0, pos }, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 5);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{ pp_token_category::identifier, u8"t" }, pp_token{ pp_token_category::identifier, u8"a" }, pp_token{ pp_token_category::op_or_punc, u8"(" }, pp_token{ pp_token_category::pp_number, u8"0" }, pp_token{ pp_token_category::op_or_punc, u8")" }});
      }

      // マクロの開きかっこまでに別の文字がある場合2
      pos = ll.emplace_after(pos, 31, 31);
      (*pos).line = u8"F4(t t  (0))";

      {
        // 実引数1つのリストと実引数全体のリスト
        std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
        std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
        token_list.emplace_back(pp_token_category::identifier, u8"t", 3, pos);
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 4, pos);
        token_list.emplace_back(pp_token_category::identifier, u8"t", 5, pos);
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 6, pos);
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 7, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 8, pos);
        token_list.emplace_back(pp_token_category::pp_number, u8"0", 9, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 10, pos);
        args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"F4", 0, pos }, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 2);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{ pp_token_category::identifier, u8"t" }, pp_token{ pp_token_category::pp_number, u8"0" }});
      }

      // 途中で中断するマクロ呼び出し
      pos = ll.emplace_after(pos, 31, 31);
      (*pos).line = u8"F4(t(  )";

      {
        // 実引数1つのリストと実引数全体のリスト
        std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
        std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
        token_list.emplace_back(pp_token_category::identifier, u8"t", 3, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 4, pos);
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 5, pos);
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 6, pos);
        args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"F4", 0, pos }, args);

        REQUIRE_UNARY(success);
        // マクロ展開は完了していない
        CHECK_UNARY_FALSE(complete);

        CHECK_EQ(result.size(), 2);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{ pp_token_category::identifier, u8"t" }, pp_token{ pp_token_category::op_or_punc, u8"(" }});
      }

      // ネストするマクロ呼び出し
      pos = ll.emplace_after(pos, 31, 31);
      (*pos).line = u8"F4(t(1 + t(0)))";

      {
        // 実引数1つのリストと実引数全体のリスト
        std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
        std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
        token_list.emplace_back(pp_token_category::identifier, u8"t", 3, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 4, pos);
        token_list.emplace_back(pp_token_category::pp_number, u8"1", 5, pos);
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 6, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"+", 7, pos);
        token_list.emplace_back(pp_token_category::whitespaces, u8" ", 8, pos);
        token_list.emplace_back(pp_token_category::identifier, u8"t", 9, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 10, pos);
        token_list.emplace_back(pp_token_category::pp_number, u8"0", 11, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 12, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 13, pos);
        args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"F4", 0, pos }, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 3);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{ pp_token_category::pp_number, u8"1" }, pp_token{ pp_token_category::op_or_punc, u8"+" }, pp_token{ pp_token_category::pp_number, u8"0" }});
      }

      // ネストするマクロ呼び出し2
      pos = ll.emplace_after(pos, 31, 31);
      (*pos).line = u8"F4(t(t(A)))";

      {
        // 実引数1つのリストと実引数全体のリスト
        std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
        std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
        token_list.emplace_back(pp_token_category::identifier, u8"t", 3, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 4, pos);
        token_list.emplace_back(pp_token_category::identifier, u8"t", 5, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 6, pos);
        token_list.emplace_back(pp_token_category::identifier, u8"A", 7, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 8, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 9, pos);
        args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"F4", 0, pos }, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 1);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{ pp_token_category::pp_number, u8"20" }});
      }

      // マクロ展開後にマクロ呼び出しが現れる
      pos = ll.emplace_after(pos, 32, 32);
      (*pos).line = u8"F4(g(0))";

      {
        // 実引数1つのリストと実引数全体のリスト
        std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
        std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
        token_list.emplace_back(pp_token_category::identifier, u8"g", 3, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 4, pos);
        token_list.emplace_back(pp_token_category::pp_number, u8"0", 3, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 5, pos);
        args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"F4", 0, pos }, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 1);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{ pp_token_category::pp_number, u8"0" }});
      }

      // マクロ展開後にマクロ呼び出しが現れる
      pos = ll.emplace_after(pos, 32, 32);
      (*pos).line = u8"F4(t(g)(0))";

      {
        // 実引数1つのリストと実引数全体のリスト
        std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
        std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
        token_list.emplace_back(pp_token_category::identifier, u8"t", 3, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 4, pos);
        token_list.emplace_back(pp_token_category::identifier, u8"g", 5, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 6, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 7, pos);
        token_list.emplace_back(pp_token_category::pp_number, u8"0", 8, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 9, pos);
        args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"F4", 0, pos }, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 1);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{ pp_token_category::pp_number, u8"0" }});
      }

      // マクロ展開後にマクロ呼び出しが現れる
      pos = ll.emplace_after(pos, 32, 32);
      (*pos).line = u8"F4(t(t(g)(0)))";

      {
        // 実引数1つのリストと実引数全体のリスト
        std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
        std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
        token_list.emplace_back(pp_token_category::identifier, u8"t", 3, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 4, pos);
        token_list.emplace_back(pp_token_category::identifier, u8"t", 5, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 6, pos);
        token_list.emplace_back(pp_token_category::identifier, u8"g", 7, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 8, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 9, pos);
        token_list.emplace_back(pp_token_category::pp_number, u8"0", 10, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 11, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 12, pos);
        args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"F4", 0, pos }, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 4);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{ pp_token_category::not_macro_name_identifier, u8"t" }, pp_token{ pp_token_category::op_or_punc, u8"(" }, pp_token{ pp_token_category::pp_number, u8"0" }, pp_token{ pp_token_category::op_or_punc, u8")" }});
      }

      // マクロ展開後にマクロ呼び出しが現れる2
      pos = ll.emplace_after(pos, 32, 32);
      (*pos).line = u8"t(t(g2)(0))";

      {
        // 実引数1つのリストと実引数全体のリスト
        std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
        std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
        token_list.emplace_back(pp_token_category::identifier, u8"t", 2, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 3, pos);
        token_list.emplace_back(pp_token_category::identifier, u8"g2", 4, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 6, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8"(", 7, pos);
        token_list.emplace_back(pp_token_category::pp_number, u8"0", 8, pos);
        token_list.emplace_back(pp_token_category::op_or_punc, u8")", 9, pos);
        args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"t", 0, pos }, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 8);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{ pp_token_category::not_macro_name_identifier, u8"fn" }, pp_token{ pp_token_category::op_or_punc, u8"(" }, pp_token{ pp_token_category::pp_number, u8"2" }, pp_token{ pp_token_category::op_or_punc, u8"*" }, pp_token{ pp_token_category::op_or_punc, u8"(" }, pp_token{ pp_token_category::pp_number, u8"0" }, pp_token{ pp_token_category::op_or_punc, u8")" }, pp_token{ pp_token_category::op_or_punc, u8")" }});
      }

      // マクロ展開後にマクロ呼び出しが現れる3
      pos = ll.emplace_after(pos, 32, 32);
      (*pos).line = u8"z";

      {
        // 実引数1つのリストと実引数全体のリスト
        std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
        std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
        args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"z", 0, pos }, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 4);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{ pp_token_category::not_macro_name_identifier, u8"z" }, pp_token{ pp_token_category::op_or_punc, u8"[" }, pp_token{ pp_token_category::pp_number, u8"0" }, pp_token{ pp_token_category::op_or_punc, u8"]" }});
      }

      // 引数に再帰しうるマクロがある
      // マクロ登録時に再帰しうる識別子をマークする処理を追加する必要がある
      pos = ll.emplace_after(pos, 32, 32);
      (*pos).line = u8"fn(z)";

      {
        // 実引数1つのリストと実引数全体のリスト
        std::pmr::list<pp_token> token_list{ &kusabira::def_mr };
        std::pmr::vector<std::pmr::list<pp_token>> args{ &kusabira::def_mr };
        token_list.emplace_back(pp_token_category::identifier, u8"z", 3, pos);
        args.emplace_back(std::exchange(token_list, std::pmr::list<pp_token>{&kusabira::def_mr}));

        const auto [success, complete, result, memo] = pp.expand_funcmacro(*reporter, { pp_token_category::identifier, u8"fn", 0, pos }, args);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);

        CHECK_EQ(result.size(), 11);
        CHECK_EQ(result, std::pmr::list<pp_token>{pp_token{ pp_token_category::not_macro_name_identifier, u8"fn" }, pp_token{ pp_token_category::op_or_punc, u8"(" }, pp_token{ pp_token_category::pp_number, u8"2" }, pp_token{ pp_token_category::op_or_punc, u8"*" }, pp_token{ pp_token_category::op_or_punc, u8"(" }, pp_token{ pp_token_category::not_macro_name_identifier, u8"z" }, pp_token{ pp_token_category::op_or_punc, u8"[" }, pp_token{ pp_token_category::pp_number, u8"0" }, pp_token{ pp_token_category::op_or_punc, u8"]" }, pp_token{ pp_token_category::op_or_punc, u8")" }, pp_token{ pp_token_category::op_or_punc, u8")" }});

      }
    }
  }

  TEST_CASE("predfined macro test") {

    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_predefined_macro.hpp"};
    //テスト開始時刻
    const auto test_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    auto pos = ll.before_begin();

    //__LINE__のテスト（#lineのテストも兼ねる）
    {
      //論理行で5行目に__LINE__を実行
      pos = ll.emplace_after(pos, 20, 5);
      (*pos).line = u8"__LINE__";
      pp_token line_token{pp_token_category::identifier, u8"__LINE__", 0, pos};

      //#lineの引数トークン列
      std::vector<pp_token> line_args{};

      {
        auto opt = pp.is_macro(line_token.token);
        REQUIRE_UNARY(bool(opt));
        CHECK_UNARY_FALSE(*opt);
        
        const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, line_token);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1, result.size());

        auto &res_token = result.front();
        CHECK_EQ(pp_token_category::pp_number, res_token.category);
        CHECK_UNARY(res_token.token == u8"5"sv);
      }

      //#line 1234567を実行、2行目で実行されてたことにする
      auto pos2 = ll.emplace_after(pos, 18, 2);
      (*pos2).line = u8"#line 1234567";
      line_args.emplace_back(pp_token_category::pp_number, u8"1234567", 7, pos2);

      CHECK_UNARY(pp.line(*reporter, line_args));

      {
        const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, line_token);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1, result.size());

        auto &res_token = result.front();
        CHECK_EQ(pp_token_category::pp_number, res_token.category);
        CHECK_UNARY(res_token.token == u8"1234570"sv);
      }

      //#line 0を実行、6行目で実行されてたことにする
      auto pos3 = ll.emplace_after(pos, 23, 6);
      (*pos3).line = u8"#line 0";
      line_args.clear();
      line_args.emplace_back(pp_token_category::pp_number, u8"0", 7, pos2);

      CHECK_UNARY(pp.line(*reporter, line_args));

      {
        const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, line_token);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1, result.size());

        auto &res_token = result.front();
        CHECK_EQ(pp_token_category::pp_number, res_token.category);
        CHECK_UNARY(res_token.token == u8"1234570"sv);
      }

      //#line 99を実行、4行目で実行されてたことにする
      auto pos4 = ll.emplace_after(pos, 21, 4);
      (*pos4).line = u8"#line 99";
      line_args.clear();
      line_args.emplace_back(pp_token_category::pp_number, u8"99", 7, pos4);

      CHECK_UNARY(pp.line(*reporter, line_args));

      {
        const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, line_token);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1, result.size());

        auto &res_token = result.front();
        CHECK_EQ(pp_token_category::pp_number, res_token.category);
        CHECK_UNARY(res_token.token == u8"100"sv);
      }

      //#lineと__LINE__が同じ行で実行される場合は先に__LINE__が読まれることになるはず、テストしない
    }

    //__FILE__のテスト（#lineのテストも兼ねる）
    {
      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"__FILE__";
      pp_token file_token{pp_token_category::identifier, u8"__FILE__", 0, pos };

      {
        auto opt = pp.is_macro(file_token.token);
        REQUIRE_UNARY(bool(opt));
        CHECK_UNARY_FALSE(*opt);

        const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, file_token);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1, result.size());

        auto& res_token = result.front();
        CHECK_EQ(pp_token_category::string_literal, res_token.category);
        CHECK_UNARY(res_token.token == u8"test_predefined_macro.hpp"sv);
      }

      //#lineの引数トークン列
      std::vector<pp_token> line_args{};

      //#line 10 replace_file.cppを実行
      auto pos2 = ll.emplace_after(pos, 2, 2);
      (*pos2).line = u8R"(#line 10 "replace_file.cpp")";
      line_args.emplace_back(pp_token_category::pp_number, u8"10", 6, pos2);
      line_args.emplace_back(pp_token_category::string_literal, u8R"("replace_file.cpp")", 9, pos2);
      line_args.emplace_back(pp_token_category::newline, u8"", 27, pos2);

      CHECK_UNARY(pp.line(*reporter, line_args));

      {
        const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, file_token);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1, result.size());

        auto& res_token = result.front();
        CHECK_EQ(pp_token_category::string_literal, res_token.category);
        CHECK_UNARY(res_token.token == u8"replace_file.cpp"sv);
      }

      //#line 1000 R"(rwastr_filename.h)"を実行
      auto pos3 = ll.emplace_after(pos, 11, 11);
      (*pos3).line = u8R"+(#line 1000 R"(rwastr_filename.h)")+";
      line_args.clear();
      line_args.emplace_back(pp_token_category::pp_number, u8"1000", 6, pos3);
      line_args.emplace_back(pp_token_category::raw_string_literal, u8R"+(R"*+(rwastr_filename.h)*+")+", 11, pos3);
      line_args.emplace_back(pp_token_category::newline, u8"", 33, pos3);

      CHECK_UNARY(pp.line(*reporter, line_args));

      {
        const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, file_token);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1, result.size());

        auto& res_token = result.front();
        CHECK_EQ(pp_token_category::string_literal, res_token.category);
        CHECK_UNARY(res_token.token == u8"rwastr_filename.h"sv);
      }
    }

    //__DATE__のテスト
    {
      std::stringstream stm{};

      //time_tをtm構造体へ変換
#ifdef _MSC_VER
      tm temp{};
      gmtime_s(&temp, &test_time);
      //Mmm dd yyyy
      stm << std::put_time(&temp, "%b %e %Y");
#else
      //Mmm dd yyyy
      stm << std::put_time(gmtime(&test_time), "%b %e %Y");
#endif // _MSC_VER

      auto expect_str = stm.str();
      std::u8string_view expect{reinterpret_cast<const char8_t*>(expect_str.data()), expect_str.length()};

      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"__DATE__";
      pp_token date_token{pp_token_category::identifier, u8"__DATE__", 0, pos};

      auto opt = pp.is_macro(date_token.token);
      REQUIRE_UNARY(bool(opt));
      CHECK_UNARY_FALSE(*opt);

      //何回読んでも同じ結果になる
      for (int i = 0; i < 5; ++i) {
        const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, date_token);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1, result.size());

        auto &res_token = result.front();
        CHECK_EQ(pp_token_category::string_literal, res_token.category);
        CHECK_UNARY(res_token.token == expect);
      }
    }

    //__TIME__のテスト
    {
      std::stringstream stm{};

#ifdef _MSC_VER
      tm temp{};
      gmtime_s(&temp, &test_time);
      //hh:mm:ss
      stm << std::put_time(&temp, "%H:%M:%S");
#else
      //hh:mm:ss
      stm << std::put_time(gmtime(&test_time), "%H:%M:%S");
#endif // _MSC_VER

      auto expect_str = stm.str();
      //少なくとも分単位までは会うはず
      std::u8string_view expect{reinterpret_cast<const char8_t*>(expect_str.data()), 6};

      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"__TIME__";
      pp_token time_token{pp_token_category::identifier, u8"__TIME__", 0, pos};

      auto opt = pp.is_macro(time_token.token);
      REQUIRE_UNARY(bool(opt));
      CHECK_UNARY_FALSE(*opt);

      //何回読んでも同じ結果になる
      for (int i = 0; i < 5; ++i) {
        const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, time_token);

        REQUIRE_UNARY(success);
        CHECK_UNARY(complete);
        CHECK_EQ(1, result.size());

        auto &res_token = result.front();
        CHECK_EQ(pp_token_category::string_literal, res_token.category);
        //ごく低い確率で落ちるかもしれない
        CHECK_UNARY(res_token.token.to_view().substr(0, 6) == expect);
        //秒単位は数字であることだけ確認しておく
        auto sec = res_token.token.to_view().substr(6, 2);
        CHECK_UNARY(std::isdigit(static_cast<unsigned char>(sec[0])) != 0);
        CHECK_UNARY(std::isdigit(static_cast<unsigned char>(sec[1])) != 0);
      }
    }
    //_­_­cplusplus
    {
      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"_­_­cplusplus";
      pp_token macro_token{pp_token_category::identifier, u8"_­_­cplusplus", 0, pos};

      auto opt = pp.is_macro(macro_token.token);
      REQUIRE_UNARY(bool(opt));
      CHECK_UNARY_FALSE(*opt);

      const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, macro_token);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(1, result.size());

      auto &res_token = result.front();
      CHECK_EQ(pp_token_category::pp_number, res_token.category);
      CHECK_UNARY(res_token.token == u8"202002L"sv);
    }
    //__STDC_­HOSTED__
    {
      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"__STDC_­HOSTED__";
      pp_token macro_token{pp_token_category::identifier, u8"__STDC_­HOSTED__", 0, pos};

      auto opt = pp.is_macro(macro_token.token);
      REQUIRE_UNARY(bool(opt));
      CHECK_UNARY_FALSE(*opt);

      const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, macro_token);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(1, result.size());

      auto &res_token = result.front();
      CHECK_EQ(pp_token_category::pp_number, res_token.category);
      CHECK_UNARY(res_token.token == u8"1"sv);
    }
    //__STDCPP_­DEFAULT_­NEW_­ALIGNMENT__
    {
      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"__STDCPP_­DEFAULT_­NEW_­ALIGNMENT__";
      pp_token macro_token{pp_token_category::identifier, u8"__STDCPP_­DEFAULT_­NEW_­ALIGNMENT__", 0, pos};

      auto opt = pp.is_macro(macro_token.token);
      REQUIRE_UNARY(bool(opt));
      CHECK_UNARY_FALSE(*opt);

      const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, macro_token);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(1, result.size());

      auto &res_token = result.front();
      CHECK_EQ(pp_token_category::pp_number, res_token.category);
      CHECK_UNARY(res_token.token == u8"16ull"sv);
    }
    //__STDCPP_­THREADS__
    {
      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"__STDCPP_­THREADS__";
      pp_token macro_token{pp_token_category::identifier, u8"__STDCPP_­THREADS__", 0, pos};

      auto opt = pp.is_macro(macro_token.token);
      REQUIRE_UNARY(bool(opt));
      CHECK_UNARY_FALSE(*opt);

      const auto [success, complete, result, memo] = pp.expand_objmacro(*reporter, macro_token);

      REQUIRE_UNARY(success);
      CHECK_UNARY(complete);
      CHECK_EQ(1, result.size());

      auto &res_token = result.front();
      CHECK_EQ(pp_token_category::pp_number, res_token.category);
      CHECK_UNARY(res_token.token == u8"1"sv);
    }
  }

} // namespace kusabira_test::preprocessor
