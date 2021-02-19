#pragma once

#include "doctest/doctest.h"
#include "PP/pp_directive_manager.hpp"
#include "../report_output_test.hpp"

namespace kusabira_test::cpp {
  using kusabira::PP::logical_line;
  using kusabira::PP::pp_token;
  using kusabira::PP::pp_token_category;
  using namespace std::string_view_literals;

  TEST_CASE("#error test") {

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};
    //トークン列
    std::vector<pp_token> tokens{};
    tokens.reserve(20);
    //エラー出力先
    auto reporter = kusabira::report::reporter_factory<report::test_out>::create();
    //プリプロセッサ
    kusabira::PP::pp_directive_manager pp{"/kusabira/test_error_directive.hpp"};

    // 溜まっているログを消す
    [[maybe_unused]] auto trash = report::test_out::extract_string();

    auto pos = ll.before_begin();

    {
      //トークン列の構成
      pos = ll.emplace_after(pos, 1, 1);
      (*pos).line = u8"#error test error directive!";

      pp_token err_token{pp_token_category::identifier, u8"error", 1, pos};
      pp_token message_token{pp_token_category::identifier, u8"test", 7, pos};

      // #error実行
      pp.error(*reporter, err_token, message_token);

      auto expect_text = u8"test_error_directive.hpp:1:1: error: test error directive!\n"sv;
      auto str = report::test_out::extract_string();

      CHECK_UNARY(expect_text == str);
    }

    tokens.clear();
    {
      //トークン列の構成
      pos = ll.emplace_after(pos, 137879, 137879);
      (*pos).line = u8"#error";

      pp_token err_token{pp_token_category::identifier, u8"error", 1, pos};
      pp_token message_token{pp_token_category::newline, u8"", 6, pos};

      // #error実行
      pp.error(*reporter, err_token, message_token);

      auto expect_text = u8"test_error_directive.hpp:137879:1: error: \n"sv;
      auto&& str = report::test_out::extract_string();

      CHECK_UNARY(expect_text == str);
    }
  }

}