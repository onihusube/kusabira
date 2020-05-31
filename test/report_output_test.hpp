#pragma once

#include <sstream>
#include "report_output.hpp"

namespace kusabira_test::report
{
  
  struct test_out {

    static inline std::stringstream stream{};

    static auto extract_string() -> std::u8string {
      std::stringstream init{};
      std::swap(stream, init);

      auto&& str = init.str();
      auto punned_str = reinterpret_cast<const char8_t*>(str.data());

      return {punned_str, str.length()};
    }

    static void output_u8string(const std::u8string_view str) {
      auto punned_str = reinterpret_cast<const char*>(str.data());

      stream << punned_str;
    }

    template<typename... Args>
    static void output(Args&&... args) {
      (stream << ... << args);
    }

    static void endl() {
      stream << std::endl;
    }
  };
    
  TEST_CASE("print_report() test") {

    using namespace std::literals;
    using kusabira::PP::lex_token;
    using kusabira::PP::logical_line;
    using kusabira::PP::pp_tokenize_status;
    using kusabira::PP::pp_tokenize_result;

    // 現在の出力状態をリセットする
    test_out::extract_string();

    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};

    auto pos = ll.before_begin();

    //論理行オブジェクト1
    pos = ll.emplace_after(pos, 1);
    (*pos).line = u8"#error test error out";
    lex_token token1(pp_tokenize_result{ .status = pp_tokenize_status::Identifier }, u8"error", 1, pos);

    auto reporter = kusabira::report::reporter_factory<test_out>::create();
    reporter->print_report(u8"test error out", L"testdummy.cpp", token1);

    auto expect_text = u8"testdummy.cpp:1:1: error: test error out\n"sv;
    auto&& str = test_out::extract_string();

    CHECK_UNARY(str == expect_text);
  }
} // namespace kusabira_test::report
