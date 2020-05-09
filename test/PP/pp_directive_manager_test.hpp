#pragma once

#include "doctest/doctest.h"
#include "PP/pp_directive_manager.hpp"
#include "../report_output_test.hpp"

namespace kusabira_test::preprocessor
{

  TEST_CASE("rawstrliteral extract test") {
    using namespace std::string_view_literals;

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

  TEST_CASE("error test") {
    using namespace std::literals;
    using kusabira::PP::lex_token;
    using kusabira::PP::logical_line;
    using kusabira::PP::pp_tokenize_status;
    using kusabira::PP::pp_tokenize_result;

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //トークン列
    std::vector<lex_token> tokens{};
    tokens.reserve(20);
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_error_directive.hpp"};

    auto pos = ll.before_begin();

    {
      //トークン列の構成
      pos = ll.emplace_after(pos, 1);
      (*pos).line = u8"#error test error directive!";
      tokens.push_back({ pp_tokenize_result{.status = pp_tokenize_status::Identifier }, u8"error", 1, pos });
      tokens.push_back({ pp_tokenize_result{.status = pp_tokenize_status::Identifier }, u8"test", 7, pos });
      tokens.push_back({ pp_tokenize_result{.status = pp_tokenize_status::Identifier }, u8"error", 12, pos });
      tokens.push_back({ pp_tokenize_result{.status = pp_tokenize_status::Identifier }, u8"directive", 18, pos });
      tokens.push_back({ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc }, u8"!", 27, pos });
      tokens.push_back({ pp_tokenize_result{.status = pp_tokenize_status::NewLine }, u8"", 28, pos });

      auto it = std::begin(tokens);
      pp.error(*reporter, it, std::end(tokens));

      auto expect_text = u8"test_error_directive.hpp:1:1: error: test error directive!\n"sv;
      auto&& str = report::test_out::extract_string();

      CHECK_UNARY(expect_text == str);
    }

    tokens.clear();
    {
      //トークン列の構成
      pos = ll.emplace_after(pos, 137879);
      (*pos).line = u8"#             error       test   error  directive !";
      tokens.push_back({ pp_tokenize_result{.status = pp_tokenize_status::Identifier }, u8"error", 14, pos });
      tokens.push_back({ pp_tokenize_result{.status = pp_tokenize_status::Identifier }, u8"test", 26, pos });
      tokens.push_back({ pp_tokenize_result{.status = pp_tokenize_status::Identifier }, u8"error", 33, pos });
      tokens.push_back({ pp_tokenize_result{.status = pp_tokenize_status::Identifier }, u8"directive", 40, pos });
      tokens.push_back({ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc }, u8"!", 50, pos });
      tokens.push_back({ pp_tokenize_result{.status = pp_tokenize_status::NewLine }, u8"", 51, pos });

      auto it = std::begin(tokens);
      pp.error(*reporter, it, std::end(tokens));

      auto expect_text = u8"test_error_directive.hpp:137879:14: error: test   error  directive !\n"sv;
      auto&& str = report::test_out::extract_string();

      CHECK_UNARY(expect_text == str);
    }
  }

  TEST_CASE("line directive test") {
    using kusabira::PP::lex_token;
    using kusabira::PP::logical_line;
    using kusabira::PP::pp_token_category;
    using kusabira::PP::pp_tokenize_result;
    using kusabira::PP::pp_tokenize_status;
    using kusabira::PP::pp_token;
    using kusabira::PP::pp_token_category;
    using namespace std::literals;

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //PPトークン列
    std::vector<pp_token> pptokens{};
    pptokens.reserve(20);
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_line_directive.hpp"};

    auto pos = ll.before_begin();
    pos = ll.emplace_after(pos, 0);
    (*pos).line = u8"#line 1234";

    //行数のみの変更
    {
      //字句トークン
      lex_token lt{pp_tokenize_result{.status = pp_tokenize_status::NumberLiteral}, u8"1234", 6, pos};
      lex_token nl{pp_tokenize_result{.status = pp_tokenize_status::NewLine}, u8"", 10, pos};

      //PPトークン列
      pptokens.emplace_back(pp_token_category::pp_number, std::move(lt));
      pptokens.emplace_back(pp_token_category::newline, std::move(nl));

      CHECK_UNARY(pp.line(*reporter, pptokens));

      const auto& [linenum, filename] = pp.get_state();

      CHECK_EQ(1234, linenum);
      CHECK_EQ(filename, "test_line_directive.hpp");

      auto&& str = report::test_out::extract_string();
      CHECK_UNARY(str.empty());
    }

    pptokens.clear();
    pos = ll.emplace_after(pos, 1);
    (*pos).line = u8R"(#line 5678 "replace_filename.hpp")";

    //行数とファイル名の変更
    {
      //字句トークン
      lex_token lt1{pp_tokenize_result{.status = pp_tokenize_status::NumberLiteral}, u8"5678", 6, pos};
      lex_token lt2{pp_tokenize_result{.status = pp_tokenize_status::StringLiteral}, u8R"("replace_filename.hpp")", 11, pos};
      lex_token nl{pp_tokenize_result{.status = pp_tokenize_status::NewLine}, u8"", 32, pos};

      //PPトークン列
      pptokens.emplace_back(pp_token_category::pp_number, std::move(lt1));
      pptokens.emplace_back(pp_token_category::string_literal, std::move(lt2));
      pptokens.emplace_back(pp_token_category::newline, std::move(nl));

      CHECK_UNARY(pp.line(*reporter, pptokens));

      const auto& [linenum, filename] = pp.get_state();

      CHECK_EQ(5678, linenum);
      CHECK_EQ(filename, "replace_filename.hpp");

      auto&& str = report::test_out::extract_string();
      CHECK_UNARY(str.empty());
    }

    //あとマクロ展開した上で#lineディレクティブ実行する場合のテストが必要、マクロ実装後
  }

  TEST_CASE("object like macro test") {

    using kusabira::PP::lex_token;
    using kusabira::PP::logical_line;
    using kusabira::PP::pp_token;
    using kusabira::PP::pp_token_category;
    using kusabira::PP::pp_tokenize_result;
    using kusabira::PP::pp_tokenize_status;
    using namespace std::literals;

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_line_directive.hpp"};

    auto pos = ll.before_begin();
    pos = ll.emplace_after(pos, 0);
    (*pos).line = u8"#define N 3";

    //単純な定数定義
    {
      //PPトークン列
      std::pmr::list<pp_token> pptokens{&kusabira::def_mr};

      //字句トークン
      lex_token lt1{pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"N", 8, pos};
      lex_token lt2{pp_tokenize_result{.status = pp_tokenize_status::NumberLiteral}, u8"3", 10, pos};

      //PPトークン列
      pptokens.emplace_back(pp_token_category::pp_number, std::move(lt2));

      CHECK_UNARY(pp.define(*reporter, lt1.token, pptokens));

      auto list = pp.objmacro(lt1.token);

      CHECK_UNARY(bool(list));
      CHECK_EQ(list->size(), 1u);
      CHECK_EQ(pptokens, *list);
    }

    pos = ll.emplace_after(pos, 1);
    (*pos).line = u8"#define MACRO macro replacement test";

    //長めの置換リスト
    {
      //PPトークン列
      std::pmr::list<pp_token> pptokens{&kusabira::def_mr};

      //字句トークン
      lex_token lt1{pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"MACRO", 8, pos};
      lex_token lt2{pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"macro", 14, pos};
      lex_token lt3{pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"replacement", 20, pos};
      lex_token lt4{pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"test", 32, pos};

      //PPトークン列
      pptokens.emplace_back(pp_token_category::identifier, std::move(lt2));
      pptokens.emplace_back(pp_token_category::identifier, std::move(lt3));
      pptokens.emplace_back(pp_token_category::identifier, std::move(lt4));

      CHECK_UNARY(pp.define(*reporter, lt1.token, pptokens));

      auto list = pp.objmacro(lt1.token);

      CHECK_UNARY(bool(list));
      CHECK_EQ(list->size(), 3u);
      CHECK_EQ(pptokens, *list);
    }

    //適当なトークンを投げてみる
    CHECK_EQ(pp.objmacro(u8"M"), std::nullopt);
    CHECK_EQ(pp.objmacro(u8"NN"), std::nullopt);
    CHECK_EQ(pp.objmacro(u8"TEST"), std::nullopt);
    CHECK_EQ(pp.objmacro(u8"NUMBER"), std::nullopt);
    CHECK_EQ(pp.objmacro(u8"MACRO_"), std::nullopt);
  }


  TEST_CASE("function like macro") {

    using kusabira::PP::lex_token;
    using kusabira::PP::logical_line;
    using kusabira::PP::pp_token;
    using kusabira::PP::pp_token_category;
    using kusabira::PP::pp_tokenize_result;
    using kusabira::PP::pp_tokenize_status;
    using namespace std::literals;

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_funcmacro.hpp"};

    auto pos = ll.before_begin();
    pos = ll.emplace_after(pos, 0);
    (*pos).line = u8"#define max(a, b) ((a) > (b) ? (a) : (b))";

    {
      //仮引数列と置換リスト
      std::pmr::vector<std::u8string_view> params{&kusabira::def_mr};
      std::pmr::list<pp_token> rep_list{&kusabira::def_mr};

      //字句トークン
      //lex_token lt{ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8"#", 0, pos };
      //lex_token lt0{ pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"define", 1, pos };
      lex_token lt1 { pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"max", 8, pos};
      //lex_token lt2 { pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8"(", 11, pos};
      lex_token lt3 { pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"a", 12, pos };
      //lex_token lt4 { pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8",", 13, pos };
      lex_token lt5 { pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"b", 15, pos};
      //lex_token lt6 { pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8")", 16, pos};
      lex_token lt7 { pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8"(", 18, pos };
      lex_token lt8 { pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8"(", 19, pos };
      lex_token lt9 { pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"a", 20, pos };
      lex_token lt10{ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8")", 21, pos };
      lex_token lt11{ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8">", 23, pos };
      lex_token lt12{ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8"(", 25, pos };
      lex_token lt13{ pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"b", 26, pos };
      lex_token lt14{ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8")", 27, pos };
      lex_token lt15{ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8"?", 29, pos };
      lex_token lt16{ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8"(", 31, pos };
      lex_token lt17{ pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"a", 32, pos };
      lex_token lt18{ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8")", 33, pos };
      lex_token lt19{ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8":", 35, pos };
      lex_token lt20{ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8"(", 37, pos };
      lex_token lt21{ pp_tokenize_result{.status = pp_tokenize_status::Identifier}, u8"b", 38, pos };
      lex_token lt22{ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8")", 39, pos };
      lex_token lt23{ pp_tokenize_result{.status = pp_tokenize_status::OPorPunc}, u8")", 40, pos };

      //マクロ仮引数トークン列
      params.emplace_back(lt3.token);
      params.emplace_back(lt5.token);
      //置換リスト
      rep_list.emplace_back(pp_token_category::op_or_punc, lt7);
      rep_list.emplace_back(pp_token_category::op_or_punc, lt8);
      rep_list.emplace_back(pp_token_category::identifier, lt9);
      rep_list.emplace_back(pp_token_category::op_or_punc, lt10);
      rep_list.emplace_back(pp_token_category::op_or_punc, lt11);
      rep_list.emplace_back(pp_token_category::op_or_punc, lt12);
      rep_list.emplace_back(pp_token_category::identifier, lt13);
      rep_list.emplace_back(pp_token_category::op_or_punc, lt14);
      rep_list.emplace_back(pp_token_category::op_or_punc, lt15);
      rep_list.emplace_back(pp_token_category::op_or_punc, lt16);
      rep_list.emplace_back(pp_token_category::identifier, lt17);
      rep_list.emplace_back(pp_token_category::op_or_punc, lt18);
      rep_list.emplace_back(pp_token_category::op_or_punc, lt19);
      rep_list.emplace_back(pp_token_category::op_or_punc, lt20);
      rep_list.emplace_back(pp_token_category::identifier, lt21);
      rep_list.emplace_back(pp_token_category::op_or_punc, lt22);
      rep_list.emplace_back(pp_token_category::op_or_punc, lt23);

      //関数マクロ登録
      CHECK_UNARY(pp.define(*reporter, lt1.token, params, rep_list, false));

      //実引数リスト作成
      pos = ll.emplace_after(pos, 1);
      (*pos).line = u8"max(1, 2)";

      lex_token arg1{ pp_tokenize_result{.status = pp_tokenize_status::NumberLiteral}, u8"1", 4, pos };
      lex_token arg2{ pp_tokenize_result{.status = pp_tokenize_status::NumberLiteral}, u8"2", 7, pos };

      std::pmr::list<pp_token> args{&kusabira::def_mr};
      args.emplace_back(pp_token_category::pp_number,arg1);
      args.emplace_back(pp_token_category::pp_number,arg2);

      //関数マクロ実行
      const auto [is_success, result] = pp.funcmacro(lt1.token, args);

      CHECK_UNARY(is_success);
      REQUIRE_UNARY(bool(result));
      CHECK_EQ(rep_list.size(), result->size());

      std::pmr::list<pp_token> expect{ &kusabira::def_mr };
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt7));
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt8));
      expect.emplace_back(pp_token_category::pp_number, arg1);
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt10));
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt11));
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt12));
      expect.emplace_back(pp_token_category::pp_number, arg2);
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt14));
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt15));
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt16));
      expect.emplace_back(pp_token_category::pp_number, arg1);
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt18));
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt19));
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt20));
      expect.emplace_back(pp_token_category::pp_number, arg2);
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt22));
      expect.emplace_back(pp_token_category::op_or_punc, std::move(lt23));

      //結果の比較
      CHECK_EQ(expect, *result);

      //引数の数が合わないような呼び出し、失敗
      args.pop_back();
      const auto [faile, null_result] = pp.funcmacro(lt1.token, args);
      CHECK_UNARY_FALSE(faile);
      CHECK_EQ(null_result, std::nullopt);

      //再登録、成功する（登録は行われない）
      CHECK_UNARY(pp.define(*reporter, lt1.token, params, rep_list, false));

      //一部でも違うと失敗
      params.pop_back();
      CHECK_UNARY_FALSE(pp.define(*reporter, lt1.token, params, rep_list, false));

      params.emplace_back(lt5.token);
      rep_list.pop_back();
      CHECK_UNARY_FALSE(pp.define(*reporter, lt1.token, params, rep_list, false));
    }

    //適当なトークンを投げてみる（登録されていない場合は失敗にはならない
    {
      const auto [is_success, result] = pp.funcmacro(u8"min", {});
      CHECK_UNARY(is_success);
      CHECK_EQ(result, std::nullopt);
    }
    {
      const auto [is_success, result] = pp.funcmacro(u8"max_", {});
      CHECK_UNARY(is_success);
      CHECK_EQ(result, std::nullopt);
    }
    {
      const auto [is_success, result] = pp.funcmacro(u8"MAX", {});
      CHECK_UNARY(is_success);
      CHECK_EQ(result, std::nullopt);
    }
  }

} // namespace kusabira_test::preprocessor
