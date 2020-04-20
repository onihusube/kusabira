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
    tokens.reserve(10);
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"C:\\kusabira\\test_error_directive.hpp"};

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

} // namespace kusabira_test::preprocessor
