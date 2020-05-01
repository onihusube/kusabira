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
  }

} // namespace kusabira_test::preprocessor
