#pragma once

#include "doctest/doctest.h"

#include "common.hpp"

namespace kusabira_test::common
{

  TEST_CASE("lex token test") {
    using kusabira::PP::lex_token;
    using kusabira::PP::logical_line;
    using kusabira::PP::pp_tokenize_result;
    using kusabira::PP::pp_tokenize_status;
    using namespace std::literals;


    //論理行保持コンテナ
    std::pmr::forward_list<logical_line> ll{};

    auto pos = ll.before_begin();

    //論理行オブジェクト1
    pos = ll.emplace_after(pos, 1);
    (*pos).line = u8"int main() {";

    {
      //テスト用トークン1
      lex_token test_token{ {pp_tokenize_status::Identifier}, u8"int", 0u, pos };

      CHECK_UNARY_FALSE(test_token.is_multiple_phlines());
      CHECK_UNARY(test_token.get_line_string() == u8"int main() {");
      auto [row, col] = test_token.get_phline_pos();
      CHECK_EQ(row, 1u);
      CHECK_EQ(col, 0u);
    }

    //論理行オブジェクト2
    pos = ll.emplace_after(pos, 2);
    (*pos).line = u8"  return 0; }";
    //"  "の後、";"の後、で行継続が行われているとする
    //"  \\nreturn 0;\\n }"みたいな感じ
    (*pos).line_offset.push_back(2);
    (*pos).line_offset.push_back(9);

    {
      //テスト用トークン2
      lex_token test_token{ {pp_tokenize_status::Identifier}, u8"return", 2u, pos };

      //行継続が行われている
      CHECK_UNARY(test_token.is_multiple_phlines());
      CHECK_UNARY(test_token.get_line_string() == u8"  return 0; }");
      auto [row, col] = test_token.get_phline_pos();
      CHECK_EQ(row, 3u);
      CHECK_EQ(col, 0u);
    }

    {
      //テスト用トークン3
      lex_token test_token{ {pp_tokenize_status::OPorPunc}, u8"}", 12u, pos };

      //行継続が行われている
      CHECK_UNARY(test_token.is_multiple_phlines());
      CHECK_UNARY(test_token.get_line_string() == u8"  return 0; }");
      auto [row, col] = test_token.get_phline_pos();
      CHECK_EQ(row, 4u);
      CHECK_EQ(col, 1u);
    }
  }

    
} // namespace kusabira_test::common
